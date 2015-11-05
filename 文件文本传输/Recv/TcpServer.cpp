// CTcpServer
// XSoft
// Author:  alone
// Contact: lovelypengpeng@eyou.com or hongxing777@msn.com
// Update Date: 2005.1.17
// Comment: if you find any bugs or have any suggestions please contact me.
// �����շ�������Ϣ, ͬʱ�ɽ����ļ���֧�ֶϵ�������֧��ͬʱ���ն���ļ�
//
#include "stdafx.h"
#include "TcpServer.h"
#include "SysUtils.h"

#pragma comment(lib, "Ws2_32.lib")

/////////////////////////////////////////////////////////////////////////
// CTcpServer
/////////////////////////////////////////////////////////////////////////
CTcpServer::CTcpServer(void *pNotifyObj)
{
	ASSERT(pNotifyObj != NULL);

	memset(&m_WSAData, 0, sizeof(WSADATA));
	m_pNotifyObj = pNotifyObj;
	strcpy(m_szBindIpAddr, "");
	m_wPort = DFT_PORT;
	m_nSendTimeOut = DFT_SENDTIMEOUT;
	m_dwProgressTimeInterval = DFT_PROGRESSTIMEINTERVAL;
	m_nAcceptThreadPriority = DFT_ACCEPTTHREADPRIORITY;
	m_nSocketRecvThreadPriority = DFT_SOCKETRECVTHREADPRIORITY;

	// ��ʼ���¼�ָ��
	m_OnAccept = NULL;
	m_OnAcceptErr = NULL;
	m_OnSocketConnect = NULL;
	m_OnSocketClose = NULL;
	m_OnOneNetMsg = NULL;
	m_OnRecvFileRequest = NULL;
	m_OnRecvFileProgress = NULL;
	m_OnRecvFileSucc = NULL;
	m_OnRecvFileFail = NULL;

	StartupSocket(); // ��ʼ��windows socket
	
	RegisterAcceptWndClass(); // ע�����ڽ���socket���ӵĴ�����
	RegisterRecvWndClass(); // ע�����ڽ������ݵĴ�����
	RegisterFileRecverWndClass(); // ע�������ļ��������Ĵ�����

	m_pAcceptSocket = new CAcceptSocket(this, m_pNotifyObj); // ����socket���ӵ�socket����
}

CTcpServer::~CTcpServer(void)
{
	StopAccept(); // ֹͣ����socket����
	CancelAllRecvFile(); // ȡ�����е��ļ�����
	CloseAllServerClientSocket(); // �ر����еĿͻ�������

	UnregisterClass(STR_RECVHIDEWNDCLASSNAME, AfxGetInstanceHandle());
	UnregisterClass(STR_ACCEPTHIDEWNDCLASSNAME, AfxGetInstanceHandle());
	UnregisterClass(STR_FILERECVERHIDEWNDCLASSNAME, AfxGetInstanceHandle());
	
	CleanupSocket(); 

	// �رղ�ɾ������socket����
	delete m_pAcceptSocket;
	m_pAcceptSocket = NULL;
}

// ��ʼ��windows socket
BOOL CTcpServer::StartupSocket(void)
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
void CTcpServer::CleanupSocket(void)
{
	if(m_WSAData.wVersion != 0)
	{
		WSACleanup();
		memset(&m_WSAData, 0, sizeof(WSADATA));
	}
}

// �ر����еĿͻ���socket����
void CTcpServer::CloseAllServerClientSocket(void)
{
	CServerClientSocket	*pServerClientSocket;
	
	while(!m_ServerClientSockets.IsEmpty())
	{
		pServerClientSocket = m_ServerClientSockets.RemoveHead();
		delete pServerClientSocket;
	}
}

// ע�����ڽ����߳����ӵĴ�����
BOOL CTcpServer::RegisterAcceptWndClass(void)
{
	WNDCLASS wc;

	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = (WNDPROC)(CAcceptSocket::HideWndProc);
	wc.hInstance = AfxGetInstanceHandle();
	wc.lpszClassName = STR_ACCEPTHIDEWNDCLASSNAME;

	return (RegisterClass(&wc) != 0);
}

// ע�����ڽ������ݵĴ�����
BOOL CTcpServer::RegisterRecvWndClass(void)
{
	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = (WNDPROC)(CServerClientSocket::HideWndProc);
	wc.hInstance = AfxGetInstanceHandle();
	wc.lpszClassName = STR_RECVHIDEWNDCLASSNAME;

	return (RegisterClass(&wc) != 0);
}

// ע�������ļ��������Ĵ�����
BOOL CTcpServer::RegisterFileRecverWndClass(void)
{
	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = (WNDPROC)(CFileRecver::HideWndProc);
	wc.hInstance = AfxGetInstanceHandle();
	wc.lpszClassName = STR_FILERECVERHIDEWNDCLASSNAME;

	return (RegisterClass(&wc) != 0);
}

// ��ʼ����socket����
BOOL CTcpServer::StartAccept(void)
{
	StopAccept();

	m_pAcceptSocket->SetBindIpAddr(m_szBindIpAddr);
	m_pAcceptSocket->SetPort(m_wPort);
	m_pAcceptSocket->SetAcceptThreadPriority(m_nAcceptThreadPriority);
	m_pAcceptSocket->SetOnAccept(m_OnAccept);
	m_pAcceptSocket->SetOnAcceptErr(m_OnAcceptErr);
	m_pAcceptSocket->SetOnSocketConnect(m_OnSocketConnect);

	return m_pAcceptSocket->StartAccept();
}

