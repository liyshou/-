// CTcpServer
// XSoft
// Author:  alone
// Contact: lovelypengpeng@eyou.com or hongxing777@msn.com
// Update Date: 2005.1.17
// Comment: if you find any bugs or have any suggestions please contact me.
// 可以收发网络消息, 同时可接收文件，支持断点续传，支持同时接收多个文件
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

	// 初始化事件指针
	m_OnAccept = NULL;
	m_OnAcceptErr = NULL;
	m_OnSocketConnect = NULL;
	m_OnSocketClose = NULL;
	m_OnOneNetMsg = NULL;
	m_OnRecvFileRequest = NULL;
	m_OnRecvFileProgress = NULL;
	m_OnRecvFileSucc = NULL;
	m_OnRecvFileFail = NULL;

	StartupSocket(); // 初始化windows socket
	
	RegisterAcceptWndClass(); // 注册用于接收socket连接的窗体类
	RegisterRecvWndClass(); // 注册用于接收数据的窗体类
	RegisterFileRecverWndClass(); // 注册用于文件接收器的窗体类

	m_pAcceptSocket = new CAcceptSocket(this, m_pNotifyObj); // 接收socket连接的socket对象
}

CTcpServer::~CTcpServer(void)
{
	StopAccept(); // 停止接收socket连接
	CancelAllRecvFile(); // 取消所有的文件接收
	CloseAllServerClientSocket(); // 关闭所有的客户端连接

	UnregisterClass(STR_RECVHIDEWNDCLASSNAME, AfxGetInstanceHandle());
	UnregisterClass(STR_ACCEPTHIDEWNDCLASSNAME, AfxGetInstanceHandle());
	UnregisterClass(STR_FILERECVERHIDEWNDCLASSNAME, AfxGetInstanceHandle());
	
	CleanupSocket(); 

	// 关闭并删除接受socket对象
	delete m_pAcceptSocket;
	m_pAcceptSocket = NULL;
}

// 初始化windows socket
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

// 清除windows socket
void CTcpServer::CleanupSocket(void)
{
	if(m_WSAData.wVersion != 0)
	{
		WSACleanup();
		memset(&m_WSAData, 0, sizeof(WSADATA));
	}
}

// 关闭所有的客户端socket连接
void CTcpServer::CloseAllServerClientSocket(void)
{
	CServerClientSocket	*pServerClientSocket;
	
	while(!m_ServerClientSockets.IsEmpty())
	{
		pServerClientSocket = m_ServerClientSockets.RemoveHead();
		delete pServerClientSocket;
	}
}

// 注册用于接收线程连接的窗体类
BOOL CTcpServer::RegisterAcceptWndClass(void)
{
	WNDCLASS wc;

	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = (WNDPROC)(CAcceptSocket::HideWndProc);
	wc.hInstance = AfxGetInstanceHandle();
	wc.lpszClassName = STR_ACCEPTHIDEWNDCLASSNAME;

	return (RegisterClass(&wc) != 0);
}

// 注册用于接收数据的窗体类
BOOL CTcpServer::RegisterRecvWndClass(void)
{
	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = (WNDPROC)(CServerClientSocket::HideWndProc);
	wc.hInstance = AfxGetInstanceHandle();
	wc.lpszClassName = STR_RECVHIDEWNDCLASSNAME;

	return (RegisterClass(&wc) != 0);
}

// 注册用于文件接收器的窗体类
BOOL CTcpServer::RegisterFileRecverWndClass(void)
{
	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.lpfnWndProc = (WNDPROC)(CFileRecver::HideWndProc);
	wc.hInstance = AfxGetInstanceHandle();
	wc.lpszClassName = STR_FILERECVERHIDEWNDCLASSNAME;

	return (RegisterClass(&wc) != 0);
}

// 开始接收socket连接
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

// 停止接收socket连接
void CTcpServer::StopAccept(void)
{
	m_pAcceptSocket->StopAccept();
}

// 添加socket对象
CServerClientSocket *CTcpServer::AddServerClientSocket(SOCKET hSocket)
{
	// 创建socket对象
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

	// 初始化接收socket
	if(!pServerClientSocket->InitServerClientSocket())
	{
		delete pServerClientSocket;
		return NULL;
	}
	
	// 添加到列表
	m_ServerClientSockets.AddTail(pServerClientSocket);

	return pServerClientSocket;
}

