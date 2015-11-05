// TcpClient
// 可以发送网络信息，可以发送文件，支持断点续传
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

	// 事件指针
	m_OnSocketClose = NULL;
	m_OnOneNetMsg = NULL;
	m_OnSendFileSucc = NULL;
	m_OnSendFileFail = NULL;
	m_OnSendFileProgress = NULL;

	StartupSocket(); // 初始化windows socket
	RegisterClientSocketHideWndClass(); // 出错隐藏窗体类
}

CTcpClient::~CTcpClient(void)
{
	Disconnect(); // 断开socket连接
	
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

//  初始化windows socket
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

// 清除windows socket
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

// 隐藏窗体消息处理函数
LRESULT CTcpClient::HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CTcpClient *pTcpClient = (CTcpClient *)lParam;
	
	switch(uMsg)
	{
	case WM_SOCKETCLOSE:// socket被关闭
		pTcpClient->HandleSocketCloseMsg();
		break;// socket被关闭
	case WM_SOCKETSENDERR: // socket发送出错
		pTcpClient->HandleSocketSendErrMsg();
		break;
	case WM_SOCKETRECVERR: // socket接收出错
		pTcpClient->HandleSocketRecvErrMsg();
		break;
	case WM_RECVERACCEPTFILE: // 接收方接受文件
		pTcpClient->HandleRecverAcceptFileMsg((DWORD)wParam);
		break;
	case WM_RECVERREFUSEFILE: // 接收方拒绝文件
		pTcpClient->HandleRecverRefuseFileMsg();
		break;
	case WM_RECVERSUCC: // 接收方接收文件成功
		pTcpClient->HandleRecverSuccMsg();
		break;
	case WM_RECVERFAIL: // 接收方失败
		pTcpClient->HandleRecverFailMsg();
		break;
	case WM_RECVERCANCEL: // 接收方取消
		pTcpClient->HandleRecverCancelMsg();
		break;
	case WM_SENDERFAIL: // 接收方失败
		pTcpClient->HandleSenderFailMsg();
		break;
	case WM_SENDFILEPROGRESS: // 文件发送进度
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

// 创建隐含窗体
BOOL CTcpClient::AllocateWindow(void)
{
	if(m_hHideWnd == NULL)
	{
		m_hHideWnd = CreateWindow(STR_CLIENTSOCKETHIDEWNDCLASSNAME, NULL, WS_POPUP,  
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, NULL, NULL, AfxGetInstanceHandle(), this);
	}

	return (m_hHideWnd != NULL);
}

// 释放隐含窗体
void CTcpClient::DeallocateWindow(void)
{
	if(m_hHideWnd != NULL)
	{
		DestroyWindow(m_hHideWnd);
		m_hHideWnd = NULL;
	}
}

// 连接服务端
BOOL CTcpClient::Connect(void)
{
	u_long			ulTmp;
	SOCKADDR_IN		SockAddr;
	
	Disconnect(); // 断开连接

	if(!AllocateWindow())
		return FALSE;
	
	// 创建socket
	m_hSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
	if(m_hSocket == INVALID_SOCKET)
		goto ErrEntry;
	
	// 设置socket为阻塞方式
	ulTmp = 0;
	if(ioctlsocket(m_hSocket, FIONBIO, &ulTmp) != 0)
		goto ErrEntry;

	// 设置socket发送超时
	if(setsockopt(m_hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&m_nSendTimeOut, 
		sizeof(m_nSendTimeOut)) != 0)
		goto ErrEntry;
	
	// 连接服务端
	InitSockAddr(&SockAddr, m_szServerIpAddr, m_wPort);
	if(connect(m_hSocket, (sockaddr *)&SockAddr, sizeof(SOCKADDR_IN)) != 0)
		goto ErrEntry;
	
	// 创建接收线程
	m_pClientSocketRecvThread = new CClientSocketRecvThread(this, m_nSocketRecvThreadPriority);
	m_pClientSocketRecvThread->SetSocketHandle(m_hSocket);
	m_pClientSocketRecvThread->SetHideWndHandle(m_hHideWnd);
	m_pClientSocketRecvThread->Resume();
	
	return TRUE;
ErrEntry:
	Disconnect();
	return FALSE;
}

// 断开连接
void CTcpClient::Disconnect(void)
{
	// 清除文件发送线程
	if(m_pSendFileThread != NULL)
	{
		m_pSendFileThread->CloseSendFileThreadMute();
		delete m_pSendFileThread;
		m_pSendFileThread = NULL;
	}

	DeallocateWindow(); // 销毁窗体, 不再响应窗体消息

	// 关闭socket连接
	if(m_hSocket != INVALID_SOCKET)
	{
		closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}
	
	// 销毁接收线程
	if(m_pClientSocketRecvThread != NULL)
	{
		m_pClientSocketRecvThread->Terminate();
		WaitForSingleObject(m_pClientSocketRecvThread->GetThreadHandle(), INFINITE);
		delete m_pClientSocketRecvThread;
		m_pClientSocketRecvThread = NULL;
	}
}

// 返回当前socket连接状态
BOOL CTcpClient::IsConnect(void)
{
	return (m_hSocket != INVALID_SOCKET);
}

// 判断是否在发送
BOOL CTcpClient::IsSending(void)
{
	return (m_pSendFileThread != NULL);
}

BOOL CTcpClient::SendFile(char *szPathName)
{
	if(!IsConnect())
		return FALSE; // socket连接尚未打开
	if(IsSending())
		return FALSE; // 正在发送文件

	// 设置文件发送器的属性
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

	m_pSendFileThread->Resume(); // 开始发送
	
	return TRUE;
}

// 取消发送文件
void CTcpClient::CancelSendFile(void)
{
	if(IsSending())
	{
		m_pSendFileThread->CloseSendFileThreadCancel();
		delete m_pSendFileThread;
		m_pSendFileThread = NULL;
	}
}

// 发送网络消息
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

// 处理socket发送出错窗体消息
// socket发送出错后强制关闭socket连接
void CTcpClient::HandleSocketSendErrMsg(void)
{
	if(IsSending() && m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_SOCKETSENDERR);

	if(IsConnect() && m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_SOCKETSENDERR);

	Disconnect();
}

// 处理socket接收出错窗体消息
void CTcpClient::HandleSocketRecvErrMsg(void)
{
	if(IsSending() && m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_SOCKETRECVERR);

	if(IsConnect () && m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_SOCKETRECVERR);

	Disconnect();
}

// 处理socket关闭窗体消息
void CTcpClient::HandleSocketCloseMsg(void)
{
	if(IsSending() && m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_SOCKETCLOSE);
	
	if(IsConnect() && m_OnSocketClose != NULL)
		m_OnSocketClose(m_pNotifyObj, m_hSocket, SCR_PEERCLOSE);

	Disconnect(); // 断开socket连接
}

// 处理完整的网络消息
void CTcpClient::HandleOneNetMsg(char *Msg, int nMsgLen)
{
	if(m_OnOneNetMsg != NULL)
		m_OnOneNetMsg(m_pNotifyObj, Msg, nMsgLen);
}

// 处理接收端接受文件窗体消息
void CTcpClient::HandleRecverAcceptFileMsg(DWORD dwRecvedBytes)
{
	if(!IsSending())
		return;
	
	// 触发发送文件线程
	m_pSendFileThread->TrigAcceptFile(dwRecvedBytes);
}

// 处理接收端拒绝文件窗体消息
void CTcpClient::HandleRecverRefuseFileMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // 关闭文件发送线程
	
	if(m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_RECVERREFUSEFILE);

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// 处理接收方接收文件成功窗体消息
void CTcpClient::HandleRecverSuccMsg(void)
{
	if(!IsSending())
		return;
	
	// 接收到该窗体消息时文件必需已经被接受
	if(!m_pSendFileThread->IsFileAccepted())
		return;

	m_pSendFileThread->TrigRecvFileSucc();
	WaitForSingleObject(m_pSendFileThread->GetThreadHandle(), INFINITE); // 等待线程结束

	if(m_OnSendFileSucc != NULL)
		m_OnSendFileSucc(m_pNotifyObj, m_pSendFileThread->GetPathName());

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// 处理接收方失败窗体消息
void CTcpClient::HandleRecverFailMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // 关闭文件发送线程

	if(m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_RECVERFAIL);

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// 处理接收方取消文件接收窗体消息
void CTcpClient::HandleRecverCancelMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // 关闭文件发送线程

	if(m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_RECVERCANCEL);

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// 处理文件发送进度窗体消息
void CTcpClient::HandleSendFileProgressMsg(void)
{
	DWORD dwSentBytes; // 已发送字节数
	DWORD dwFileSize; // 文件大小

	if(!IsSending())
		return;

	if(m_OnSendFileProgress != NULL)
	{
		dwSentBytes = m_pSendFileThread->GetSentBytes();
		dwFileSize = m_pSendFileThread->GetFileSize();
		m_OnSendFileProgress(m_pNotifyObj, dwSentBytes, dwFileSize);
	}
}

// 处理文件发送失败窗体消息 
void CTcpClient::HandleSenderFailMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // 关闭文件发送线程

	if(m_OnSendFileFail != NULL)
		m_OnSendFileFail(m_pNotifyObj, m_pSendFileThread->GetPathName(), SFFR_SENDERFAIL);

	delete m_pSendFileThread;
	m_pSendFileThread = NULL;
}

// 处理等待超时窗体消息
void CTcpClient::HandleWaitTimeOutMsg(void)
{
	if(!IsSending())
		return;

	m_pSendFileThread->CloseSendFileThreadMute(); // 关闭文件发送线程

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

// 线程执行函数
void CSendFileThread::Execute(void)
{
	ASSERT(m_hHideWnd != NULL);
	ASSERT(m_hSocket != INVALID_SOCKET);

	// 等待接收方接受文件
	HANDLE hWaitAcceptFileEvents[3];
	hWaitAcceptFileEvents[0] = m_hCloseThreadMute;
	hWaitAcceptFileEvents[1] = m_hCloseThreadCancel;
	hWaitAcceptFileEvents[2] = m_hAcceptFile;
	
	// 等待接收方接收文件成功
	HANDLE hWaitRecvFileSuccEvents[3]; 
	hWaitRecvFileSuccEvents[0] = m_hCloseThreadMute;
	hWaitRecvFileSuccEvents[1] = m_hCloseThreadCancel;
	hWaitRecvFileSuccEvents[2] = m_hRecvFileSucc;

	// 请求发送文件
	if(!SendRequestSendFileNetMsg())
	{
		ResetSendFile();
		ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
	}

	// 等待接收方接受文件, 一旦接收方接受文件即开始发送文件数据
	switch(WaitForMultipleObjects(3, hWaitAcceptFileEvents, FALSE, m_nWaitTimeOut))
	{
	case WAIT_OBJECT_0: // 关闭线程
		ResetSendFile();
		return;
		break;
	case WAIT_OBJECT_0 + 1: // 发送端取消导致线程关闭
		ResetSendFile();
		if(!SendSenderCancelNetMsg())
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	case WAIT_OBJECT_0 + 2: // 接收端接受文件
		break;
	case WAIT_TIMEOUT: // 等待超时
		ResetSendFile();
		ForcePostMessage(WM_WAITTIMEOUT, 0, (LPARAM)m_pTcpClient);
		if(!SendSenderFailNetMsg())
			ForcePostMessage(WM_SOCKETRECVERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	default: // 出错
		ResetSendFile();
		ForcePostMessage(WM_SENDERFAIL, 0, (LPARAM)m_pTcpClient);
		if(!SendSenderFailNetMsg())
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	}

	// 发送文件数据
	SendFileData();

	// 等待接收方接收文件成功
	switch(WaitForMultipleObjects(3, hWaitRecvFileSuccEvents, FALSE, m_nWaitTimeOut))
	{
	case WAIT_OBJECT_0: // 关闭线程
		ResetSendFile();
		return;
		break;
	case WAIT_OBJECT_0 + 1: // 发送端取消导致线程关闭
		ResetSendFile();
		if(!SendSenderCancelNetMsg())
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	case WAIT_OBJECT_0 + 2: // 接收端接收文件成功
		ResetSendFile();
		break;
	case WAIT_TIMEOUT: // 等待超时
		ResetSendFile();
		ForcePostMessage(WM_WAITTIMEOUT, 0, (LPARAM)m_pTcpClient);
		if(!SendSenderFailNetMsg())
			ForcePostMessage(WM_SOCKETRECVERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	default: // 出错
		ResetSendFile();
		ForcePostMessage(WM_SENDERFAIL, 0, (LPARAM)m_pTcpClient);
		if(!SendSenderFailNetMsg())
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);
		return;
		break;
	}
}

// 准备发送文件
BOOL CSendFileThread::PrepareSendFile(char *szPathName)
{
	// 打开待发送的文件
	if (!m_File.Open(szPathName, CFile::modeRead | CFile::shareExclusive))
		return FALSE;
	
	// 获取文件大小
	try
	{
		m_dwFileSize = m_File.GetLength();
	}
	catch(...)
	{
		m_File.Abort();
		return FALSE;
	}

	strcpy(m_szPathName, szPathName); // 保存文件名
	m_dwSentBytes = 0; // 初始化已发送字节数

	return TRUE;
}

// 复位文件发送线程
void CSendFileThread::ResetSendFile(void)
{
	// 关闭文件
	if(m_File.m_hFile != CFile::hFileNull)
	{
		m_File.Close();
	}
}

// 检查文件是否已经被接收端接受
BOOL CSendFileThread::IsFileAccepted(void)
{
	return (WaitForSingleObject(m_hAcceptFile, 0) == WAIT_OBJECT_0);
}

// 发送文件数据
void CSendFileThread::SendFileData(void)
{
	DWORD dwTickCount;
	char Buf[MAX_NETMSGPACKAGESIZE + 6]; // 发送缓冲区
	int nBytes;

	// 消息头
	strncpy(&(Buf[6]), "-19", 3);

	// 发送进度
	ForcePostMessage(WM_SENDFILEPROGRESS, 0, (LPARAM)m_pTcpClient);

	while(!m_bTerminated && m_dwSentBytes < m_dwFileSize)
	{
		// 读取文件数据包
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

		// 发送
		if(!m_pTcpClient->SendNetMsg(Buf, 3 + nBytes))
		{
			ResetSendFile();
			ForcePostMessage(WM_SOCKETSENDERR, 0, (LPARAM)m_pTcpClient);

			return;
		}

		m_dwSentBytes += nBytes;

		// 发送进度
		dwTickCount = GetTickCount();
		if(dwTickCount < m_dwLastProgressTick) m_dwLastProgressTick = 0;
		if(dwTickCount - m_dwLastProgressTick > m_dwProgressTimeInterval || m_dwSentBytes >= m_dwFileSize)
		{
			ForcePostMessage(WM_SENDFILEPROGRESS, 0, (LPARAM)m_pTcpClient); // 文件接收进度
			m_dwLastProgressTick = dwTickCount;
		}
		else
			Sleep(0); // 强迫windows调度线程
	}
}

// 关闭文件发送线程
// 不发送任何网络消息给接收端
void CSendFileThread::CloseSendFileThreadMute(void)
{
	Terminate();
	SetEvent(m_hCloseThreadMute); // 关闭发送文件线程
	WaitForSingleObject(m_hThread, INFINITE); // 等待线程结束
}

// 关闭文件发送线程
// 发送取消发送文件网络消息给接收端
void CSendFileThread::CloseSendFileThreadCancel(void)
{
	Terminate();
	SetEvent(m_hCloseThreadCancel); // 关闭发送文件线程
	WaitForSingleObject(m_hThread, INFINITE); // 等待线程结束
}

// 发送请求发送文件网络消息
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

// 发送取消发送文件网络消息
BOOL CSendFileThread::SendSenderCancelNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-21", 3);
	return m_pTcpClient->SendNetMsg(Msg, 3);
}

// 发送发送文件失败网络消息
BOOL CSendFileThread::SendSenderFailNetMsg(void)
{
	char Msg[9];

	strncpy(&(Msg[6]), "-23", 3);
	return m_pTcpClient->SendNetMsg(Msg, 3);
}

// 接收端接受文件
void CSendFileThread::TrigAcceptFile(DWORD dwRecvedBytes)
{
	// 检查该消息的合法性
	if(WaitForSingleObject(m_hAcceptFile, 0) == WAIT_OBJECT_0)
		return;

	m_dwSentBytes = dwRecvedBytes;
	SetEvent(m_hAcceptFile);
}

// 接收端接收文件成功
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

// 线程执行体
void CClientSocketRecvThread::Execute(void)
{
	char Buf[SOCKET_RECVBUFSIZE]; // 接收缓冲区
	int nErr;

	while(!m_bTerminated)
	{
		nErr = recv(m_hSocket, Buf, SOCKET_RECVBUFSIZE, 0);
		if(nErr == SOCKET_ERROR) // 有错误发生
		{
			nErr = WSAGetLastError();
			if(nErr == WSAECONNRESET) // TODO: 请帮忙测试有无其它值会引起socket复位
			{
				// socket连接被peer端关闭，虚电路复位等错误，出现此类型的错误时socket必须被关闭。
				ForcePostMessage(WM_SOCKETCLOSE, 0, (LPARAM)m_pTcpClient);
				break;
			}
			else
			{
				// 正常的socket错误，接收数据出错
				ForcePostMessage(WM_SOCKETRECVERR, 0, (LPARAM)m_pTcpClient);
			}
		}
		else if(nErr == 0) // socket被被动关闭
		{
			ForcePostMessage(WM_SOCKETCLOSE, 0, (LPARAM)m_pTcpClient);
			break;
		}
		else // 接收到数据
		{
			DoRecv(Buf, nErr); // 处理接收到的数据
		}
	}; // while
}

// 将网络消息组装完整,然后调用相应的处理函数
void CClientSocketRecvThread::DoRecv(char *Buf, int nBytes)
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
		if(m_nRecvBufSize < 6) return; // 消息长度未完整接收,退出
		strncpy(szMsgLen, &(m_RecvBuf[1]), 5);
		szMsgLen[5] = 0;
		nMsgLen = atoi(szMsgLen);
		nTotalLen = 1 + 5 + nMsgLen;

		// 判断消息是否完整接收
		if(nTotalLen > m_nRecvBufSize) return; // 消息没有接收完整
		HandleRecvedNetMsg(&(m_RecvBuf[6]), nMsgLen); // 处理该网络消息
		m_nRecvBufSize -= nTotalLen;
		memcpy(m_RecvBuf, &(m_RecvBuf[nTotalLen]), m_nRecvBufSize); // 从消息缓冲区中去除该消息
	};
}


// 处理接收到的网络消息
void CClientSocketRecvThread::HandleRecvedNetMsg(char *Msg, int nMsgLen)
{
	char szCmd[4];
	int nCmd;

	strncpy(szCmd, Msg, 3);
	szCmd[3] = 0;
	nCmd = atoi(szCmd);

	switch(nCmd)
	{
	case -17: // 接收端接受文件
		{
			DWORD dwSentBytes;
			char szSentBytes[11];

			strncpy(szSentBytes, &(Msg[3]), 10);
			szSentBytes[10] = 0;
			dwSentBytes = atoi(szSentBytes);
			ForcePostMessage(WM_RECVERACCEPTFILE, dwSentBytes, (LPARAM)m_pTcpClient);
		}
		break;
	case -18: // 接收端拒绝接受文件
		ForcePostMessage(WM_RECVERREFUSEFILE, 0, (LPARAM)m_pTcpClient);
		break;
	case -20: // 接收端接收文件成功
		ForcePostMessage(WM_RECVERSUCC, 0, (LPARAM)m_pTcpClient);
		break;
	case -22: // 接收端取消接收文件
		ForcePostMessage(WM_RECVERCANCEL, 0, (LPARAM)m_pTcpClient);
		break;
	case -24: // 接收端接收文件出错
		ForcePostMessage(WM_RECVERFAIL, 0, (LPARAM)m_pTcpClient);
		break;
	default:
		m_pTcpClient->HandleOneNetMsg(Msg, nMsgLen);
		break;
	}
}
