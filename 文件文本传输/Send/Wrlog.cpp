// wrLog.cpp: implementation of the CwrLog class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WrLog.h"
#include "Tools.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//extern CGlobal gbl;

#define MAX 8192
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWrLog::CWrLog()
{

}

CWrLog::~CWrLog()
{

}
char CWrLog::g_pos_no[8+1] = "";
int  CWrLog::log_sd = 0;
CWrLog log;

//hex_len：hex的长度
void wrHex(char *name,BYTE *hex,int hex_len)		//bcd转成字符串后写入log文件
{
//	BYTE *str = new BYTE[8192];
	BYTE str[81920];
	memset(str,0,sizeof(str));
	CTools::byte_to_hex(hex, str, hex_len);

	wrLog("%s[%d]：[%s]",name,hex_len*2,str);
//	delete[] str;
}


//写log
void wrLog(const char *format,...)
{
	va_list ap;
	int n;
	char buff[MAX];
	char strTime[15+1];
	char strDay[8+1];
	char temp[MAX+30];
	
	memset(buff,0,sizeof(buff));
	va_start (ap,format);
	vsprintf(buff,format,ap);
	va_end(ap);
	
	n = strlen(buff);
	memset( strDay, 0, sizeof(strDay) );
	get_date8(strDay);
	memset( strTime, 0, sizeof(strTime) );
	memset(temp,0,sizeof(temp));
	CTools::getTime(temp, 1);
	memset( strTime, 0, sizeof(strTime) );
	memcpy(strTime, temp, 2);
	memcpy(strTime+2, ":", 1);
	memcpy(strTime+3, temp+2, 2);
	memcpy(strTime+5, ":", 1);
	memcpy(strTime+6, temp+4, 2);
	memcpy(strTime+8, ".", 1);
	memcpy(strTime+9, temp+6, 3);
	
	memset( temp, 0, sizeof(temp) );
	sprintf( temp, "log\\%s",strDay);
	if( CreateDir(temp) )
	{
		FILE *fp;
		strcat(temp, "\\");
		if(strcmp("LOG", "")) 
			strcat(temp, "LOG");	//取ePOS终端号作为文件名，或其他亦可
		else
			strcat(temp, "system");
		strcat(temp, ".log");
		fp = fopen( temp, "a");
		
		if( fp == NULL)
		{
			fp = fopen( temp, "w+");
			if(fp == NULL)
				return;
		}
		memset( temp, 0, sizeof(temp) );
		sprintf(temp, "[%s %s %d] %s\n", strDay, strTime, log.log_sd, buff);
		fputs(temp, fp);
		fclose(fp);
	}
	else
		return;
}

void get_date8 (char *cDate)	//取8位日期，e.g 20060809
{
	static char buff[8+1];
	struct tm     *tm_now;
	time_t        time_now;
	
	time_now = time (NULL);
	tm_now = localtime (&time_now);
	sprintf (buff, "%04d%02d%02d", tm_now->tm_year + 1900,
		tm_now->tm_mon + 1, tm_now->tm_mday);
	strcpy(cDate,buff);
}

void get_time8(char *cTime)		//取8位时间，e.g 15:56:24
{
	static char buff[8+1];
	time_t t;
	struct tm tm;
	time(&t);
	memcpy(&tm, localtime(&t), sizeof(tm));
	sprintf(buff, "%.2d:%.2d:%.2d",tm.tm_hour,tm.tm_min,tm.tm_sec);
	strcpy(cTime, buff);
}


//判断目录是否存在
bool DirExist(const char *pszDirName)
{
	WIN32_FIND_DATA fileinfo;
    char	_szDir[_MAX_PATH];
    strcpy(_szDir, pszDirName);
    int nLen = strlen(_szDir);
    if( (_szDir[nLen-1] == '\\') || (_szDir[nLen-1] == '/') )
    {
    	_szDir[nLen-1] = '\0';
    }
	HANDLE	hFind = ::FindFirstFile(_szDir, &fileinfo);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}
    if( fileinfo.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY )
    {
    	::FindClose(hFind);
        return true;
    }
    ::FindClose(hFind);
	return false;
}