// 移除客户端socket对象
void CTcpServer::RemoveServerClientSocket(CServerClientSocket *pServerClientSocket)
{
	POSITION pos;

	pos = m_ServerClientSockets.Find(pServerClientSocket);
	if(pos != NULL)
		m_ServerClientSockets.RemoveAt(pos);
}

// 取消所有的接收
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

// 获取客户端数目
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

// 隐藏窗体消息处理函数
LRESULT CAcceptSocket::HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CAcceptSocket *pAcceptSocket = (CAcceptSocket *)lParam;

	switch(uMsg)
	{
	case WM_ACCEPT:
		pAcceptSocket->HandleAcceptMsg(wParam); // wParam为新socket句柄
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

// 创建隐含窗体
BOOL CAcceptSocket::AllocateWindow(void)
{
	if(m_hHideWnd == NULL)
	{
		m_hHideWnd = CreateWindow(STR_ACCEPTHIDEWNDCLASSNAME, NULL, WS_POPUP, CW_USEDEFAULT,  CW_USEDEFAULT, 
			0, 0, NULL, NULL, AfxGetInstanceHandle(), NULL);
	}
	
	return (m_hHideWnd != NULL);
}

// 释放隐含窗体
void CAcceptSocket::DeallocateWindow(void)
{
	if(m_hHideWnd != NULL)
	{
		DestroyWindow(m_hHideWnd);
		m_hHideWnd = NULL;
	}
}

// 处理接收socket连接窗体消息
void CAcceptSocket::HandleAcceptMsg(SOCKET hSocket)
{
	// 询问用户是否接受连接
	BOOL bAccept = TRUE;
	if(m_OnAccept != NULL)
		m_OnAccept(m_pNotifyObj, hSocket, bAccept);

	// 不接受连接
	if(!bAccept)
	{
		closesocket(hSocket);
		return;
	}

	// 添加连接
	CServerClientSocket *pServerClientSocket = m_pTcpServer->AddServerClientSocket(hSocket);
	if(pServerClientSocket == NULL)
		return;

	// 接受了一个新的socket连接
	if(m_OnSocketConnect != NULL)
		m_OnSocketConnect(m_pNotifyObj, hSocket);

	pServerClientSocket->ResumeServerClientRecvThread(); // go
}

// 处理接收出错窗体消息
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

	// 协议族
	pSockAddr->sin_family = PF_INET;
    
	// 绑定地址
	if(strcmp(szBindIpAddr, "") == 0)
		AIp = INADDR_NONE;
	else
		AIp = inet_addr(szBindIpAddr); // 转换点分十进制ip地址

    if(AIp == INADDR_NONE)
		pSockAddr->sin_addr.S_un.S_addr = INADDR_ANY; // 绑定到所有网络接口
    else
      pSockAddr->sin_addr.S_un.S_addr = AIp;

    pSockAddr->sin_port = htons(wPort); // 绑定端口
}

