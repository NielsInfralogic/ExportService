#include "stdafx.h"
#include "SFtpClient.h"
#include "Defs.h"
#include "Utils.h"
#include <CkSFtp.h>
#include <CkSFtpDir.h>
#include <CkSFtpFile.h>
#include <CkTask.h>
#include <CkSshKey.h>

CSFTPClient::CSFTPClient()
{
	m_heartbeatMS = 0;
	m_account = _T("");

	m_proxyhostname = _T("");
	m_proxyport = 0;
	m_proxymethod = 0;
	m_proxyusername = _T("");
	m_proxypassword = _T("");
}

CSFTPClient::~CSFTPClient()
{
}

// CSFTPClient member functions

void CSFTPClient::SetProxyMethod(int nProxyMethod, CString sProxyHostname, CString sProxyUsername, CString sProxyPassword, int nProxyPort)
{
	m_proxymethod = nProxyMethod;
	m_proxyhostname = sProxyHostname;
	m_proxyusername = sProxyUsername;
	m_proxypassword = sProxyPassword;
	m_proxyport = nProxyPort;

	m_homedir = "";

}

BOOL CSFTPClient::InitSession()
{
//	return m_ftp.UnlockComponent("INFRAL.CB1092019_4czEsHZS615x");;
	return m_ftp.UnlockComponent("NFRLGC.CB1112020_vZdFLucd1RAZ");;

}

CString CSFTPClient::LimitErrorMessage(CString s)
{
	if (s.GetLength() >= MAX_ERRMSG)
		return s.Left(MAX_ERRMSG - 1);

	return s;
}


CString CSFTPClient::GetConnectFailReason()
{
	int failReason = m_ftp.get_AuthFailReason();

	switch (failReason) {
	case 0:
		return _T("Success");
	case 1:
		return _T("Transport failure");
	case 2:
		return _T("Invalid key for public key authentication");
	case 3:
		return _T("No matching authentication method");
	case 4:
		return _T("SSH authentication protocol error");
	case 5:
		return _T("The incorrect password or private key");
	case 6:
		return _T("Already authenticated");
	case 7:
		return _T("Password change request");
	
	}

	return _T("");
}

BOOL CSFTPClient::OpenConnection(CString &sErrorMessage, CString &sConnectFailReason)
{

	sErrorMessage = _T("");
	sConnectFailReason = _T("");

	if (m_heartbeatMS > 0)
		m_ftp.put_HeartbeatMs(m_heartbeatMS);

	if (m_proxymethod > 0 && m_proxyhostname != _T("") && m_proxyport > 0) {
		m_ftp.put_HttpProxyHostname(m_proxyhostname);
		m_ftp.put_HttpProxyPort(m_proxyport);
		//  Note: Your FTP Proxy server may or may not require authentication.
		m_ftp.put_HttpProxyUsername(m_proxyusername);
		m_ftp.put_HttpProxyPassword(m_proxypassword);
	}


	//m_ftp.put_Passive(m_passive ? true : false);

	if (m_timeout > 0)
	{
		m_ftp.put_ConnectTimeoutMs(m_timeout * 1000);
		m_ftp.put_IdleTimeoutMs(m_timeout * 1000);
	}
	

	//if (m_account != _T(""))
	//	m_ftp.put_Account(m_account);

	//  Connect and login to the FTP server.
	m_connected = m_ftp.Connect(m_host, m_port);
	if (m_connected != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		sConnectFailReason = LimitErrorMessage(GetConnectFailReason());
		return false;
	}


	m_connected = m_ftp.AuthenticatePw(m_username, m_password);
	if (m_connected != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		sConnectFailReason = LimitErrorMessage(GetConnectFailReason());
		return false;
	}
	//  After authenticating, the SFTP subsystem must be initialized:
	bool success = m_ftp.InitializeSftp();
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		CkString sk;
		m_ftp.get_InitializeFailReason(sk);
		sConnectFailReason = LimitErrorMessage(sk.getString());
		return false;
	}

	GetCurrentDirectory(m_homedir, sErrorMessage);
	
	//m_ftp.SetTypeBinary();

//	bool success = true;
//	if (m_subdir != "") {
//		success = m_ftp.ChangeRemoteDir(m_subdir);
//		if (success != true)
//			sErrorMessage = LimitErrorMessage(GetLastError());
//	}
	return success;


}

