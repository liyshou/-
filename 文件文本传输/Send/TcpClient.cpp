// TcpClient
// ���Է���������Ϣ�����Է����ļ���֧�ֶϵ�����
// XSoft
// Contact: hongxing777@msn.com or lovelypengpeng@eyou.com
//
#include "stdafx.h"
#include "TcpClient.h"

#pragma comment(lib, "Ws2_32.lib")

//////////////////////////////////////////////////////////////////////
// CTcpClient
//////////////////////////////////////////////////////////////////////
CTcpClient::CTcpClient(void *pNotifyObj)
{
	memset(&m_WSAData, 0, sizeof(WSADATA));
	m_pNotifyObj = pNotifyObj;
	m_hHideWnd = NULL;
	m_hSocket = INVALID_SOCKET;
	m_pClientSocketRecvThread = NULL;
	m_pSendFileThread = NULL;

	strcpy(m_szServerIpAddr, "");
	m_wPort = DFT_SOCKETPORT;
	m_dwFilePackageSize = DFT_FILEPACKAGESIZE;
	m_nWaitTimeOut = DFT_WAITTIMEOUT;
	m_nSendTimeOut = DFT_SENDTIMEOUT;
	m_dwProgressTimeInterval = DFT_PROGRESSTIMEINTERVAL;
	m_nSocketRecvThreadPriority = DFT_SOCKETRECVTHREADPRIORITY;
	m_nSendFileThreadPriority = DFT_SENDFILETHREADPRIORITY;

	// �¼�ָ��
	m_OnSocketClose = NULL;
	m_OnOneNetMsg = NULL;
	m_OnSendFileSucc = NULL;
	m_OnSendFileFail = NULL;
	m_OnSendFileProgress = NULL;

	StartupSocket(); // ��ʼ��windows socket
	RegisterClientSocketHideWndClass(); // �������ش�����
}

CTcpClient::~CTcpClient(void)
{
	Disconnect(); // �Ͽ�socket����
	
	CleanupSocket();
	UnregisterClass(STR_CLIENTSOCKETHIDEWNDCLASSNAME, AfxGetInstanceHandle());
}

void CTcpClient::SetServerIpAddr(char *szServerIpAddr)
{
	strcpy(m_szServerIpAddr, szServerIpAddr);
}

void CTcpClient::SetPort(WORD wPort)
{
	ASSERT(wPort > 1024);

	m_wPort = wPort;
}

void CTcpClient::SetFilePackageSize(DWORD dwFilePackageSize)
{
	ASSERT(dwFilePackageSize > 0 && dwFilePackageSize <= MAX_FILEPACKAESIZE);

	m_dwFilePackageSize = dwFilePackageSize;
}

void CTcpClient::SetWaitTimeOut(int nWaitTimeOut)
{
	ASSERT(nWaitTimeOut >= 1000);
	m_nWaitTimeOut = nWaitTimeOut;
}

void CTcpClient::SetSendTimeOut(int nSendTimeOut)
{
	ASSERT(nSendTimeOut >= 1000);
	m_nSendTimeOut = nSendTimeOut;
}

void CTcpClient::SetProgressTimeInterval(DWORD dwProgressTimeInterval)
{
	m_dwProgressTimeInterval = dwProgressTimeInterval;
}

void CTcpClient::SetSocketRecvThreadPriority(int nPriority)
{
	m_nSocketRecvThreadPriority = nPriority;
}

void CTcpClient::SetSendFileThreadPriority(int nPriority)
{
	m_nSendFileThreadPriority = nPriority;
}

void CTcpClient::SetOnSocketClose(SocketCloseFun OnSocketClose)
{
	m_OnSocketClose = OnSocketClose;
}

void CTcpClient::SetOnOneNetMsg(OneNetMsgFun OnOneNetMsg)
{
	m_OnOneNetMsg = OnOneNetMsg;
}

void CTcpClient::SetOnSendFileSucc(SendFileFun OnSendFileSucc)
{
	m_OnSendFileSucc = OnSendFileSucc;
}

void CTcpClient::SetOnSendFileFail(SendFileFailFun OnSendFileFail)
{
	m_OnSendFileFail = OnSendFileFail;
}

void CTcpClient::SetOnSendFileProgress(SendFileProgressFun OnSendFileProgress)
{
	m_OnSendFileProgress = OnSendFileProgress;
}

