// SendDlg.h : header file
//

#if !defined(AFX_SENDDLG_H__B1FA7D63_8F13_43C1_896B_67F13D5733FC__INCLUDED_)
#define AFX_SENDDLG_H__B1FA7D63_8F13_43C1_896B_67F13D5733FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TcpClient.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CSendDlg dialog

class CSendDlg : public CDialog
{
// Construction
public:
	void DispInfo(CString strInfo);
	CSendDlg(CWnd* pParent = NULL);	// standard constructor
	~CSendDlg(void);

// Dialog Data
	//{{AFX_DATA(CSendDlg)
	enum { IDD = IDD_SEND_DIALOG };
	CStatic	m_ctlCnnStatus;
	CStatic	m_ctlInfo;
	DWORD	m_dwFilePackageSize;
	CString	m_strServerIp;
	int		m_nPort;
	CString	m_strFileName;
	CString	m_strMsg;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSendDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CSendDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnConnect();
	afx_msg void OnDisconnect();
	afx_msg void OnSendFile();
	afx_msg void OnSendMsg();
	afx_msg void OnCancelSend();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CTcpClient *m_pTcpClient;
	
	static void OnSocketClose(void *pNotifyObj, SOCKET hSocket, EMSocketCloseReason Socket);
	static void OnOneNetMsg(void *pNotifyObj, char *Msg, int nMsgLen);
	static void OnSendFileSucc(void *pNotifyObj, char *szPathName);
	static void OnSendFileFail(void *pNotifyObj, char *szPathName, EMSendFileFailReason SendFileFailReason);
	static void OnSendFileProgress(void *pNotifyObj, int nSentBytes, int nTotalBytes);
public:
	CEdit m_edit;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SENDDLG_H__B1FA7D63_8F13_43C1_896B_67F13D5733FC__INCLUDED_)
