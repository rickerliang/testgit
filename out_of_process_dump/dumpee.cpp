// dumpee.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>

#include <windows.h>

static wchar_t sarg[128] = {};
static wchar_t spid[16] = {};
static wchar_t sthreadid[16] = {};
static wchar_t sexceptionpointers[16] = {};
static wchar_t dumpfilehandle[16] = {};

LONG WINAPI ExceptionFilter(
	_EXCEPTION_POINTERS *ExceptionInfo
	)
{
	DWORD pid = ::GetProcessId(::GetCurrentProcess());
	_itow_s(pid, spid, 10);
	_itow_s(reinterpret_cast<int>(ExceptionInfo), sexceptionpointers, 10);
	_itow_s(static_cast<int>(::GetCurrentThreadId()), sthreadid, 10);
	std::wcout << L"dumpee pid " << spid << std::endl; 
	std::wcout << L"dumpee exp " << sexceptionpointers << std::endl; 
	std::wcout << L"dumpee tid " << sthreadid << std::endl; 

	SECURITY_ATTRIBUTES sa = {};
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	HANDLE file = ::CreateFileW(
		L"dump.dmp", GENERIC_READ | GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL,  NULL);
	_itow_s(reinterpret_cast<int>(file), dumpfilehandle, 10);
	std::wcout << L"dumpee file handle " << dumpfilehandle << std::endl;

	swprintf_s(
		sarg, L"%s -pid=%s -tid=%s -exp=%s -h=%s", L"dumper.exe", spid, 
		sthreadid, sexceptionpointers, dumpfilehandle);
	std::wcout << L"dumpee sarg " << sarg << std::endl; 

	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWDEFAULT;

	BOOL result = ::CreateProcessW(
		L"dumper.exe", sarg, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

	std::wcout << L"in uhef : " << result << std::endl;
	if (result)
	{
		std::wcout << L"wait" << std::endl;
		DWORD waitResult = ::WaitForSingleObject(pi.hProcess, INFINITE);
		std::wcout << L"wait result : " << waitResult << std::endl;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void RaiseException()
{
	std::wcout << L"raise exception "<< std::endl;
	int *p = nullptr;
	*p = 0;
}

int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep)
{
	std::wcout << L"in filter" << std::endl;
	return EXCEPTION_CONTINUE_SEARCH;
}

int _tmain(int argc, _TCHAR* argv[])
{
	::SetUnhandledExceptionFilter(ExceptionFilter);

	__try
	{
		RaiseException();
	}
	__except(filter(GetExceptionCode(), GetExceptionInformation()))
	{
		std::wcout << L"in handler" << std::endl;
	}
	
	return 0;
}

