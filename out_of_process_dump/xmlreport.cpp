#include "xmlreport.h"

#include <algorithm>
#include <memory>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

bool XmlReport::createExceptionRecordNode(
	HANDLE targetProcess, EXCEPTION_RECORD* pExceptionRecord, 
	ER_EXCEPTIONRECORD& outRecord)
{
	EXCEPTION_RECORD r = {};
	SIZE_T readed = 0;
	if (::ReadProcessMemory(targetProcess, pExceptionRecord, &r, sizeof(r), &readed))
	{
		return false;
	}

	const size_t nameLen = 512;
	std::unique_ptr<char[]> name(new char[nameLen]);
	memset(name.get(), 0, nameLen * sizeof(name.get()[0]));
	::GetModuleFileNameA(NULL, name.get(), nameLen);
	outRecord.ModuleName = name.get();
	
	outRecord.ExceptionCode = QString("0x%1").arg(r.ExceptionCode, 8, 16, QChar('0'));

	switch (r.ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		outRecord.ExceptionDescription = "EXCEPTION_ACCESS_VIOLATION";
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		outRecord.ExceptionDescription = "EXCEPTION_DATATYPE_MISALIGNMENT";
		break;
	case EXCEPTION_BREAKPOINT:
		outRecord.ExceptionDescription = "EXCEPTION_BREAKPOINT";
		break;
	case EXCEPTION_SINGLE_STEP:
		outRecord.ExceptionDescription = "EXCEPTION_SINGLE_STEP";
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		outRecord.ExceptionDescription = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		outRecord.ExceptionDescription = "EXCEPTION_FLT_DENORMAL_OPERAND";
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		outRecord.ExceptionDescription = "EXCEPTION_FLT_DIVIDE_BY_ZERO";
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		outRecord.ExceptionDescription = "EXCEPTION_FLT_INEXACT_RESULT";
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		outRecord.ExceptionDescription = "EXCEPTION_FLT_INVALID_OPERATION";
		break;
	case EXCEPTION_FLT_OVERFLOW :
		outRecord.ExceptionDescription = "EXCEPTION_FLT_OVERFLOW ";
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		outRecord.ExceptionDescription = "EXCEPTION_FLT_STACK_CHECK";
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		outRecord.ExceptionDescription = "EXCEPTION_FLT_UNDERFLOW";
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		outRecord.ExceptionDescription = "EXCEPTION_INT_DIVIDE_BY_ZERO";
		break;
	case EXCEPTION_INT_OVERFLOW:
		outRecord.ExceptionDescription = "EXCEPTION_INT_OVERFLOW";
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		outRecord.ExceptionDescription = "EXCEPTION_PRIV_INSTRUCTION";
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		outRecord.ExceptionDescription = "EXCEPTION_IN_PAGE_ERROR";
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		outRecord.ExceptionDescription = "EXCEPTION_ILLEGAL_INSTRUCTION";
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		outRecord.ExceptionDescription = "EXCEPTION_NONCONTINUABLE_EXCEPTION";
		break;
	case EXCEPTION_STACK_OVERFLOW:
		outRecord.ExceptionDescription = "EXCEPTION_STACK_OVERFLOW";
		break;
	case EXCEPTION_INVALID_DISPOSITION:
		outRecord.ExceptionDescription = "EXCEPTION_INVALID_DISPOSITION";
		break;
	case EXCEPTION_GUARD_PAGE:
		outRecord.ExceptionDescription = "EXCEPTION_GUARD_PAGE";
		break;
	case EXCEPTION_INVALID_HANDLE:
		outRecord.ExceptionDescription = "EXCEPTION_INVALID_HANDLE";
		break;
	default:
		outRecord.ExceptionDescription = "EXCEPTION_UNKNOWN";   // 未知的异常。
		break;
	}

	outRecord.ExceptionAddress = QString("0x%1").arg(
		reinterpret_cast<unsigned int>(r.ExceptionAddress), 8, 16, QChar('0'));

	return true;
}

