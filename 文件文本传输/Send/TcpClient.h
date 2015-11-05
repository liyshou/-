// TcpClient
// 可以发送网络信息，可以发送文件，支持断点续传
// XSoft
// Contact: hongxing777@msn.com or lovelypengpeng@eyou.com
//
#ifndef _TCPCLIENT_INCLUDE_
#define _TCPCLIENT_INCLUDE_

#include <winsock2.h>
#include "Thread.h"

// 缺省值定义
#define DFT_SOCKETPORT						(8000) // 缺省端口
#define DFT_FILEPACKAGESIZE					(1000) // 缺省文件包大小
#define DFT_WAITTIMEOUT						(60000) // 等待超时
#define DFT_SENDTIMEOUT						(10000) // 发送超时
#define DFT_SOCKETRECVTHREADPRIORITY		(THREAD_PRIORITY_NORMAL)
#define DFT_SENDFILETHREADPRIORITY			(THREAD_PRIORITY_BELOW_NORMAL)
#define DFT_PROGRESSTIMEINTERVAL			(100) // 进度消息间隔时间

// 常量定义
#define MSG_TAG								('@')
#define SOCKET_RECVBUFSIZE					(32768) // socket接收缓冲区的大小
#define MAX_NETMSGPACKAGESIZE				(99999) // 网络消息包的大小, 该值决定于协议中用于表示消息长度的字节数。
#define MAX_FILEPACKAESIZE					(MAX_NETMSGPACKAGESIZE - 3) // 文件包的大小
#define NETMSG_RECVBUFSIZE					(2 * MAX_NETMSGPACKAGESIZE) // 网络消息缓冲区大小
#define STR_CLIENTSOCKETHIDEWNDCLASSNAME	("client-socket-hide-wndclass") // 隐藏窗体类名

#define WM_TCPCLIENTBASE					(WM_USER + 1000)
#define WM_SOCKETRECVERR					(WM_TCPCLIENTBASE + 0) // socket接收出错
#define WM_SOCKETSENDERR					(WM_TCPCLIENTBASE + 1) // socket发送出错
#define	WM_SOCKETCLOSE						(WM_TCPCLIENTBASE + 2) // socket被关闭

#define WM_RECVERACCEPTFILE					(WM_TCPCLIENTBASE + 20) // 接收端接受文件
#define WM_RECVERREFUSEFILE					(WM_TCPCLIENTBASE + 21) // 接收端拒绝文件
#define WM_RECVERSUCC						(WM_TCPCLIENTBASE + 22) // 接收端成功
#define WM_RECVERCANCEL						(WM_TCPCLIENTBASE + 23) // 接收端取消
#define WM_RECVERFAIL						(WM_TCPCLIENTBASE + 14) // 接收端出错

#define WM_WAITTIMEOUT						(WM_TCPCLIENTBASE + 40) // 等待超时
#define WM_SENDERFAIL						(WM_TCPCLIENTBASE + 41) // 发送端出错
#define WM_SENDFILEPROGRESS					(WM_TCPCLIENTBASE + 42) // 发送文件进度
// 前置声明
class CTcpClient; // Tcp客户端类
class CClientSocketRecvThread; // socket数据接收线程
class CSendFileThread; // 发送文件线程

// socket关闭的原因
typedef enum {
	SCR_PEERCLOSE, // socket被对等端关闭
	SCR_SOCKETSENDERR, // socket发送出错
	SCR_SOCKETRECVERR // socket接收出错
}EMSocketCloseReason;

// 发送文件失败原因
typedef enum {
	SFFR_SOCKETCLOSE, // socket被关闭
	SFFR_SOCKETSENDERR, // socket发送出错
	SFFR_SOCKETRECVERR, // socket接收出错
	SFFR_RECVERREFUSEFILE, // 接收端拒绝
	SFFR_RECVERCANCEL, // 接收端取消
	SFFR_RECVERFAIL, // 接收端失败
	SFFR_WAITTIMEOUT, // 等待超时
	SFFR_SENDERCANCEL, // 发送端取消
	SFFR_SENDERFAIL, // 发送端失败
}EMSendFileFailReason;

// 回调函数声明
typedef void (*SocketCloseFun)(void *pNotifyObj, SOCKET hSocket, EMSocketCloseReason scr);
typedef void (*OneNetMsgFun)(void *pNotifyObj, char *Msg, int nMsgLen); // 接收到一条完整的网络消息
typedef void (*SendFileFun)(void *pNotifyObj, char *szPathName);
typedef void (*SendFileFailFun)(void *pNotifyObj, char *szPathName, EMSendFileFailReason SendFileFailReason);
typedef void (*SendFileProgressFun)(void *pNotifyObj, int nSentBytes, int nTotalBytes);

// Tcp客户端
class CTcpClient
{
private:
	WSADATA									m_WSAData;
	void									*m_pNotifyObj; // 事件通知对象指针
	HWND									m_hHideWnd; // 隐藏窗体
	SOCKET									m_hSocket; // socket句柄
	char									m_szServerIpAddr[MAX_PATH]; // 服务器ip地址
	WORD									m_wPort; // 端口
	DWORD									m_dwFilePackageSize; // 文件包大小
	int										m_nWaitTimeOut; // 等待超时
	int										m_nSendTimeOut; // 发送数据超时
	DWORD									m_dwProgressTimeInterval; // 进度消息间隔时间
	int										m_nSocketRecvThreadPriority;
	int										m_nSendFileThreadPriority;

	CClientSocketRecvThread					*m_pClientSocketRecvThread; // socket接收线程
	CSendFileThread							*m_pSendFileThread; // 发送文件线程