//  ��ʼ��windows socket
BOOL CTcpClient::StartupSocket(void)
{
	int nErrCode = 0;

	if(m_WSAData.wVersion == 0)
	{
		nErrCode = WSAStartup(0x0101, &m_WSAData);
		if(nErrCode != 0)
			memset(&m_WSAData, 0, sizeof(WSADATA));
	}

	return (nErrCode == 0);
}

// ���windows socket
void CTcpClient::CleanupSocket(void)
{
  if(m_WSAData.wVersion != 0)
    WSACleanup();

  memset(&m_WSAData, 0, sizeof(WSADATA));
}

void CTcpClient::InitSockAddr(SOCKADDR_IN  *pSockAddr, char *szBindIpAddr, WORD wPort)
{
	int AIp;

	memset(pSockAddr, 0, sizeof(SOCKADDR_IN));

	// Э����
	pSockAddr->sin_family = PF_INET;
    
	// �󶨵�ַ
	if(strcmp(szBindIpAddr, "") == 0)
		AIp = INADDR_NONE;
	else
		AIp = inet_addr(szBindIpAddr); // ת�����ʮ����ip��ַ

    if(AIp == INADDR_NONE)
		pSockAddr->sin_addr.S_un.S_addr = INADDR_ANY; // �󶨵���������ӿ�
    else
      pSockAddr->sin_addr.S_un.S_addr = AIp;

    pSockAddr->sin_port = htons(wPort); // �󶨶˿�
}

// ���ش�����Ϣ������
LRESULT CTcpClient::HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CTcpClient *pTcpClient = (CTcpClient *)lParam;
	
	switch(uMsg)
	{
	case WM_SOCKETCLOSE:// socket���ر�
		pTcpClient->HandleSocketCloseMsg();
		break;// socket���ر�
	case WM_SOCKETSENDERR: // socket���ͳ���
		pTcpClient->HandleSocketSendErrMsg();
		break;
	case WM_SOCKETRECVERR: // socket���ճ���
		pTcpClient->HandleSocketRecvErrMsg();
		break;
	case WM_RECVERACCEPTFILE: // ���շ������ļ�
		pTcpClient->HandleRecverAcceptFileMsg((DWORD)wParam);
		break;
	case WM_RECVERREFUSEFILE: // ���շ��ܾ��ļ�
		pTcpClient->HandleRecverRefuseFileMsg();
		break;
	case WM_RECVERSUCC: // ���շ������ļ��ɹ�
		pTcpClient->HandleRecverSuccMsg();
		break;
	case WM_RECVERFAIL: // ���շ�ʧ��
		pTcpClient->HandleRecverFailMsg();
		break;
	case WM_RECVERCANCEL: // ���շ�ȡ��
		pTcpClient->HandleRecverCancelMsg();
		break;
	case WM_SENDERFAIL: // ���շ�ʧ��
		pTcpClient->HandleSenderFailMsg();
		break;
	case WM_SENDFILEPROGRESS: // �ļ����ͽ���
		pTcpClient->HandleSendFileProgressMsg();
		break;
	case WM_WAITTIMEOUT:
		pTcpClient->HandleWaitTimeOutMsg();
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	return 0;
}

BOOL CTcpClient::RegisterClientSocketHideWndClass(void)
{
	WNDCLASS wc;

	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = (WNDPROC)HideWndProc;
	wc.hInstance = AfxGetInstanceHandle();
	wc.lpszClassName = STR_CLIENTSOCKETHIDEWNDCLASSNAME;

	return (RegisterClass(&wc) != 0);
}

// ������������
BOOL CTcpClient::AllocateWindow(void)
{
	if(m_hHideWnd == NULL)
	{
		m_hHideWnd = CreateWindow(STR_CLIENTSOCKETHIDEWNDCLASSNAME, NULL, WS_POPUP,  
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, AfxGetInstanceHandle(), this);
	}

	return (m_hHideWnd != NULL);
}

// �ͷ���������
void CTcpClient::DeallocateWindow(void)
{
	if(m_hHideWnd != NULL)
	{
		DestroyWindow(m_hHideWnd);
		m_hHideWnd = NULL;
	}
}

