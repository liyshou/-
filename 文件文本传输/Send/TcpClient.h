// TcpClient
// ���Է���������Ϣ�����Է����ļ���֧�ֶϵ�����
// XSoft
// Contact: hongxing777@msn.com or lovelypengpeng@eyou.com
//
#ifndef _TCPCLIENT_INCLUDE_
#define _TCPCLIENT_INCLUDE_

#include <winsock2.h>
#include "Thread.h"

// ȱʡֵ����
#define DFT_SOCKETPORT						(8000) // ȱʡ�˿�
#define DFT_FILEPACKAGESIZE					(1000) // ȱʡ�ļ�����С
#define DFT_WAITTIMEOUT						(60000) // �ȴ���ʱ
#define DFT_SENDTIMEOUT						(10000) // ���ͳ�ʱ
#define DFT_SOCKETRECVTHREADPRIORITY		(THREAD_PRIORITY_NORMAL)
#define DFT_SENDFILETHREADPRIORITY			(THREAD_PRIORITY_BELOW_NORMAL)
#define DFT_PROGRESSTIMEINTERVAL			(100) // ������Ϣ���ʱ��

// ��������
#define MSG_TAG								('@')
#define SOCKET_RECVBUFSIZE					(32768) // socket���ջ������Ĵ�С
#define MAX_NETMSGPACKAGESIZE				(99999) // ������Ϣ���Ĵ�С, ��ֵ������Э�������ڱ�ʾ��Ϣ���ȵ��ֽ�����
#define MAX_FILEPACKAESIZE					(MAX_NETMSGPACKAGESIZE - 3) // �ļ����Ĵ�С
#define NETMSG_RECVBUFSIZE					(2 * MAX_NETMSGPACKAGESIZE) // ������Ϣ��������С
#define STR_CLIENTSOCKETHIDEWNDCLASSNAME	("client-socket-hide-wndclass") // ���ش�������

#define WM_TCPCLIENTBASE					(WM_USER + 1000)
#define WM_SOCKETRECVERR					(WM_TCPCLIENTBASE + 0) // socket���ճ���
#define WM_SOCKETSENDERR					(WM_TCPCLIENTBASE + 1) // socket���ͳ���
#define	WM_SOCKETCLOSE						(WM_TCPCLIENTBASE + 2) // socket���ر�

#define WM_RECVERACCEPTFILE					(WM_TCPCLIENTBASE + 20) // ���ն˽����ļ�
#define WM_RECVERREFUSEFILE					(WM_TCPCLIENTBASE + 21) // ���ն˾ܾ��ļ�
#define WM_RECVERSUCC						(WM_TCPCLIENTBASE + 22) // ���ն˳ɹ�
#define WM_RECVERCANCEL						(WM_TCPCLIENTBASE + 23) // ���ն�ȡ��
#define WM_RECVERFAIL						(WM_TCPCLIENTBASE + 14) // ���ն˳���

#define WM_WAITTIMEOUT						(WM_TCPCLIENTBASE + 40) // �ȴ���ʱ
#define WM_SENDERFAIL						(WM_TCPCLIENTBASE + 41) // ���Ͷ˳���
#define WM_SENDFILEPROGRESS					(WM_TCPCLIENTBASE + 42) // �����ļ�����
// ǰ������
class CTcpClient; // Tcp�ͻ�����
class CClientSocketRecvThread; // socket���ݽ����߳�
class CSendFileThread; // �����ļ��߳�

// socket�رյ�ԭ��
typedef enum {
	SCR_PEERCLOSE, // socket���Եȶ˹ر�
	SCR_SOCKETSENDERR, // socket���ͳ���
	SCR_SOCKETRECVERR // socket���ճ���
}EMSocketCloseReason;

// �����ļ�ʧ��ԭ��
typedef enum {
	SFFR_SOCKETCLOSE, // socket���ر�
	SFFR_SOCKETSENDERR, // socket���ͳ���
	SFFR_SOCKETRECVERR, // socket���ճ���
	SFFR_RECVERREFUSEFILE, // ���ն˾ܾ�
	SFFR_RECVERCANCEL, // ���ն�ȡ��
	SFFR_RECVERFAIL, // ���ն�ʧ��
	SFFR_WAITTIMEOUT, // �ȴ���ʱ
	SFFR_SENDERCANCEL, // ���Ͷ�ȡ��
	SFFR_SENDERFAIL, // ���Ͷ�ʧ��
}EMSendFileFailReason;

