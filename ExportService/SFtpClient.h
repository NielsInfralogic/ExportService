#pragma once
#pragma once

#include "stdafx.h"
#include "afxinet.h"
#include "Utils.h"
#include "Defs.h"
#include <CkSFtp.h>
#include <CkSFtpProgress.h>

extern BOOL g_breaktransfer;
typedef int(__stdcall *SFTPREGISTERPROGRESS)(int nProgressNumber, int nProgressCB, TCHAR *szCurrentFile);

class CSFTPClientProgress : public CkSFtpProgress
{
public:
	int m_clientNumber;
	SFTPREGISTERPROGRESS m_pProgressCallbackProc;
	TCHAR m_currentFile[MAX_PATH];

	CSFTPClientProgress(void) {
		m_pProgressCallbackProc = NULL;
		m_clientNumber = 0;
		strcpy(m_currentFile, "");
	}

	virtual ~CSFTPClientProgress(void) { }

	void SetProgressJob(int nClient, CString sCurrentFile, SFTPREGISTERPROGRESS pProgressCallbackProc)
	{
		m_clientNumber = nClient;
		strcpy(m_currentFile, sCurrentFile);
		m_pProgressCallbackProc = pProgressCallbackProc;
	}


	// Override the AbortCheck method (which is a virtual method in CkFtp2Progress)
	// Called periodically according to the value of the HeartbeatMs property.
	void AbortCheck(bool *abort)
	{
		// If your application wishes to abort the email sending/receiving operation,
		// set the abort boolean like this:
		*abort = (bool)g_breaktransfer;
	}

	// Percentage completion events may also be overridden to abort time-consuming operations.
	void SendPercentDone(long pctDone, bool *abort)
	{

		if (m_pProgressCallbackProc != NULL)
			(*m_pProgressCallbackProc)(m_clientNumber, pctDone, m_currentFile);

		*abort = (bool)g_breaktransfer;
	}

	void ReadPercentDone(long pctDone, bool *abort)
	{
		if (m_pProgressCallbackProc != NULL)
			(*m_pProgressCallbackProc)(m_clientNumber, pctDone, m_currentFile);

		*abort = (bool)g_breaktransfer;
	}
};

class CSFTPClient
{
public:
	CSFTPClient();
	virtual ~CSFTPClient();

	CkSFtp	m_ftp;
	CUtils	m_util;
	CSFTPClientProgress m_progressfunc;

	bool	m_connected;

	CString m_subdir;
	CString m_host;
	CString m_username;
	CString m_password;
	int		m_port;
	int		m_timeout;
	BOOL	m_passive;
	CString m_account;
	CString m_homedir;

	int		m_heartbeatMS;

	CString m_proxyhostname;
	int		m_proxyport;
	int		m_proxymethod;
	CString m_proxyusername;
	CString m_proxypassword;

	CString LimitErrorMessage(CString s);

	BOOL	InitSession();

	void	SetProxyMethod(int nProxyMethod, CString sProxyHostname, CString sProxyUsername, CString sProxyPassword, int nProxyPort);

	BOOL	EnsureConnected(CString &sErrorMessage);
	BOOL	OpenConnection(CString &sErrorMessage, CString &sConnectFailReason);
	BOOL	OpenConnection(CString sServerName, CString sUserName, CString sPassword, BOOL bPassiveMode, CString sSubDir, CString &sErrorMessage, CString &sConnectFailReason);
	BOOL	OpenConnection(CString sServerName, CString sUserName, CString sPassword, BOOL bPassiveMode, CString sSubDir, int dwPort, int nTimeout, CString &sErrorMessage, CString &sConnectFailReason);
	BOOL	OpenConnection(CString sServerName, CString sUserName, CString sPassword, BOOL bPassiveMode, CString sSubDir, int nPort, int nTimeout, CString sAccount, CString &sErrorMessage, CString &sConnectFailReason);

	BOOL	GetFile(CString sRemoteFile, CString sLocalFile, BOOL bCloseAfterTransfer, CString &sErrorMessage);
	BOOL	GetFileAsync(CString sRemoteFile, CString sLocalFile, BOOL bCloseAfterTransfer, SFTPREGISTERPROGRESS pProgressCallbackProc, int nProgressNumber, CString &sErrorMessage);
	BOOL	PutFile(CString sLocalFile, CString sRemoteFile, BOOL bCloseAfterTransfer, CString &sErrorMessage);
	BOOL	PutFileAsync(CString sLocalFile, CString sRemoteFile, BOOL bCloseAfterTransfer, SFTPREGISTERPROGRESS pProgressCallbackProc, int nProgressNumber, CString &sErrorMessage);

	BOOL	RenameFile(CString sExistingRemoteFileName, CString sNewRemoteFileName, CString &sErrorMessage);

	BOOL	DeleteFile(CString sRemoteFile, BOOL bCloseAfterTransfer, CString &sErrorMessage);
	int		GetFileSize(CString sRemoteFile, CString &sErrorMessage);
	BOOL	DeleteDirectory(CString sRemoteDir, CString &sErrorMessage);
	void	SetMaxPacketSize(DWORD dwSize);



	void	SetHeartheatMS(int nHeartbeatMS);
	BOOL	GetCurrentDirectory(CString &sCurrentDir, CString &sErrorMessage);

	BOOL	ChangeDirectory(CString sDirectory, CString &sErrorMessage);
	int		ScanDir(CString sSearchMask, FILEINFOSTRUCT aFileList[], int nMaxFiles, CString sIgnoreMask, CString &sErrorMessage);

	BOOL	DirectoryExists(CString sRemoteFile, CString &sErrorMessage);
	BOOL	FileExists(CString sRemoteFile, CString &sErrorMessage);
	BOOL	CreateDirectory(CString sRemoteDir, CString &sErrorMessage);
	BOOL	CreateDirectoryEx(CString sRemoteDir, CString &sErrorMessage);

	CString TrimSlashes(CString sDir);

	void	Disconnect();
	CString  GetServerFeatures(CString &sErrorMessage);

	CString GetLastReply();
	CString GetLastError();

	CString GetConnectFailReason();

};


