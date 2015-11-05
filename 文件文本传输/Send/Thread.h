// �̶߳����װ
//
#ifndef _THREAD_INCLUDE_
#define _THREAD_INCLUDE_

class CThread
{
private:
	static DWORD WINAPI ThreadProc(LPVOID pVoid);
protected:
	BOOL	m_bTerminated; // �߳��Ƿ���ֹ�ı�־

	virtual void Execute(void) = 0;
public:
	HANDLE	m_hThread; // �߳̾��
	
	CThread(int nPriority = THREAD_PRIORITY_NORMAL);
	~CThread(void);

	void Resume(void);
	void Terminate(void);
	BOOL Terminated(void);
	HANDLE GetThreadHandle(void);
};

#endif // #ifndef _THREAD_INCLUDE_