// ֹͣ����socket����
void CTcpServer::StopAccept(void)
{
	m_pAcceptSocket->StopAccept();
}

// ���socket����
CServerClientSocket *CTcpServer::AddServerClientSocket(SOCKET hSocket)
{
	// ����socket����
	CServerClientSocket *pServerClientSocket = new CServerClientSocket(this, m_pNotifyObj);
	pServerClientSocket->SetSocketHandle(hSocket);
	pServerClientSocket->SetSendTimeOut(m_nSendTimeOut);
	pServerClientSocket->SetProgressTimeInterval(m_dwProgressTimeInterval);
	pServerClientSocket->SetSocketRecvThreadPriority(m_nSocketRecvThreadPriority);
	pServerClientSocket->SetOnSocketClose(m_OnSocketClose);
	pServerClientSocket->SetOnOneNetMsg(m_OnOneNetMsg);
	pServerClientSocket->SetOnRecvFileRequest(m_OnRecvFileRequest);
	pServerClientSocket->SetOnRecvFileProgress(m_OnRecvFileProgress);
	pServerClientSocket->SetOnRecvFileSucc(m_OnRecvFileSucc);
	pServerClientSocket->SetOnRecvFileFail(m_OnRecvFileFail);

	// ��ʼ������socket
	if(!pServerClientSocket->InitServerClientSocket())
	{
		delete pServerClientSocket;
		return NULL;
	}
	
	// ��ӵ��б�
	m_ServerClientSockets.AddTail(pServerClientSocket);

	return pServerClientSocket;
}

// �Ƴ��ͻ���socket����
void CTcpServer::RemoveServerClientSocket(CServerClientSocket *pServerClientSocket)
{
	POSITION pos;

	pos = m_ServerClientSockets.Find(pServerClientSocket);
	if(pos != NULL)
		m_ServerClientSockets.RemoveAt(pos);
}

// ȡ�����еĽ���
void CTcpServer::CancelAllRecvFile(void)
{
	CServerClientSocket		*pServerClientSocket;
	POSITION				pos;

	pos = m_ServerClientSockets.GetHeadPosition();
	for(int i = 0; i < m_ServerClientSockets.GetCount(); i++)
	{
		pServerClientSocket = m_ServerClientSockets.GetNext(pos);
		pServerClientSocket->CancelRecvFile();
	}
}

// ��ȡ�ͻ�����Ŀ
int CTcpServer::GetClientCount(void)
{
	return m_ServerClientSockets.GetCount();
}

void CTcpServer::SetBindIpAddr(char *szBindIpAddr)
{
	strcpy(m_szBindIpAddr, szBindIpAddr);
}

void CTcpServer::SetPort(WORD wPort)
{
	m_wPort = wPort;
}

void CTcpServer::SetSendTimeOut(int nSendTimeOut)
{
	m_nSendTimeOut = nSendTimeOut;
}

void CTcpServer::SetProgressTimeInterval(DWORD dwProgressTimeInterval)
{
	m_dwProgressTimeInterval = dwProgressTimeInterval;
}

void CTcpServer::SetAcceptThreadPriority(int nPriority)
{
	m_nAcceptThreadPriority = nPriority;
}

void CTcpServer::SetSocketRecvThreadPriority(int nPriority)
{
	m_nSocketRecvThreadPriority = nPriority;
}

void CTcpServer::SetOnAccept(AcceptFun OnAccept)
{
	m_OnAccept = OnAccept;
}

void CTcpServer::SetOnAcceptErr(AcceptErrFun OnAcceptErr)
{
	m_OnAcceptErr = OnAcceptErr;
}

void CTcpServer::SetOnSocketConnect(SocketConnectFun OnSocketConnect)
{
	m_OnSocketConnect = OnSocketConnect;
}

void CTcpServer::SetOnSocketClose(SocketCloseFun OnSocketClose)
{
	m_OnSocketClose = OnSocketClose;
}

void CTcpServer::SetOnOneNetMsg(OneNetMsgFun OnOneNetMsg)
{
	m_OnOneNetMsg = OnOneNetMsg;
}

void CTcpServer::SetOnRecvFileRequest(RecvFileRequestFun OnRecvFileRequest)
{
	m_OnRecvFileRequest = OnRecvFileRequest;
}

void CTcpServer::SetOnRecvFileProgress(RecvFileProgressFun OnRecvFileProgress)
{
	m_OnRecvFileProgress = OnRecvFileProgress;
}


void CTcpServer::SetOnRecvFileSucc(RecvFileSuccFun OnRecvFileSucc)
{
	m_OnRecvFileSucc = OnRecvFileSucc;
}

void CTcpServer::SetOnRecvFileFail(RecvFileFailFun OnRecvFileFail)
{
	m_OnRecvFileFail = OnRecvFileFail;
}

/////////////////////////////////////////////////////////////////////////
// CAcceptSocket
/////////////////////////////////////////////////////////////////////////

// ���ش�����Ϣ������
LRESULT CAcceptSocket::HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CAcceptSocket *pAcceptSocket = (CAcceptSocket *)lParam;

	switch(uMsg)
	{
	case WM_ACCEPT:
		pAcceptSocket->HandleAcceptMsg(wParam); // wParamΪ��socket���
		break;
	case WM_ACCEPTERR:
		pAcceptSocket->HandleAcceptErrMsg();
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0L;
}