// 创建目录，包含子目录，可以创建多级子目录
bool CreateDir(const char *pszDirName)
{
	BOOL bRet = FALSE;
    char  _szDir[_MAX_PATH];
    char  _szTmp[_MAX_DIR];
    int  nLen = 0;
    int  idx ;
	if( (DirExist(pszDirName)) == true )
    	return true;
    strcpy(_szDir, pszDirName);
    nLen = strlen(_szDir);
    if( _szDir[nLen-1] == '\\' || _szDir[nLen-1] == '/' )
    {
    	_szDir[nLen-1] = '\0';
    }
    nLen = strlen(_szDir);
    memset(_szTmp, 0, _MAX_DIR);
    char  _str[2];
    for(idx = 0; idx < nLen; idx++)
    {
    	if(_szDir[idx] != '\\' )
        {
        	_str[0] = _szDir[idx];
            _str[1] = 0;
			strcat(_szTmp, _str);
        }
        else
        {
            bRet = ::CreateDirectory(_szTmp, NULL);
            if( bRet )
            {
            	::SetFileAttributes(_szTmp, FILE_ATTRIBUTE_NORMAL);
            }
        	_str[0] = _szDir[idx];
            _str[1] = 0;
			strcat(_szTmp, _str);
        }
        if( idx == nLen-1 )
        {
            bRet = ::CreateDirectory(_szTmp, NULL);
            if( bRet )
            {
            	::SetFileAttributes(_szTmp, FILE_ATTRIBUTE_NORMAL);
            }
        }
    }
	if( DirExist(_szTmp) )
    	return true;
	return false;
}


//写交易流水日志
/*void wrPosLog(int flag, int trFlag, CGlobal gbl, CPack pack)
{
	char strTime[8+1];
	char strDay[8+1];
	char temp[100];
	char trType[3];
	char correctCode[10];

	if( trFlag == 1 )
		strcpy(trType, "01");
	if( trFlag == 2 )
		strcpy(trType, "02");
	if( trFlag == 3 )
		strcpy(trType, "05");

	memset( strDay, 0, sizeof(strDay) );
	get_date8(strDay);
	memset( strTime, 0, sizeof(strTime) );
	get_time8(strTime);
	
	memset( temp, 0, sizeof(temp) );
	sprintf( temp, "log\\%s",strDay);
	if( CreateDir(temp) )
	{
		FILE *fp;
		strcat(temp, "\\");
		if( !flag )  //终端上送
			strcat(temp, "pos_up");
		else  //中心下发
			strcat(temp, "pe_down");
		strcat(temp, ".log");

		fp = fopen( temp, "a");
		if( fp == NULL)
		{
			fp = fopen( temp, "w+");
			if(fp == NULL)
				return;
		}
		memset( temp, 0, sizeof(temp) );
		if( !flag )
		{
			sprintf(temp, "%s; %s; %s; %s; %s; %s\n", strDay, strTime, gbl.pos_serial, trType, gbl.pos_no, gbl.fee_type);
		}
		else
		{
			bbzero(correctCode);
			if( atoi((char *)pack.FIELD39) == 80)
			{
				memcpy(correctCode, pack.FIELD62, 9);
			}
			sprintf(temp, "%s; %s; %s; %s; %s; %s; %s; %s\n", strDay, strTime, pack.FIELD37, trType, pack.FIELD39, gbl.pos_no, gbl.fee_type, correctCode);
		}
		fputs(temp, fp);
		fclose(fp);
	}
}
*/