bool XmlReport::createAdditionalInfoNode(ER_ADDITIONALINFO& outInfo)
{
	QString infoGuid;
	QString distSrc;
	{
		QSettings qReg(
			"HKEY_CURRENT_USER\\Software\\Kingsoft\\Office\\6.0\\Common",
			QSettings::NativeFormat);
		QVariant qVar;
		qVar = qReg.value(QString("infoGUID"), QVariant(""));
		infoGuid = qVar.toString();
	}

	{
		QSettings qRegDist(
			"Software\\Kingsoft\\Office\\6.0\\common",
			QSettings::NativeFormat);
		QVariant qVar;
		qVar = qRegDist.value(QString("DistSrc"), QVariant(""));
		distSrc = qVar.toString();
	}

	if (infoGuid.isEmpty() || distSrc.isEmpty())
	{
		return false;
	}

	outInfo.Guid = infoGuid;
	outInfo.DistSrc = distSrc;

	return true;
}

void XmlReport::createProcessorNode( ER_PROCESSOR& outProcessor )
{
	SYSTEM_INFO si = {};
	::GetSystemInfo(&si);

	// 设置 Architecture 属性。
	switch (si.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_INTEL";
		break;
	case PROCESSOR_ARCHITECTURE_MIPS:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_MIPS";
		break;
	case PROCESSOR_ARCHITECTURE_ALPHA:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_ALPHA";
		break;
	case PROCESSOR_ARCHITECTURE_PPC:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_PPC";
		break;
	case PROCESSOR_ARCHITECTURE_SHX:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_SHX";
		break;
	case PROCESSOR_ARCHITECTURE_ARM:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_ARM";
		break;
	case PROCESSOR_ARCHITECTURE_IA64:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_IA64";
		break;
	case PROCESSOR_ARCHITECTURE_ALPHA64:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_ALPHA64";
		break;
	case PROCESSOR_ARCHITECTURE_UNKNOWN:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_UNKNOWN";
		break;
	case PROCESSOR_ARCHITECTURE_AMD64:
		outProcessor.Architecture = "PROCESSOR_ARCHITECTURE_AMD64";
		break;
	default:
		outProcessor.Architecture = "Unknown";
	}

	// 设置 Level 属性。
	if (PROCESSOR_ARCHITECTURE_INTEL == si.wProcessorArchitecture)
	{
		switch (si.wProcessorLevel)
		{
		case 3:
			outProcessor.Level = "Intel 80386";
			break;
		case 4:
			outProcessor.Level = "Intel 80486";
			break;
		case 5:
			outProcessor.Level = "Intel Pentium";
			break;
		case 6:
			outProcessor.Level = "Intel Pentium Pro or Pentium II";
			break;
		default:
			outProcessor.Level = "Unknown";
		}
	}

	outProcessor.NumberOfProcessors = (int)si.dwNumberOfProcessors;
}

void XmlReport::createOSNode( ER_OPERATINGSYSTEM& outOS )
{
	OSVERSIONINFOA  oi = {};
	oi.dwOSVersionInfoSize = sizeof(oi);
	GetVersionExA(&oi);

	// 设置 MajorVersion 属性。
	outOS.MajorVersion = (int)oi.dwMajorVersion;

	// 设置 MinorVersion 属性。
	outOS.MinorVersion = (int)oi.dwMinorVersion;

	// 设置 BuildNumber  属性。
	outOS.BuildNumber  = (int)oi.dwBuildNumber;

	// 设置 CSDVersion   属性。
	outOS.CSDVersion   = oi.szCSDVersion;

	// 设置 OSLanguage   属性。
	LANGID localID = GetSystemDefaultLangID();
	switch (localID)
	{
	case 0x0404:
		outOS.OSLanguage = "Chinese (Taiwan)";
		break;
	case 0x0804:
		outOS.OSLanguage = "Chinese (PRC)";
		break;
	case 0x0c04:
		outOS.OSLanguage = "Chinese (Hong Kong SAR, PRC)";
		break;
	case 0x1004:
		outOS.OSLanguage = "Chinese (Singapore)";
		break;
	case 0x0411:
		outOS.OSLanguage = "Japanese";
		break;
	case 0x0409:
		outOS.OSLanguage = "English (United States)";
		break;
	case 0x0809:
		outOS.OSLanguage = "English (United Kingdom)";
		break;
	case 0x0c09:
		outOS.OSLanguage = "English (Australian)";
		break;
	case 0x1009:
		outOS.OSLanguage = "English (Canadian)";
		break;
	case 0x1409:
		outOS.OSLanguage = "English (New Zealand)";
		break;
	case 0x1809:
		outOS.OSLanguage = "English (Ireland)";
		break;
	case 0x1c09:
		outOS.OSLanguage = "English (South Africa)";
		break;
	case 0x2009:
		outOS.OSLanguage = "English (Jamaica)";
		break;
	case 0x2409:
		outOS.OSLanguage = "English (Caribbean)";
		break;
	case 0x2809:
		outOS.OSLanguage = "English (Belize)";
		break;
	case 0x2c09:
		outOS.OSLanguage = "English (Trinidad)";
		break;

	default:
		outOS.OSLanguage  = "Unknown";
	}
}

