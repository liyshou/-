// CTcpServer
// XSoft
// Author:  alone
// Contact: lovelypengpeng@eyou.com or hongxing777@msn.com
// Update Date: 2005.1.17
// Comment: if you find any bugs or have any suggestions please contact me.
// 可以收发网络消息, 同时可接收文件，支持断点续传，支持同时接收多个文件
//
#ifndef _TCPSERVER_INCLUDE_
#define _TCPSERVER_INCLUDE_

#include <afxtempl.h>
#include <winsock2.h>
#include "Thread.h"
#include "afxmt.h"

// 缺省值定义
#define DFT_PORT						(8000) // socket端口
#define DFT_SENDTIMEOUT					(15000) // socket发送超时
#define DFT_PROGRESSTIMEINTERVAL		(100) // 100ms
#define DFT_ACCEPTTHREADPRIORITY		(THREAD_PRIORITY_ABOVE_NORMAL)
#define DFT_SOCKETRECVTHREADPRIORITY	(THREAD_PRIORITY_BELOW_NORMAL)

// 常量定义
#define MSG_TAG							('@')
#define SOCKET_RECVBUFSIZE				(32768)
#define MAX_PACKAGESIZE					(99999)
#define MAX_FILEPACKAESIZE				(MAX_PACKAGESIZE - 3)
#define NETMSG_RECVBUFSIZE				(2 * MAX_PACKAGESIZE) // 网络消息缓冲区大小
#define STR_ACCEPTHIDEWNDCLASSNAME		_T("accept-socket-hide-wndclass")
#define STR_RECVHIDEWNDCLASSNAME		_T("server-client-socket-hide-wndclass")
#define STR_FILERECVERHIDEWNDCLASSNAME	_T("filerecver-hide-wndclass")

#define WM_TCPSERVERBASE				(WM_USER + 1000)
#define WM_ACCEPT						(WM_TCPSERVERBASE + 0) // 接收到socket连接
#define WM_ACCEPTERR					(WM_TCPSERVERBASE + 1) // 接收出错

#define WM_SOCKETCLOSE					(WM_TCPSERVERBASE + 2) // socket被关闭
#define WM_SOCKETSENDERR				(WM_TCPSERVERBASE + 3) // socket发送出错
#define WM_SOCKETRECVERR				(WM_TCPSERVERBASE + 4) // socket接收出错

#define WM_REQUESTSENDFILE				(WM_TCPSERVERBASE + 5) // 请求发送文件
#define WM_RECVFILEPROGRESS				(WM_TCPSERVERBASE + 6) // 文件接收进度
#define WM_SENDERFAIL					(WM_TCPSERVERBASE + 7) // 发送端失败
#define WM_SENDERCANCEL					(WM_TCPSERVERBASE + 8) // 发送端取消
#define WM_RECVFILEFAIL					(WM_TCPSERVERBASE + 9) // 文件接收失败
#define WM_RECVFILESUCC					(WM_TCPSERVERBASE + 10) // 文件接收成功

// 前置声明
class CTcpServer;
class CAcceptSocket; // 用于接收的socket
class CAcceptThread; // socket接受线程
class CServerClientSocket; // 接收文件socket
class CServerClientSocketRecvThread; // 接收线程
class CFileRecver; // 文件接收器

// socket关闭的原因
typedef enum {
	SCR_PEERCLOSE, // socket被对等端关闭
	SCR_SOCKETSENDERR, // socket发送出错
	SCR_SOCKETRECVERR // socket接收出错
}EMSocketCloseReason;

// 接收文件出错的原因
typedef enum {
	RFFR_SOCKETCLOSE, // socket被关闭
	RFFR_SOCKETSENDERR, // socket发送出错
	RFFR_SOCKETRECVERR, // socket接收出错
	RFFR_SENDERCANCEL, // 发送端取消
	RFFR_SENDERFAIL, // 发送端拒绝
	RFFR_RECVERCANCEL, // 接收端取消
	RFFR_RECVERFAIL, // 接收端失败
}EMRecvFileFailReason;