//写在线统计日志
//tim: 统计时间
//num: 当前连接数
/*void wrOnlineLog(int num)
{
	char strTime[8+1];
	char strDay[8+1];
	char temp[100];

	memset( strDay, 0, sizeof(strDay) );
	get_date8(strDay);
	memset( strTime, 0, sizeof(strTime) );
	get_time8(strTime);
	
	memset( temp, 0, sizeof(temp) );
	sprintf( temp, "log\\%s",strDay);
	if( CreateDir(temp) )
	{
		strcat(temp, "\\在线统计");
		strcat(temp, strDay);
		strcat(temp, ".log");

		FILE *fp;
		fp = fopen( temp, "a");
		if( fp == NULL)
		{
			fp = fopen( temp, "w+");
			if(fp == NULL)
				return;
		}
		memset( temp, 0, sizeof(temp) );
		sprintf(temp, "[%s %s]当前在线数: %d\n", strDay, strTime, num);
		fputs(temp, fp);
		fclose(fp);
	}
}
*/


//写监控日志
//type: 交易类型
//stat: 交易状态
//rspCode: 返回码
/*void wrMonitorLog(int type, int stat, int rspCode, CGlobal gbl, CPack pack)
{
	char strTime[8+1];
	char strDay[8+1];
	char temp[300], path[100];
	MonitorDetail detail;

	wrLog("【wrMonitorLog】type[%d] stat[%d] rspCode[%d]", type, stat, rspCode);

	/*----------------填写日志内容------------------*/
