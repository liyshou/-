// 线程对象封装
//
#include "stdafx.h"
#include "Thread.h"

CThread::CThread(int nPriority)
{
	m_bTerminated = FALSE;

	DWORD dwThreadID;
	m_hThread = CreateThread(NULL, 0, ThreadProc, this, CREATE_SUSPENDED, &dwThreadID);
	SetThreadPriority(m_hThread, nPriority);
}

CThread::~CThread(void)
{
	CloseHandle(m_hThread);
	m_hThread = NULL;
}

DWORD CThread::ThreadProc(LPVOID pVoid)
{
	((CThread *)(pVoid))->Execute();

	return 0;
}

void CThread::Resume(void)
{
	ResumeThread(m_hThread);
}

void CThread::Terminate(void)
{
	m_bTerminated = TRUE;
}

BOOL CThread::Terminated(void)
{
	return m_bTerminated;
}

HANDLE CThread::GetThreadHandle(void)
{
	return m_hThread;
}