// 事件定义
typedef void (*AcceptFun)(void *pNotifyObj, SOCKET hSocket, BOOL &bAccept);
typedef void (*AcceptErrFun)(void *pNotifyObj, SOCKET hAccept);
typedef void (*SocketConnectFun)(void *pNoitfyObj, SOCKET hSocket);
typedef void (*SocketCloseFun)(void *pNotifyObj, SOCKET hSocket, EMSocketCloseReason SocketCloseReason);
typedef void (*OneNetMsgFun)(void *pNotify, char *Msg, int nMsgLen);
typedef void (*RecvFileRequestFun)(void *pNotifyObj, char *szPathName, BOOL &bRecv); // 请求接收文件
typedef void (*RecvFileProgressFun)(void *pNotifyObj, DWORD dwRecvedBytes, DWORD dwFileSize);
typedef void (*RecvFileSuccFun)(void *pNotifyObj, char *szPathName);
typedef void (*RecvFileFailFun)(void *pNotifyObj, char *szPathName, EMRecvFileFailReason RecvFileFailReason);

class CTcpServer
{
private:
	WSADATA								m_WSAData;
	void								*m_pNotifyObj; // 事件通知对象指针
	CAcceptSocket						*m_pAcceptSocket; // 用于接收socket连接的socket
	CList <CServerClientSocket *, CServerClientSocket *> m_ServerClientSockets; // 客户端socket列表
	char								m_szBindIpAddr[MAX_PATH]; // 绑定ip地址
	WORD								m_wPort; // 端口
	int									m_nSendTimeOut; // 发送超时
	DWORD								m_dwProgressTimeInterval; // 进度时间间隔
	int									m_nAcceptThreadPriority; // 接收线程优先级
	int									m_nSocketRecvThreadPriority; // socket数据接收线程优先级

	// 事件指针
	AcceptFun							m_OnAccept;
	AcceptErrFun						m_OnAcceptErr;
	SocketConnectFun					m_OnSocketConnect;
	SocketCloseFun						m_OnSocketClose;
	OneNetMsgFun						m_OnOneNetMsg;
	RecvFileRequestFun					m_OnRecvFileRequest;
	RecvFileProgressFun					m_OnRecvFileProgress;
	RecvFileSuccFun						m_OnRecvFileSucc;
	RecvFileFailFun						m_OnRecvFileFail;

	BOOL StartupSocket(void); // 初始化windows socket
	void CleanupSocket(void); // 清除windows socket
	BOOL RegisterAcceptWndClass(void);
	BOOL RegisterRecvWndClass(void);
	BOOL RegisterFileRecverWndClass(void);
public:
	CTcpServer(void *pNotifyObj);
	~CTcpServer(void);

	// get / set 函数
	void SetBindIpAddr(char *szBindAddr);
	void SetPort(WORD wPort);
	void SetSendTimeOut(int nSendTimeOut);
	void SetProgressTimeInterval(DWORD dwProgressTimeInterval);
	void SetAcceptThreadPriority(int nPriority);
	void SetSocketRecvThreadPriority(int nPriority);
	void SetOnAccept(AcceptFun OnAccept);
	void SetOnAcceptErr(AcceptErrFun OnAcceptErr);
	void SetOnSocketConnect(SocketConnectFun OnSocketConnect);
	void SetOnSocketClose(SocketCloseFun OnSocketClose);
	void SetOnOneNetMsg(OneNetMsgFun OnOneNetMsg);
	void SetOnRecvFileRequest(RecvFileRequestFun OnRecvFileRequest);
	void SetOnRecvFileProgress(RecvFileProgressFun OnRecvFileProgress);
	void SetOnRecvFileSucc(RecvFileSuccFun OnRecvFileSucc);
	void SetOnRecvFileFail(RecvFileFailFun OnRecvFileFail);

	BOOL StartAccept(void);
	void StopAccept(void);
	CServerClientSocket *AddServerClientSocket(SOCKET hSocket);
	void RemoveServerClientSocket(CServerClientSocket *pServerClientSocket);
	int GetClientCount();
	void CancelAllRecvFile(void);
	void CloseAllServerClientSocket(void);
};

// 用于接受socket连接的socket
class CAcceptSocket
{
private:
	CTcpServer							*m_pTcpServer;
	void								*m_pNotifyObj;
	HWND								m_hHideWnd; // 隐藏窗体句柄
	SOCKET								m_hSocket; // socket句柄
	char								m_szBindIpAddr[MAX_PATH]; // 绑定ip地址
	WORD								m_wPort; // 端口
	int									m_nAcceptThreadPriority;
	CAcceptThread						*m_pAcceptThread; // 接收线程

