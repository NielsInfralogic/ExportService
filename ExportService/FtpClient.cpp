#include "stdafx.h"
#include "FtpClient.h"
#include "Defs.h"
#include "Utils.h"
#include <CkFtp2.h>

extern CUtils g_util;


CFTPClient::CFTPClient()
{
	m_heartbeatMS = 0;
	m_account = _T("");

	m_proxyhostname = _T("");
	m_proxyport = 0;
	m_proxymethod = 0;
	m_proxyusername = _T("");
	m_proxypassword = _T("");
}

CFTPClient::~CFTPClient()
{
}

// CFTPClient member functions

void CFTPClient::SetProxyMethod(int nProxyMethod, CString sProxyHostname, CString sProxyUsername, CString sProxyPassword, int nProxyPort)
{
	m_proxymethod = nProxyMethod;
	m_proxyhostname = sProxyHostname;
	m_proxyusername = sProxyUsername;
	m_proxypassword = sProxyPassword;
	m_proxyport = nProxyPort;

}

BOOL CFTPClient::InitSession()
{
//	return m_ftp.UnlockComponent("INFRAL.CB1092019_4czEsHZS615x");;
	return m_ftp.UnlockComponent("NFRLGC.CB1112020_vZdFLucd1RAZ");

}

CString CFTPClient::LimitErrorMessage(CString s)
{
	if (s.GetLength() >= MAX_ERRMSG)
		return s.Left(MAX_ERRMSG - 1);

	return s;
}


CString CFTPClient::GetConnectFailReason()
{
	int failReason = m_ftp.get_ConnectFailReason();

	switch (failReason) {
	case 0: 
		return _T("Success");
	case 1:
		return _T("Empty hostname");
	case 2:
		return _T("DNS lookup failed");
	case 3:
		return _T("DNS timeout");
	case 4:
		return _T("Aborted by application");
	case 5:
		return _T("Internal failure");
	case 6:
		return _T("Connect Timed Out");
	case 7:
		return _T("Connect Rejected (or failed for some other reason)");
	case 100:
		return _T("Internal schannel error");
	case 101:
		return _T("Failed to create credentials");
	case 102:
		return _T("Failed to send initial message to proxy");
	case 103:
		return _T("Handshake failed");
	case 104:
		return _T("Failed to obtain remote certificate");
	case 105:
		return _T("Failed to verify server certificate");
	case 106:
		return _T("Server certificate requirement failed");
	case 300:
		return _T("Asynch operation in progress");
	case 301:
		return _T("Login failure");
	case 200:
		return _T("Connected, but failed to receive greeting from FTP server");
	case 201:
		return _T("Failed to do AUTH TLS or AUTH SSL");
	}

	return _T("");
}

BOOL CFTPClient::OpenConnection(CString &sErrorMessage, CString &sConnectFailReason)
{

	sErrorMessage = _T("");
	sConnectFailReason = _T("");

	if (m_heartbeatMS > 0)
		m_ftp.put_HeartbeatMs(m_heartbeatMS);

	m_ftp.put_Hostname(m_host);
	m_ftp.put_Username(m_username);
	m_ftp.put_Password(m_password);

	if (m_proxymethod > 0 && m_proxyhostname != _T("") && m_proxyport > 0) {
		m_ftp.put_ProxyHostname(m_proxyhostname);
		m_ftp.put_ProxyPort(m_proxyport);
		//  Note: Your FTP Proxy server may or may not require authentication.
		m_ftp.put_ProxyUsername(m_proxyusername);
		m_ftp.put_ProxyPassword(m_proxypassword);
	}


	m_ftp.put_Passive(m_passive ? true : false);

	if (m_timeout > 0)
		m_ftp.put_ConnectTimeout(m_timeout);

	if (m_port > 0)
		m_ftp.put_Port(m_port);

	if (m_account != _T(""))
		m_ftp.put_Account(m_account);

	//  Connect and login to the FTP server.
	m_connected = m_ftp.Connect();
	if (m_connected != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		sConnectFailReason =  LimitErrorMessage(GetConnectFailReason());
		return false;
	}
	//m_ftp.put_Passive(m_passive ? true : false);

	m_ftp.SetTypeBinary();

	bool success = true;
	if (m_subdir != "") {
		success = m_ftp.ChangeRemoteDir(m_subdir);
		if (success != true) 
			sErrorMessage =  LimitErrorMessage(GetLastError());
	}
	return success;
}

