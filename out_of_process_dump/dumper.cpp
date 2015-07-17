// dumper.cpp : Defines the entry point for the console application.
//
#include "dumper.h"

#include <memory>

#include <QList>
#include <QFileInfo>
#include <Psapi.h>

#include "xmlreport.h"

namespace
{
typedef BOOL (_stdcall *PMINIDUMPWRITE)(HANDLE,
	DWORD,
	HANDLE,
	MINIDUMP_TYPE,
	PMINIDUMP_EXCEPTION_INFORMATION,
	PMINIDUMP_USER_STREAM_INFORMATION,
	PMINIDUMP_CALLBACK_INFORMATION);

const char* const ArgProcessID = "-pid=";
const char* const ArgThreadID = "-tid=";
const char* const ArgExceptionPointer = "-exp=";
const char* const ArgFullDump = "-f=";

const int ParseResultPID = 0x1;
const int ParseResultTID = 0x2;
const int ParseResultExp = 0x4;
const int ParseResultFullDump = 0x8; 

const int MandatoryArgCount = 4;

/* 带上
unloaded module list，检查注入dll
process thread data，记录peb，teb，特别是teb，记录线程栈大小及起始终止地址，用于栈破坏时辅佐栈重建
full memory info，记录堆地址段信息，用于检查地址属性，例如 !address 命令
handle data，用于 !handle 命令
*/
MINIDUMP_TYPE DumpTypeMini = 
	static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithUnloadedModules
	| MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData);

/*
minidump基础上加上堆内存
*/
MINIDUMP_TYPE DumpTypeFull = 
	static_cast<MINIDUMP_TYPE>(DumpTypeMini | MiniDumpWithFullMemory);

bool createReportXml(
	HANDLE targetProcess, PEXCEPTION_POINTERS p, 
	const QList<MINIDUMP_MODULE_CALLBACK>* modules, const QString& dir)
{
	if (modules == NULL)
	{
		return false;
	}

	EXCEPTION_POINTERS pointers = {};
	SIZE_T readed = 0;
	if (::ReadProcessMemory(targetProcess, p, &pointers, sizeof(pointers), &readed) == 0)
	{
		return false;
	}

	ER_EXCEPTIONRECORD exceptionRecord;
	ULONG64 targetProcessExceptionAddr = 0;
	if (!XmlReport::createExceptionRecordNode(
		targetProcess, pointers.ExceptionRecord, exceptionRecord, targetProcessExceptionAddr))
	{
		return false;
	}

	ER_ADDITIONALINFO info;
	XmlReport::createAdditionalInfoNode(info);
	
	ER_PROCESSOR processor;
	XmlReport::createProcessorNode(processor);

	ER_OPERATINGSYSTEM os;
	XmlReport::createOSNode(os);

	ER_MEMORY memory;
	XmlReport::createMemoryNode(memory);

	MODULE_LIST moduleList;
	QString exceptionModule;
	XmlReport::createModulesNode(
		*modules, targetProcessExceptionAddr, moduleList,
		exceptionModule);

	exceptionRecord.ExceptionModuleName = exceptionModule;

	return XmlReport::createInformationLog(
		dir, exceptionRecord, info, processor, os, memory, moduleList);
}
}

BOOL WINAPI Dumper::miniDumpCallback(
	__inout PVOID CallbackParam,
	__in    PMINIDUMP_CALLBACK_INPUT CallbackInput,
	__inout PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
	)
{
	if (ModuleCallback == CallbackInput->CallbackType)
	{
		QList<MINIDUMP_MODULE_CALLBACK>* moduleList = 
			reinterpret_cast<QList<MINIDUMP_MODULE_CALLBACK>*>(CallbackParam);

		moduleList->push_back(CallbackInput->Module);
		moduleList->back().FullPath = _wcsdup(CallbackInput->Module.FullPath);
	}

	return TRUE;
}

QString Dumper::createDumpDir()
{
	const size_t tempDirLen = 320;
	std::unique_ptr<wchar_t[]> temp(new wchar_t[tempDirLen]);
	memset(temp.get(), 0, tempDirLen * sizeof(temp.get()[0]));

	::GetTempPathW(tempDirLen, temp.get());

	wchar_t szGUIDBuffer[40] = {0};
	GUID guid;
	::CoCreateGuid(&guid);
	::StringFromGUID2(guid, szGUIDBuffer, 40);

	QString directory = QString::fromWCharArray(temp.get());
	directory += QString::fromWCharArray(szGUIDBuffer);
	::CreateDirectoryW(directory.toStdWString().c_str(), NULL);

	return directory;
}

QString Dumper::getProcessName(HANDLE process)
{
	const size_t nameLen = 512;
	std::unique_ptr<wchar_t[]> name(new wchar_t[nameLen]);
	memset(name.get(), 0, nameLen * sizeof(name.get()[0]));
	::GetModuleBaseNameW(process, NULL, name.get(), nameLen);

	return QString::fromWCharArray(name.get());
}

