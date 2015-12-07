// WrLog.h: interface for the CWrLog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WRLOG_H__EF6864A0_8E88_4116_A187_B59072B3390D__INCLUDED_)
#define AFX_WRLOG_H__EF6864A0_8E88_4116_A187_B59072B3390D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CGlobal;
class CPack;

void wrLog(const char *format,...);
void wrHex(char *name,BYTE *hex,int hex_len);

void get_date8 (char *cDate);	//取8位日期，e.g 20060809
void get_time8(char *cTime)	;	//取8位时间，e.g 15:56:24

bool DirExist(const char *pszDirName);	//判断目录是否存在
bool CreateDir(const char *pszDirName);	// 创建目录，包含子目录，可以创建多级子目录


class CWrLog  
{
public:
	CWrLog();
	virtual ~CWrLog();
	__declspec(thread) static char g_pos_no[8+1];
	__declspec(thread) static int  log_sd;

};

#endif // !defined(AFX_WRLOG_H__EF6864A0_8E88_4116_A187_B59072B3390D__INCLUDED_)