BOOL CAcceptSocket::StartAccept(void)
{
	u_long			ulTmp;
	SOCKADDR_IN		SockAddr;

	StopAccept();

	// 创建隐藏窗体
	if(!AllocateWindow())
		goto ErrEntry;

	// 创建socket
	m_hSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
	if(m_hSocket == INVALID_SOCKET)
		goto ErrEntry;

	// 设置socket为阻塞方式
	ulTmp = 0;
	if(ioctlsocket(m_hSocket, FIONBIO, &ulTmp) != 0)
		goto ErrEntry;

	// 绑定socket
	InitSockAddr(&SockAddr, m_szBindIpAddr, m_wPort);
	if(bind(m_hSocket, (sockaddr *)&SockAddr, sizeof(SOCKADDR_IN)) != 0)
		goto ErrEntry;

	// 监听
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

// 停止接收socket连接
void CAcceptSocket::StopAccept(void)
{
	// 关闭用于接受socket连接的socket
	if(m_hSocket != INVALID_SOCKET)
	{
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}
	
	// 关闭并删除socket接收线程
	if(m_pAcceptThread != NULL)
	{
		m_pAcceptThread->Terminate();
		WaitForSingleObject(m_pAcceptThread->m_hThread, INFINITE);
		delete m_pAcceptThread;
		m_pAcceptThread = NULL;
	}
	
	// 处理消息队列中的socket
	// 关闭未被接受的socket
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

// 接收线程执行体
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
			// 出错，结束侦听
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
// 隐藏窗体消息处理函数
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

	// 事件指针
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
	// 创建隐藏窗体
	if(!AllocateWindow())
		return FALSE;
	
	// 设置socket发送超时
	if(setsockopt(m_hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&m_nSendTimeOut, sizeof(m_nSendTimeOut)) != 0)
		return FALSE;
	
	// 创建接收文件线程
	m_pServerClientSocketRecvThread = new CServerClientSocketRecvThread(this, m_nSocketRecvThreadPriority);
	m_pServerClientSocketRecvThread->SetHideWndHandle(m_hHideWnd);
	m_pServerClientSocketRecvThread->SetSocketHandle(m_hSocket);

	// 创建文件接收器
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
	DeallocateWindow(); // 释放隐藏窗体
	
	// 关闭socket连接
	if(m_hSocket != INVALID_SOCKET)
	{
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	// 删除数据接收线程
	if(m_pServerClientSocketRecvThread != NULL)
	{
		m_pServerClientSocketRecvThread->Terminate();
		WaitForSingleObject(m_pServerClientSocketRecvThread->GetThreadHandle(), INFINITE);
		delete m_pServerClientSocketRecvThread;
		m_pServerClientSocketRecvThread = NULL;
	}
	
	// 删除文件接收器
	delete m_pFileRecver;
	m_pFileRecver = NULL;
}

void CServerClientSocket::ResumeServerClientRecvThread(void)
{
	// 委托给文件接收器
	m_pServerClientSocketRecvThread->Resume();
}

// 取消接收文件
 void CServerClientSocket::CancelRecvFile(void)
{
	ASSERT(m_pFileRecver != NULL);
	
	m_pFileRecver->CancelRecvFile();
}

// 创建隐含窗体
BOOL CServerClientSocket::AllocateWindow(void)
{
	if(m_hHideWnd == NULL)
	{
		m_hHideWnd = CreateWindow(STR_RECVHIDEWNDCLASSNAME, NULL, WS_POPUP,  CW_USEDEFAULT, CW_USEDEFAULT,  
			0, 0, NULL, NULL, AfxGetInstanceHandle(), NULL);
	}

	return (m_hHideWnd != NULL);
}

// 释放隐含窗体
void CServerClientSocket::DeallocateWindow(void)
{
	if(m_hHideWnd != NULL)
	{
		DestroyWindow(m_hHideWnd);
		m_hHideWnd = NULL;
	}
}

// 发送网络消息
BOOL CServerClientSocket::SendNetMsg(char *Msg, int nMsgLen)
{
	char szMsgLen[6];
	int nTotalLen;
	int nErr;
	int nSent = 0;
	
	ASSERT(nMsgLen <= MAX_PACKAGESIZE);

	// 计算网络消息信息
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
	m_pTcpServer->RemoveServerClientSocket(this); // 从列表中移除

	if(m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_PEERCLOSE);
}

// socket发送数据出错
void CServerClientSocket::HandleSocketSendErrMsg(void)
{
	m_pTcpServer->RemoveServerClientSocket(this); // 从列表中移除

	if(m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_SOCKETSENDERR);
}

// socket接收数据出错
void CServerClientSocket::HandleSocketRecvErrMsg(void)
{
	m_pTcpServer->RemoveServerClientSocket(this); // 从列表中移除

	if(m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_SOCKETRECVERR);
}

// 处理接收到的网络消息
// 注意: 该函数在接收线程中运行!!!
void CServerClientSocket::HandleRecvedNetMsg(char *Msg, int nMsgLen)
{
	char	szCmd[4];
	int		nCmd;

	strncpy(szCmd, Msg, 3);
	szCmd[3] = 0;
	nCmd = atoi(szCmd);
	if(nCmd >= 0) // 网络消息
		HandleOneNetMsg(Msg, nMsgLen);
	else // 与文件接收有关的网络消息，委托给文件接收器去处理
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
	int nMsgLen; // 消息长度
	int nTotalLen;

	// 拷贝数据到接收缓冲区
	if(m_nRecvBufSize + nBytes > NETMSG_RECVBUFSIZE)
		m_nRecvBufSize = 0; // 清空接收缓冲区中的数据
	memcpy(&(m_RecvBuf[m_nRecvBufSize]), Buf, nBytes);
	m_nRecvBufSize += nBytes;

	while(!m_bTerminated)
	{
		if(m_nRecvBufSize == 0) return; // 缓冲区中无数据,退出
		
		// 缓冲区中第一个字节必须是消息标志
		if(m_RecvBuf[0] != MSG_TAG) // 第一个字节不是消息标志
		{
			for(i = 0; i < m_nRecvBufSize; i++)
				if(m_RecvBuf[i] == MSG_TAG) break;
			m_nRecvBufSize -= i;
			memcpy(m_RecvBuf, &(m_RecvBuf[i]), m_nRecvBufSize);
		}

		// 获取消息内容字节数
		if(m_nRecvBufSize < 6)
			return; // 消息长度未完整接收,退出
		strncpy(szMsgLen, &(m_RecvBuf[1]), 5);
		szMsgLen[5] = 0;
		nMsgLen = atoi(szMsgLen);
		nTotalLen = 1 + 5 + nMsgLen;

		// 判断消息是否完整接收
		if(nTotalLen > m_nRecvBufSize)
			return; // 消息没有接收完整

		m_pServerClientSocket->HandleRecvedNetMsg(&(m_RecvBuf[6]), nMsgLen);
		m_nRecvBufSize -= nTotalLen;
		memcpy(m_RecvBuf, &(m_RecvBuf[nTotalLen]), m_nRecvBufSize); // 从消息缓冲区中去除该消息
	};
}

void CServerClientSocketRecvThread::Execute(void)
{
	int		nErr;
	char	Buf[SOCKET_RECVBUFSIZE];

	while(!m_bTerminated)
	{
		nErr = recv(m_hSocket, Buf, SOCKET_RECVBUFSIZE, 0);
		if(nErr == SOCKET_ERROR) // 有错误发生
		{
			nErr = WSAGetLastError();
			if(nErr == WSAECONNRESET)
			{
				// socket连接被peer端关闭，虚电路复位等错误
				// 出现如此类型的错误时socket必须被关闭。
				ForcePostMessage(WM_SOCKETCLOSE, 0, (LPARAM)m_pServerClientSocket);
				break;
			}
			else
			{
				// 正常的socket错误：接收数据出错
				ForcePostMessage(WM_SOCKETRECVERR, 0, (LPARAM)m_pServerClientSocket);
			}
		}
		else if(nErr == 0) // socket被被动关闭
		{
			ForcePostMessage(WM_SOCKETCLOSE, 0, (LPARAM)m_pServerClientSocket);
			break;
		}
		else // 接收到数据
			DoRecv(Buf, nErr); // 处理接收到的数据
	} // while
}

/////////////////////////////////////////////////////////////////////////
// CFileRecver
/////////////////////////////////////////////////////////////////////////
// 隐藏窗体消息处理函数
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

	// 事件指针
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

// 创建隐含窗体
BOOL CFileRecver::AllocateWindow(void)
{
	if(m_hFileRecverHideWnd == NULL)
	{
		m_hFileRecverHideWnd = CreateWindow(STR_FILERECVERHIDEWNDCLASSNAME, NULL, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 
			0, 0, NULL, NULL, AfxGetInstanceHandle(), NULL);
	}

	return (m_hFileRecverHideWnd != NULL);
}

// 释放隐含窗体
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

// 处理接收到的网络消息
// 注意：该函数在接收线程中运行!!!
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

// 处理请求发送文件网络消息
// 注意:该函数在socket接收线程中运行!!!
void CFileRecver::HandleRequestSendFileNetMsg(char *Msg, int nMsgLen)
{
	char szFileSize[11];

	ResetFileRecver(); // 复位文件接收器

	// 获取文件大小
	strncpy(szFileSize, &(Msg[3]), 10);
	szFileSize[10] = 0;
	m_dwFileSize = (DWORD)_atoi64(szFileSize);

	// 获取文件名
	strncpy(m_szPathName, &(Msg[13]), nMsgLen - 13);
	m_szPathName[nMsgLen - 13] = 0;

	// 发送窗体消息，询问用户是否接收此文件
	ForcePostMessage(m_hFileRecverHideWnd, WM_REQUESTSENDFILE, 0, (LPARAM)this);
}

// 处理文件数据网络消息
void CFileRecver::HandleFileDataNetMsg(char *Msg, int nMsgLen)
{
	if(!IsRecving())
		return;

	// 写入文件数据
	try
	{
		m_File.SeekToEnd(); // 添加数据到文件尾部
		m_File.Write(&(Msg[3]), nMsgLen - 3); // 写数据到文件, 去除消息头
		m_dwRecvedBytes = m_File.GetLength();
	}
	catch(...) // 操作文件失败，接收失败
	{
		ResetFileRecver(); // 复位文件接收器
		ForcePostMessage(m_hFileRecverHideWnd, WM_RECVFILEFAIL, 0, (LPARAM)this); // 接收文件出错
		if(!SendRecverFailNetMsg()) // 发送接收端出错网络消息
			ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket); // socket出错
	
		return;
	}

	DWORD dwTickCount = GetTickCount();
	if(dwTickCount < m_dwLastProgressTick) m_dwLastProgressTick = 0;
	if(dwTickCount - m_dwLastProgressTick > m_dwProgressTimeInterval || m_dwRecvedBytes >= m_dwFileSize)
	{
		ForcePostMessage(m_hFileRecverHideWnd, WM_RECVFILEPROGRESS, 0, (LPARAM)this); // 文件接收进度
		m_dwLastProgressTick = dwTickCount;
	}
	else
		Sleep(0); // 强迫windows调度线程

	CheckIfRecvFileSucc(); // 检查接收文件是否成功
}

// 处理取消发送文件消息
void CFileRecver::HandleSenderCancelNetMsg(char *Msg, int nMsgLen)
{
	if(!IsRecving())
		return;

	ResetFileRecver(); // 复位文件接收器
	ForcePostMessage(m_hFileRecverHideWnd, WM_SENDERCANCEL, 0, (LPARAM)this);
}

// 处理发送方出错网络消息
void CFileRecver::HandleSenderFailNetMsg(char *Msg, int nMsgLen)
{
	if(!IsRecving())
		return;

	ResetFileRecver(); // 复位文件接收器
	ForcePostMessage(m_hFileRecverHideWnd, WM_SENDERFAIL, 0, (LPARAM)this);
}

// 发送接受文件发送请求网络消息
BOOL CFileRecver::SendAcceptSendFileRequestNetMsg(void)
{
	char szSize[11];
	char Msg[19];

	strncpy(&(Msg[6]), "-17", 3);
	sprintf(szSize, "%010d", m_dwRecvedBytes);
	strncpy(&(Msg[9]), szSize, 10);
	return m_pServerClientSocket->SendNetMsg(Msg, 13);
}

// 发送拒绝文件发送请求网络消息
BOOL CFileRecver::SendRefuseSendFileRequestNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-18", 3);
	return m_pServerClientSocket->SendNetMsg(Msg, 3);
}