BOOL CFTPClient::OpenConnection(CString sServerName, CString sUserName, CString sPassword, BOOL bPassiveMode,
								CString sSubDir, CString &sErrorMessage, CString &sConnectFailReason)
{
	return OpenConnection(sServerName, sUserName, sPassword, bPassiveMode, sSubDir, 0, 0, sErrorMessage, sConnectFailReason);
}

BOOL CFTPClient::OpenConnection(CString sServerName, CString sUserName, CString sPassword, BOOL bPassiveMode,
	CString sSubDir, int nPort, int nTimeout, CString &sErrorMessage, CString &sConnectFailReason)
{
	return OpenConnection(sServerName, sUserName, sPassword, bPassiveMode, sSubDir, nPort, nTimeout, _T(""), sErrorMessage, sConnectFailReason);
}

BOOL CFTPClient::OpenConnection(CString sServerName, CString sUserName, CString sPassword, BOOL bPassiveMode,
								CString sSubDir, int nPort, int nTimeout, CString sAccount, CString &sErrorMessage, CString &sConnectFailReason)
{

	sErrorMessage = _T("");

	m_host = sServerName;
	m_username = sUserName;
	m_password = sPassword;
	m_port = nPort;
	m_timeout = nTimeout;
	m_subdir = sSubDir;
	m_passive = bPassiveMode;
	m_account = sAccount;
	return OpenConnection(sErrorMessage, sConnectFailReason);

}

void CFTPClient::Disconnect()
{
	m_ftp.Disconnect();
	m_connected = FALSE;
}


BOOL CFTPClient::EnsureConnected(CString &sErrorMessage)
{
	CString sConnectFailReason = _T("");

	sErrorMessage = _T("");

	if (m_connected == false) {
		if (OpenConnection(sErrorMessage, sConnectFailReason) == false)
			return FALSE;
	}
	if (m_ftp.CheckConnection() == false) {
		if (OpenConnection(sErrorMessage, sConnectFailReason) == false)
			return FALSE;
	}
	return TRUE;
}

BOOL CFTPClient::GetFile(CString sRemoteFile, CString sLocalFile, BOOL bCloseAfterTransfer, CString &sErrorMessage)
{

	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	bool success = m_ftp.GetFile(sRemoteFile, sLocalFile);
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return TRUE;
}