// ���ӷ����
BOOL CTcpClient::Connect(void)
{
	u_long			ulTmp;
	SOCKADDR_IN		SockAddr;
	
	Disconnect(); // �Ͽ�����

	if(!AllocateWindow())
		return FALSE;
	
	// ����socket
	m_hSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
	if(m_hSocket == INVALID_SOCKET)
		goto ErrEntry;
	
	// ����socketΪ������ʽ
	ulTmp = 0;
	if(ioctlsocket(m_hSocket, FIONBIO, &ulTmp) != 0)
		goto ErrEntry;

	// ����socket���ͳ�ʱ
	if(setsockopt(m_hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&m_nSendTimeOut, 
		sizeof(m_nSendTimeOut)) != 0)
		goto ErrEntry;
	
	// ���ӷ����
	InitSockAddr(&SockAddr, m_szServerIpAddr, m_wPort);
	if(connect(m_hSocket, (sockaddr *)&SockAddr, sizeof(SOCKADDR_IN)) != 0)
		goto ErrEntry;
	
	// ���������߳�
	m_pClientSocketRecvThread = new CClientSocketRecvThread(this, m_nSocketRecvThreadPriority);
	m_pClientSocketRecvThread->SetSocketHandle(m_hSocket);
	m_pClientSocketRecvThread->SetHideWndHandle(m_hHideWnd);
	m_pClientSocketRecvThread->Resume();
	
	return TRUE;
ErrEntry:
	Disconnect();
	return FALSE;
}

// �Ͽ�����
void CTcpClient::Disconnect(void)
{
	// ����ļ������߳�
	if(m_pSendFileThread != NULL)
	{
		m_pSendFileThread->CloseSendFileThreadMute();
		delete m_pSendFileThread;
		m_pSendFileThread = NULL;
	}

	DeallocateWindow(); // ���ٴ���, ������Ӧ������Ϣ

	// �ر�socket����
	if(m_hSocket != INVALID_SOCKET)
	{
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}
	
	// ���ٽ����߳�
	if(m_pClientSocketRecvThread != NULL)
	{
		m_pClientSocketRecvThread->Terminate();
		WaitForSingleObject(m_pClientSocketRecvThread->GetThreadHandle(), INFINITE);
		delete m_pClientSocketRecvThread;
		m_pClientSocketRecvThread = NULL;
	}
}

// ���ص�ǰsocket����״̬
BOOL CTcpClient::IsConnect(void)
{
	return (m_hSocket != INVALID_SOCKET);
}

// �ж��Ƿ��ڷ���
BOOL CTcpClient::IsSending(void)
{
	return (m_pSendFileThread != NULL);
}

BOOL CTcpClient::SendFile(char *szPathName)
{
	if(!IsConnect())
		return FALSE; // socket������δ��
	if(IsSending())
		return FALSE; // ���ڷ����ļ�

	// �����ļ�������������
	m_pSendFileThread = new CSendFileThread(this, m_nSendFileThreadPriority);
	m_pSendFileThread->SetHideWndHandle(m_hHideWnd);
	m_pSendFileThread->SetSocketHandle(m_hSocket);
	m_pSendFileThread->SetFilePackageSize(m_dwFilePackageSize);
	m_pSendFileThread->SetWaitTimeOut(m_nWaitTimeOut);
	m_pSendFileThread->SetProgressTimeInterval(m_dwProgressTimeInterval);

	if(!m_pSendFileThread->PrepareSendFile(szPathName))
	{
		delete m_pSendFileThread;
		m_pSendFileThread = NULL;
		return FALSE;
	}

	m_pSendFileThread->Resume(); // ��ʼ����
	
	return TRUE;
}

// ȡ�������ļ�
void CTcpClient::CancelSendFile(void)
{
	if(IsSending())
	{
		m_pSendFileThread->CloseSendFileThreadCancel();
		delete m_pSendFileThread;
		m_pSendFileThread = NULL;
	}
}

