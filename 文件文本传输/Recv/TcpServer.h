// CTcpServer
// XSoft
// Author:  alone
// Contact: lovelypengpeng@eyou.com or hongxing777@msn.com
// Update Date: 2005.1.17
// Comment: if you find any bugs or have any suggestions please contact me.
// �����շ�������Ϣ, ͬʱ�ɽ����ļ���֧�ֶϵ�������֧��ͬʱ���ն���ļ�
//
#ifndef _TCPSERVER_INCLUDE_
#define _TCPSERVER_INCLUDE_

#include <afxtempl.h>
#include <winsock2.h>
#include "Thread.h"
#include "afxmt.h"

// ȱʡֵ����
#define DFT_PORT						(8000) // socket�˿�
#define DFT_SENDTIMEOUT					(15000) // socket���ͳ�ʱ
#define DFT_PROGRESSTIMEINTERVAL		(100) // 100ms
#define DFT_ACCEPTTHREADPRIORITY		(THREAD_PRIORITY_ABOVE_NORMAL)
#define DFT_SOCKETRECVTHREADPRIORITY	(THREAD_PRIORITY_BELOW_NORMAL)

// ��������
#define MSG_TAG							('@')
#define SOCKET_RECVBUFSIZE				(32768)
#define MAX_PACKAGESIZE					(99999)
#define MAX_FILEPACKAESIZE				(MAX_PACKAGESIZE - 3)
#define NETMSG_RECVBUFSIZE				(2 * MAX_PACKAGESIZE) // ������Ϣ��������С
#define STR_ACCEPTHIDEWNDCLASSNAME		_T("accept-socket-hide-wndclass")
#define STR_RECVHIDEWNDCLASSNAME		_T("server-client-socket-hide-wndclass")
#define STR_FILERECVERHIDEWNDCLASSNAME	_T("filerecver-hide-wndclass")

#define WM_TCPSERVERBASE				(WM_USER + 1000)
#define WM_ACCEPT						(WM_TCPSERVERBASE + 0) // ���յ�socket����
#define WM_ACCEPTERR					(WM_TCPSERVERBASE + 1) // ���ճ���

#define WM_SOCKETCLOSE					(WM_TCPSERVERBASE + 2) // socket���ر�
#define WM_SOCKETSENDERR				(WM_TCPSERVERBASE + 3) // socket���ͳ���
#define WM_SOCKETRECVERR				(WM_TCPSERVERBASE + 4) // socket���ճ���

#define WM_REQUESTSENDFILE				(WM_TCPSERVERBASE + 5) // �������ļ�
#define WM_RECVFILEPROGRESS				(WM_TCPSERVERBASE + 6) // �ļ����ս���
#define WM_SENDERFAIL					(WM_TCPSERVERBASE + 7) // ���Ͷ�ʧ��
#define WM_SENDERCANCEL					(WM_TCPSERVERBASE + 8) // ���Ͷ�ȡ��
#define WM_RECVFILEFAIL					(WM_TCPSERVERBASE + 9) // �ļ�����ʧ��
#define WM_RECVFILESUCC					(WM_TCPSERVERBASE + 10) // �ļ����ճɹ�

// ǰ������
class CTcpServer;
class CAcceptSocket; // ���ڽ��յ�socket
class CAcceptThread; // socket�����߳�
class CServerClientSocket; // �����ļ�socket
class CServerClientSocketRecvThread; // �����߳�
class CFileRecver; // �ļ�������

// socket�رյ�ԭ��
typedef enum {
	SCR_PEERCLOSE, // socket���Եȶ˹ر�
	SCR_SOCKETSENDERR, // socket���ͳ���
	SCR_SOCKETRECVERR // socket���ճ���
}EMSocketCloseReason;

// �����ļ������ԭ��
typedef enum {
	RFFR_SOCKETCLOSE, // socket���ر�
	RFFR_SOCKETSENDERR, // socket���ͳ���
	RFFR_SOCKETRECVERR, // socket���ճ���
	RFFR_SENDERCANCEL, // ���Ͷ�ȡ��
	RFFR_SENDERFAIL, // ���Ͷ˾ܾ�
	RFFR_RECVERCANCEL, // ���ն�ȡ��
	RFFR_RECVERFAIL, // ���ն�ʧ��
}EMRecvFileFailReason;