CAcceptSocket::CAcceptSocket(CTcpServer *pTcpServer, void *pNotifyObj)
{
	m_pTcpServer = pTcpServer;
	m_pNotifyObj = pNotifyObj;
	m_hHideWnd = NULL;
	m_hSocket = INVALID_SOCKET;
	m_pAcceptThread = NULL;
	m_nAcceptThreadPriority = DFT_ACCEPTTHREADPRIORITY;

	m_OnSocketConnect = NULL;
	m_OnAccept = NULL;
	m_OnAcceptErr = NULL;
}

CAcceptSocket::~CAcceptSocket(void)
{
	StopAccept();
}

// ������������
BOOL CAcceptSocket::AllocateWindow(void)
{
	if(m_hHideWnd == NULL)
	{
		m_hHideWnd = CreateWindow(STR_ACCEPTHIDEWNDCLASSNAME, NULL, WS_POPUP, CW_USEDEFAULT,  CW_USEDEFAULT, 
			0, 0, NULL, NULL, AfxGetInstanceHandle(), NULL);
	}
	
	return (m_hHideWnd != NULL);
}

// �ͷ���������
void CAcceptSocket::DeallocateWindow(void)
{
	if(m_hHideWnd != NULL)
	{
		DestroyWindow(m_hHideWnd);
		m_hHideWnd = NULL;
	}
}

// �������socket���Ӵ�����Ϣ
void CAcceptSocket::HandleAcceptMsg(SOCKET hSocket)
{
	// ѯ���û��Ƿ��������
	BOOL bAccept = TRUE;
	if(m_OnAccept != NULL)
		m_OnAccept(m_pNotifyObj, hSocket, bAccept);

	// ����������
	if(!bAccept)
	{
		closesocket(hSocket);
		return;
	}

	// �������
	CServerClientSocket *pServerClientSocket = m_pTcpServer->AddServerClientSocket(hSocket);
	if(pServerClientSocket == NULL)
		return;

	// ������һ���µ�socket����
	if(m_OnSocketConnect != NULL)
		m_OnSocketConnect(m_pNotifyObj, hSocket);

	pServerClientSocket->ResumeServerClientRecvThread(); // go
}

// ������ճ�������Ϣ
void CAcceptSocket::HandleAcceptErrMsg(void)
{
	if(m_OnAcceptErr != NULL)
		m_OnAcceptErr(m_pNotifyObj, m_hSocket);

	StopAccept();
}

void CAcceptSocket::InitSockAddr(SOCKADDR_IN  *pSockAddr, char *szBindIpAddr, WORD wPort)
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

BOOL CAcceptSocket::StartAccept(void)
{
	u_long			ulTmp;
	SOCKADDR_IN		SockAddr;

	StopAccept();

	// �������ش���
	if(!AllocateWindow())
		goto ErrEntry;

	// ����socket
	m_hSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
	if(m_hSocket == INVALID_SOCKET)
		goto ErrEntry;

	// ����socketΪ������ʽ
	ulTmp = 0;
	if(ioctlsocket(m_hSocket, FIONBIO, &ulTmp) != 0)
		goto ErrEntry;

	// ��socket
	InitSockAddr(&SockAddr, m_szBindIpAddr, m_wPort);
	if(bind(m_hSocket, (sockaddr *)&SockAddr, sizeof(SOCKADDR_IN)) != 0)
		goto ErrEntry;

	// ����
	if(listen(m_hSocket, SOMAXCONN) != 0)
		goto ErrEntry;

	m_pAcceptThread = new CAcceptThread(this, m_nAcceptThreadPriority);
	m_pAcceptThread->SetHideWndHandle(m_hHideWnd);
	m_pAcceptThread->SetSocketHandle(m_hSocket);
	m_pAcceptThread->Resume();

	return TRUE;
ErrEntry:
  StopAccept();
  return FALSE;
}

// ֹͣ����socket����
void CAcceptSocket::StopAccept(void)
{
	// �ر����ڽ���socket���ӵ�socket
	if(m_hSocket != INVALID_SOCKET)
	{
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}
	
	// �رղ�ɾ��socket�����߳�
	if(m_pAcceptThread != NULL)
	{
		m_pAcceptThread->Terminate();
		WaitForSingleObject(m_pAcceptThread->m_hThread, INFINITE);
		delete m_pAcceptThread;
		m_pAcceptThread = NULL;
	}
	
	// ������Ϣ�����е�socket
	// �ر�δ�����ܵ�socket
	if(m_hHideWnd != NULL)
	{
		MSG msg;
		while(PeekMessage(&msg, m_hHideWnd, WM_ACCEPT, WM_ACCEPT, PM_REMOVE))
			closesocket(msg.wParam);
	}

	DeallocateWindow();
}

void CAcceptSocket::SetHideWndHandle(HWND hHideWnd)
{
	m_hHideWnd = hHideWnd;
}

void CAcceptSocket::SetBindIpAddr(char *szBindIpAddr)
{
	strcpy(m_szBindIpAddr, szBindIpAddr);
}

void CAcceptSocket::SetPort(WORD wPort)
{
	m_wPort = wPort;
}

void CAcceptSocket::SetAcceptThreadPriority(int nPriority)
{
	m_nAcceptThreadPriority = nPriority;
}

void CAcceptSocket::SetOnSocketConnect(SocketConnectFun OnSocketConnect)
{
	m_OnSocketConnect = OnSocketConnect;
}

void CAcceptSocket::SetOnAccept(AcceptFun OnAccept)
{
	m_OnAccept = OnAccept;
}