BOOL CSFTPClient::OpenConnection(CString sServerName, CString sUserName, CString sPassword, BOOL bPassiveMode,
	CString sSubDir, CString &sErrorMessage, CString &sConnectFailReason)
{
	return OpenConnection(sServerName, sUserName, sPassword, bPassiveMode, sSubDir, 0, 0, sErrorMessage, sConnectFailReason);
}

BOOL CSFTPClient::OpenConnection(CString sServerName, CString sUserName, CString sPassword, BOOL bPassiveMode,
	CString sSubDir, int nPort, int nTimeout, CString &sErrorMessage, CString &sConnectFailReason)
{
	return OpenConnection(sServerName, sUserName, sPassword, bPassiveMode, sSubDir, nPort, nTimeout, _T(""), sErrorMessage, sConnectFailReason);
}

BOOL CSFTPClient::OpenConnection(CString sServerName, CString sUserName, CString sPassword, BOOL bPassiveMode,
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

void CSFTPClient::Disconnect()
{
	m_ftp.Disconnect();
	m_connected = FALSE;
}


BOOL CSFTPClient::EnsureConnected(CString &sErrorMessage)
{
	CString sConnectFailReason = _T("");

	sErrorMessage = _T("");

	if (m_connected == false) {
		if (OpenConnection(sErrorMessage, sConnectFailReason) == false)
			return FALSE;
	}
	if (m_ftp.get_IsConnected() == false) {
		if (OpenConnection(sErrorMessage, sConnectFailReason) == false)
			return FALSE;
	}
	return TRUE;
}

BOOL CSFTPClient::GetFile(CString sRemoteFile, CString sLocalFile, BOOL bCloseAfterTransfer, CString &sErrorMessage)
{

	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	if (m_subdir != _T(""))
		sRemoteFile = m_subdir + _T("/") + sRemoteFile;
	if (m_homedir != _T(""))
		sRemoteFile = m_homedir + _T("/") + sRemoteFile;
	bool success = m_ftp.DownloadFileByName(sRemoteFile, sLocalFile);
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return TRUE;
}

BOOL CSFTPClient::PutFile(CString sLocalFile, CString sRemoteFile, BOOL bCloseAfterTransfer, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	int nSize = m_util.GetFileSize(sLocalFile);
//	m_ftp.put_ProgressMonSize(nSize);

	if (m_subdir != _T(""))
		sRemoteFile = m_subdir + _T("/") + sRemoteFile;
	if (m_homedir != _T(""))
		sRemoteFile = m_homedir + _T("/") + sRemoteFile;
	bool success = m_ftp.UploadFileByName(sRemoteFile, sLocalFile);
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return TRUE;
}

BOOL CSFTPClient::PutFileAsync(CString sLocalFile, CString sRemoteFile, BOOL bCloseAfterTransfer, SFTPREGISTERPROGRESS pProgressCallbackProc, int nProgressNumber, CString &sErrorMessage)
{

	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	DWORD dwFullSize = m_util.GetFileSize(sLocalFile);


	TCHAR szCurrentFile[MAX_PATH];
	_tcscpy(szCurrentFile, m_util.GetFileName(sLocalFile));

	if (pProgressCallbackProc != NULL)
		(*pProgressCallbackProc)(nProgressNumber, 0, szCurrentFile);

	if (m_subdir != _T(""))
		sRemoteFile = m_subdir + _T("/") + sRemoteFile;
	if (m_homedir != _T(""))
		sRemoteFile = m_homedir + _T("/") + sRemoteFile;

	CkTask *task = m_ftp.UploadFileByNameAsync(sRemoteFile, sLocalFile);
	if (task == NULL) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	task->put_KeepProgressLog(false);

	bool success = task->Run();
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		delete task;
		return FALSE;
	}

	int curPctDone = 0;
	const char *name = 0;
	const char *value = 0;

	while (task->get_Finished() != true) {

		if (task->get_PercentDone() != curPctDone) {
			curPctDone = task->get_PercentDone();

		}

		int ret = 0;
		if (pProgressCallbackProc != NULL)
			ret = (*pProgressCallbackProc)(nProgressNumber, curPctDone, szCurrentFile);
		if (ret == -1)
			task->Cancel();


		/*while ((task->get_ProgressLogSize() > 0)) {
			//  Get the 1st entry, emit it, and then remove it..
			name = task->progressInfoName(0);
			value = task->progressInfoValue(0);

			//  Entries reporting the received byte count will have the name "RcvByteCount"
			//  Entries reporting the current bytes-per-second will have the name "RcvBytesPerSec"

			// std::cout << name << ": " << value << "\r\n";
			task->RemoveProgressInfo(0);
		}*/


		task->SleepMs(1000);
	}


	if (pProgressCallbackProc != NULL)
		(*pProgressCallbackProc)(nProgressNumber, 0, szCurrentFile);

	if (task->get_StatusInt() != 7) {
		sErrorMessage = LimitErrorMessage(task->status());
		delete task;
		return FALSE;
	}

	success = task->GetResultBool();

	delete task;

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return success;
}