// ����������Ϣ
BOOL CTcpClient::SendNetMsg(char *Msg, int nMsgLen)
{
	char szMsgLen[6];
	int nTotalLen;
	int nErr;
	int nSent = 0;

	ASSERT(nMsgLen <= MAX_NETMSGPACKAGESIZE);

	Msg[0] = MSG_TAG;
	sprintf(szMsgLen, "%05d", nMsgLen);
	strncpy(&(Msg[1]), szMsgLen, 5);

	nTotalLen = nMsgLen + 6;
	while(nSent < nTotalLen)
	{
		nErr = send(m_hSocket, &(Msg[nSent]), nTotalLen - nSent, 0);
		if(nErr == SOCKET_ERROR)
			return FALSE;
		nSent += nErr;
	}

	return TRUE;
}

// ����socket���ͳ�������Ϣ
// socket���ͳ����ǿ�ƹر�socket����
void CTcpClient::HandleSocketSendErrMsg(void)
{
	if(IsSending() && m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_SOCKETSENDERR);

	if(IsConnect() && m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_SOCKETSENDERR);

	Disconnect();
}

// ����socket���ճ�������Ϣ
void CTcpClient::HandleSocketRecvErrMsg(void)
{
	if(IsSending() && m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_SOCKETRECVERR);

	if(IsConnect () && m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_SOCKETRECVERR);

	Disconnect();
}

// ����socket�رմ�����Ϣ
void CTcpClient::HandleSocketCloseMsg(void)
{
	if(IsSending() && m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_SOCKETCLOSE);
	
	if(IsConnect() && m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_PEERCLOSE);

	Disconnect(); // �Ͽ�socket����
}

// ����������������Ϣ
void CTcpClient::HandleOneNetMsg(char *Msg, int nMsgLen)
{
	if(m_OnOneNetMsg != NULL)
		m_OnOneNetMsg(m_pNotifyObj, Msg, nMsgLen);
}

// ������ն˽����ļ�������Ϣ
void CTcpClient::HandleRecverAcceptFileMsg(DWORD dwRecvedBytes)
{
	if(!IsSending())
		return;
	
	// ���������ļ��߳�
	m_pSendFileThread->TrigAcceptFile(dwRecvedBytes);
}

// ������ն˾ܾ��ļ�������Ϣ
void CTcpClient::HandleRecverRefuseFileMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // �ر��ļ������߳�
	
	if(m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_RECVERREFUSEFILE);

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// ������շ������ļ��ɹ�������Ϣ
void CTcpClient::HandleRecverSuccMsg(void)
{
	if(!IsSending())
		return;
	
	// ���յ��ô�����Ϣʱ�ļ������Ѿ�������
	if(!m_pSendFileThread->IsFileAccepted())
		return;

	m_pSendFileThread->TrigRecvFileSucc();
	WaitForSingleObject(m_pSendFileThread->GetThreadHandle(), INFINITE); // �ȴ��߳̽���

	if(m_OnSendFileSucc != NULL)
		m_OnSendFileSucc(m_pNotifyObj, m_pSendFileThread->GetPathName());

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// ������շ�ʧ�ܴ�����Ϣ
void CTcpClient::HandleRecverFailMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // �ر��ļ������߳�

	if(m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_RECVERFAIL);

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// ������շ�ȡ���ļ����մ�����Ϣ
void CTcpClient::HandleRecverCancelMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // �ر��ļ������߳�

	if(m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_RECVERCANCEL);

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// �����ļ����ͽ��ȴ�����Ϣ
void CTcpClient::HandleSendFileProgressMsg(void)
{
	DWORD dwSentBytes; // �ѷ����ֽ���
	DWORD dwFileSize; // �ļ���С

	if(!IsSending())
		return;

	if(m_OnSendFileProgress != NULL)
	{
		dwSentBytes = m_pSendFileThread->GetSentBytes();
		dwFileSize = m_pSendFileThread->GetFileSize();
		m_OnSendFileProgress(m_pNotifyObj, dwSentBytes, dwFileSize);
	}
}

// �����ļ�����ʧ�ܴ�����Ϣ 
void CTcpClient::HandleSenderFailMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // �ر��ļ������߳�

	if(m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_SENDERFAIL);

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// ����ȴ���ʱ������Ϣ
void CTcpClient::HandleWaitTimeOutMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // �ر��ļ������߳�

	if(m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_WAITTIMEOUT);

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

//////////////////////////////////////////////////////////////////////
// CSendFileThread
//////////////////////////////////////////////////////////////////////
CSendFileThread::CSendFileThread(CTcpClient *pTcpClient, int nPriority): CThread(nPriority)
{
	m_pTcpClient = pTcpClient;
	m_hHideWnd = NULL;
	m_hSocket = INVALID_SOCKET;
	m_dwFilePackageSize = DFT_FILEPACKAGESIZE;
	m_nWaitTimeOut = DFT_WAITTIMEOUT;
	m_dwProgressTimeInterval = DFT_PROGRESSTIMEINTERVAL;

	m_hCloseThreadMute = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hCloseThreadCancel = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hAcceptFile = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hRecvFileSucc = CreateEvent(NULL, TRUE, FALSE, NULL);

	// multi read single write variants
	strcpy(m_szPathName, "");
	m_dwSentBytes = 0;
	m_dwFileSize = 0;
	m_dwLastProgressTick = GetTickCount();
}

CSendFileThread::~CSendFileThread(void)
{
	CloseHandle(m_hCloseThreadMute);
	CloseHandle(m_hCloseThreadCancel);
	CloseHandle(m_hAcceptFile);
	CloseHandle(m_hRecvFileSucc);
}

void CSendFileThread::SetHideWndHandle(HWND hHideWnd)
{
	m_hHideWnd = hHideWnd;
}

void CSendFileThread::SetSocketHandle(SOCKET hSocket)
{
	m_hSocket = hSocket;
}

void CSendFileThread::SetFilePackageSize(DWORD dwFilePackageSize)
{
	m_dwFilePackageSize = dwFilePackageSize;
}

void CSendFileThread::SetWaitTimeOut(int nWaitTimeOut)
{
	m_nWaitTimeOut = nWaitTimeOut;
}

void CSendFileThread::SetProgressTimeInterval(DWORD dwProgressTimeInterval)
{
	m_dwProgressTimeInterval = dwProgressTimeInterval;
}

char *CSendFileThread::GetPathName(void)
{
	return m_szPathName;
}

DWORD CSendFileThread::GetSentBytes(void)
{
	return m_dwSentBytes;
}

DWORD CSendFileThread::GetFileSize(void)
{
	return m_dwFileSize;
}

void CSendFileThread::ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	while(!m_bTerminated)
	{
		if(PostMessage(m_hHideWnd, Msg, wParam, lParam))
			return;
		if(GetLastError() != ERROR_NOT_ENOUGH_QUOTA)
			return;
		Sleep(1000);
	}
}