void CAcceptSocket::SetOnAcceptErr(AcceptErrFun OnAcceptErr)
{
	m_OnAcceptErr = OnAcceptErr;
}

/////////////////////////////////////////////////////////////////////////
// CAcceptThread
/////////////////////////////////////////////////////////////////////////
CAcceptThread::CAcceptThread(CAcceptSocket *pAcceptSocket, int nPriority): CThread(nPriority)
{
	m_pAcceptSocket = pAcceptSocket;
	m_hHideWnd = NULL;
	m_hSocket = INVALID_SOCKET;
}

void CAcceptThread::ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
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

// �����߳�ִ����
void CAcceptThread::Execute(void)
{
	SOCKADDR_IN		sai;
	int				nAddrLen;
	SOCKET			hSocket;

	while(!m_bTerminated)
	{
		nAddrLen = sizeof(SOCKADDR_IN);
		memset(&sai, 0, nAddrLen);
		hSocket = accept(m_hSocket, (sockaddr *)&sai, &nAddrLen);
		if(hSocket != INVALID_SOCKET)
			ForcePostMessage(WM_ACCEPT, hSocket, (LPARAM)m_pAcceptSocket);
		else
		{
			// ������������
			ForcePostMessage(WM_ACCEPTERR, 0, (LPARAM)m_pAcceptSocket);
			break;
		}
	} // while
}

void CAcceptThread::SetHideWndHandle(HWND hHideWnd)
{
	m_hHideWnd = hHideWnd;
}

void CAcceptThread::SetSocketHandle(SOCKET hSocket)
{
	m_hSocket = hSocket;
}

/////////////////////////////////////////////////////////////////////////
// CServerClientSocket
/////////////////////////////////////////////////////////////////////////
// ���ش�����Ϣ������
LRESULT CServerClientSocket::HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CServerClientSocket *pServerClientSocket = (CServerClientSocket *)lParam;

	switch(uMsg)
	{
	case WM_SOCKETCLOSE:
		pServerClientSocket->HandleSocketCloseMsg();
		delete pServerClientSocket;
		break;
	case WM_SOCKETSENDERR:
		pServerClientSocket->HandleSocketSendErrMsg();
		delete pServerClientSocket;
		break;
	case WM_SOCKETRECVERR:
		pServerClientSocket->HandleSocketRecvErrMsg();
		delete pServerClientSocket;
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return 0L;
}

CServerClientSocket::CServerClientSocket(CTcpServer *pTcpServer, void *pNotifyObj)
{
	m_pTcpServer = pTcpServer;
	m_pNotifyObj = pNotifyObj;
	m_hHideWnd = NULL;
	m_hSocket = INVALID_SOCKET;
	m_nSendTimeOut = DFT_SENDTIMEOUT;
	m_nSocketRecvThreadPriority = DFT_SOCKETRECVTHREADPRIORITY;

	m_pServerClientSocketRecvThread = NULL;
	m_pFileRecver = NULL;

	// �¼�ָ��
	m_OnSocketClose = NULL;
	m_OnOneNetMsg = NULL;
	m_OnRecvFileRequest = NULL;
	m_OnRecvFileProgress = NULL;
	m_OnRecvFileSucc = NULL;
	m_OnRecvFileFail = NULL;
}

CServerClientSocket::~CServerClientSocket(void)
{
	FiniRecvFileSocket();
}

void CServerClientSocket::SetSocketHandle(SOCKET hSocket)
{
	m_hSocket = hSocket;
}

void CServerClientSocket::SetSendTimeOut(int nSendTimeOut)
{
	m_nSendTimeOut = nSendTimeOut;
}

void CServerClientSocket::SetProgressTimeInterval(DWORD dwProgressTimeInterval)
{
	m_dwProgressTimeInterval = dwProgressTimeInterval;
}

void CServerClientSocket::SetSocketRecvThreadPriority(int nPriority)
{
	m_nSocketRecvThreadPriority = nPriority;
}

void CServerClientSocket::SetOnSocketClose(SocketCloseFun OnSocketClose)
{
	m_OnSocketClose = OnSocketClose;
}

void CServerClientSocket::SetOnOneNetMsg(OneNetMsgFun OnOneNetMsg)
{
	m_OnOneNetMsg = OnOneNetMsg;
}

void CServerClientSocket::SetOnRecvFileRequest(RecvFileRequestFun OnRecvFileRequest)
{
	m_OnRecvFileRequest = OnRecvFileRequest;
}

void CServerClientSocket::SetOnRecvFileProgress(RecvFileProgressFun OnRecvFileProgress)
{
	m_OnRecvFileProgress = OnRecvFileProgress;
}

void CServerClientSocket::SetOnRecvFileSucc(RecvFileSuccFun OnRecvFileSucc)
{
	m_OnRecvFileSucc = OnRecvFileSucc;
}

void CServerClientSocket::SetOnRecvFileFail(RecvFileFailFun OnRecvFileFail)
{
	m_OnRecvFileFail = OnRecvFileFail;
}