BOOL CSFTPClient::GetFileAsync(CString sRemoteFile, CString sLocalFile, BOOL bCloseAfterTransfer, SFTPREGISTERPROGRESS pProgressCallbackProc, int nProgressNumber, CString &sErrorMessage)
{

	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	TCHAR szCurrentFile[MAX_PATH];
	_tcscpy(szCurrentFile, sRemoteFile);

	if (pProgressCallbackProc != NULL)
		(*pProgressCallbackProc)(nProgressNumber, 0, szCurrentFile);

	if (m_subdir != _T(""))
		sRemoteFile = m_subdir + _T("/") + sRemoteFile;

	CkTask *task = m_ftp.DownloadFileByNameAsync(sRemoteFile, sLocalFile);
	if (task == NULL) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	task->put_KeepProgressLog(false);

	bool success = task->Run();
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		delete task;
		return FALSE;
	}

	double f = 0.0;

	int curPctDone = 0;
	const char *name = 0;
	const char *value = 0;
	while (task->get_Finished() != true) {


		if (task->get_PercentDone() != curPctDone) {
			curPctDone = task->get_PercentDone();
			
		}

		int ret = 0;
		if (pProgressCallbackProc != NULL)
			ret = (*pProgressCallbackProc)(nProgressNumber, curPctDone, szCurrentFile);
		if (ret == -1)
			task->Cancel();


		/*while ((task->get_ProgressLogSize() > 0)) {
			//  Get the 1st entry, emit it, and then remove it..
			name = task->progressInfoName(0);
			value = task->progressInfoValue(0);

			//  Entries reporting the received byte count will have the name "RcvByteCount"
			//  Entries reporting the current bytes-per-second will have the name "RcvBytesPerSec"

			// std::cout << name << ": " << value << "\r\n";
			task->RemoveProgressInfo(0);
		}*/


		task->SleepMs(1000);
	}


	if (pProgressCallbackProc != NULL)
		(*pProgressCallbackProc)(nProgressNumber, 0, szCurrentFile);

	if (task->get_StatusInt() != 7) {
		sErrorMessage = LimitErrorMessage(task->status());
		delete task;
		return FALSE;
	}

	success = task->GetResultBool();
	delete task;

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return success;
}


BOOL CSFTPClient::RenameFile(CString sExistingRemoteFileName, CString sNewRemoteFileName, CString &sErrorMessage)
{

	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	if (m_subdir != _T(""))
		sExistingRemoteFileName = m_subdir + _T("/") + sExistingRemoteFileName;
	if (m_homedir != _T(""))
		sExistingRemoteFileName = m_homedir + _T("/") + sExistingRemoteFileName;
	if (m_subdir != _T(""))
		sNewRemoteFileName = m_subdir + _T("/") + sNewRemoteFileName;
	if (m_homedir != _T(""))
		sNewRemoteFileName = m_homedir + _T("/") + sNewRemoteFileName;
	bool success = m_ftp.RenameFileOrDir(sExistingRemoteFileName, sNewRemoteFileName);
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	return TRUE;
}


BOOL CSFTPClient::DeleteFile(CString sRemoteFile, BOOL bCloseAfterTransfer, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	CString sCurrentDir = "";

	GetCurrentDirectory(sCurrentDir, sErrorMessage);


	if (m_subdir != _T(""))
		sRemoteFile = m_subdir + _T("/") + sRemoteFile;
	if (m_homedir != _T(""))
		sRemoteFile = m_homedir + _T("/") + sRemoteFile;

	bool success = m_ftp.RemoveFile(sRemoteFile);
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	if (bCloseAfterTransfer)
		m_ftp.Disconnect();

	return TRUE;
}

