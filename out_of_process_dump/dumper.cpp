// dumper.cpp : Defines the entry point for the console application.
//
#include "dumper.h"

#include <memory>

#include <QDir>
#include <QList>
#include <QFileInfo>
#include <QLibrary>
#include <Psapi.h>
#include <winternl.h>
#include <ntstatus.h>

#include "xmlreport.h"
#include <kso/appfeature/kappfeature.h>

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

bool CreateReportXml(
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

BOOL __stdcall MiniDumpCallback(
	PVOID CallbackParam,
	PMINIDUMP_CALLBACK_INPUT CallbackInput,
	PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
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

QString CreateDumpDir()
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

QString GetProcessName(HANDLE process)
{
	const size_t nameLen = 512;
	std::unique_ptr<wchar_t[]> name(new wchar_t[nameLen]);
	memset(name.get(), 0, nameLen * sizeof(name.get()[0]));
	::GetModuleBaseNameW(process, NULL, name.get(), nameLen);

	return QString::fromWCharArray(name.get());
}

HANDLE CreateDumpFile(const QString& processName, const QString& dumpDir)
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



bool GetArgValue(const char* arg, const char* argKey, QString* outArgValue)
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

bool ParseDumpeeArg(int argc, char* argv[], DumpeeArg* outDumpeeArg)
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
			if (GetArgValue(argv[i], ArgProcessID, &value))
			{
				parseResult |= ParseResultPID;
				pid = value.toUInt();
				continue;
			}
		}

		if (!(parseResult & ParseResultTID))
		{
			QString value;
			if (GetArgValue(argv[i], ArgThreadID, &value))
			{
				parseResult |= ParseResultTID;
				tid = value.toUInt();
				continue;
			}
		}

		if (!(parseResult & ParseResultExp))
		{
			QString value;
			if (GetArgValue(argv[i], ArgExceptionPointer, &value))
			{
				parseResult |= ParseResultExp;
				exp = value.toULongLong();
				continue;
			}
		}

		if (!(parseResult & ParseResultFullDump))
		{
			QString value;
			if (GetArgValue(argv[i], ArgFullDump, &value))
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

typedef NTSTATUS (__stdcall *pNtQueryInformationProcess)(
	HANDLE           ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID            ProcessInformation,
	ULONG            ProcessInformationLength,
	PULONG           ReturnLength
	);

// peb->ProcessParameters->CommandLine
QString GetProcessCommandLine(HANDLE process)
{
	QLibrary ntdll("Ntdll");
	ntdll.load();
	pNtQueryInformationProcess queryInfoProce = 
		reinterpret_cast<pNtQueryInformationProcess>(
		ntdll.resolve("NtQueryInformationProcess"));

	if (queryInfoProce == NULL)
	{
		return QString();
	}

	PROCESS_BASIC_INFORMATION info = {};
	ULONG queryLen = 0;
	if (STATUS_SUCCESS != queryInfoProce(
		process, ProcessBasicInformation, &info, sizeof(info), &queryLen))
	{
		return QString();
	}

	if (info.PebBaseAddress == NULL)
	{
		return QString();
	}
	PEB peb = {};
	SIZE_T pebReaded = 0;
	if (TRUE != ::ReadProcessMemory(
		process, info.PebBaseAddress, &peb, sizeof(peb), &pebReaded))
	{
		return QString();
	}

	if (peb.ProcessParameters == NULL)
	{
		return QString();
	}
	RTL_USER_PROCESS_PARAMETERS param = {};
	SIZE_T paramReaded = 0;
	if (TRUE != ::ReadProcessMemory(
		process, peb.ProcessParameters, &param, sizeof(param), &paramReaded))
	{
		return QString();
	}

	if (param.CommandLine.Buffer == NULL || param.CommandLine.Length == 0)
	{
		return QString();
	}
	// param.CommandLine.Length
	// Specifies the length, in bytes, of the string pointed to by the Buffer member, not including the terminating NULL character, if any.
	const size_t cmdLineLen = param.CommandLine.Length / 2 + 1;
	std::unique_ptr<wchar_t[]> cmdLine(new wchar_t[cmdLineLen]);
	memset(cmdLine.get(), 0, cmdLineLen * sizeof(cmdLine.get()[0]));
	SIZE_T cmdLineReaded = 0;
	if (TRUE != ::ReadProcessMemory(
		process, param.CommandLine.Buffer, cmdLine.get(), param.CommandLine.Length, 
		&cmdLineReaded))
	{
		return QString();
	}

	return QString::fromWCharArray(cmdLine.get());
}

typedef int (__stdcall *pQueryFeature)(int);

bool NeedMailFeedback()
{
	QLibrary kso("kso");
	pQueryFeature query = 
		reinterpret_cast<pQueryFeature>(kso.resolve("_kso_QueryFeatureState"));

	if (query)
	{
		return query(kaf_kso_MailFeedback) != 0;
	}

	return false;
}

QString GenerateCommandLine(HANDLE process, const QString& dir)
{
	QString cmdLine = GetProcessCommandLine(process);
	
	int crashCount = 0;
	if (!cmdLine.isEmpty())
	{
		QString mark("recover");
		int index = cmdLine.indexOf(mark);
		if (index >= 0)
			crashCount = cmdLine.mid(index + mark.length(), 1).toInt();
	}

	QString transErrorCmdLine;
	if (NeedMailFeedback())
	{
		transErrorCmdLine = QString(
			"-app=\"%1\" -dir=\"%2\" -ccrash=\"%3\" -mailback=\"%4\"")
			.arg(GetProcessName(process)).arg(dir).arg(crashCount).arg(1);
	}
	else
	{
		transErrorCmdLine = QString("-app=\"%1\" -dir=\"%2\" -ccrash=\"%3\"")
			.arg(GetProcessName(process)).arg(dir).arg(crashCount);
	}

	return transErrorCmdLine;
}

void SendErrorReport(const QString& cmdLine)
{
	QLibrary kso("kso");
	pQueryFeature query = 
		reinterpret_cast<pQueryFeature>(kso.resolve("_kso_QueryFeatureState"));

	if (query)
	{
		if (query(kaf_kso_NoSendErrorReport))
		{
			return ;
		}
	}

	::ShellExecuteW(NULL, NULL, L"transerr.exe", 
		cmdLine.toStdWString().c_str(), NULL, SW_SHOWNORMAL);
}

bool CopyDirectory(const QString& srcPath, const QString& dstPath)
{
	if (QDir(srcPath) == QDir(dstPath))
	{
		return true;
	}

	QDir parentDstDir(QFileInfo(dstPath).path());
	if (!parentDstDir.mkpath(QFileInfo(dstPath).fileName()))
	{
		return false;
	}

	QDir srcDir(srcPath);
	foreach(
		const QFileInfo &info, 
		srcDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) 
	{
		QString srcItemPath = srcPath + QDir::separator() + info.fileName();
		QString dstItemPath = dstPath + QDir::separator() + info.fileName();
		if (info.isDir()) 
		{
			if (!CopyDirectory(srcItemPath, dstItemPath)) 
			{
				return false;
			}
		} 
		else if (info.isFile()) 
		{
			if (QFile::exists(dstItemPath))
			{
				QFile::remove(dstItemPath);
			}
			if (!QFile::copy(srcItemPath, dstItemPath)) 
			{
				return false;
			}
		} 
	}
	return true;
}

void CopyLogToTempGUIDFolder(const QString& destFolder, const QString& appBaseName)
{
	QString tmpPath = QDir::toNativeSeparators(QDir::tempPath());
	QString logPath= QString("%1\\log_%2").arg(tmpPath).arg(appBaseName);

	QString toDir = QString("%1\\%2\\log_%3")
		.arg(tmpPath)
		.arg(appBaseName)
		.arg(appBaseName);

	CopyDirectory(logPath, toDir);
}
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

	QString processName = QFileInfo(GetProcessName(process.get())).baseName();
	QString dumpDir = CreateDumpDir();
	if (dumpeeArg.DumpFile == INVALID_HANDLE_VALUE)
	{
		dumpeeArg.DumpFile = CreateDumpFile(processName, dumpDir);
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
		callbackInfo.CallbackRoutine = MiniDumpCallback;
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
		CreateReportXml(
			process.get(), reinterpret_cast<PEXCEPTION_POINTERS>(dumpeeArg.Exp), 
			modules.get(), dumpDir);

		CopyLogToTempGUIDFolder(dumpDir, processName);

		SendErrorReport(GenerateCommandLine(process.get(), dumpDir));

		return 1;
	}

	return -2;
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
		if (ParseDumpeeArg(argc, argv, &arg))
		{
			int ret = Dumper::dump(arg);
			return ret;
		}		
	}

	return 0;
}