BOOL CServerClientSocket::InitServerClientSocket(void)
{
	// �������ش���
	if(!AllocateWindow())
		return FALSE;
	
	// ����socket���ͳ�ʱ
	if(setsockopt(m_hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&m_nSendTimeOut, sizeof(m_nSendTimeOut)) != 0)
		return FALSE;
	
	// ���������ļ��߳�
	m_pServerClientSocketRecvThread = new CServerClientSocketRecvThread(this, m_nSocketRecvThreadPriority);
	m_pServerClientSocketRecvThread->SetHideWndHandle(m_hHideWnd);
	m_pServerClientSocketRecvThread->SetSocketHandle(m_hSocket);

	// �����ļ�������
	m_pFileRecver = new CFileRecver(this, m_pServerClientSocketRecvThread, m_pNotifyObj);
	m_pFileRecver->SetSocketHideWndHandle(m_hHideWnd);
	m_pFileRecver->SetProgressTimeInterval(m_dwProgressTimeInterval);
	m_pFileRecver->SetOnRecvFileRequest(m_OnRecvFileRequest);
	m_pFileRecver->SetOnRecvFileProgress(m_OnRecvFileProgress);
	m_pFileRecver->SetOnRecvFileSucc(m_OnRecvFileSucc);
	m_pFileRecver->SetOnRecvFileFail(m_OnRecvFileFail);
	if(!m_pFileRecver->InitFileRecver())
		return FALSE;

	return TRUE;
}

void CServerClientSocket::FiniRecvFileSocket(void)
{
	DeallocateWindow(); // �ͷ����ش���
	
	// �ر�socket����
	if(m_hSocket != INVALID_SOCKET)
	{
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	// ɾ�����ݽ����߳�
	if(m_pServerClientSocketRecvThread != NULL)
	{
		m_pServerClientSocketRecvThread->Terminate();
		WaitForSingleObject(m_pServerClientSocketRecvThread->GetThreadHandle(), INFINITE);
		delete m_pServerClientSocketRecvThread;
		m_pServerClientSocketRecvThread = NULL;
	}
	
	// ɾ���ļ�������
	delete m_pFileRecver;
	m_pFileRecver = NULL;
}

void CServerClientSocket::ResumeServerClientRecvThread(void)
{
	// ί�и��ļ�������
	m_pServerClientSocketRecvThread->Resume();
}

// ȡ�������ļ�
 void CServerClientSocket::CancelRecvFile(void)
{
	ASSERT(m_pFileRecver != NULL);
	
	m_pFileRecver->CancelRecvFile();
}

// ������������
BOOL CServerClientSocket::AllocateWindow(void)
{
	if(m_hHideWnd == NULL)
	{
		m_hHideWnd = CreateWindow(STR_RECVHIDEWNDCLASSNAME, NULL, WS_POPUP,  CW_USEDEFAULT, CW_USEDEFAULT,  
			0, 0, NULL, NULL, AfxGetInstanceHandle(), NULL);
	}

	return (m_hHideWnd != NULL);
}

// �ͷ���������
void CServerClientSocket::DeallocateWindow(void)
{
	if(m_hHideWnd != NULL)
	{
		DestroyWindow(m_hHideWnd);
		m_hHideWnd = NULL;
	}
}

// ����������Ϣ
BOOL CServerClientSocket::SendNetMsg(char *Msg, int nMsgLen)
{
	char szMsgLen[6];
	int nTotalLen;
	int nErr;
	int nSent = 0;
	
	ASSERT(nMsgLen <= MAX_PACKAGESIZE);

	// ����������Ϣ��Ϣ
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

void CServerClientSocket::HandleSocketCloseMsg(void)
{
	m_pTcpServer->RemoveServerClientSocket(this); // ���б����Ƴ�

	if(m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_PEERCLOSE);
}

// socket�������ݳ���
void CServerClientSocket::HandleSocketSendErrMsg(void)
{
	m_pTcpServer->RemoveServerClientSocket(this); // ���б����Ƴ�

	if(m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_SOCKETSENDERR);
}

// socket�������ݳ���
void CServerClientSocket::HandleSocketRecvErrMsg(void)
{
	m_pTcpServer->RemoveServerClientSocket(this); // ���б����Ƴ�

	if(m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_SOCKETRECVERR);
}

// ������յ���������Ϣ
// ע��: �ú����ڽ����߳�������!!!
void CServerClientSocket::HandleRecvedNetMsg(char *Msg, int nMsgLen)
{
	char	szCmd[4];
	int		nCmd;

	strncpy(szCmd, Msg, 3);
	szCmd[3] = 0;
	nCmd = atoi(szCmd);
	if(nCmd >= 0) // ������Ϣ
		HandleOneNetMsg(Msg, nMsgLen);
	else // ���ļ������йص�������Ϣ��ί�и��ļ�������ȥ����
		m_pFileRecver->HandleRecvedNetMsg(nCmd, Msg, nMsgLen);
}

void CServerClientSocket::HandleOneNetMsg(char *Msg, int nMsgLen)
{
	if(m_OnOneNetMsg != NULL)
	{
		m_OnOneNetMsg(m_pNotifyObj, Msg, nMsgLen);
	}
}

/////////////////////////////////////////////////////////////////////////
// CServerClientSocketRecvThread
/////////////////////////////////////////////////////////////////////////
CServerClientSocketRecvThread::CServerClientSocketRecvThread(CServerClientSocket *pServerClientSocket, int nPriority): CThread(nPriority)
{
	m_pServerClientSocket = pServerClientSocket;
	m_hHideWnd = NULL;
	m_hSocket = INVALID_SOCKET;
	m_nRecvBufSize = 0;
}

void CServerClientSocketRecvThread::SetHideWndHandle(HWND hHideWnd)
{
	m_hHideWnd = hHideWnd;
}

void CServerClientSocketRecvThread::SetSocketHandle(SOCKET hSocket)
{
	m_hSocket = hSocket;
}

void CServerClientSocketRecvThread::ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
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

void CServerClientSocketRecvThread::DoRecv(char *Buf, int nBytes)
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
		if(m_nRecvBufSize < 6)
			return; // ��Ϣ����δ��������,�˳�
		strncpy(szMsgLen, &(m_RecvBuf[1]), 5);
		szMsgLen[5] = 0;
		nMsgLen = atoi(szMsgLen);
		nTotalLen = 1 + 5 + nMsgLen;

		// �ж���Ϣ�Ƿ���������
		if(nTotalLen > m_nRecvBufSize)
			return; // ��Ϣû�н�������

		m_pServerClientSocket->HandleRecvedNetMsg(&(m_RecvBuf[6]), nMsgLen);
		m_nRecvBufSize -= nTotalLen;
		memcpy(m_RecvBuf, &(m_RecvBuf[nTotalLen]), m_nRecvBufSize); // ����Ϣ��������ȥ������Ϣ
	};
}