CString CSFTPClient::TrimSlashes(CString sDir)
{
	sDir.Trim();
	if (sDir.Left(1) == "/")
		sDir = sDir.Mid(1);
	if (sDir.Right(1) == "/")
		sDir = sDir.Mid(0, sDir.GetLength() - 1);

	return sDir.Trim();

}



extern CUtils g_util;

BOOL CSFTPClient::CreateDirectory(CString sRemoteDir, CString &sErrorMessage)
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
			g_util.Logprintf("INFO: ftp GetCurrentDirectory(%d) %s", 1 + 1, sCurrentDirNow); // BT/BTHA/20190408
		}
	}

	ChangeDirectory(sCurrentDir, sErrorMessage);

	return TRUE;
}


BOOL CSFTPClient::CreateDirectoryEx(CString sRemoteDir, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");
	
	if (DirectoryExists(sRemoteDir, sErrorMessage))
		return TRUE;

	bool success = m_ftp.CreateDir(sRemoteDir);
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	return TRUE;
}


BOOL CSFTPClient::DeleteDirectory(CString sRemoteDir, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	// Delete a directory on the FTP server.  The directory
	// must not contain any files, otherwise an error status
	// will be returned.  Most FTP servers do not allow
	// directories containing files to be deleted.
	bool success = m_ftp.RemoveDir(sRemoteDir);
	if (success != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	return TRUE;
}



void CSFTPClient::SetMaxPacketSize(DWORD dwSize)
{
	if (dwSize > 0)
		m_ftp.put_MaxPacketSize(dwSize);
}







void CSFTPClient::SetHeartheatMS(int nHeartbeatMS)
{
	m_heartbeatMS = nHeartbeatMS;
}


CString CSFTPClient::GetLastReply()
{
	return  m_ftp.get_LastMethodSuccess() ? "OK" : "Error";
}

CString CSFTPClient::GetLastError()
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



BOOL CSFTPClient::GetCurrentDirectory(CString &sCurrentDir, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	sCurrentDir = _T("");

	const char *absPath = m_ftp.realPath(".", "");
	if (m_ftp.get_LastMethodSuccess() != true) {
		sErrorMessage = LimitErrorMessage(GetLastError());
		return FALSE;
	}

	if (absPath != NULL)
		sCurrentDir = absPath;
	return TRUE;
}

BOOL CSFTPClient::ChangeDirectory(CString sDirectory, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	if (sDirectory == ".") // Stay in current?
		return TRUE;
	

	if (sDirectory == "..")
		sDirectory = "";
	m_subdir = sDirectory;
	return TRUE;
}

// NOTE: Call SetCurrentDirectory before ScanDir..
int CSFTPClient::ScanDir(CString sSearchMask, FILEINFOSTRUCT aFileList[], int nMaxFiles, CString sIgnoreMask, CString &sErrorMessage)
{
	CUtils util;
	int nFiles = 0;

	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");

	const char *handle = NULL;

	CString sRemoteDir = ".";
	if (m_homedir != _T(""))
		sRemoteDir = m_homedir;

	if (m_subdir != _T(""))
		sRemoteDir = sRemoteDir + _T("/") + m_subdir;

	handle = m_ftp.openDir(sRemoteDir);
	if (m_ftp.get_LastMethodSuccess() != true || handle == NULL) {
		sErrorMessage = m_ftp.lastErrorText();
		return 0;
	}

	//  Download the directory listing:
	CkSFtpDir *dirListing = m_ftp.ReadDir(handle);
	if (m_ftp.get_LastMethodSuccess() != true || dirListing == NULL) {
		sErrorMessage = m_ftp.lastErrorText();
		return 0;
	}

	//  Iterate over the files.

	int n = dirListing->get_NumFilesAndDirs();
	if (n == 0) {
		delete dirListing;
		m_ftp.CloseHandle(handle);
		return 0;
	}

	BOOL bFound = FALSE;
	int nSize = 0;
	for (int i = 0; i <= n - 1; i++) {
		CkSFtpFile *fileObj = 0;
		fileObj = dirListing->GetFileObject(i);
		if (fileObj == NULL)
			continue;
		if (fileObj->get_IsDirectory() == false) {
			CString sFile = fileObj->filename();
			nSize = fileObj->get_Size32();
			
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
				tSysTime.wYear = 1975;

				aFileList[nFiles].sFileName = sFile;
				aFileList[nFiles].nFileSize = nSize;
				fileObj->get_CreateTime(tSysTime);
				aFileList[nFiles].tWriteTime = util.Systime2CTime(tSysTime);
				if (tSysTime.wYear <= 1975)
					aFileList[nFiles].tWriteTime = CTime(1975, 1, 1, 0, 0, 0);
				else
					aFileList[nFiles++].tJobTime = aFileList[nFiles].tWriteTime;

				if (nFiles >= nMaxFiles)
					break;
			}
		}
			
		delete fileObj;
	}

	m_ftp.CloseHandle(handle);

	return nFiles;
}

