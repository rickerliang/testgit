// dumper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <string>

#include <windows.h>
#include "dbghelp/dbghelp.h"

namespace
{
const wchar_t* const ArgProcessID = L"-pid=";
const wchar_t* const ArgThreadID = L"-tid=";
const wchar_t* const ArgExceptionPointer = L"-exp=";
const wchar_t* const ArgFullDump = L"-f";
const wchar_t* const ArgDumpFileHandle = L"-h=";

const int ParseResultPID = 0x1;
const int ParseResultTID = 0x2;
const int ParseResultExp = 0x4;
const int ParseResultFullDump = 0x8;
const int ParseResultDumpFileHandle = 0x10;

const int MandatoryArgCount = 5;

struct DumpeeArg
{
	DWORD PID;
	DWORD TID;
	DWORD Exp;
	HANDLE DumpFile;
	bool FullDump;
};

/* 带上
unloaded module list，检查注入dll
process thread data，记录peb，teb，特别是teb，记录线程栈大小及起始终止地址，用于栈破坏时辅佐栈重建
full memory info，记录堆地址段信息，用于检查地址属性，例如 !address 命令
*/
MINIDUMP_TYPE DumpTypeMini = 
	static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithUnloadedModules 
	| MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo);

/*
minidump基础上加上堆内存
*/
MINIDUMP_TYPE DumpTypeFull = 
	static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithUnloadedModules 
	| MiniDumpWithProcessThreadData | MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo);


BOOL dump(const DumpeeArg& arg)
{
	std::cout << "dumper: dumpee pid " << arg.PID << std::endl;
	std::cout << "dumper: dumpee exp " << arg.Exp << std::endl;
	std::cout << "dumper: dumpee tid " << arg.TID << std::endl;
	std::cout << "dumper: dumpee full dump " << arg.FullDump << std::endl;
	std::cout << "dumper: dumpee dump handle " << arg.DumpFile << std::endl;

	HANDLE process = 
		::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE, 
			FALSE, arg.PID);
	std::cout << "dumper: dumpee process handle : " << process << std::endl;
	if (process == NULL)
	{
		return -1;
	}
	
	MINIDUMP_EXCEPTION_INFORMATION info = {};
	info.ClientPointers = TRUE;
	info.ThreadId = arg.TID;
	info.ExceptionPointers = reinterpret_cast<PEXCEPTION_POINTERS>(arg.Exp);

	BOOL dumpSuccess = ::MiniDumpWriteDump(
		process, arg.PID, arg.DumpFile, 
		arg.FullDump ? DumpTypeFull : DumpTypeMini,
		(arg.Exp == 0 ? NULL : &info), NULL, NULL);
	std::cout << "dumper: dump result " << dumpSuccess << std::endl;

	::CloseHandle(process);

	if (dumpSuccess)
	{
		return 1;
	}

	return -2;
}

bool getArgValue(const wchar_t* arg, const wchar_t* argKey, std::wstring* outArgValue)
{
	if (arg == nullptr || argKey == nullptr || outArgValue == nullptr)
	{
		return false;
	}

	std::wstring s(arg);
	auto pos = s.find(argKey);
	if (pos != std::wstring::npos)
	{
		*outArgValue = s.substr(pos + std::wcslen(argKey), std::wstring::npos);
		return true;
	}

	return false;
}

bool parseDumpeeArg(int argc, wchar_t* argv[], DumpeeArg* outDumpeeArg)
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
	DWORD exp = -1;
	bool fullDump = false;
	HANDLE dumpFile = INVALID_HANDLE_VALUE;

	for (int i = 1; i <= argc - 1; ++i)
	{
		if (!(parseResult & ParseResultPID))
		{
			std::wstring value;
			if (getArgValue(argv[i], ArgProcessID, &value))
			{
				parseResult |= ParseResultPID;
				pid = _wtoi(value.c_str());
				continue;
			}
		}

		if (!(parseResult & ParseResultTID))
		{
			std::wstring value;
			if (getArgValue(argv[i], ArgThreadID, &value))
			{
				parseResult |= ParseResultTID;
				tid = _wtoi(value.c_str());
				continue;
			}
		}

		if (!(parseResult & ParseResultExp))
		{
			std::wstring value;
			if (getArgValue(argv[i], ArgExceptionPointer, &value))
			{
				parseResult |= ParseResultExp;
				exp = _wtoi(value.c_str());
				continue;
			}
		}

		if (!(parseResult & ParseResultFullDump))
		{
			std::wstring value;
			if (getArgValue(argv[i], ArgFullDump, &value))
			{
				parseResult |= ParseResultFullDump;
				fullDump = true;
				continue;
			}
		}

		if (!(parseResult & ParseResultDumpFileHandle))
		{
			std::wstring value;
			if (getArgValue(argv[i], ArgDumpFileHandle, &value))
			{
				parseResult |= ParseResultDumpFileHandle;
				dumpFile = reinterpret_cast<HANDLE>(_wtoi(value.c_str()));
				continue;
			}
		}
	}

	if ((parseResult & (ParseResultPID | ParseResultTID | ParseResultExp | ParseResultDumpFileHandle)) == 
		(ParseResultPID | ParseResultTID | ParseResultExp | ParseResultDumpFileHandle))
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
}

/*
dump某个进程
dumper.exe 
	-pid=%d		必须，指定进程id
	-tid=%d		必须，线程id，如果没有传0
	-exp=%d		必须，异常指针，如果没有传0
	-h=%d		必须，dump文件句柄，需要用可继承方式打开文件，程序不负责文件创建、打开、关闭
	-f			可选，指定fulldump，否则默认minidump

返回值
1	成功
0	参数个数、格式错误
-1	无法打开目标进程
-2	minidumpwritedump函数调用失败
*/
int _tmain(int argc, _TCHAR* argv[])
{
	std::cout << "dumper: start argc " << argc << std::endl;
	if (argc >= MandatoryArgCount)
	{
		std::cout << "dumper: do dump " << std::endl;
		DumpeeArg arg = {};
		if (parseDumpeeArg(argc, argv, &arg))
		{
			BOOL ret = dump(arg);
			return ret;
		}		
	}

	return 0;
}