void CServerClientSocketRecvThread::Execute(void)
{
	int		nErr;
	char	Buf[SOCKET_RECVBUFSIZE];

	while(!m_bTerminated)
	{
		nErr = recv(m_hSocket, Buf, SOCKET_RECVBUFSIZE, 0);
		if(nErr == SOCKET_ERROR) // �д�����
		{
			nErr = WSAGetLastError();
			if(nErr == WSAECONNRESET)
			{
				// socket���ӱ�peer�˹رգ����·��λ�ȴ���
				// ����������͵Ĵ���ʱsocket���뱻�رա�
				ForcePostMessage(WM_SOCKETCLOSE, 0, (LPARAM)m_pServerClientSocket);
				break;
			}
			else
			{
				// ������socket���󣺽������ݳ���
				ForcePostMessage(WM_SOCKETRECVERR, 0, (LPARAM)m_pServerClientSocket);
			}
		}
		else if(nErr == 0) // socket�������ر�
		{
			ForcePostMessage(WM_SOCKETCLOSE, 0, (LPARAM)m_pServerClientSocket);
			break;
		}
		else // ���յ�����
			DoRecv(Buf, nErr); // ������յ�������
	} // while
}

/////////////////////////////////////////////////////////////////////////
// CFileRecver
/////////////////////////////////////////////////////////////////////////
// ���ش�����Ϣ������
LRESULT CFileRecver::HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CFileRecver *pFileRecver = (CFileRecver *)lParam;

	switch(uMsg)
	{
	case WM_REQUESTSENDFILE:
		pFileRecver->HandleRequestSendFileMsg();
		break;
	case WM_SENDERFAIL:
		pFileRecver->HandleSenderFailMsg();
		break;
	case WM_SENDERCANCEL:
		pFileRecver->HandleSenderCancelMsg();
		break;
	case WM_RECVFILEPROGRESS:
		pFileRecver->HandleRecvFileProgressMsg();
		break;
	case WM_RECVFILEFAIL:
		pFileRecver->HandleRecverFailMsg();
		break;
	case WM_RECVFILESUCC:
		pFileRecver->HandleRecverSuccMsg();
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	
	return 0L;
}

CFileRecver::CFileRecver(CServerClientSocket *pServerClientSocket, CThread *pSocketRecvThread, void *pNotifyObj)
{
	m_pServerClientSocket = pServerClientSocket;
	m_pSocketRecvThread = pSocketRecvThread;
	m_pNotifyObj = pNotifyObj;
	m_hSocketHideWnd = NULL;
	m_hFileRecverHideWnd = NULL;
	m_dwProgressTimeInterval = DFT_PROGRESSTIMEINTERVAL;

	// �¼�ָ��
	m_OnRecvFileRequest = NULL;
	m_OnRecvFileProgress = NULL;
	m_OnRecvFileSucc = NULL;
	m_OnRecvFileFail = NULL;

	// multi read single write variants
	strcpy(m_szPathName, "");
	m_dwRecvedBytes = 0;
	m_dwFileSize = 0;
	m_dwLastProgressTick = GetTickCount();
}

CFileRecver::~CFileRecver(void)
{
	DeallocateWindow();
}

void CFileRecver::ForcePostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	while(!m_pSocketRecvThread->Terminated())
	{
		if(PostMessage(hWnd, Msg, wParam, lParam))
			return;
		if(GetLastError() != ERROR_NOT_ENOUGH_QUOTA)
			return;
		Sleep(1000);
	}
}

// ������������
BOOL CFileRecver::AllocateWindow(void)
{
	if(m_hFileRecverHideWnd == NULL)
	{
		m_hFileRecverHideWnd = CreateWindow(STR_FILERECVERHIDEWNDCLASSNAME, NULL, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 
			0, 0, NULL, NULL, AfxGetInstanceHandle(), NULL);
	}

	return (m_hFileRecverHideWnd != NULL);
}

// �ͷ���������
void CFileRecver::DeallocateWindow(void)
{
	if(m_hFileRecverHideWnd != NULL)
	{
		DestroyWindow(m_hFileRecverHideWnd);
		m_hFileRecverHideWnd = NULL;
	}
}

void CFileRecver::SetSocketHideWndHandle(HWND hSocketHideWnd)
{
	m_hSocketHideWnd = hSocketHideWnd;
}

void CFileRecver::SetProgressTimeInterval(DWORD dwProgressTimeInterval)
{
	m_dwProgressTimeInterval = dwProgressTimeInterval;
}