HANDLE Dumper::createDumpFile(const QString& processName, const QString& dumpDir)
{
	QString fileName(processName);
	fileName += "Dump.dmp";

	//
	// 生成文件。 生成的文件保存在系统的临时文件夹下。
	//
	QString filePath = dumpDir + "\\" + fileName;
	HANDLE dumpFile = ::CreateFileW(filePath.toStdWString().c_str(),
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	return dumpFile;
}

int Dumper::dump(const DumpeeArg& arg)
{
	DumpeeArg dumpeeArg = arg;

	std::shared_ptr<void> process( 
		::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
			FALSE, dumpeeArg.PID), ::CloseHandle);
	if (!process)
	{
		return -1;
	}

	QString processName = QFileInfo(getProcessName(process.get())).baseName();
	QString dumpDir = createDumpDir();
	if (dumpeeArg.DumpFile == INVALID_HANDLE_VALUE)
	{
		dumpeeArg.DumpFile = createDumpFile(processName, dumpDir);
	}
	
	MINIDUMP_EXCEPTION_INFORMATION info = {};
	info.ClientPointers = TRUE;
	info.ThreadId = dumpeeArg.TID;
	info.ExceptionPointers = reinterpret_cast<PEXCEPTION_POINTERS>(dumpeeArg.Exp);

	std::shared_ptr<void> dbgHelp(::LoadLibraryW(L".\\dbghelp.dll"), ::FreeLibrary);
	if (dbgHelp.get() == NULL)
	{
		// fallback to system's dbghelp
		dbgHelp.reset(::LoadLibraryW(L"dbghelp.dll"), ::FreeLibrary);
		if (dbgHelp.get() == NULL)
		{
			return -3;
		}
	}

	PMINIDUMPWRITE pMiniDumpWriteDump = 
		(PMINIDUMPWRITE)GetProcAddress(
		static_cast<HMODULE>(dbgHelp.get()), "MiniDumpWriteDump");

	BOOL dumpSuccess = FALSE;
	std::unique_ptr<QList<MINIDUMP_MODULE_CALLBACK>> modules(
		new QList<MINIDUMP_MODULE_CALLBACK>());
	if (pMiniDumpWriteDump != NULL)
	{
		MINIDUMP_CALLBACK_INFORMATION callbackInfo = {0};
		callbackInfo.CallbackRoutine = miniDumpCallback;
		callbackInfo.CallbackParam = modules.get();

		dumpSuccess = pMiniDumpWriteDump(
			process.get(), dumpeeArg.PID, dumpeeArg.DumpFile, 
			dumpeeArg.FullDump ? DumpTypeFull : DumpTypeMini,
			(dumpeeArg.Exp == 0 ? NULL : &info), NULL, &callbackInfo);

		if (dumpSuccess == FALSE)
		{
			// fallback to minimum require
			dumpSuccess = pMiniDumpWriteDump(
				process.get(), dumpeeArg.PID, dumpeeArg.DumpFile, 
				dumpeeArg.FullDump ? MiniDumpWithFullMemory : MiniDumpNormal,
				(dumpeeArg.Exp == 0 ? NULL : &info), NULL, &callbackInfo);
		}
	}

	if (dumpSuccess)
	{
		createReportXml(
			process.get(), reinterpret_cast<PEXCEPTION_POINTERS>(dumpeeArg.Exp), 
			modules.get(), dumpDir);

		return 1;
	}

	return -2;
}

bool Dumper::getArgValue(const char* arg, const char* argKey, QString* outArgValue)
{
	if (arg == nullptr || argKey == nullptr || outArgValue == nullptr)
	{
		return false;
	}

	QString s(arg);
	auto pos = s.indexOf(argKey);
	if (pos != -1)
	{
		*outArgValue = s.mid(pos + QString(argKey).length());
		return true;
	}

	return false;
}

bool Dumper::parseDumpeeArg(int argc, char* argv[], DumpeeArg* outDumpeeArg)
{
	if (argc < MandatoryArgCount)
	{
		return false;
	}

	if (outDumpeeArg == nullptr)
	{
		return false;
	}

	int parseResult = 0;
	DWORD pid = -1;
	DWORD tid = -1;
	ULONG64 exp = 0;
	bool fullDump = false;
	HANDLE dumpFile = INVALID_HANDLE_VALUE;

	for (int i = 1; i <= argc - 1; ++i)
	{
		if (!(parseResult & ParseResultPID))
		{
			QString value;
			if (getArgValue(argv[i], ArgProcessID, &value))
			{
				parseResult |= ParseResultPID;
				pid = value.toUInt();
				continue;
			}
		}

		if (!(parseResult & ParseResultTID))
		{
			QString value;
			if (getArgValue(argv[i], ArgThreadID, &value))
			{
				parseResult |= ParseResultTID;
				tid = value.toUInt();
				continue;
			}
		}

		if (!(parseResult & ParseResultExp))
		{
			QString value;
			if (getArgValue(argv[i], ArgExceptionPointer, &value))
			{
				parseResult |= ParseResultExp;
				exp = value.toULongLong();
				continue;
			}
		}

		if (!(parseResult & ParseResultFullDump))
		{
			QString value;
			if (getArgValue(argv[i], ArgFullDump, &value))
			{
				parseResult |= ParseResultFullDump;
				fullDump = true;
				continue;
			}
		}
	}

	if ((parseResult & (ParseResultPID | ParseResultTID | ParseResultExp)) == 
		(ParseResultPID | ParseResultTID | ParseResultExp))
	{
		outDumpeeArg->PID = pid;
		outDumpeeArg->TID = tid;
		outDumpeeArg->Exp = exp;
		outDumpeeArg->FullDump = fullDump;
		outDumpeeArg->DumpFile = dumpFile;

		return true;
	}

	return false;
}

/*
dump某个进程
dumper.exe 
	-pid=%d		必须，指定进程id
	-tid=%d		必须，线程id，如果没有传0
	-exp=%d		必须，异常指针，如果没有传0
	-f			可选，指定fulldump，否则默认minidump

返回值
1	成功
0	参数个数、格式错误
-1	无法打开目标进程
-2	minidumpwritedump函数调用失败
-3  dbghelp.dll加载不成功
*/
int main(int argc, char* argv[])
{
	if (argc >= MandatoryArgCount)
	{
		DumpeeArg arg = {};
		if (Dumper::parseDumpeeArg(argc, argv, &arg))
		{
			int ret = Dumper::dump(arg);
			return ret;
		}		
	}

	return 0;
}