// �¼�����
typedef void (*AcceptFun)(void *pNotifyObj, SOCKET hSocket, BOOL &bAccept);
typedef void (*AcceptErrFun)(void *pNotifyObj, SOCKET hAccept);
typedef void (*SocketConnectFun)(void *pNoitfyObj, SOCKET hSocket);
typedef void (*SocketCloseFun)(void *pNotifyObj, SOCKET hSocket, EMSocketCloseReason SocketCloseReason);
typedef void (*OneNetMsgFun)(void *pNotify, char *Msg, int nMsgLen);
typedef void (*RecvFileRequestFun)(void *pNotifyObj, char *szPathName, BOOL &bRecv); // ��������ļ�
typedef void (*RecvFileProgressFun)(void *pNotifyObj, DWORD dwRecvedBytes, DWORD dwFileSize);
typedef void (*RecvFileSuccFun)(void *pNotifyObj, char *szPathName);
typedef void (*RecvFileFailFun)(void *pNotifyObj, char *szPathName, EMRecvFileFailReason RecvFileFailReason);

class CTcpServer
{
private:
	WSADATA								m_WSAData;
	void								*m_pNotifyObj; // �¼�֪ͨ����ָ��
	CAcceptSocket						*m_pAcceptSocket; // ���ڽ���socket���ӵ�socket
	CList <CServerClientSocket *, CServerClientSocket *> m_ServerClientSockets; // �ͻ���socket�б�
	char								m_szBindIpAddr[MAX_PATH]; // ��ip��ַ
	WORD								m_wPort; // �˿�
	int									m_nSendTimeOut; // ���ͳ�ʱ
	DWORD								m_dwProgressTimeInterval; // ����ʱ����
	int									m_nAcceptThreadPriority; // �����߳����ȼ�
	int									m_nSocketRecvThreadPriority; // socket���ݽ����߳����ȼ�

	// �¼�ָ��
	AcceptFun							m_OnAccept;
	AcceptErrFun						m_OnAcceptErr;
	SocketConnectFun					m_OnSocketConnect;
	SocketCloseFun						m_OnSocketClose;
	OneNetMsgFun						m_OnOneNetMsg;
	RecvFileRequestFun					m_OnRecvFileRequest;
	RecvFileProgressFun					m_OnRecvFileProgress;
	RecvFileSuccFun						m_OnRecvFileSucc;
	RecvFileFailFun						m_OnRecvFileFail;

	BOOL StartupSocket(void); // ��ʼ��windows socket
	void CleanupSocket(void); // ���windows socket
	BOOL RegisterAcceptWndClass(void);
	BOOL RegisterRecvWndClass(void);
	BOOL RegisterFileRecverWndClass(void);
public:
	CTcpServer(void *pNotifyObj);
	~CTcpServer(void);

	// get / set ����
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

// ���ڽ���socket���ӵ�socket
class CAcceptSocket
{
private:
	CTcpServer							*m_pTcpServer;
	void								*m_pNotifyObj;
	HWND								m_hHideWnd; // ���ش�����
	SOCKET								m_hSocket; // socket���
	char								m_szBindIpAddr[MAX_PATH]; // ��ip��ַ
	WORD								m_wPort; // �˿�
	int									m_nAcceptThreadPriority;
	CAcceptThread						*m_pAcceptThread; // �����߳�

	// �¼�ָ��
	AcceptFun							m_OnAccept; // socket�����¼�
	AcceptErrFun						m_OnAcceptErr; // ����socket���ӳ����¼�
	SocketConnectFun					m_OnSocketConnect; // socket�����¼�

	void InitSockAddr(SOCKADDR_IN *pSockAddr, char *szBindIpAddr, WORD wPort);
	BOOL AllocateWindow(void);
	void DeallocateWindow(void);

	// windows��Ϣ������
	void HandleAcceptMsg(SOCKET hSocket);
	void HandleAcceptErrMsg(void);
public:
	static LRESULT CALLBACK HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CAcceptSocket(CTcpServer *pTcpServer, void *pNotifyObj);
	~CAcceptSocket(void);

	BOOL StartAccept(void);
	void StopAccept(void);

	// get / set ����
	void SetHideWndHandle(HWND hHideWnd);
	void SetBindIpAddr(char *szBindAddr);
	void SetPort(WORD wPort);
	void SetAcceptThreadPriority(int nPriority);
	void SetOnAccept(AcceptFun OnAccept);
	void SetOnAcceptErr(AcceptErrFun OnAcceptErr);
	void SetOnSocketConnect(SocketConnectFun OnSocketConnect);
};

// ���ڽ���socket���ӵ��̶߳���
class CAcceptThread: public CThread
{
private:
	CAcceptSocket						*m_pAcceptSocket;
	HWND								m_hHideWnd; // ���ش�����
	SOCKET								m_hSocket; // socket���

	void ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	void Execute(void);
public:
	CAcceptThread(CAcceptSocket *pAcceptSocket, int nPriority);

	// get / set ����
	void SetHideWndHandle(HWND hHideWnd);
	void SetSocketHandle(SOCKET hSocket);
};

// �����ļ�socket
class CServerClientSocket
{
private:
	CTcpServer							*m_pTcpServer;
	void								*m_pNotifyObj;
	HWND								m_hHideWnd; // ���ش�����
	SOCKET								m_hSocket; // socket���
	int									m_nSendTimeOut; // socket���ͳ�ʱ
	DWORD								m_dwProgressTimeInterval; // ����ʱ����
	int									m_nSocketRecvThreadPriority;
	CServerClientSocketRecvThread		*m_pServerClientSocketRecvThread; // socket�����߳�
	CFileRecver							*m_pFileRecver; // �ļ�������

	// �¼�ָ��
	OneNetMsgFun						m_OnOneNetMsg;
	SocketCloseFun						m_OnSocketClose;
	RecvFileRequestFun					m_OnRecvFileRequest;
	RecvFileProgressFun					m_OnRecvFileProgress;
	RecvFileSuccFun						m_OnRecvFileSucc;
	RecvFileFailFun						m_OnRecvFileFail;

	void FiniRecvFileSocket(void);
	BOOL AllocateWindow(void);
	void DeallocateWindow(void);

	// ������Ϣ������
	void HandleSocketCloseMsg(void);
	void HandleSocketSendErrMsg(void);
	void HandleSocketRecvErrMsg(void);

	// ������Ϣ������
	void HandleOneNetMsg(char *Msg, int nMsgLen); // ���յ�һ��������������Ϣ
public:
	static LRESULT CALLBACK HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CServerClientSocket(CTcpServer *pTcpServer, void *pNotifyObj);
	~CServerClientSocket(void);

	// get / set ����
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
	void HandleRecvedNetMsg(char *Msg, int nMsgLen); // ������յ���������Ϣ
	BOOL SendNetMsg(char *Msg, int nMsgLen);
};

// �ͻ��˽����߳�
class CServerClientSocketRecvThread: public CThread
{
private:
	CServerClientSocket					*m_pServerClientSocket;
	HWND								m_hHideWnd; // ���ش�����
	SOCKET								m_hSocket; // socket���
	char								m_RecvBuf[NETMSG_RECVBUFSIZE]; // ������Ϣ���ջ�����
	int									m_nRecvBufSize; // ���ջ��������ֽ���

	void ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	void Execute(void);
	void DoRecv(char *Buf, int nBytes);
public:
	CServerClientSocketRecvThread(CServerClientSocket *pServerClientSocket, int nPriority);

	// get / set ����
	void SetHideWndHandle(HWND hHideWnd);
	void SetSocketHandle(SOCKET hSocket);
};

// �ļ�������
class CFileRecver
{
private:
	CServerClientSocket					*m_pServerClientSocket;
	void								*m_pNotifyObj;
	CThread								*m_pSocketRecvThread;
	HWND								m_hSocketHideWnd; // socket���ش�����
	HWND								m_hFileRecverHideWnd; // �ļ����������ش�����
	DWORD								m_dwProgressTimeInterval;
	CFile								m_File;
	CMutex								m_Mutex;

	// ״̬����
	char								m_szPathName[MAX_PATH];
	DWORD								m_dwFileSize; // �ļ���С
	DWORD								m_dwRecvedBytes; // �ѽ��մ�С
	DWORD								m_dwLastProgressTick; // �ϴη��ͽ�����Ϣ��ʱ��

	// �¼�ָ��
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

	// ����������Ϣ
	BOOL SendAcceptSendFileRequestNetMsg(void); // ���ͽ��ܷ����ļ�����������Ϣ
	BOOL SendRefuseSendFileRequestNetMsg(void); // �ܾ������ļ�����������Ϣ
	BOOL SendRecverSuccNetMsg(void); // ���ͽ��ն˳ɹ�������Ϣ
	BOOL SendRecverFailNetMsg(void); // ���ͽ��ն�ʧ��������Ϣ
	BOOL SendRecverCancelNetMsg(void); // ���ͽ��ն�ȡ��������Ϣ

	// ������Ϣ������
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

	// get / set ����
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
	BOOL IsRecving(void); // �Ƿ����ڽ����ļ�
	void CancelRecvFile(void); // ȡ�������ļ�
};

#endif // #ifndef _TCPSERVER_INCLUDE_