void XmlReport::createMemoryNode(ER_MEMORY& outMemory)
{
	MEMORYSTATUSEX ms = {};
	ms.dwLength = sizeof(ms);
	GlobalMemoryStatusEx(&ms);

	DWORDLONG sizeM = (1024 * 1024);
	// 设置 MemoryLoad    属性。
	outMemory.MemoryLoad = (int)ms.dwMemoryLoad;

	// 设置 TotalPhys     属性。
	outMemory.TotalPhys  = ms.ullTotalPhys / sizeM;

	// 设置 AvailPhys     属性。
	outMemory.AvailPhys  = ms.ullAvailPhys / sizeM;

	// 设置 TotalPageFile 属性。
	outMemory.TotalPageFile =  ms.ullTotalPageFile / sizeM;

	// 设置 AvailPageFile 属性。
	outMemory.AvailPageFile =  ms.ullAvailPageFile / sizeM;

	// 设置 TotalVirtual  属性。
	outMemory.TotalVirtual  =  ms.ullTotalVirtual / sizeM;

	// 设置 AvailVirtual  属性。
	outMemory.AvailVirtual  =  ms.ullAvailVirtual / sizeM;
}

void XmlReport::createModulesNode(
	const QList<MINIDUMP_MODULE_CALLBACK>& callList, ULONG64 exceptionAddress, 
	MODULE_LIST& outModules, QString& outExceptionModule)
{
	std::for_each(callList.begin(), callList.end(), 
		[exceptionAddress, &outModules, &outExceptionModule](
		const MINIDUMP_MODULE_CALLBACK& m)
	{
		ER_MODULE module;
		module.FullPath.fromStdWString(m.FullPath);

		module.BaseAddress = QString("0x%1").arg(m.BaseOfImage, 8, 16, QChar('0'));
		module.Size = QString("0x%1").arg(m.SizeOfImage, 8, 16, QChar('0'));

		if ((exceptionAddress >= m.BaseOfImage) &&
			(exceptionAddress < (m.BaseOfImage + m.SizeOfImage)))
		{
			outExceptionModule = module.FullPath;
		}

		QFileInfo fileInfo(module.FullPath);
		QDateTime lastMod = fileInfo.lastModified();
		module.TimeStamp = lastMod.toString("MM/dd/yyyy hh:mm:ss");

		module.FileVersion = 
			QString("%1.%2.%3.%4").
			arg(HIWORD(m.VersionInfo.dwFileVersionMS)).
			arg(LOWORD(m.VersionInfo.dwFileVersionMS)).
			arg(HIWORD(m.VersionInfo.dwFileVersionLS)).
			arg(LOWORD(m.VersionInfo.dwFileVersionLS));

		module.ProductVersion = 
			QString("%1.%2.%3.%4").
			arg(HIWORD(m.VersionInfo.dwProductVersionMS)).
			arg(LOWORD(m.VersionInfo.dwProductVersionMS)).
			arg(HIWORD(m.VersionInfo.dwProductVersionLS)).
			arg(LOWORD(m.VersionInfo.dwProductVersionLS));

		outModules.push_back(module);
	});
}

bool XmlReport::createInformationLog(
	const QString& dir, const ER_EXCEPTIONRECORD& exceptionRecord, 
	const ER_ADDITIONALINFO& info, const ER_PROCESSOR& processor, 
	const ER_OPERATINGSYSTEM& os, const ER_MEMORY& memory, const MODULE_LIST& modules)
{
	return false;
}


