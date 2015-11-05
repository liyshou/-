//
// ϵͳ��չ
// ������: 2003.08.18
// ��ʷ:
// 1. create by alone on 2003.08.18
// 2. modified by alone on 2004.06.14: ��ӹر�Windows����
//
#include "stdafx.h"
#include "SysUtils.h"

#pragma comment(lib, "shlwapi.lib")

// �ж��ļ��Ƿ����
BOOL IsFileExists(char *pszPathName)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	
	hFind = FindFirstFile(pszPathName, &FindFileData);
	if(hFind == INVALID_HANDLE_VALUE)
		return FALSE;
	else
	{
		FindClose(hFind);
		return TRUE;
	}
}

// ��ȡ�ļ�·��
char *ExtractFilePath(char *pszSrc, char *pszDst)
{
	int i;

	ASSERT(pszSrc != NULL);
	ASSERT(pszDst != NULL);
	int nLen = strlen(pszSrc);
	ASSERT(nLen < MAX_PATH);


	for(i = nLen - 1; i >= 0; i-- )
	{
		if(pszSrc[i] == ':' || pszSrc[i] == '\\')
		{
			break;
		}
	}

	if(i == -1)
	{
		pszDst[0] = 0;
	}
	else
	{
		strncpy(pszDst, pszSrc, i + 1);
		pszDst[i + 1] = 0; // zero terminated;
	}

	return pszDst;
}

// �������Ŀ¼
BOOL ForceDirectories(char *pszDir)
{
	// ��֤�ַ����ĺϷ���
	ASSERT(pszDir != NULL);
	int nLen = strlen(pszDir);
	if(nLen == 0 || nLen >= MAX_PATH) return FALSE;

	char pszCurDir[MAX_PATH]; // ��ǰ��Ҫ������Ŀ¼
	char pszUpDir[MAX_PATH]; // ����һ���Ŀ¼

	// ȥβ
	strcpy(pszCurDir, pszDir);
	if(pszCurDir[nLen - 1] == '\\')
	{
		pszCurDir[nLen -1 ] = 0;
		nLen-- ;
	}
  
	if (nLen < 3 || PathFileExists(pszCurDir) || strcmp(ExtractFilePath(pszCurDir, pszUpDir), pszCurDir) == 0)
	{
		return TRUE;
	}

  return  ForceDirectories(pszUpDir) && CreateDirectory(pszCurDir, NULL);
}

// ��������Windows 9x
BOOL RebootWin9x()
{
	ExitWindowsEx(EWX_REBOOT, 0);
	return TRUE;
}

// ��������Windows NT/2000/XP
BOOL RebootWinNT()
{
	HANDLE	hToken = 0, hThread = 0;
	DWORD	dwRtn = 0;
	hThread =(HANDLE)AfxGetApp()->m_hThread;

	TOKEN_PRIVILEGES tp;
	LUID luid;
	tp.PrivilegeCount = 1;

	if (!::OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
//		MessageBox("Current proccess can't apply for the reboot priviledge!");
		return FALSE;
	}
	else
	{
		if (!LookupPrivilegeValue(NULL,		// lookup privilege on local system
			SE_SHUTDOWN_NAME,				// privilege to lookup 
			&luid))
		{
//			MessageBox("Current proccess can't apply for the reboot priviledge!");
			CloseHandle(hToken);
			return FALSE;
		}
		else
		{
			// receives LUID of privilege
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			
			// Enable the privilege or disable all privileges.							
			AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tp, &dwRtn);
			if (GetLastError() == ERROR_SUCCESS)
			{
				ExitWindowsEx(EWX_REBOOT, 0);
				CloseHandle(hToken);
				return TRUE;
			}
			else
			{
//				MessageBox("Current proccess can't apply for the reboot priviledge!");
				CloseHandle(hToken);
				return FALSE;
			}
		}
	}
}

// ������������ϵͳ
BOOL RebootWindows()
{
	// ��ȡ����ϵͳ�汾
	DWORD dwVersion = GetVersion();
	if (dwVersion < 0x80000000) // Windows NT/2000/XP
	{
		return RebootWinNT();
	}
	else // Windows 98
	{
		return RebootWin9x();
	}
}

// ���ó����Ƿ��ڲ���ϵͳ�������Զ�����
void SetAutoRun(BOOL bEnable)
{
	HKEY hKey;

	// �򿪼�
	if(RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey) !=ERROR_SUCCESS)
		return;

	if(bEnable) //����ʱ����,���ü�ֵ
	{
		char szModule[MAX_PATH];
		GetModuleFileName(NULL, szModule, MAX_PATH);
		RegSetValueEx(hKey, _T("VLPRModule"), 0, REG_SZ, (CONST BYTE *)szModule, strlen(szModule));
	}
	else //ɾ����ֵ
	{
		RegDeleteValue(hKey, _T("VLPRModule"));
	}
	RegCloseKey(hKey);
}