void CFileRecver::SetOnRecvFileRequest(RecvFileRequestFun OnRecvFileRequest)
{
	m_OnRecvFileRequest = OnRecvFileRequest;
}

void CFileRecver::SetOnRecvFileProgress(RecvFileProgressFun OnRecvFileProgress)
{
	m_OnRecvFileProgress = OnRecvFileProgress;
}

void CFileRecver::SetOnRecvFileSucc(RecvFileSuccFun OnRecvFileSucc)
{
	m_OnRecvFileSucc = OnRecvFileSucc;
}

void CFileRecver::SetOnRecvFileFail(RecvFileFailFun OnRecvFileFail)
{
	m_OnRecvFileFail = OnRecvFileFail;
}

// ������յ���������Ϣ
// ע�⣺�ú����ڽ����߳�������!!!
void CFileRecver::HandleRecvedNetMsg(int nCmd, char *Msg, int nMsgLen)
{
	m_Mutex.Lock();
	switch(nCmd)
	{
	case -16:
		HandleRequestSendFileNetMsg(Msg, nMsgLen);
		break;
	case -19:
		HandleFileDataNetMsg(Msg, nMsgLen);
		break;
	case -21:
		HandleSenderCancelNetMsg(Msg, nMsgLen);
		break;
	case -23:
		HandleSenderFailNetMsg(Msg, nMsgLen);
		break;
	default:
		break;
	}
	m_Mutex.Unlock();
}

// �����������ļ�������Ϣ
// ע��:�ú�����socket�����߳�������!!!
void CFileRecver::HandleRequestSendFileNetMsg(char *Msg, int nMsgLen)
{
	char szFileSize[11];

	ResetFileRecver(); // ��λ�ļ�������

	// ��ȡ�ļ���С
	strncpy(szFileSize, &(Msg[3]), 10);
	szFileSize[10] = 0;
	m_dwFileSize = (DWORD)_atoi64(szFileSize);

	// ��ȡ�ļ���
	strncpy(m_szPathName, &(Msg[13]), nMsgLen - 13);
	m_szPathName[nMsgLen - 13] = 0;

	// ���ʹ�����Ϣ��ѯ���û��Ƿ���մ��ļ�
	ForcePostMessage(m_hFileRecverHideWnd, WM_REQUESTSENDFILE, 0, (LPARAM)this);
}

// �����ļ�����������Ϣ
void CFileRecver::HandleFileDataNetMsg(char *Msg, int nMsgLen)
{
	if(!IsRecving())
		return;

	// д���ļ�����
	try
	{
		m_File.SeekToEnd(); // ������ݵ��ļ�β��
		m_File.Write(&(Msg[3]), nMsgLen - 3); // д���ݵ��ļ�, ȥ����Ϣͷ
		m_dwRecvedBytes = m_File.GetLength();
	}
	catch(...) // �����ļ�ʧ�ܣ�����ʧ��
	{
		ResetFileRecver(); // ��λ�ļ�������
		ForcePostMessage(m_hFileRecverHideWnd, WM_RECVFILEFAIL, 0, (LPARAM)this); // �����ļ�����
		if(!SendRecverFailNetMsg()) // ���ͽ��ն˳���������Ϣ
			ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket); // socket����
	
		return;
	}

	DWORD dwTickCount = GetTickCount();
	if(dwTickCount < m_dwLastProgressTick) m_dwLastProgressTick = 0;
	if(dwTickCount - m_dwLastProgressTick > m_dwProgressTimeInterval || m_dwRecvedBytes >= m_dwFileSize)
	{
		ForcePostMessage(m_hFileRecverHideWnd, WM_RECVFILEPROGRESS, 0, (LPARAM)this); // �ļ����ս���
		m_dwLastProgressTick = dwTickCount;
	}
	else
		Sleep(0); // ǿ��windows�����߳�

	CheckIfRecvFileSucc(); // �������ļ��Ƿ�ɹ�
}

// ����ȡ�������ļ���Ϣ
void CFileRecver::HandleSenderCancelNetMsg(char *Msg, int nMsgLen)
{
	if(!IsRecving())
		return;

	ResetFileRecver(); // ��λ�ļ�������
	ForcePostMessage(m_hFileRecverHideWnd, WM_SENDERCANCEL, 0, (LPARAM)this);
}

// �����ͷ�����������Ϣ
void CFileRecver::HandleSenderFailNetMsg(char *Msg, int nMsgLen)
{
	if(!IsRecving())
		return;

	ResetFileRecver(); // ��λ�ļ�������
	ForcePostMessage(m_hFileRecverHideWnd, WM_SENDERFAIL, 0, (LPARAM)this);
}

// ���ͽ����ļ���������������Ϣ
BOOL CFileRecver::SendAcceptSendFileRequestNetMsg(void)
{
	char szSize[11];
	char Msg[19];

	strncpy(&(Msg[6]), "-17", 3);
	sprintf(szSize, "%010d", m_dwRecvedBytes);
	strncpy(&(Msg[9]), szSize, 10);
	return m_pServerClientSocket->SendNetMsg(Msg, 13);
}

// ���;ܾ��ļ���������������Ϣ
BOOL CFileRecver::SendRefuseSendFileRequestNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-18", 3);
	return m_pServerClientSocket->SendNetMsg(Msg, 3);
}

// ���ͽ��ն�ʧ��������Ϣ
BOOL CFileRecver::SendRecverFailNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-24", 3);
	return m_pServerClientSocket->SendNetMsg(Msg, 3);
}

