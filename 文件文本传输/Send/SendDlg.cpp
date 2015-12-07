// SendDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Send.h"
#include "SendDlg.h"
#include "Wrlog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSendDlg dialog

CSendDlg::CSendDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSendDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSendDlg)
	m_dwFilePackageSize = 0;
	m_strServerIp = _T("");
	m_nPort = 0;
	m_strFileName = _T("");
	m_strMsg = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pTcpClient = new CTcpClient(this);//����һ��tcp�ͻ��˵�ָ��
}

CSendDlg::~CSendDlg(void)
{
	delete m_pTcpClient;
}

void CSendDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSendDlg)
	DDX_Control(pDX, IDC_CNN_STATUS, m_ctlCnnStatus);
	DDX_Control(pDX, IDC_INFO, m_ctlInfo);
	DDX_Text(pDX, IDC_FILE_PACKAGE_SIZE, m_dwFilePackageSize);
	DDV_MinMaxInt(pDX, m_dwFilePackageSize, 1, 99996);
	DDX_Text(pDX, IDC_SERVER_IP, m_strServerIp);
	DDX_Text(pDX, IDC_PORT, m_nPort);
	DDX_Text(pDX, IDC_FILE_NAME, m_strFileName);
	DDX_Text(pDX, IDC_MSG, m_strMsg);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_MSG, m_edit);
}

BEGIN_MESSAGE_MAP(CSendDlg, CDialog)
	//{{AFX_MSG_MAP(CSendDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CONNECT, OnConnect)
	ON_BN_CLICKED(IDC_DISCONNECT, OnDisconnect)
	ON_BN_CLICKED(IDC_SEND, OnSendFile)
	ON_BN_CLICKED(IDC_SEND_MSG, OnSendMsg)
	ON_BN_CLICKED(IDC_CANCEL_SEND, OnCancelSend)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSendDlg message handlers

BOOL CSendDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	m_strServerIp = "127.0.0.1";//���ӷ������ĵ�ַ���˿�
	m_nPort = 8000;
	m_dwFilePackageSize = 1024;
	m_strFileName = "d:\\cn_visio_professional_2013_x64_1138440.exe";//Ĭ�ϴ�����ļ�
	
	UpdateData(FALSE);

	m_pTcpClient->SetProgressTimeInterval(100);   //����ʱ����
	m_pTcpClient->SetOnSocketClose(OnSocketClose);
	m_pTcpClient->SetOnOneNetMsg(OnOneNetMsg);//��Ϣ����

	m_pTcpClient->SetOnSendFileSucc(OnSendFileSucc);//�ļ����ͳɹ�
	m_pTcpClient->SetOnSendFileFail(OnSendFileFail);//�ļ�����ʧ��
	m_pTcpClient->SetOnSendFileProgress(OnSendFileProgress);//�����ļ��Ľ���

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSendDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSendDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSendDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CSendDlg::OnConnect() 
{
	if(!UpdateData())
		return;
	

	m_pTcpClient->SetServerIpAddr((char *)(LPCTSTR)m_strServerIp);//�ӽ����ϻ�ȡIP
	m_pTcpClient->SetPort(m_nPort);
	m_pTcpClient->SetFilePackageSize(m_dwFilePackageSize);//�ļ�����С
	//////////////////////////////////////////////////////////////////////////��ȡ��ֵ��������m_ptcpclient ָ��
	m_ctlCnnStatus.SetWindowText("��ȴ�...");
	wrLog("���ӷ�������ȴ�������������");
	if(!m_pTcpClient->Connect())
	{
		m_ctlCnnStatus.SetWindowText("����ʧ��!");
		wrLog("����װ̬:%s",m_ctlCnnStatus);
	}
	else
		m_ctlCnnStatus.SetWindowText("������");
}

void CSendDlg::OnDisconnect() 
{
	m_pTcpClient->Disconnect();
	m_ctlCnnStatus.SetWindowText("�Ͽ�����");
}

void CSendDlg::OnSendFile() 
{
	DispInfo(""); // �����Ϣ

	if(!UpdateData())
		return;

	m_pTcpClient->SetFilePackageSize(m_dwFilePackageSize);
	if(!m_pTcpClient->SendFile((char *)(LPCTSTR)m_strFileName))
		AfxMessageBox("�����ļ�ʧ��");
}

void CSendDlg::OnSendMsg(void)
{
	char s[99999];

	if(!UpdateData())
		return;
	sprintf(s, "·�˼ף�%s", m_strMsg);
	m_pTcpClient->SendNetMsg(s, strlen(s) - 6);
	wrLog("SEND ���͵���Ϣ�ǣ�%s",s);
	m_edit.SetWindowText("");
}

void CSendDlg::OnSocketClose(void *pNotifyObj, SOCKET hSocket, EMSocketCloseReason scr)
{
	CSendDlg *pSendDlg = (CSendDlg *)pNotifyObj;
	CString strInfo;

	strInfo.Format("�Ͽ����ӣ�SocketCloseReason = %d", scr);
	pSendDlg->m_ctlCnnStatus.SetWindowText(strInfo);
}

void CSendDlg::OnOneNetMsg(void *pNotifyObj, char *Msg, int nMsgLen)
{
	CSendDlg *pSendDlg = (CSendDlg *)pNotifyObj;
	char s[9999];
	CString strInfo;

	strncpy(s, Msg, nMsgLen);
	s[nMsgLen] = 0;
	strInfo = s;

	pSendDlg->DispInfo(strInfo);
}

void CSendDlg::OnSendFileSucc(void *pNotifyObj, char *szPathName)
{
	CSendDlg *pSendDlg = (CSendDlg *)pNotifyObj;
	
	pSendDlg->DispInfo("OnSendFileSucc");
}

void CSendDlg::OnSendFileFail(void *pNotifyObj, char *szPathName, EMSendFileFailReason SendFileFailReadson)//�ļ�����ʧ��
{
	CSendDlg *pSendDlg = (CSendDlg *)pNotifyObj;
	CString strInfo;

	strInfo.Format("OnSendFileFail,SendFileFailReason = %d", SendFileFailReadson);
	pSendDlg->DispInfo(strInfo);
}

void CSendDlg::OnSendFileProgress(void *pNotifyObj, int nSentBytes, int nTotalBytes) //�ļ����ͽ���
{
	CSendDlg *pSendDlg = (CSendDlg *)pNotifyObj;
	CString strInfo;
	
	strInfo.Format("%d / %d", nSentBytes, nTotalBytes);
	pSendDlg->DispInfo(strInfo);
}

void CSendDlg::DispInfo(CString strInfo)//��ʾ�ı���Ϣ
{
	m_ctlInfo.SetWindowText(strInfo);
	
}

void CSendDlg::OnCancelSend() //ȡ������
{
	m_pTcpClient->CancelSendFile();	
}