// �ر�Windows98
BOOL ShutDownWin98()
{
	ExitWindowsEx(EWX_SHUTDOWN, 0);
	return TRUE;
}

// �ر�Windows NT/2000/XP
BOOL ShutDownWinNT()
{
	HANDLE	hToken = 0, hThread = 0;
	DWORD	dwRtn = 0;
	hThread =(HANDLE)AfxGetApp()->m_hThread;

	TOKEN_PRIVILEGES tp;
	LUID luid;
	tp.PrivilegeCount = 1;

	if (!::OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
//		MessageBox("Current proccess can't apply for the reboot priviledge!");
		return FALSE;
	}
	else
	{
		if (!LookupPrivilegeValue(NULL,		// lookup privilege on local system
			SE_SHUTDOWN_NAME,				// privilege to lookup 
			&luid))
		{
//			MessageBox("Current proccess can't apply for the reboot priviledge!");
			CloseHandle(hToken);
			return FALSE;
		}
		else
		{
			// receives LUID of privilege
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			
			// Enable the privilege or disable all privileges.							
			AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tp, &dwRtn);
			if (GetLastError() == ERROR_SUCCESS)
			{
				ExitWindowsEx(EWX_SHUTDOWN, 0);
				CloseHandle(hToken);
				return TRUE;
			}
			else
			{
//				MessageBox("Current proccess can't apply for the reboot priviledge!");
				CloseHandle(hToken);
				return FALSE;
			}
		}
	}
}

// ɾ���ļ�
BOOL DeleteFileEx(char *szPathName, BOOL bAllowUndo)
{
	SHFILEOPSTRUCT	fo;
	char chPathName[MAX_PATH + 1];

	memset(chPathName, 0, MAX_PATH + 1);
	strcpy(chPathName, szPathName);

	memset(&fo, 0, sizeof(SHFILEOPSTRUCT));
	fo.wFunc = FO_DELETE;
	fo.pFrom = chPathName;
	if(bAllowUndo)
		fo.fFlags = FOF_ALLOWUNDO;
	fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
	fo.fAnyOperationsAborted = FALSE;
	
	return (SHFileOperation(&fo) == 0);
}

BOOL RenameFileEx(char *szOldPathName, char *szNewPathName)
{
	SHFILEOPSTRUCT fo;
	char chOldPathName[MAX_PATH + 1];
	char chNewPathName[MAX_PATH + 1];

	memset(chOldPathName, 0, MAX_PATH + 1);
	strcpy(chOldPathName, szOldPathName);

	memset(chNewPathName, 0, MAX_PATH + 1);
	strcpy(chNewPathName, szNewPathName);

	memset(&fo, 0, sizeof(SHFILEOPSTRUCT));
	fo.wFunc = FO_RENAME;
	fo.pFrom = chOldPathName;
	fo.pTo = chNewPathName;
	fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
	fo.fAnyOperationsAborted = FALSE;

	return (SHFileOperation(&fo) == 0);
}

BOOL MoveFileEx(char *szSrcPathName, char *szDstPathName)
{
	SHFILEOPSTRUCT fo;
	char chSrcPathName[MAX_PATH + 1];
	char chDstPathName[MAX_PATH + 1];

	memset(chSrcPathName, 0, MAX_PATH + 1);
	strcpy(chSrcPathName, szSrcPathName);

	memset(chDstPathName, 0, MAX_PATH + 1);
	strcpy(chDstPathName, szDstPathName);

	memset(&fo, 0, sizeof(SHFILEOPSTRUCT));
	fo.wFunc = FO_MOVE;
	fo.pFrom = chSrcPathName;
	fo.pTo = chDstPathName;
	fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
	fo.fAnyOperationsAborted = FALSE;

	return (SHFileOperation(&fo) == 0);
}

BOOL CopyFileEx(char *szSrcPathName, char *szDstPathName)
{
	SHFILEOPSTRUCT fo;
	char chSrcPathName[MAX_PATH + 1];
	char chDstPathName[MAX_PATH + 1];

	memset(chSrcPathName, 0, MAX_PATH + 1);
	strcpy(chSrcPathName, szSrcPathName);

	memset(chDstPathName, 0, MAX_PATH + 1);
	strcpy(chDstPathName, szDstPathName);

	memset(&fo, 0, sizeof(SHFILEOPSTRUCT));
	fo.wFunc = FO_COPY;
	fo.pFrom = chSrcPathName;
	fo.pTo = chDstPathName;
	fo.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
	fo.fAnyOperationsAborted = FALSE;

	return (SHFileOperation(&fo) == 0);
}

// �ж��ַ����Ƿ��ǺϷ����ļ���
// �ļ������ܰ������ַ���9���ַ�\/:*?"<>|
BOOL IsLegalFileName(char *szFileName)
{
	return (strpbrk(szFileName, "\\/:*?\"<>|") == NULL);
}