BOOL CFTPClient::PutFile(CString sLocalFile, CString sRemoteFile, BOOL bCloseAfterTransfer, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage =  _T("");

	int nSize = m_util.GetFileSize(sLocalFile);
	m_ftp.put_ProgressMonSize(nSize);


	bool success = m_ftp.PutFile(sLocalFile, sRemoteFile);
	if (success != true) {
		sErrorMessage =  LimitErrorMessage(GetLastError());
		return FALSE;
	}

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return TRUE;
}
/*
BOOL CFTPClient::PutFileAsync(CString sLocalFile, CString sRemoteFile, BOOL bCloseAfterTransfer, FTPREGISTERPROGRESS pProgressCallbackProc, int nProgressNumber, CString &sErrorMessage)
{

	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;
	
	sErrorMessage = _T("");

	DWORD dwFullSize = m_util.GetFileSize(sLocalFile);
	

	TCHAR szCurrentFile[MAX_PATH];
	_tcscpy(szCurrentFile, m_util.GetFileName(sLocalFile));

	if (pProgressCallbackProc != NULL)
		(*pProgressCallbackProc)(nProgressNumber, 0, szCurrentFile);

	bool success = m_ftp.AsyncPutFileStart(sLocalFile, sRemoteFile);
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	double f = 0.0;
	while (m_ftp.get_AsyncFinished() != true) {
			
		if (dwFullSize > 0)
			f = (double)m_ftp.get_AsyncBytesSent() * 100.0 / (double)dwFullSize;
		int i = (int)f;
		int ret = 0;
		if (pProgressCallbackProc != NULL)
			(*pProgressCallbackProc)(nProgressNumber, i, szCurrentFile);
		if (ret == -1)
			m_ftp.AsyncAbort();

		m_ftp.SleepMs(1000);
	}


	if (pProgressCallbackProc != NULL)
		(*pProgressCallbackProc)(nProgressNumber, 0, szCurrentFile);

	int ret;
	if (m_ftp.get_AsyncSuccess() == true)
		ret = TRUE;
	else {
		sErrorMessage = LimitErrorMessage(m_ftp.asyncLog());
		ret = FALSE;
	}

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return ret;
}

BOOL CFTPClient::GetFileAsync(CString sRemoteFile, CString sLocalFile, BOOL bCloseAfterTransfer, FTPREGISTERPROGRESS pProgressCallbackProc, int nProgressNumber, CString &sErrorMessage)
{

	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	m_ftp.put_ListPattern("*");

	int fileSize = m_ftp.GetSizeByName(sRemoteFile);
	if (fileSize <= 0) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	TCHAR szCurrentFile[MAX_PATH];
	_tcscpy(szCurrentFile, sRemoteFile);

	if (pProgressCallbackProc != NULL)
		(*pProgressCallbackProc)(nProgressNumber, 0, szCurrentFile);



	bool success = m_ftp.AsyncGetFileStart(sRemoteFile, sLocalFile);
	if (success != true) {
		sErrorMessage =  LimitErrorMessage(GetLastError());
		return FALSE;
	}

	
	double f = 0.0;


	while (m_ftp.get_AsyncFinished() != true) {

		if (fileSize>0)
			f = (double)m_ftp.get_AsyncBytesReceived() * 100.0 / (double)fileSize;
		int i = (int)f;
		int ret = 0;
		if (pProgressCallbackProc != NULL)
			ret = (*pProgressCallbackProc)(nProgressNumber, i, szCurrentFile);
		if (ret == -1)
			m_ftp.AsyncAbort();
		m_ftp.SleepMs(1000);
	}


	if (pProgressCallbackProc != NULL)
		(*pProgressCallbackProc)(nProgressNumber, 0, szCurrentFile);

	int ret;
	if (m_ftp.get_AsyncSuccess() == true)
		ret = TRUE;
	else {
		sErrorMessage = LimitErrorMessage(m_ftp.asyncLog());
		ret = FALSE;
	}

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return ret;
}
*/

BOOL CFTPClient::RenameFile(CString sExistingRemoteFileName, CString sNewRemoteFileName, CString &sErrorMessage)
{

	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	bool success = m_ftp.RenameRemoteFile(sExistingRemoteFileName, sNewRemoteFileName);
	if (success != true) {
		sErrorMessage =  LimitErrorMessage(GetLastError());
		return FALSE;
	}

	return TRUE;
}


BOOL CFTPClient::DeleteFile(CString sRemoteFile, BOOL bCloseAfterTransfer, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;
	
	sErrorMessage =  _T("");

	bool success = m_ftp.DeleteRemoteFile(sRemoteFile);
	if (success != true) {
		sErrorMessage =  LimitErrorMessage(GetLastError());
		return FALSE;
	}

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return TRUE;
}


