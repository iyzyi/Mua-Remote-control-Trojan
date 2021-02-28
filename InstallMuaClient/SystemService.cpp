#include "SystemService.h"
#include <shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")


BOOL RegisterSystemService(WCHAR lpszDriverPath[]) {

	BOOL bRet = TRUE;
	WCHAR szName[MAX_PATH] = { 0 };

	::lstrcpy(szName, lpszDriverPath);
	// 过滤掉文件目录，获取文件名
	::PathStripPath(szName);

	SC_HANDLE shOSCM = NULL, shCS = NULL;
	SERVICE_STATUS ss;
	DWORD dwErrorCode = 0;
	BOOL bSuccess = FALSE;
	// 打开服务控制管理器数据库
	shOSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!shOSCM)
	{
		MessageBox(0, L"OpenSCManager", L"", 0);
		return FALSE;
	}

	// 创建服务
	// SERVICE_AUTO_START   随系统自动启动
	// SERVICE_DEMAND_START 手动启动
	shCS = ::CreateService(shOSCM, szName, szName,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
		SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL,
		lpszDriverPath, NULL, NULL, NULL, NULL, NULL);
	if (!shCS)
	{
		MessageBox(0, L"CreateService", L"", 0);
		return FALSE;
	}

	// 启动服务
	if (!::StartService(shCS, 0, NULL))
	{
		MessageBox(0, L"StartService", L"", 0);
		return FALSE;
	}

	// 关闭句柄
	if (shCS)
	{
		::CloseServiceHandle(shCS);
		shCS = NULL;
	}
	if (shOSCM)
	{
		::CloseServiceHandle(shOSCM);
		shOSCM = NULL;
	}

	return bRet;
}