// �߳�ִ�к���
void CSendFileThread::Execute(void)
{
	ASSERT(m_hHideWnd != NULL);
	ASSERT(m_hSocket != INVALID_SOCKET);

	// �ȴ����շ������ļ�
	HANDLE hWaitAcceptFileEvents[3];
	hWaitAcceptFileEvents[0] = m_hCloseThreadMute;
	hWaitAcceptFileEvents[1] = m_hCloseThreadCancel;
	hWaitAcceptFileEvents[2] = m_hAcceptFile;
	
	// �ȴ����շ������ļ��ɹ�
	HANDLE hWaitRecvFileSuccEvents[3]; 
	hWaitRecvFileSuccEvents[0] = m_hCloseThreadMute;
	hWaitRecvFileSuccEvents[1] = m_hCloseThreadCancel;
	hWaitRecvFileSuccEvents[2] = m_hRecvFileSucc;

	// �������ļ�
	if(!SendRequestSendFileNetMsg())
	{
		ResetSendFile();
		ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
	}

	// �ȴ����շ������ļ�, һ�����շ������ļ�����ʼ�����ļ�����
	switch(WaitForMultipleObjects(3, hWaitAcceptFileEvents, FALSE, m_nWaitTimeOut))
	{
	case WAIT_OBJECT_0: // �ر��߳�
		ResetSendFile();
		return;
		break;
	case WAIT_OBJECT_0 + 1: // ���Ͷ�ȡ�������̹߳ر�
		ResetSendFile();
		if(!SendSenderCancelNetMsg())
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	case WAIT_OBJECT_0 + 2: // ���ն˽����ļ�
		break;
	case WAIT_TIMEOUT: // �ȴ���ʱ
		ResetSendFile();
		ForcePostMessage(WM_WAITTIMEOUT, 0, (LPARAM)m_pTcpClient);
		if(!SendSenderFailNetMsg())
			ForcePostMessage(WM_SOCKETRECVERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	default: // ����
		ResetSendFile();
		ForcePostMessage(WM_SENDERFAIL, 0, (LPARAM)m_pTcpClient);
		if(!SendSenderFailNetMsg())
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	}

	// �����ļ�����
	SendFileData();

	// �ȴ����շ������ļ��ɹ�
	switch(WaitForMultipleObjects(3, hWaitRecvFileSuccEvents, FALSE, m_nWaitTimeOut))
	{
	case WAIT_OBJECT_0: // �ر��߳�
		ResetSendFile();
		return;
		break;
	case WAIT_OBJECT_0 + 1: // ���Ͷ�ȡ�������̹߳ر�
		ResetSendFile();
		if(!SendSenderCancelNetMsg())
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	case WAIT_OBJECT_0 + 2: // ���ն˽����ļ��ɹ�
		ResetSendFile();
		break;
	case WAIT_TIMEOUT: // �ȴ���ʱ
		ResetSendFile();
		ForcePostMessage(WM_WAITTIMEOUT, 0, (LPARAM)m_pTcpClient);
		if(!SendSenderFailNetMsg())
			ForcePostMessage(WM_SOCKETRECVERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	default: // ����
		ResetSendFile();
		ForcePostMessage(WM_SENDERFAIL, 0, (LPARAM)m_pTcpClient);
		if(!SendSenderFailNetMsg())
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	}
}

// ׼�������ļ�
BOOL CSendFileThread::PrepareSendFile(char *szPathName)
{
	// �򿪴����͵��ļ�
	if (!m_File.Open(szPathName, CFile::modeRead | CFile::shareExclusive))
		return FALSE;
	
	// ��ȡ�ļ���С
	try
	{
		m_dwFileSize = m_File.GetLength();
	}
	catch(...)
	{
		m_File.Abort();
		return FALSE;
	}

	strcpy(m_szPathName, szPathName); // �����ļ���
	m_dwSentBytes = 0; // ��ʼ���ѷ����ֽ���

	return TRUE;
}

// ��λ�ļ������߳�
void CSendFileThread::ResetSendFile(void)
{
	// �ر��ļ�
	if(m_File.m_hFile != CFile::hFileNull)
	{
		m_File.Close();
	}
}

// ����ļ��Ƿ��Ѿ������ն˽���
BOOL CSendFileThread::IsFileAccepted(void)
{
	return (WaitForSingleObject(m_hAcceptFile, 0) == WAIT_OBJECT_0);
}

// �����ļ�����
void CSendFileThread::SendFileData(void)
{
	DWORD dwTickCount;
	char Buf[MAX_NETMSGPACKAGESIZE + 6]; // ���ͻ�����
	int nBytes;

	// ��Ϣͷ
	strncpy(&(Buf[6]), "-19", 3);

	// ���ͽ���
	ForcePostMessage(WM_SENDFILEPROGRESS, 0, (LPARAM)m_pTcpClient);

	while(!m_bTerminated && m_dwSentBytes < m_dwFileSize)
	{
		// ��ȡ�ļ����ݰ�
		nBytes = min(m_dwFilePackageSize, m_dwFileSize - m_dwSentBytes);
		try
		{
			m_File.Seek(m_dwSentBytes, CFile::begin);
			m_File.Read(&(Buf[9]), nBytes);
		}
		catch(...)
		{
			ResetSendFile();
			ForcePostMessage(WM_SENDERFAIL, 0, (LPARAM)m_pTcpClient);
			if(!SendSenderFailNetMsg())
				ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);

			return;
		}

		// ����
		if(!m_pTcpClient->SendNetMsg(Buf, 3 + nBytes))
		{
			ResetSendFile();
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);

			return;
		}

		m_dwSentBytes += nBytes;

		// ���ͽ���
		dwTickCount = GetTickCount();
		if(dwTickCount < m_dwLastProgressTick) m_dwLastProgressTick = 0;
		if(dwTickCount - m_dwLastProgressTick > m_dwProgressTimeInterval || m_dwSentBytes >= m_dwFileSize)
		{
			ForcePostMessage(WM_SENDFILEPROGRESS, 0, (LPARAM)m_pTcpClient); // �ļ����ս���
			m_dwLastProgressTick = dwTickCount;
		}
		else
			Sleep(0); // ǿ��windows�����߳�
	}
}