	// 回调函数指针
	SocketCloseFun							m_OnSocketClose;
	OneNetMsgFun							m_OnOneNetMsg;
	SendFileFun								m_OnSendFileSucc;
	SendFileFailFun							m_OnSendFileFail;
	SendFileProgressFun						m_OnSendFileProgress;

	BOOL RegisterClientSocketHideWndClass(void);
	BOOL StartupSocket(void);
	void CleanupSocket(void);
	void InitSockAddr(SOCKADDR_IN  *pSockAddr, char *szBindIpAddr, WORD wPort);
	BOOL AllocateWindow(void);
	void DeallocateWindow(void);

	// 窗体消息处理函数
	void HandleSocketSendErrMsg(void);
	void HandleSocketRecvErrMsg(void);
	void HandleSocketCloseMsg(void);
	void HandleRecverAcceptFileMsg(DWORD dwRecvedBytes);
	void HandleRecverRefuseFileMsg(void);
	void HandleRecverSuccMsg(void);
	void HandleRecverFailMsg(void);
	void HandleRecverCancelMsg(void);
	void HandleSenderFailMsg(void);
	void HandleWaitTimeOutMsg(void);
	void HandleSendFileProgressMsg(void);
public:
	static LRESULT CALLBACK HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam); // 窗体消息函数
	CTcpClient(void *pNotifyObj);
	~CTcpClient(void);

	// get/set函数
	void SetServerIpAddr(char *szServerIpAddr);
	void SetPort(WORD wPort);
	void SetFilePackageSize(DWORD dwFilePackageSize);
	void SetWaitTimeOut(int nWaitTimeOut);
	void SetSendTimeOut(int nSendTimeOut);
	void SetProgressTimeInterval(DWORD dwProgressTimeInterval);
	void SetSocketRecvThreadPriority(int nPriority);
	void SetSendFileThreadPriority(int nPriority);
	void SetOnSocketClose(SocketCloseFun OnSocketClose);
	void SetOnOneNetMsg(OneNetMsgFun OnOneNetMsg);
	void SetOnSendFileSucc(SendFileFun OnSendSucc);
	void SetOnSendFileFail(SendFileFailFun OnSendFail);
	void SetOnSendFileProgress(SendFileProgressFun OnSendFileProgressFun);

	BOOL Connect(void);
	void Disconnect(void);
	BOOL IsConnect(void);
	BOOL IsSending(void);
	BOOL SendFile(char *szPathName);
	void CancelSendFile(void);
	BOOL SendNetMsg(char *Msg, int nMsgLen);
	
	// 网络消息处理函数
	void HandleOneNetMsg(char *Msg, int nMsgLen);
};

// 文件发送线程
class CSendFileThread: public CThread
{
private:
	CTcpClient								*m_pTcpClient;
	HWND									m_hHideWnd; // 隐藏窗体句柄
	SOCKET									m_hSocket;
	DWORD									m_dwFilePackageSize; // 包大小
	int										m_nWaitTimeOut; // 等待超时
	DWORD									m_dwProgressTimeInterval; // 进度消息间隔时间
	CFile									m_File; // 待发送文件对象

	// 状态变量
	char									m_szPathName[MAX_PATH]; // 文件路径名
	DWORD									m_dwSentBytes; // 已发送字节数
	DWORD									m_dwFileSize; // 文件大小
	DWORD									m_dwLastProgressTick;

	// 事件句柄
	HANDLE									m_hCloseThreadMute; // 关闭线程
	HANDLE									m_hCloseThreadCancel; // 由于发送端取消发送而关闭线程
	HANDLE									m_hAcceptFile; // 接收端接受文件
	HANDLE									m_hRecvFileSucc; // 接收端接收文件成功

	void SendFileData(void);
	BOOL SendRequestSendFileNetMsg(void);
	BOOL SendSenderCancelNetMsg(void);
	BOOL SendSenderFailNetMsg(void);
	
	void ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	void ResetSendFile(void);
	void Execute(void);
public:
	CSendFileThread(CTcpClient *pTcpClientSocket, int nPriority);
	~CSendFileThread(void);

	// get / set函数
	void SetHideWndHandle(HWND hHideWnd);
	void SetSocketHandle(SOCKET hSocket);
	void SetFilePackageSize(DWORD dwFilePackageSize);
	void SetWaitTimeOut(int nWaitTimeOut);
	void SetProgressTimeInterval(DWORD dwProgressTimeInterval);
	char *GetPathName(void);
	DWORD GetSentBytes(void);
	DWORD GetFileSize(void);

	void TrigAcceptFile(DWORD dwRecvedBytes);
	void TrigRecvFileSucc(void);

	BOOL PrepareSendFile(char *szPathName);
	BOOL IsFileAccepted(void); // 检查文件是否已经接收端接受
	void CloseSendFileThreadMute(void);
	void CloseSendFileThreadCancel(void);
};

// 客户端socket接收线程
class CClientSocketRecvThread: public CThread
{
private:
	CTcpClient								*m_pTcpClient;
	SOCKET									m_hSocket;
	HWND									m_hHideWnd;
	char									m_RecvBuf[NETMSG_RECVBUFSIZE];
	int										m_nRecvBufSize; // 接收缓冲区中字节数

	void ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	void Execute(void);
	void DoRecv(char *Buf, int nBytes);
	void HandleRecvedNetMsg(char *Msg, int nMsgLen);
public:
	CClientSocketRecvThread(CTcpClient *pTcpClient, int nPriority);
	~CClientSocketRecvThread(void);

	// get / set 函数
	void SetSocketHandle(SOCKET hSocket);
	void SetHideWndHandle(HWND hHideWnd);
};


#endif // #ifndef _TCPCLIENT_INCLUDE_