// ���ͽ��ն�ȡ��������Ϣ
BOOL CFileRecver::SendRecverCancelNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-22", 3);
	return m_pServerClientSocket->SendNetMsg(Msg, 3);
}

// ���ͽ��ն˳ɹ�������Ϣ
BOOL CFileRecver::SendRecverSuccNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-20", 3);
	return m_pServerClientSocket->SendNetMsg(Msg, 3);
}

// ��ʼ���ļ�������
BOOL CFileRecver::InitFileRecver(void)
{
	if(!AllocateWindow())
		return FALSE;

	return TRUE;
}

// �Ƿ����ڽ����ļ�
BOOL CFileRecver::IsRecving(void)
{
	return (m_File.m_hFile != CFile::hFileNull);
}

// ȡ�������ļ�, ֻ�ܴ����߳��е��ã�������������
void CFileRecver::CancelRecvFile(void)
{
	m_Mutex.Lock();

	if(IsRecving())
	{
		if(!SendRecverCancelNetMsg())
			ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);
	}

	ResetFileRecver();
	m_Mutex.Unlock();
}

// ��λ�ļ�������
void CFileRecver::ResetFileRecver(void)
{
	// ��������е���Ϣ
	MSG msg;
	while(PeekMessage(&msg, m_hFileRecverHideWnd, WM_RECVFILEPROGRESS, WM_RECVFILEPROGRESS, PM_REMOVE));
	
	// �رմ����յ��ļ�
	if(m_File.m_hFile != CFile::hFileNull)
		m_File.Close();
}

// �򿪴����յ��ļ�
BOOL CFileRecver::OpenRecvFile(char *szPathName)
{
	UINT nOpenFlags;

	// ���ļ�������ļ��������򴴽����ļ�
	nOpenFlags = CFile::modeWrite | CFile::shareExclusive;
	if(!IsFileExists(szPathName))
		nOpenFlags |= CFile::modeCreate;

	return m_File.Open(szPathName, nOpenFlags);
}

// ����ļ��Ƿ���ճɹ�
void CFileRecver::CheckIfRecvFileSucc(void)
{
	ASSERT(m_File.m_hFile != CFile::hFileNull);

	if(m_File.GetLength() < m_dwFileSize)
		return; // û�н��յ�ȫ�����ݣ�����

	// ���յ�ȫ������
	ResetFileRecver(); // ��λ�ļ�������

	if(!SendRecverSuccNetMsg())
		ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);
	else
		ForcePostMessage(m_hFileRecverHideWnd, WM_RECVFILESUCC, 0, (LPARAM)this);
}

// �����������ļ�������Ϣ�������߳�������
void CFileRecver::HandleRequestSendFileMsg(void)
{
	m_Mutex.Lock();

	if(IsRecving())
	{
		m_Mutex.Unlock();
		return;
	}

	// ѯ���û��Ƿ���մ��ļ�, ��������ո��ļ������֪���ͷ��ܾ��ļ���������
	BOOL bRecv = TRUE;
	if(m_OnRecvFileRequest != NULL)
		m_OnRecvFileRequest(m_pNotifyObj, m_szPathName, bRecv);
	if(!bRecv)
	{
		ResetFileRecver();
		if(!SendRefuseSendFileRequestNetMsg())
			ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);
		
		m_Mutex.Unlock();
		return;
	}
	
	// �򿪴����͵��ļ�
	if(!OpenRecvFile(m_szPathName)) 
	{
		ResetFileRecver();
		if(m_OnRecvFileFail != NULL)
			m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERFAIL);

		if(!SendRecverFailNetMsg()) // ���ն�ʧ��
			ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);

		m_Mutex.Unlock();
		return;
	}

	// ��ȡ�ѽ����ֽ���
	try
	{
		m_dwRecvedBytes = m_File.GetLength();
	}
	catch(...)
	{
		ResetFileRecver();
		if(m_OnRecvFileFail != NULL)
			m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERFAIL);

		if(!SendRecverFailNetMsg()) // ���ն�ʧ��
			ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);

		m_Mutex.Unlock();
		return;
	}
	
	// ���ͽ����ļ���������������Ϣ
	if(!SendAcceptSendFileRequestNetMsg())
	{
		ResetFileRecver();
		ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);

		m_Mutex.Unlock();
		return;
	}

	// ����Ƿ���ճɹ�
	CheckIfRecvFileSucc();
	m_Mutex.Unlock();
}

// �����Ͷ�ʧ�ܴ�����Ϣ
void CFileRecver::HandleSenderFailMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileFail != NULL)
		m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERFAIL);
	m_Mutex.Unlock();
}

// �����Ͷ�ȡ��������Ϣ
void CFileRecver::HandleSenderCancelMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileFail != NULL)
		m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERCANCEL);
	m_Mutex.Unlock();
}

// ��������ļ����ȴ�����Ϣ
void CFileRecver::HandleRecvFileProgressMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileProgress != NULL)
		m_OnRecvFileProgress(m_pNotifyObj, m_dwRecvedBytes, m_dwFileSize);
	m_Mutex.Unlock();
}

// ������ն˽��ճɹ���Ϣ
void CFileRecver::HandleRecverSuccMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileSucc != NULL)
		m_OnRecvFileSucc(m_pNotifyObj, m_szPathName);
	m_Mutex.Unlock();
}

// ������ն˽���ʧ����Ϣ
void CFileRecver::HandleRecverFailMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileFail != NULL)
		m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERFAIL);
	m_Mutex.Unlock();
}