	// 事件指针
	AcceptFun							m_OnAccept; // socket连接事件
	AcceptErrFun						m_OnAcceptErr; // 接收socket连接出错事件
	SocketConnectFun					m_OnSocketConnect; // socket连接事件

	void InitSockAddr(SOCKADDR_IN *pSockAddr, char *szBindIpAddr, WORD wPort);
	BOOL AllocateWindow(void);
	void DeallocateWindow(void);

	// windows消息处理函数
	void HandleAcceptMsg(SOCKET hSocket);
	void HandleAcceptErrMsg(void);
public:
	static LRESULT CALLBACK HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CAcceptSocket(CTcpServer *pTcpServer, void *pNotifyObj);
	~CAcceptSocket(void);

	BOOL StartAccept(void);
	void StopAccept(void);

	// get / set 函数
	void SetHideWndHandle(HWND hHideWnd);
	void SetBindIpAddr(char *szBindAddr);
	void SetPort(WORD wPort);
	void SetAcceptThreadPriority(int nPriority);
	void SetOnAccept(AcceptFun OnAccept);
	void SetOnAcceptErr(AcceptErrFun OnAcceptErr);
	void SetOnSocketConnect(SocketConnectFun OnSocketConnect);
};

// 用于接受socket连接的线程对象
class CAcceptThread: public CThread
{
private:
	CAcceptSocket						*m_pAcceptSocket;
	HWND								m_hHideWnd; // 隐藏窗体句柄
	SOCKET								m_hSocket; // socket句柄

	void ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	void Execute(void);
public:
	CAcceptThread(CAcceptSocket *pAcceptSocket, int nPriority);

	// get / set 函数
	void SetHideWndHandle(HWND hHideWnd);
	void SetSocketHandle(SOCKET hSocket);
};

// 接收文件socket
class CServerClientSocket
{
private:
	CTcpServer							*m_pTcpServer;
	void								*m_pNotifyObj;
	HWND								m_hHideWnd; // 隐藏窗体句柄
	SOCKET								m_hSocket; // socket句柄
	int									m_nSendTimeOut; // socket发送超时
	DWORD								m_dwProgressTimeInterval; // 进度时间间隔
	int									m_nSocketRecvThreadPriority;
	CServerClientSocketRecvThread		*m_pServerClientSocketRecvThread; // socket接收线程
	CFileRecver							*m_pFileRecver; // 文件接收器

	// 事件指针
	OneNetMsgFun						m_OnOneNetMsg;
	SocketCloseFun						m_OnSocketClose;
	RecvFileRequestFun					m_OnRecvFileRequest;
	RecvFileProgressFun					m_OnRecvFileProgress;
	RecvFileSuccFun						m_OnRecvFileSucc;
	RecvFileFailFun						m_OnRecvFileFail;

	void FiniRecvFileSocket(void);
	BOOL AllocateWindow(void);
	void DeallocateWindow(void);

	// 窗体消息处理函数
	void HandleSocketCloseMsg(void);
	void HandleSocketSendErrMsg(void);
	void HandleSocketRecvErrMsg(void);

	// 网络消息处理函数
	void HandleOneNetMsg(char *Msg, int nMsgLen); // 接收到一条完整的网络消息
public:
	static LRESULT CALLBACK HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CServerClientSocket(CTcpServer *pTcpServer, void *pNotifyObj);
	~CServerClientSocket(void);

	// get / set 函数
	void SetSocketHandle(SOCKET hSocket);
	void SetSendTimeOut(int nSendTimeOut);
	void SetProgressTimeInterval(DWORD dwProgressTimeInterval);
	void SetSocketRecvThreadPriority(int nPriority);
	void SetOnSocketClose(SocketCloseFun OnSocketClose);
	void SetOnOneNetMsg(OneNetMsgFun OnOneNetMsg);
	void SetOnRecvFileRequest(RecvFileRequestFun OnRecvFileRequest);
	void SetOnRecvFileProgress(RecvFileProgressFun OnRecvFileProgress);
	void SetOnRecvFileSucc(RecvFileSuccFun OnRecvFileSucc);
	void SetOnRecvFileFail(RecvFileFailFun OnRecvFileFail);

	BOOL InitServerClientSocket(void);
	void ResumeServerClientRecvThread(void);
	void CancelRecvFile(void);
	void HandleRecvedNetMsg(char *Msg, int nMsgLen); // 处理接收到的网络消息
	BOOL SendNetMsg(char *Msg, int nMsgLen);
};