BOOL CFTPClient::CreateDirectory(CString sRemoteDir, CString &sErrorMessage)
{
	CString sCurrentDir = _T("");

	g_util.Logprintf("INFO: ftp CreateDirectory(%s)..", sRemoteDir); // BT/BTHA/20190408
	GetCurrentDirectory(sCurrentDir, sErrorMessage);
	g_util.Logprintf("INFO: ftp GetCurrentDirectory(0) %s", sCurrentDir); // BT/BTHA/20190408
	
	CString sRemoteDirOrg = sRemoteDir;
	if (sRemoteDir.GetLength() == 0)
		return TRUE;

	sRemoteDir = TrimSlashes(sRemoteDir);

	if (sRemoteDir.Find("/") == -1)	
		return CreateDirectoryEx(sRemoteDir, sErrorMessage);

	CStringArray aS;
	g_util.StringSplitter(sRemoteDir, "/", aS);
	CString sDir = _T("");
	BOOL success = TRUE;
	for (int i = 0; i < aS.GetCount(); i++)
	{
		
		if (sDir != _T("")) 
			ChangeDirectory(sDir, sErrorMessage);

		sDir = TrimSlashes(aS[i]);
		g_util.Logprintf("INFO: creating subdir %s..", sDir);
		if (DirectoryExists(sDir, sErrorMessage) == FALSE)
		{
			success = CreateDirectoryEx(sDir, sErrorMessage);
			if (success == FALSE)
				return FALSE;
			CString sCurrentDirNow;
			GetCurrentDirectory(sCurrentDirNow, sErrorMessage);
			g_util.Logprintf("INFO: ftp GetCurrentDirectory(%d) %s", 1+1,sCurrentDirNow); // BT/BTHA/20190408
		}
	}


	ChangeDirectory(sCurrentDir, sErrorMessage);
	return TRUE;
}

BOOL CFTPClient::CreateDirectoryEx(CString sRemoteDir, CString &sErrorMessage)
{
	g_util.Logprintf("INFO: ftp CreateDirectoryEx(%s)..", sRemoteDir);
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	if (DirectoryExists(sRemoteDir, sErrorMessage))
		return TRUE;

	bool success = m_ftp.CreateRemoteDir(sRemoteDir);
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	return TRUE;
}


BOOL CFTPClient::DeleteDirectory(CString sRemoteDir, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	// Delete a directory on the FTP server.  The directory
	// must not contain any files, otherwise an error status
	// will be returned.  Most FTP servers do not allow
	// directories containing files to be deleted.
	bool success = m_ftp.RemoveRemoteDir(sRemoteDir);
	if (success != true) {
		sErrorMessage =  LimitErrorMessage(GetLastError());
		return FALSE;
	}

	return TRUE;
}



void CFTPClient::SetSendBufferSize(DWORD dwSize)
{
	return; /// DISABLED!!!!!!!!!!!
	if (dwSize > 0)
		m_ftp.put_AllocateSize(dwSize);
}




void CFTPClient::SetAutoSYST(BOOL bUseSYST)
{
	m_ftp.put_AutoSyst((bool)bUseSYST);
}

void CFTPClient::SetAutoXCRC(BOOL bUseXCRC)
{
	m_ftp.put_AutoXcrc((bool)bUseXCRC);
}

void CFTPClient::SetAuthTLS(BOOL bUseTLS)
{
	m_ftp.put_AuthTls((bool)bUseTLS);
}

void CFTPClient::SetSSL(BOOL bUseSSL)
{
	m_ftp.put_Ssl((bool)bUseSSL);
}


void CFTPClient::SetHeartheatMS(int nHeartbeatMS)
{
	m_heartbeatMS = nHeartbeatMS;
}


CString CFTPClient::GetLastReply()
{
	CString s = m_ftp.lastReply();
	if (s.GetLength() > MAX_ERRMSG)
		s = s.Left(MAX_ERRMSG - 1);
	return s;
}

CString CFTPClient::GetLastError()
{
	CString s = m_ftp.lastErrorText();
	if (s.GetLength() > MAX_ERRMSG)
		s = s.Left(MAX_ERRMSG - 1);

	int n = s.Find("reply:");
	if (n == -1)
		n = s.Find("statusCode:");
	if (n != -1) {
		int m = s.Find("\r\n");
		if (m != -1) {
			s = s.Mid(n, m - n + 1);
			s.Replace("\r", "");
			s.Replace("\n", "");
		}
	}
	return s;
}



BOOL CFTPClient::GetCurrentDirectory(CString &sCurrentDir, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	sCurrentDir = _T("");

	CkString outStr;

	if (m_ftp.GetCurrentRemoteDir(outStr) == false) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}
	sCurrentDir = outStr.getString();
	return TRUE;
}

BOOL CFTPClient::ChangeDirectory(CString sDirectory, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage =  _T("");

	if (m_ftp.ChangeRemoteDir(sDirectory) == false) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	return TRUE;
}