// �ص���������
typedef void (*SocketCloseFun)(void *pNotifyObj, SOCKET hSocket, EMSocketCloseReason scr);
typedef void (*OneNetMsgFun)(void *pNotifyObj, char *Msg, int nMsgLen); // ���յ�һ��������������Ϣ
typedef void (*SendFileFun)(void *pNotifyObj, char *szPathName);
typedef void (*SendFileFailFun)(void *pNotifyObj, char *szPathName, EMSendFileFailReason SendFileFailReason);
typedef void (*SendFileProgressFun)(void *pNotifyObj, int nSentBytes, int nTotalBytes);

// Tcp�ͻ���
class CTcpClient
{
private:
	WSADATA									m_WSAData;
	void									*m_pNotifyObj; // �¼�֪ͨ����ָ��
	HWND									m_hHideWnd; // ���ش���
	SOCKET									m_hSocket; // socket���
	char									m_szServerIpAddr[MAX_PATH]; // ������ip��ַ
	WORD									m_wPort; // �˿�
	DWORD									m_dwFilePackageSize; // �ļ�����С
	int										m_nWaitTimeOut; // �ȴ���ʱ
	int										m_nSendTimeOut; // �������ݳ�ʱ
	DWORD									m_dwProgressTimeInterval; // ������Ϣ���ʱ��
	int										m_nSocketRecvThreadPriority;
	int										m_nSendFileThreadPriority;

	CClientSocketRecvThread					*m_pClientSocketRecvThread; // socket�����߳�
	CSendFileThread							*m_pSendFileThread; // �����ļ��߳�

	// �ص�����ָ��
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

	// ������Ϣ������
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
	static LRESULT CALLBACK HideWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam); // ������Ϣ����
	CTcpClient(void *pNotifyObj);
	~CTcpClient(void);

	// get/set����
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
	
	// ������Ϣ������
	void HandleOneNetMsg(char *Msg, int nMsgLen);
};

// �ļ������߳�
class CSendFileThread: public CThread
{
private:
	CTcpClient								*m_pTcpClient;
	HWND									m_hHideWnd; // ���ش�����
	SOCKET									m_hSocket;
	DWORD									m_dwFilePackageSize; // ����С
	int										m_nWaitTimeOut; // �ȴ���ʱ
	DWORD									m_dwProgressTimeInterval; // ������Ϣ���ʱ��
	CFile									m_File; // �������ļ�����

	// ״̬����
	char									m_szPathName[MAX_PATH]; // �ļ�·����
	DWORD									m_dwSentBytes; // �ѷ����ֽ���
	DWORD									m_dwFileSize; // �ļ���С
	DWORD									m_dwLastProgressTick;

	// �¼����
	HANDLE									m_hCloseThreadMute; // �ر��߳�
	HANDLE									m_hCloseThreadCancel; // ���ڷ��Ͷ�ȡ�����Ͷ��ر��߳�
	HANDLE									m_hAcceptFile; // ���ն˽����ļ�
	HANDLE									m_hRecvFileSucc; // ���ն˽����ļ��ɹ�

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

	// get / set����
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
	BOOL IsFileAccepted(void); // ����ļ��Ƿ��Ѿ����ն˽���
	void CloseSendFileThreadMute(void);
	void CloseSendFileThreadCancel(void);
};

// �ͻ���socket�����߳�
class CClientSocketRecvThread: public CThread
{
private:
	CTcpClient								*m_pTcpClient;
	SOCKET									m_hSocket;
	HWND									m_hHideWnd;
	char									m_RecvBuf[NETMSG_RECVBUFSIZE];
	int										m_nRecvBufSize; // ���ջ��������ֽ���

	void ForcePostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	void Execute(void);
	void DoRecv(char *Buf, int nBytes);
	void HandleRecvedNetMsg(char *Msg, int nMsgLen);
public:
	CClientSocketRecvThread(CTcpClient *pTcpClient, int nPriority);
	~CClientSocketRecvThread(void);

	// get / set ����
	void SetSocketHandle(SOCKET hSocket);
	void SetHideWndHandle(HWND hHideWnd);
};


#endif // #ifndef _TCPCLIENT_INCLUDE_