// 客户端接收线程
class CServerClientSocketRecvThread: public CThread
{
private:
	CServerClientSocket					*m_pServerClientSocket;
	HWND								m_hHideWnd; // 隐藏窗体句柄
	SOCKET								m_hSocket; // socket句柄
	char								m_RecvBuf[NETMSG_RECVBUFSIZE]; // 网络消息接收缓冲区
	int									m_nRecvBufSize; // 接收缓冲区中字节数

	void ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	void Execute(void);
	void DoRecv(char *Buf, int nBytes);
public:
	CServerClientSocketRecvThread(CServerClientSocket *pServerClientSocket, int nPriority);

	// get / set 函数
	void SetHideWndHandle(HWND hHideWnd);
	void SetSocketHandle(SOCKET hSocket);
};

// 文件接收器
class CFileRecver
{
private:
	CServerClientSocket					*m_pServerClientSocket;
	void								*m_pNotifyObj;
	CThread								*m_pSocketRecvThread;
	HWND								m_hSocketHideWnd; // socket隐藏窗体句柄
	HWND								m_hFileRecverHideWnd; // 文件接收器隐藏窗体句柄
	DWORD								m_dwProgressTimeInterval;
	CFile								m_File;
	CMutex								m_Mutex;

	// 状态变量
	char								m_szPathName[MAX_PATH];
	DWORD								m_dwFileSize; // 文件大小
	DWORD								m_dwRecvedBytes; // 已接收大小
	DWORD								m_dwLastProgressTick; // 上次发送进度消息的时间

	// 事件指针
	RecvFileRequestFun					m_OnRecvFileRequest;
	RecvFileProgressFun					m_OnRecvFileProgress;
	RecvFileSuccFun						m_OnRecvFileSucc;
	RecvFileFailFun						m_OnRecvFileFail;

	void ForcePostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	BOOL AllocateWindow(void);
	void DeallocateWindow(void);
	void ResetFileRecver(void);
	BOOL OpenRecvFile(char *szPathName);

	void CheckIfRecvFileSucc(void);

	// 发送网络消息
	BOOL SendAcceptSendFileRequestNetMsg(void); // 发送接受发送文件请求网络消息
	BOOL SendRefuseSendFileRequestNetMsg(void); // 拒绝发送文件请求网络消息
	BOOL SendRecverSuccNetMsg(void); // 发送接收端成功网络消息
	BOOL SendRecverFailNetMsg(void); // 发送接收端失败网络消息
	BOOL SendRecverCancelNetMsg(void); // 发送接收端取消网络消息

	// 窗体消息处理函数
	void HandleRequestSendFileMsg(void);
	void HandleSenderCancelMsg(void);
	void HandleSenderFailMsg(void);
	void HandleRecvFileProgressMsg(void);
	void HandleRecverSuccMsg(void);
	void HandleRecverFailMsg(void);
public:
	static LRESULT HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CFileRecver(CServerClientSocket *pServerClientSocket, CThread *pSocketRecvThread, void *pNotifyObj);
	~CFileRecver(void);

	// get / set 函数
	void SetProgressTimeInterval(DWORD dwProgressTimeInterval);
	void SetSocketHideWndHandle(HWND hHideWnd);
	void SetOnRecvFileRequest(RecvFileRequestFun OnRecvFileRequest);
	void SetOnRecvFileProgress(RecvFileProgressFun OnRecvFileProgress);
	void SetOnRecvFileSucc(RecvFileSuccFun OnRecvFileSucc);
	void SetOnRecvFileFail(RecvFileFailFun OnRecvFileFail);

	void HandleRecvedNetMsg(int nCmd, char *Msg, int nMsgLen);
	void HandleRequestSendFileNetMsg(char *Msg, int nMsgLen);
	void HandleFileDataNetMsg(char *Msg, int nMsgLen);
	void HandleSenderCancelNetMsg(char *Msg, int nMsgLen);
	void HandleSenderFailNetMsg(char *Msg, int nMsgLend);

	BOOL InitFileRecver(void);
	BOOL IsRecving(void); // 是否正在接收文件
	void CancelRecvFile(void); // 取消接收文件
};

#endif // #ifndef _TCPSERVER_INCLUDE_