BOOL CSFTPClient::DirectoryExists(CString sRemoteDir, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");
	sRemoteDir.Trim();

	const char *handle = NULL;
	handle = m_ftp.openDir(".");
	if (m_ftp.get_LastMethodSuccess() != true || handle == NULL) {
		sErrorMessage = m_ftp.lastErrorText();
		return FALSE;
	}

	//  Download the directory listing:
	CkSFtpDir *dirListing = m_ftp.ReadDir(handle);
	if (m_ftp.get_LastMethodSuccess() != true || dirListing == NULL) {
		sErrorMessage = m_ftp.lastErrorText();
		return FALSE;
	}



	//  Iterate over the files.

	int n = dirListing->get_NumFilesAndDirs();
	if (n == 0) {
		delete dirListing;
		m_ftp.CloseHandle(handle);
		return FALSE;
	}

	BOOL bFound = FALSE;
	for (int i = 0; i <= n - 1; i++) {
		CkSFtpFile *fileObj = 0;
		fileObj = dirListing->GetFileObject(i);
		if (fileObj == NULL)
			continue;
		if (fileObj->get_IsDirectory()) {
			CString s = fileObj->filename();
			s.Trim();
			delete fileObj;
			if (s.CompareNoCase(sRemoteDir) == 0) {
				bFound = TRUE;
				break;
			}
		}

	}

	m_ftp.CloseHandle(handle);

	return bFound;
}


BOOL CSFTPClient::FileExists(CString sRemoteFile, CString &sErrorMessage)
{
	int nSize = GetFileSize(sRemoteFile, sErrorMessage);

	return nSize > 0;
}

int CSFTPClient::GetFileSize(CString sRemoteFile, CString &sErrorMessage)
{
	if (EnsureConnected(sErrorMessage) == FALSE)
		return FALSE;

	sErrorMessage = _T("");
	sRemoteFile.Trim();

	const char *handle = NULL;

	CString sRemoteDir = ".";
	if (m_homedir != _T(""))
		sRemoteDir = m_homedir;

	if (m_subdir != _T(""))
		sRemoteDir = sRemoteDir + _T("/") + m_subdir;

	handle = m_ftp.openDir(sRemoteDir);
	if (m_ftp.get_LastMethodSuccess() != true || handle == NULL) {
		sErrorMessage = m_ftp.lastErrorText();
		return -1;
	}

	//  Download the directory listing:
	CkSFtpDir *dirListing = m_ftp.ReadDir(handle);
	if (m_ftp.get_LastMethodSuccess() != true || dirListing == NULL) {
		sErrorMessage = m_ftp.lastErrorText();
		return -1;
	}



	//  Iterate over the files.

	int n = dirListing->get_NumFilesAndDirs();
	if (n == 0) {
		delete dirListing;
		m_ftp.CloseHandle(handle);
		return -1;
	}

	BOOL bFound = FALSE;
	int nSize = 0;
	for (int i = 0; i <= n - 1; i++) {
		CkSFtpFile *fileObj = 0;
		fileObj = dirListing->GetFileObject(i);
		if (fileObj == NULL)
			continue;
		if (fileObj->get_IsDirectory() == false) {
			CString s = fileObj->filename();
			nSize = fileObj->get_Size32();
			s.Trim();
			delete fileObj;
			if (s.CompareNoCase(sRemoteFile) == 0) {
				bFound = TRUE;
				break;
			}
		}

	}

	m_ftp.CloseHandle(handle);



	return nSize;
}


CString  CSFTPClient::GetServerFeatures(CString &sErrorMessage)
{
	sErrorMessage = "";
	return "";

}
