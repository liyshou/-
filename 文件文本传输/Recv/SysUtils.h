//
// ϵͳ��չ
// ������: 2003.08.18
// ��ʷ:
// 1 create by alone on 2003.08.18
//
#ifndef _SYSUTILS_INCLUDE__
#define _SYSUTILS_INCLUDE__

#include "Shlwapi.h"

// �ж��ļ��Ƿ����
BOOL IsFileExists(char *pszPathName);
// �������Ŀ¼,�ɹ�����TRUE��ʶ�𷵻�FALSE
BOOL ForceDirectories(char *pszDir);
// ��չ�ļ�����
BOOL DeleteFileEx(char *szPathName, BOOL bAllowUndo = FALSE);
BOOL RenameFileEx(char *szOldPathName, char *szNewPathName);
BOOL MoveFileEx(char *szSrcPathName, char *szDstPathName);
BOOL CopyFileEx(char *szSrcPathName, char *szDstPathName);
// ������������ϵͳ
BOOL RebootWindows();
// ���ó����Ƿ��ڲ���ϵͳ�������Զ�����
void SetAutoRun(BOOL bEnable);
BOOL ShutDownWin98();
BOOL ShutDownWinNT();

BOOL IsLegalFileName(char *szFileName);

#endif // #ifndef _SYSUTILS_INCLUDE__