// �ر��ļ������߳�
// �������κ�������Ϣ�����ն�
void CSendFileThread::CloseSendFileThreadMute(void)
{
	Terminate();
	SetEvent(m_hCloseThreadMute); // �رշ����ļ��߳�
	WaitForSingleObject(m_hThread, INFINITE); // �ȴ��߳̽���
}

// �ر��ļ������߳�
// ����ȡ�������ļ�������Ϣ�����ն�
void CSendFileThread::CloseSendFileThreadCancel(void)
{
	Terminate();
	SetEvent(m_hCloseThreadCancel); // �رշ����ļ��߳�
	WaitForSingleObject(m_hThread, INFINITE); // �ȴ��߳̽���
}

// �����������ļ�������Ϣ
BOOL CSendFileThread::SendRequestSendFileNetMsg(void)
{
	int nFileNameLen;
	char Buf[19 + MAX_PATH];
	char szFileSize[11];
	CString strFileName = m_File.GetFileName();
	nFileNameLen = strFileName.GetLength();

	sprintf(szFileSize, "%010d", m_File.GetLength());
	strncpy(&(Buf[6]), "-16", 3);
	strncpy(&(Buf[9]), szFileSize, 10);
	strncpy(&(Buf[19]), strFileName, nFileNameLen);

	return m_pTcpClient->SendNetMsg(Buf, 13 + nFileNameLen);
}

