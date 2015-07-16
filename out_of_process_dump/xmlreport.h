// -----------------------------------------------------------------------
//
// 文 件 名 ：xmlreport.h
// 创 建 者 ：梁永康
// 创建时间 ：2015-5
// 功能描述 ：创建dump的xml report，与kso error report相同，改为进程外创建
//
// -----------------------------------------------------------------------

#ifndef _XMLREPORT_H_
#define _XMLREPORT_H_

#include <QString>
#include <windows.h>
#include <dbghelp/dbghelp.h>

struct ER_EXCEPTIONRECORD
{
	QString ModuleName;
	QString ExceptionCode;
	QString ExceptionDescription;
	QString ExceptionAddress;
	QString ExceptionModuleName;
};

struct ER_ADDITIONALINFO
{
	QString Guid;
	QString DistSrc;
};

struct ER_PROCESSOR
{
	QString Architecture;
	QString Level;
	int         NumberOfProcessors;
};

struct ER_OPERATINGSYSTEM
{
	int         MajorVersion;
	int         MinorVersion;
	int         BuildNumber;
	QString CSDVersion;
	QString OSLanguage;
};

struct ER_MEMORY
{
	int MemoryLoad;        // 结果是一个百分比。
	int TotalPhys;         // 单位：MB
	int AvailPhys;         // 单位：MB
	int TotalPageFile;     // 单位：MB
	int AvailPageFile;     // 单位：MB
	int TotalVirtual;      // 单位：MB
	int AvailVirtual;      // 单位：MB
};

struct ER_MODULE
{
	QString FullPath;
	QString BaseAddress;
	QString Size;
	QString TimeStamp;
	QString FileVersion;
	QString ProductVersion;
};

typedef QList<ER_MODULE> MODULE_LIST;

class XmlReport
{
public:
	static bool createInformationLog(
		const QString& dir, const ER_EXCEPTIONRECORD& exceptionRecord, 
		const ER_ADDITIONALINFO& info, const ER_PROCESSOR& processor,
		const ER_OPERATINGSYSTEM& os, const ER_MEMORY& memory, 
		const MODULE_LIST& modules);

	// 获取崩溃进程的异常信息
	// 注意，异常记录指针EXCEPTION_RECORD是目标进程地址，函数内使用readprocessmemory获取其内容
	static bool createExceptionRecordNode(
		HANDLE targetProcess, EXCEPTION_RECORD* pExceptionRecord, 
		ER_EXCEPTIONRECORD& outRecord);
	static bool createAdditionalInfoNode(ER_ADDITIONALINFO& outInfo);
	static void createProcessorNode(ER_PROCESSOR& outProcessor);
	static void createOSNode(ER_OPERATINGSYSTEM& outOS);
	static void createMemoryNode(ER_MEMORY& outMemory);
	static void createModulesNode(
		const QList<MINIDUMP_MODULE_CALLBACK>& callList, 
		ULONG64 exceptionAddress, MODULE_LIST& outModules, QString& outExceptionModule);

};

#endif