// NOTE: Call SetCurrentDirectory before ScanDir..
int CFTPClient::ScanDir(CString sSearchMask, FILEINFOSTRUCT aFileList[], int nMaxFiles, CString sIgnoreMask, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return -1;

	sErrorMessage =  _T("");

	int nFiles = 0;
	//	FILETIME		WriteTime, CreateTime, LocalWriteTime;
	//	SYSTEMTIME		SysTime;
	CUtils util;

	//  The ListPattern property is our directory listing filter.
	if (sSearchMask == _T("") || sSearchMask == _T("*.*") || sSearchMask == _T(".*") || sSearchMask == _T("*"))
		m_ftp.put_ListPattern("*");
	else
		m_ftp.put_ListPattern(sSearchMask);

	//m_ftp.GetDirCount();
	int n = m_ftp.get_NumFilesAndDirs();
	if (n < 0) {
		sErrorMessage = LimitErrorMessage(GetLastError());;
		return -1;
	}

	if (n > nMaxFiles)
		n = nMaxFiles;

	if (n > 0) {
		for (int i = 0; i <= n - 1; i++) {
			if (m_ftp.GetIsDirectory(i) == false) {
				CString sFile = m_ftp.getFilename(i);
				sFile.Trim();
				int nSize = m_ftp.GetSize(i);
				if (sFile != _T(".") && sFile != _T("..") && nSize > 0) {

					if (util.GetExtension(sFile) == "part")
						continue;

					if (sIgnoreMask != "" && sIgnoreMask != "*" && sIgnoreMask != "*.*") {
						CString sExt1 = util.GetExtension(sFile);
						CString sExt2 = util.GetExtension(sIgnoreMask);
						sExt1.MakeUpper();
						sExt2.MakeUpper();
						if (sExt1 != "" && sExt2 != "" && sExt1 == sExt2)
							continue;
					}
					SYSTEMTIME tSysTime;
					
					aFileList[nFiles].sFileName = sFile;
					aFileList[nFiles].nFileSize = nSize;
					if (m_ftp.GetCreateTime(i, tSysTime))
						aFileList[nFiles].tWriteTime = util.Systime2CTime(tSysTime);
					else
						aFileList[nFiles].tWriteTime = CTime(1975, 1, 1, 0, 0, 0);

					aFileList[nFiles++].tJobTime = aFileList[nFiles].tWriteTime;
				}
			}
		}

	}

	return nFiles;
}

CString CFTPClient::TrimSlashes(CString sDir)
{
	sDir.Trim();
	if (sDir.Left(1) == "/")
		sDir = sDir.Mid(1);
	if (sDir.Right(1) == "/")
		sDir = sDir.Mid(0, sDir.GetLength() - 1);

	return sDir.Trim();

}

BOOL CFTPClient::DirectoryExists(CString sRemoteDir, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;
	
	sErrorMessage = _T("");
	sRemoteDir = TrimSlashes(sRemoteDir);

	m_ftp.put_ListPattern("*");

	int n = m_ftp.get_NumFilesAndDirs();
	if (n > 0) {

		for (int i = 0; i <= n - 1; i++) {
			if (m_ftp.GetIsDirectory(i) == true) {
				CString sDir = m_ftp.getFilename(i);
				sDir.Trim();
				if (sRemoteDir.CompareNoCase(sDir) == 0)
					return TRUE;
			}
		}
	}

	return FALSE;
}


BOOL CFTPClient::FileExists(CString sRemoteFile, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	m_ftp.put_ListPattern("*");

	int fileSize = m_ftp.GetSizeByName(sRemoteFile);

	return fileSize > 0 ? TRUE : FALSE;
}

int CFTPClient::GetFileSize(CString sRemoteFile, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	m_ftp.put_ListPattern("*");

	int fileSize = m_ftp.GetSizeByName(sRemoteFile);

	return fileSize > 0 ? fileSize : -1;
}

CString  CFTPClient::GetServerFeatures(CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return "";

	sErrorMessage = _T("");

	const char * ftpResponse = m_ftp.feat();
	
	CString s(ftpResponse);

	return s;

}
