#include "stdafx.h"
#include <direct.h>
#include  "StatusFile.h"

#include "Defs.h"
#include "Utils.h"
#include "Markup.h"
#include "FTPClient.h"
#include "Prefs.h"

extern CPrefs g_prefs;



BOOL	WriteXmlLogFile(CString sJobName, int nStatus, CString sSourceFolder, CString sMsg, CTime tEventTime, int nCRC, CString sSetupName)
{
	CUtils util;
	CString s;
	CString sCmdFileBuffer = "";
	
	CString sStatus = "Success";
	CString sTimeStamp;
	sTimeStamp.Format("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", tEventTime.GetYear(), tEventTime.GetMonth(), tEventTime.GetDay(), tEventTime.GetHour(), tEventTime.GetMinute(), tEventTime.GetSecond());


	sMsg.Replace("<", "&lt;");
	sMsg.Replace(">", "&gt;");
	sMsg.Replace("&", "&amp;");
	sMsg.Replace("'", "&apos;");
	sMsg.Replace("\"", "&quot;");
	sMsg.Trim();

	sTimeStamp.Replace("<", "&lt;");
	sTimeStamp.Replace(">", "&gt;");
	sTimeStamp.Replace("&", "&amp;");
	sTimeStamp.Replace("'", "&apos;");
	sTimeStamp.Replace("\"", "&quot;");
	sTimeStamp.Trim();

	sJobName.Trim();

	HANDLE hHndlWrite;
	DWORD nBytesWritten;

	CTime t = CTime::GetCurrentTime();
	tm t1, t2;
	t.GetLocalTm(&t1);
	t.GetGmtTm(&t2);
	int nTimeDiffGMT =  t1.tm_hour - t2.tm_hour;

	if (nStatus == JMFSTATUS_ERROR)
		sStatus = "Error";
	else if (nStatus == JMFSTATUS_WARNING)
		sStatus = "Warning";

	TCHAR szBuf[200];
	DWORD len = 200;
	::GetComputerName(szBuf,&len);


	// MNO Hack!
	int vv = sSetupName.Find("-Z");
	if (vv != -1) {
		sSetupName = sSetupName.Left(vv);
	}
		
	s.Format(_T("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\r\n<JMF TimeStamp=\"%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.0+%d:00\" SenderID=\"FileDistributor_%s\"\r\n  xmlns=\"http://www.CIP4.org/JDFSchema_1_1\" Version=\"1.2\">\r\n  <Signal ID=\"S1\" Type=\"Notification\" >\r\n"),
		t.GetYear(),t.GetMonth(),t.GetDay(),t.GetHour(),t.GetMinute(),t.GetSecond(),nTimeDiffGMT, szBuf);
			
	sCmdFileBuffer += s;
		
	s.Format(_T("    <Notification Class=\"Event\" Type=\"FileTransferJob\">\r\n"));
	sCmdFileBuffer += s;

	s.Format(_T("      <FileTransferJob Status=\"%s\" FileName=\"%s\" InputName=\"%s\" InputSource=\"%s\" JobTime=\"%s\" Message=\"%s\" CRC=\"%d\"/>\r\n"), 
				sStatus, sJobName, sSetupName, sSourceFolder, sTimeStamp, sMsg, nCRC);
	sCmdFileBuffer += s;
	
	s.Format(_T("    </Notification>\r\n  </Signal>\r\n</JMF>\r\n"));
	sCmdFileBuffer += s;

	CString sCmdFile;
	if (g_prefs.m_jmfnetworkmethod == JMFNETWORK_SHARE) {
		if (sSetupName != "")
			sCmdFile.Format(_T("%s\\%s_%s.xml"), g_prefs.m_jmffolder, sSetupName, sJobName);
		else
			sCmdFile.Format(_T("%s\\%s.xml"), g_prefs.m_jmffolder, sJobName);
	} else {
		if (sSetupName != "")
			sCmdFile.Format(_T("%s\\%s_%s.xml"), g_prefs.m_apppath, sSetupName, sJobName);
		else
			sCmdFile.Format(_T("%s\\%s.xml"), g_prefs.m_apppath, sJobName);
	}

	if ((hHndlWrite = ::CreateFile(sCmdFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		util.Logprintf("ERROR: Cannot create %s file\r\n", sCmdFile);
		return FALSE;
	}
	
	if (::WriteFile(hHndlWrite, sCmdFileBuffer, sCmdFileBuffer.GetLength(), &nBytesWritten, NULL) == FALSE) {
		util.Logprintf("ERROR: Cannot write to %s file\r\n",sCmdFile);
		CloseHandle(hHndlWrite);
		return FALSE;
	}	

	::CloseHandle(hHndlWrite);


	/*if (g_prefs.m_jmfnetworkmethod == JMFNETWORK_FTP) {
		BOOL ret = SendXMLfileFTP(sCmdFile);
		::DeleteFile(sCmdFile);
		return ret;
	}*/

	return TRUE;

}

BOOL	WriteXmlStatusFile(int nStatus)
{
	CUtils util;
	CString s;
	CString sCmdFileBuffer = "";
	
	HANDLE hHndlWrite;
	DWORD nBytesWritten;

	CTime t = CTime::GetCurrentTime();
	tm t1, t2;
	t.GetLocalTm(&t1);
	t.GetGmtTm(&t2);
	int nTimeDiffGMT =  t1.tm_hour - t2.tm_hour;

	CString sStatus = "Running";
	if (nStatus == JMFSTATUS_ERROR)
		sStatus = "Not running";
	else if (nStatus == JMFSTATUS_UNKNOWN)
		sStatus = "Unknown";

	TCHAR szBuf[200];
	DWORD len = 200;
	::GetComputerName(szBuf,&len);
		
	s.Format(_T("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\r\n<JMF TimeStamp=\"%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.0+%d:00\" SenderID=\"FileDistributor_%s\"\r\n  xmlns=\"http://www.CIP4.org/JDFSchema_1_1\" Version=\"1.2\">\r\n  <Signal ID=\"S1\" Type=\"Status\" >\r\n"),
			t.GetYear(),t.GetMonth(),t.GetDay(),t.GetHour(),t.GetMinute(),t.GetSecond(),nTimeDiffGMT, szBuf);

	sCmdFileBuffer += s;

	s.Format(_T("    <DeviceInfo DeviceStatus=\"%s\"/>\r\n"), sStatus);
	sCmdFileBuffer += s;

	s.Format(_T("  </Signal>\r\n</JMF>\r\n"));
	sCmdFileBuffer += s;

	CString sCmdFile;
	sCmdFile.Format(_T("%s\\FileDistributorStatus.xml"), g_prefs.m_jmffolder);

	if ((hHndlWrite = ::CreateFile(sCmdFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		util.Logprintf("ERROR: Cannot create %s file\r\n", sCmdFile);
		return FALSE;
	}
	
	if (::WriteFile(hHndlWrite, sCmdFileBuffer, sCmdFileBuffer.GetLength(), &nBytesWritten, NULL) == FALSE) {
		util.Logprintf("ERROR: Cannot write to %s file\r\n",sCmdFile);
		CloseHandle(hHndlWrite);
		return FALSE;
	}	

	::CloseHandle(hHndlWrite);

/*	if (g_prefs.m_jmfnetworkmethod == JMFNETWORK_FTP) {
		BOOL ret = SendXMLfileFTP(sCmdFile);
		::DeleteFile(sCmdFile);
		return ret;
	}
	*/
	return TRUE;

}


BOOL g_bStatusIsConnected = FALSE;
int  g_nStatusFTPHandle = 0;

/*
BOOL SendXMLfileFTP(CString sInputFile) 
{
	CUtils util;
	CFTPClient ftp;

	int nRetries = 3;

	BOOL bFileCopyOK = FALSE;


	ftp.InitSession();

	while (--nRetries >= 0 && bFileCopyOK == FALSE) {

		if (g_nStatusFTPHandle == 0)
			g_bStatusIsConnected = FALSE;

		if (g_nStatusFTPHandle <= 0)
			g_nStatusFTPHandle = FTPGetHandle();
	
		if (g_nStatusFTPHandle == 0) {
			g_bStatusIsConnected = FALSE;
			Sleep(1000);
			continue;
		}

		if (g_bStatusIsConnected) {
			if (FTPNoOp(g_nStatusFTPHandle) == FALSE) 
				g_bStatusIsConnected = FALSE;	
		}

		if (g_bStatusIsConnected == FALSE) {

			FTPSetFirewallMode(g_nStatusFTPHandle, g_prefs.m_jmfftppasv);

			if (g_prefs.m_jmfftpport != 21) 
				FTPSetControlPort(g_nStatusFTPHandle, g_prefs.m_jmfftpport);
		
			FTPSetConnectTimeout(g_nStatusFTPHandle, 10);

			g_bStatusIsConnected = FTPConnect(g_nStatusFTPHandle, g_prefs.m_jmfftpserver, g_prefs.m_jmfftpusername, g_prefs.m_jmfftppassword, g_prefs.m_jmfftpaccount);
							
			FTPSetTransferType(g_nStatusFTPHandle, 1);	// bin
		}

		if (g_bStatusIsConnected == FALSE) {
			FTPCloseHandle(g_nStatusFTPHandle);
			g_nStatusFTPHandle = 0;
			Sleep(1000);
			continue;
		}

		CString sFinalOutputFileTitle = util.GetFileName(sInputFile);
							
		if (g_prefs.m_jmfftpdir != "" && g_prefs.m_jmfftpdir != "." && g_prefs.m_jmfftpdir != "/" && g_prefs.m_jmfftpdir != "\\") 
			sFinalOutputFileTitle = g_prefs.m_jmfftpdir + "/" + util.GetFileName(sInputFile);

		CString sTempOutputFileTitle = sFinalOutputFileTitle + _T(".tmp");
												
		//FTPDeleteFile(nFTPHandle, sTempOutputFileTitle);

		bFileCopyOK = FTPSendFile(g_nStatusFTPHandle, sInputFile, sTempOutputFileTitle);
		if (bFileCopyOK == FALSE) {
			Sleep(1000);
			continue;
		}

		bFileCopyOK = FTPRenameFile(g_nStatusFTPHandle, sTempOutputFileTitle, sFinalOutputFileTitle);
		
		if (bFileCopyOK == FALSE) {
			FTPDisConnect(g_nStatusFTPHandle);		
			FTPCloseHandle(g_nStatusFTPHandle);
			g_bStatusIsConnected = FALSE;
			g_nStatusFTPHandle = 0;
			Sleep(1000);
			continue;
		}
	}
							
	return bFileCopyOK;

}
*/