// ����ȡ�������ļ�������Ϣ
BOOL CSendFileThread::SendSenderCancelNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-21", 3);
	return m_pTcpClient->SendNetMsg(Msg, 3);
}

// ���ͷ����ļ�ʧ��������Ϣ
BOOL CSendFileThread::SendSenderFailNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-23", 3);
	return m_pTcpClient->SendNetMsg(Msg, 3);
}

// ���ն˽����ļ�
void CSendFileThread::TrigAcceptFile(DWORD dwRecvedBytes)
{
	// ������Ϣ�ĺϷ���
	if(WaitForSingleObject(m_hAcceptFile, 0) == WAIT_OBJECT_0)
		return;

	m_dwSentBytes = dwRecvedBytes;
	SetEvent(m_hAcceptFile);
}

// ���ն˽����ļ��ɹ�
void CSendFileThread::TrigRecvFileSucc(void)
{
	if(WaitForSingleObject(m_hRecvFileSucc, 0) == WAIT_OBJECT_0)
		return;

	SetEvent(m_hRecvFileSucc);
}

//////////////////////////////////////////////////////////////////////
// CClientSocketRecvThread
//////////////////////////////////////////////////////////////////////
CClientSocketRecvThread::CClientSocketRecvThread(CTcpClient *pTcpClient, int nPriority):
CThread(nPriority)
{
	m_pTcpClient = pTcpClient;
	m_hSocket = INVALID_SOCKET;
	m_nRecvBufSize = 0;
	m_hHideWnd = NULL;
}

CClientSocketRecvThread::~CClientSocketRecvThread(void)
{
}

void CClientSocketRecvThread::SetSocketHandle(SOCKET hSocket)
{
	m_hSocket = hSocket;
}

void CClientSocketRecvThread::SetHideWndHandle(HWND hHideWnd)
{
	m_hHideWnd = hHideWnd;
}

void CClientSocketRecvThread::ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	while(!m_bTerminated)
	{
		if(PostMessage(m_hHideWnd, Msg, wParam, lParam))
			return;
		if(GetLastError() != ERROR_NOT_ENOUGH_QUOTA)
			return;
		Sleep(1000);
	}
}

// �߳�ִ����
void CClientSocketRecvThread::Execute(void)
{
	char Buf[SOCKET_RECVBUFSIZE]; // ���ջ�����
	int nErr;

	while(!m_bTerminated)
	{
		nErr = recv(m_hSocket, Buf, SOCKET_RECVBUFSIZE, 0);
		if(nErr == SOCKET_ERROR) // �д�����
		{
			nErr = WSAGetLastError();
			if(nErr == WSAECONNRESET) // TODO: ���æ������������ֵ������socket��λ
			{
				// socket���ӱ�peer�˹رգ����·��λ�ȴ��󣬳��ִ����͵Ĵ���ʱsocket���뱻�رա�
				ForcePostMessage(WM_SOCKETCLOSE, 0, (LPARAM)m_pTcpClient);
				break;
			}
			else
			{
				// ������socket���󣬽������ݳ���
				ForcePostMessage(WM_SOCKETRECVERR, 0, (LPARAM)m_pTcpClient);
			}
		}
		else if(nErr == 0) // socket�������ر�
		{
			ForcePostMessage(WM_SOCKETCLOSE, 0, (LPARAM)m_pTcpClient);
			break;
		}
		else // ���յ�����
		{
			DoRecv(Buf, nErr); // ������յ�������
		}
	}; // while
}

