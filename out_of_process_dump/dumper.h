#ifndef _DUMPER_H_
#define _DUMPER_H_

#include <windows.h>

struct DumpeeArg
{
	DWORD PID;
	DWORD TID;
	ULONG64 Exp;
	HANDLE DumpFile;
	bool FullDump;
};

class Dumper
{
public:
	static int dump(const DumpeeArg& arg);
};

#endif