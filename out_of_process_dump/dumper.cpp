// dumper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <string>

#include <windows.h>
#include "dbghelp/dbghelp.h"

#ifndef MiniDumpIgnoreInaccessibleMemory
#define MiniDumpIgnoreInaccessibleMemory 0x00020000
#endif

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


void dump(const DumpeeArg& arg)
{
	std::cout << "dumper: dumpee pid " << arg.PID << std::endl;
	std::cout << "dumper: dumpee exp " << arg.Exp << std::endl;
	std::cout << "dumper: dumpee tid " << arg.TID << std::endl;
	std::cout << "dumper: dumpee full dump " << arg.FullDump << std::endl;
	std::cout << "dumper: dumpee dump handle " << arg.DumpFile << std::endl;

	HANDLE process = 
		::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE, 
			FALSE, arg.PID);
// 	HANDLE file = ::CreateFileW(
// 		L"dump.dmp", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
// 		FILE_ATTRIBUTE_NORMAL,  NULL);

	std::cout << "dumper: dumpee process handle : " << process << std::endl;
//	std::cout << "dumper: dumpee file handle : " << file << std::endl;
	
	MINIDUMP_EXCEPTION_INFORMATION info = {};
	info.ClientPointers = TRUE;
	info.ThreadId = arg.TID;
	info.ExceptionPointers = reinterpret_cast<PEXCEPTION_POINTERS>(arg.Exp);
	MINIDUMP_TYPE dumpType = 
		static_cast<MINIDUMP_TYPE>(
		arg.FullDump ? (MiniDumpNormal | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData | MiniDumpWithFullMemory | MiniDumpIgnoreInaccessibleMemory)
		: (MiniDumpNormal | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData));

	BOOL dumpSuccess = ::MiniDumpWriteDump(
		process, arg.PID, arg.DumpFile, 
		dumpType,
		(arg.Exp == 0 ? NULL : &info), NULL, NULL);
	std::cout << "dumper: dump result " << dumpSuccess << std::endl;

	::CloseHandle(process);
	::CloseHandle(arg.DumpFile);
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
dump某个进程，指定进程id(必须)，线程id(必须，如果没有传0)，异常指针(必须，如果没有传0)，dump文件句柄(必须，需要用可继承方式打开文件)，是否fulldump(可选)
dumper.exe -pid=%d -tid=%d -exp=%d -h=%d -f
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
			dump(arg);
		}		
	}

	return 0;
}

