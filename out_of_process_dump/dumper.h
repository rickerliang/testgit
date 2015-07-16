#ifndef _DUMPER_H_
#define _DUMPER_H_

#include <QString>
#include <windows.h>
#include <dbghelp/dbghelp.h>

struct DumpeeArg
{
	DWORD PID;
	DWORD TID;
	DWORD Exp;
	HANDLE DumpFile;
	bool FullDump;
};

class Dumper
{
public:
	static BOOL WINAPI miniDumpCallback(
		__inout PVOID CallbackParam,
		__in    PMINIDUMP_CALLBACK_INPUT CallbackInput,
		__inout PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
		);
	static QString createDumpDir();
	static QString getProcessName(HANDLE process);
	static HANDLE createDumpFile(
		const QString& processName, const QString& dumpDir);
	static BOOL dump(const DumpeeArg& arg);
	static bool getArgValue(
		const wchar_t* arg, const wchar_t* argKey, std::wstring* outArgValue);
	static bool parseDumpeeArg(int argc, wchar_t* argv[], DumpeeArg* outDumpeeArg);
};

#endif