// ��������Ϣ��װ����,Ȼ�������Ӧ�Ĵ�����
void CClientSocketRecvThread::DoRecv(char *Buf, int nBytes)
{
	int i;
	char szMsgLen[6];
	int nMsgLen; // ��Ϣ����
	int nTotalLen;

	// �������ݵ����ջ�����
	if(m_nRecvBufSize + nBytes > NETMSG_RECVBUFSIZE)
		m_nRecvBufSize = 0; // ��ս��ջ������е�����
	memcpy(&(m_RecvBuf[m_nRecvBufSize]), Buf, nBytes);
	m_nRecvBufSize += nBytes;

	while(!m_bTerminated)
	{
		if(m_nRecvBufSize == 0) return; // ��������������,�˳�
		
		// �������е�һ���ֽڱ�������Ϣ��־
		if(m_RecvBuf[0] != MSG_TAG) // ��һ���ֽڲ�����Ϣ��־
		{
			for(i = 0; i < m_nRecvBufSize; i++)
				if(m_RecvBuf[i] == MSG_TAG) break;
			m_nRecvBufSize -= i;
			memcpy(m_RecvBuf, &(m_RecvBuf[i]), m_nRecvBufSize);
		}

		// ��ȡ��Ϣ�����ֽ���
		if(m_nRecvBufSize < 6) return; // ��Ϣ����δ��������,�˳�
		strncpy(szMsgLen, &(m_RecvBuf[1]), 5);
		szMsgLen[5] = 0;
		nMsgLen = atoi(szMsgLen);
		nTotalLen = 1 + 5 + nMsgLen;

		// �ж���Ϣ�Ƿ���������
		if(nTotalLen > m_nRecvBufSize) return; // ��Ϣû�н�������
		HandleRecvedNetMsg(&(m_RecvBuf[6]), nMsgLen); // �����������Ϣ
		m_nRecvBufSize -= nTotalLen;
		memcpy(m_RecvBuf, &(m_RecvBuf[nTotalLen]), m_nRecvBufSize); // ����Ϣ��������ȥ������Ϣ
	};
}


// ������յ���������Ϣ
void CClientSocketRecvThread::HandleRecvedNetMsg(char *Msg, int nMsgLen)
{
	char szCmd[4];
	int nCmd;

	strncpy(szCmd, Msg, 3);
	szCmd[3] = 0;
	nCmd = atoi(szCmd);

	switch(nCmd)
	{
	case -17: // ���ն˽����ļ�
		{
			DWORD dwSentBytes;
			char szSentBytes[11];

			strncpy(szSentBytes, &(Msg[3]), 10);
			szSentBytes[10] = 0;
			dwSentBytes = atoi(szSentBytes);
			ForcePostMessage(WM_RECVERACCEPTFILE, dwSentBytes, (LPARAM)m_pTcpClient);
		}
		break;
	case -18: // ���ն˾ܾ������ļ�
		ForcePostMessage(WM_RECVERREFUSEFILE, 0, (LPARAM)m_pTcpClient);
		break;
	case -20: // ���ն˽����ļ��ɹ�
		ForcePostMessage(WM_RECVERSUCC, 0, (LPARAM)m_pTcpClient);
		break;
	case -22: // ���ն�ȡ�������ļ�
		ForcePostMessage(WM_RECVERCANCEL, 0, (LPARAM)m_pTcpClient);
		break;
	case -24: // ���ն˽����ļ�����
		ForcePostMessage(WM_RECVERFAIL, 0, (LPARAM)m_pTcpClient);
		break;
	default:
		m_pTcpClient->HandleOneNetMsg(Msg, nMsgLen);
		break;
	}
}