// 发送接收端失败网络消息
BOOL CFileRecver::SendRecverFailNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-24", 3);
	return m_pServerClientSocket->SendNetMsg(Msg, 3);
}

// 发送接收端取消网络消息
BOOL CFileRecver::SendRecverCancelNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-22", 3);
	return m_pServerClientSocket->SendNetMsg(Msg, 3);
}

// 发送接收端成功网络消息
BOOL CFileRecver::SendRecverSuccNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-20", 3);
	return m_pServerClientSocket->SendNetMsg(Msg, 3);
}

// 初始化文件接收器
BOOL CFileRecver::InitFileRecver(void)
{
	if(!AllocateWindow())
		return FALSE;

	return TRUE;
}

// 是否正在接收文件
BOOL CFileRecver::IsRecving(void)
{
	return (m_File.m_hFile != CFile::hFileNull);
}

// 取消接收文件, 只能从主线程中调用，否则会产生死锁
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

// 复位文件接收器
void CFileRecver::ResetFileRecver(void)
{
	// 清除窗体中的消息
	MSG msg;
	while(PeekMessage(&msg, m_hFileRecverHideWnd, WM_RECVFILEPROGRESS, WM_RECVFILEPROGRESS, PM_REMOVE));
	
	// 关闭待接收的文件
	if(m_File.m_hFile != CFile::hFileNull)
		m_File.Close();
}