/*	memset(&detail, 0, sizeof(detail));
	strncpy(detail.posNum, gbl.pos_no, sizeof(detail.posNum)-1);		//终端号
	strncpy(detail.callTel, gbl.call_tel, sizeof(detail.callTel)-1);	//来电号码
	sprintf(detail.traType, "%02d", (unsigned int)type);	//交易类型
	sprintf(detail.stat, "%02d", (unsigned int)stat);		//状态码
	sprintf(detail.rspCode, "%02d", (unsigned int)rspCode);//返回码
	if(atoi(detail.rspCode) == 80)			//bit62错误码, 20080127
		memcpy(detail.corrCode, pack.FIELD62, 9);
	strncpy(detail.feeType, gbl.fee_type, sizeof(detail.feeType)-1);	//业务代码
	strncpy(detail.serial, gbl.pos_serial, sizeof(detail.serial)-1);	//终端流水
	strncpy(detail.traNum, gbl.phone_no, sizeof(detail.traNum)-1);	//交易号码
	if(type == 2 || type == 13 || type==15)	//交费或羊城通交费时
	{
		strncpy(detail.amount, gbl.amount, sizeof(detail.amount)-1);	//金额
		strncpy(detail.card, gbl.card_no, sizeof(detail.card)-1);		//卡号
		strncpy(detail.bank, gbl.bank, sizeof(detail.bank)-1);			//银行代码
	}
	if(stat == 0 || stat == 3)	//pe返回成功或失败时
		if(type == 1 || type == 2 || type == 3 || type == 13 || type==15) 
			strncpy(detail.stan, (char *)pack.FIELD37, sizeof(detail.stan)-1);	//系统参考号

	/*-----------------写日志-----------------*/
	/*bbzero(strDay);
	get_date8(strDay);
	bbzero(strTime);
	get_time8(strTime);
	
	bbzero(path);
	strcpy(path, "MonitorLog");

	if( CreateDir(path) )
	{
		strcat(path, "\\monitor");
		strcat(path, strDay);
		strcat(path, ".log");

		bbzero(temp);
		sprintf(temp, "%s %s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;\n", 
		strDay, strTime, detail.posNum, detail.serial, detail.callTel, detail.traType, detail.stat, detail.rspCode, detail.feeType, detail.amount, detail.bank, detail.card, detail.traNum, detail.stan, detail.corrCode);

		cs_FILE.Lock();	//同步控制
		FILE *fp;
		fp = fopen( path, "a");
		if( fp == NULL)
		{
			fp = fopen( path, "w+");
			if(fp == NULL)
			{
				wrLog("【wrMonitorLog】end: file open err");
				return;
			}
		}
		fputs(temp, fp);
		fclose(fp);
		cs_FILE.Unlock();
	}
	wrLog("【wrMonitorLog】end");
}

//写log
void wrDGBLog(const char *format,...)
{
	va_list ap;
	int n;
	char buff[MAX];
	char strTime[12+1];
	char strDay[8+1];
	char temp[64];
	CTools tool;
	memset(buff,0,sizeof(buff));
	va_start (ap,format);
	vsprintf(buff,format,ap);
	va_end(ap);
	
	n = strlen(buff);
	memset( strDay, 0, sizeof(strDay) );
	tool.get_date8(strDay);
	memset(temp, 0, sizeof(temp));
	tool.get_time(temp, 1);
	memset( strTime, 0, sizeof(strTime) );
	memcpy(strTime, temp, 2);
	memcpy(strTime+2, ":", 1);
	memcpy(strTime+3, temp+2, 2);
	memcpy(strTime+5, ":", 1);
	memcpy(strTime+6, temp+4, 2);
	memcpy(strTime+8, ".", 1);
	memcpy(strTime+9, temp+6, 3);
	
	memset( temp, 0, sizeof(temp) );
	sprintf( temp, "log\\%s",strDay);
	if( CreateDir(temp) )
	{
		FILE *fp;
		strcat(temp, "\\");
		strcat(temp, "DGBusTrade");
		strcat(temp, ".log");
		fp = fopen( temp, "a");
		if( fp == NULL)
		{
			fp = fopen( temp, "w+");
			if(fp == NULL)
				return;
		}
		
		memset( temp, 0, sizeof(temp) );
		sprintf(temp, "[%s %s] ", strDay, strTime);
		if(temp == NULL)
		{
			fclose(fp);
			return;
		}
		fputs(temp, fp);
		
		if(buff == NULL)
		{
			fclose(fp);
			return;
		}
		fputs(buff, fp);
		fputs("\n", fp);
		fclose(fp);
	}
}

void wrDGBusHex(char *name,BYTE *hex,int len)		//bcd转成字符串后写入log文件
{
	BYTE str[4096];
	memset(str,0,sizeof(str));
	bcd_to_asc(str, hex, len*2, 1);
	
	wrDGBLog("%s[%d]：[%s]",name,len*2,str);
}

void wrYct3MonitorLog(char *pos_no,const char *format,...) //写羊城通充值第三代终端监控日志
{
	va_list ap;
	int n;
	char buff[MAX];
	char strTime[12+1];
	char strDay[8+1];
	char temp[64];
	CTools tool;
	memset(buff,0,sizeof(buff));
	va_start (ap,format);
	vsprintf(buff,format,ap);
	va_end(ap);
	
	n = strlen(buff);
	memset( strDay, 0, sizeof(strDay) );
	tool.get_date8(strDay);
	memset(temp, 0, sizeof(temp));
	tool.get_time(temp, 1);
	memset( strTime, 0, sizeof(strTime) );
	memcpy(strTime, temp, 2);
	memcpy(strTime+2, ":", 1);
	memcpy(strTime+3, temp+2, 2);
	memcpy(strTime+5, ":", 1);
	memcpy(strTime+6, temp+4, 2);
	memcpy(strTime+8, ".", 1);
	memcpy(strTime+9, temp+6, 3);
	
	memset( temp, 0, sizeof(temp) );
	sprintf( temp, "YCT3MonitorLog\\%s",strDay);
	if( CreateDir(temp) )
	{
		FILE *fp;
		strcat(temp, "\\TermMonitor_");
		strcat(temp, pos_no);
		strcat(temp, ".log");
		fp = fopen( temp, "a");
		if( fp == NULL)
		{
			fp = fopen( temp, "w+");
			if(fp == NULL)
				return;
		}
		
		memset( temp, 0, sizeof(temp) );
		sprintf(temp, "[%s %s] ", strDay, strTime);
		if(temp == NULL)
		{
			fclose(fp);
			return;
		}
		fputs(temp, fp);
		
		if(buff == NULL)
		{
			fclose(fp);
			return;
		}
		fputs(buff, fp);
		fputs("\n", fp);
		fclose(fp);
	}
}*/