// 打开待接收的文件
BOOL CFileRecver::OpenRecvFile(char *szPathName)
{
	UINT nOpenFlags;

	// 打开文件，如果文件不存在则创建新文件
	nOpenFlags = CFile::modeWrite | CFile::shareExclusive;
	if(!IsFileExists(szPathName))
		nOpenFlags |= CFile::modeCreate;

	return m_File.Open(szPathName, nOpenFlags);
}

// 检查文件是否接收成功
void CFileRecver::CheckIfRecvFileSucc(void)
{
	ASSERT(m_File.m_hFile != CFile::hFileNull);

	if(m_File.GetLength() < m_dwFileSize)
		return; // 没有接收到全部数据，返回

	// 接收到全部数据
	ResetFileRecver(); // 复位文件接收器

	if(!SendRecverSuccNetMsg())
		ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);
	else
		ForcePostMessage(m_hFileRecverHideWnd, WM_RECVFILESUCC, 0, (LPARAM)this);
}

// 处理请求发送文件窗体消息，在主线程中运行
void CFileRecver::HandleRequestSendFileMsg(void)
{
	m_Mutex.Lock();

	if(IsRecving())
	{
		m_Mutex.Unlock();
		return;
	}

	// 询问用户是否接收此文件, 如果不接收该文件，则告知发送方拒绝文件传输请求
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
	
	// 打开待发送的文件
	if(!OpenRecvFile(m_szPathName)) 
	{
		ResetFileRecver();
		if(m_OnRecvFileFail != NULL)
			m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERFAIL);

		if(!SendRecverFailNetMsg()) // 接收端失败
			ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);

		m_Mutex.Unlock();
		return;
	}

	// 获取已接收字节数
	try
	{
		m_dwRecvedBytes = m_File.GetLength();
	}
	catch(...)
	{
		ResetFileRecver();
		if(m_OnRecvFileFail != NULL)
			m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERFAIL);

		if(!SendRecverFailNetMsg()) // 接收端失败
			ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);

		m_Mutex.Unlock();
		return;
	}
	
	// 发送接受文件发送请求网络消息
	if(!SendAcceptSendFileRequestNetMsg())
	{
		ResetFileRecver();
		ForcePostMessage(m_hSocketHideWnd, WM_SOCKETSENDERR, 0, (LPARAM)m_pServerClientSocket);

		m_Mutex.Unlock();
		return;
	}

	// 检查是否接收成功
	CheckIfRecvFileSucc();
	m_Mutex.Unlock();
}

// 处理发送端失败窗体消息
void CFileRecver::HandleSenderFailMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileFail != NULL)
		m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERFAIL);
	m_Mutex.Unlock();
}

// 处理发送端取消窗体消息
void CFileRecver::HandleSenderCancelMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileFail != NULL)
		m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERCANCEL);
	m_Mutex.Unlock();
}

// 处理接收文件进度窗体消息
void CFileRecver::HandleRecvFileProgressMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileProgress != NULL)
		m_OnRecvFileProgress(m_pNotifyObj, m_dwRecvedBytes, m_dwFileSize);
	m_Mutex.Unlock();
}

// 处理接收端接收成功消息
void CFileRecver::HandleRecverSuccMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileSucc != NULL)
		m_OnRecvFileSucc(m_pNotifyObj, m_szPathName);
	m_Mutex.Unlock();
}

// 处理接收端接收失败消息
void CFileRecver::HandleRecverFailMsg(void)
{
	m_Mutex.Lock();
	if(m_OnRecvFileFail != NULL)
		m_OnRecvFileFail(m_pNotifyObj, m_szPathName, RFFR_RECVERFAIL);
	m_Mutex.Unlock();
}
