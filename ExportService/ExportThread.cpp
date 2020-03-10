// PollThread and global functions

#include "stdafx.h"
#include <afxmt.h>
#include <afxtempl.h>
#include <direct.h>
#include "Defs.h"
#include "DatabaseManager.h"
#include "ExportThread.h"
#include "TransmissionThread.h"

#include "Utils.h"
#include <iostream>
#include <winnetwk.h>
#include "Markup.h"
#include "FtpClient.h"
#include "SFtpClient.h"
#include "StatusFile.h"
#include "Prefs.h"
#include "EmailClient.h"

extern CPrefs g_prefs;
extern CUtils g_util;
extern CTime g_lastSetupFileTime;
extern BOOL g_BreakSignal;
extern BOOL g_bAsyncCopyMode;
extern BOOL	 g_connected;
extern BOOL		g_exportthreadrunning;

CCriticalSection g_logWriteLock;
CCriticalSection g_logWriteLock2;
int g_reportingChannelID = -1;

/*
int __stdcall ProgressCallback(int nSetupIndex, int nProgressPercent, char *szCurrentFile)
{
	BOOL bBreakRequest = g_BreakSignal;

	//CString sCurrentFile(szCurrentFile);

	if (g_prefs.m_bypasscallback == FALSE) {
		if (nProgressPercent < 0)
			nProgressPercent = 0;
		if (nProgressPercent > 100)
			nProgressPercent = 100;
		
		if (g_pDlg == NULL)
			return 1;

		PROGRESSLOGITEM callbackFtpProgressLogItem;

		callbackFtpProgressLogItem.nTick = nProgressPercent;
		callbackFtpProgressLogItem.sMessage = _T("Sending file..");
		callbackFtpProgressLogItem.sFileName = szCurrentFile;
		callbackFtpProgressLogItem.nSetupIndex = nSetupIndex;
		callbackFtpProgressLogItem.nStatus = FOLDERSTATUS_INPROGRESS;
		callbackFtpProgressLogItem.bEnabled = -1;

		if (g_pDlg->GetSafeHwnd()) {
			DWORD_PTR result;
			::SendMessageTimeout(g_pDlg->GetSafeHwnd(), UWM_UPDATEFTPPROGRESS, 0, (LPARAM)&callbackFtpProgressLogItem,
				SMTO_ABORTIFHUNG | SMTO_NORMAL, MESSAGETIMEOUT, &result);
		}
	}

	return bBreakRequest ? -1 : 1;
}
*/
/*
int __stdcall ProgressCallbackIn(int nSetupIndex, int nProgressPercent, char *szCurrentFile)
{
	BOOL bBreakRequest = g_BreakSignal;

	//CString sCurrentFile(szCurrentFile);

	if (g_prefs.m_bypasscallback == FALSE) {
		if (nProgressPercent < 0)
			nProgressPercent = 0;
		if (nProgressPercent > 100)
			nProgressPercent = 100;

		if (g_pDlg == NULL)
			return 1;

		PROGRESSLOGITEM callbackFtpProgressLogItem;

		callbackFtpProgressLogItem.nTick = nProgressPercent;
		callbackFtpProgressLogItem.sMessage = _T("Receiving file..");
		callbackFtpProgressLogItem.sFileName = szCurrentFile;
		callbackFtpProgressLogItem.nSetupIndex = nSetupIndex;
		callbackFtpProgressLogItem.nStatus = FOLDERSTATUS_INPROGRESS;
		callbackFtpProgressLogItem.bEnabled = -1;

		if (g_pDlg->GetSafeHwnd()) {
			DWORD_PTR result;
			::SendMessageTimeout(g_pDlg->GetSafeHwnd(), UWM_UPDATEFTPPROGRESS, 0, (LPARAM)&callbackFtpProgressLogItem,
				SMTO_ABORTIFHUNG | SMTO_NORMAL, MESSAGETIMEOUT, &result);
		}
	}

	return bBreakRequest ? -1 : 1;
}
*/

CWinThread *g_pThread[MAXCHANNELS] = { NULL };
CWinThread *g_pTransmissionThread = NULL;

BOOL StartAllThreads()
{
	// Ini file IS loaded - startup all threads..

	g_logWriteLock.Unlock();
	g_logWriteLock2.Unlock();
	CString sErrorMessage;

	// This is shared connection for all Export threads
	// ConnectToDB() will load name caches
	g_connected = g_prefs.ConnectToDB(TRUE, sErrorMessage);
	if (g_connected == FALSE)
		g_util.LogprintfTransmit("ERROR:  ConnectToDB() failed - %s", sErrorMessage);

	g_pTransmissionThread = AfxBeginThread(TransmissionThread, (LPVOID)NULL);
	
	for (int i = 0; i < g_prefs.m_ChannelList.GetCount(); i++) {
		int nEnabled = g_prefs.m_ChannelList[i].m_enabled;
		CString sWorkFolder = g_prefs.m_workfolder + _T("\\") + g_util.Int2String(g_prefs.m_ChannelList[i].m_channelID);
		::CreateDirectory(sWorkFolder, NULL);

		// This channel reports service alive progress.
		if (g_reportingChannelID < 0) {
			if (g_prefs.m_ChannelList[i].m_enabled > 0)
				g_reportingChannelID = g_prefs.m_ChannelList[i].m_channelID;
		}

	 	g_pThread[i] = AfxBeginThread(ExportThread, (LPVOID)g_prefs.m_ChannelList[i].m_channelID);
	}


	while (g_BreakSignal == FALSE) {
		::Sleep(500);
	}

	return 0;
}

//CDatabaseManager g_DB;		// One shared connection for all Export threads


UINT ExportThread(LPVOID param)		// One thread per setup
{

	CFTPClient ftp_out;
	CSFTPClient sftp_out;
	CEmailClient email_out;

	ftp_out.InitSession();
	sftp_out.InitSession();

	ftp_out.SetHeartheatMS(g_prefs.m_heartbeatMS);
	sftp_out.SetHeartheatMS(g_prefs.m_heartbeatMS);

	BOOL bIsConnected = FALSE;;


	FILEINFOSTRUCT aFilesReady[MAXFILESPERTHREAD+1];
	
	int channelID = (int)param;

	CString s;

	BOOL	bDoBreak = false;

	CString sErrorMessage = _T("");
	CString sErrorMessage2 = _T("");
	CString sConnectFailReason = _T("");
	CString sInfo = _T("");

	int nFiles = 0;
	BOOL bReConnect = FALSE;
	int	bFirstTime = TRUE;

	
	CDatabaseManager m_ExportDB;
	CString sMsg;
	BOOL bExcludeMergePDF = FALSE;
	// name caches already loaded!!
	BOOL m_connected = m_ExportDB.InitDB(g_prefs.m_DBserver, g_prefs.m_Database, g_prefs.m_DBuser, g_prefs.m_DBpassword,
																								g_prefs.m_IntegratedSecurity, sErrorMessage);
	if (m_connected == FALSE)
		g_util.Logprintf("ERROR: ExportDB.InitDB() - %s", sErrorMessage);
	int nTick = 0;
	//int nSpoolers = (int)g_prefs.SetupList.GetCount();

	CHANNELSTRUCT *pChannel = g_prefs.GetChannelStruct(m_ExportDB, channelID);
	if (pChannel != NULL) {	
		g_util.Logprintf("INFO: ExportThread started for channel %d - %s (Enabled %d)", channelID, pChannel->m_channelname, pChannel->m_enabled);
	} else
		g_util.Logprintf("ERROR: Unable to retrieve channel structure for channel index %d ", channelID);

	CString sChannelName = pChannel != NULL ? pChannel->m_channelname : "";
	
	g_util.LogprintfExport(sChannelName, "INFO: ExportThread started");

	

	BOOL bIsStopped = FALSE;
	BOOL bReconnect = FALSE;
	do {

		g_exportthreadrunning = TRUE;

		if (g_BreakSignal)
			break;
		++nTick;

		for (int tm = 0; tm < 10; tm++) {
			if (g_BreakSignal)
				break;
			::Sleep(100);
		}
		if (g_BreakSignal)
			break;

		if (bReconnect) {
			m_ExportDB.ExitDB();
			for (int i = 0; i < 20; i++) {
				Sleep(100);
				if (g_BreakSignal)
					break;
			}
			if (g_BreakSignal)
				break;

			bReconnect = FALSE;
			BOOL m_connected = m_ExportDB.InitDB(g_prefs.m_DBserver, g_prefs.m_Database, g_prefs.m_DBuser, g_prefs.m_DBpassword,
				g_prefs.m_IntegratedSecurity, sErrorMessage);

			if (m_connected == FALSE) {
				g_util.LogprintfExport(sChannelName,"ERROR: Reconnect InitDB() returned - %s", sErrorMessage);
				bReconnect = TRUE;
				Sleep(1000);
				continue;
			}
		}

		if (channelID == g_reportingChannelID) {
			if ((nTick)%20 == 0) {
				if (g_connected)
					m_ExportDB.UpdateService(SERVICESTATUS_RUNNING, "", "", "", sErrorMessage);
			}


			if (bReConnect) {
				::Sleep(2500);
				g_util.LogprintfExport(sChannelName, "INFO: Re-connection pause..");
				::Sleep(2500);
				bReConnect = FALSE;			
			}
		}

		if (pChannel == NULL)
			continue;

		if (pChannel->m_enabled == FALSE) {
			Sleep(500);
			m_ExportDB.ReLoadChannelOnOff(channelID, sErrorMessage);
			if (pChannel->m_enabled == FALSE || pChannel->m_ownerinstance != g_prefs.m_instancenumber) {
				
				bIsStopped = TRUE;
				::Sleep(500);

				continue;
			}
		}

		// Export not stopped - proceed

		// Reload config ?

		CTime tNewChannelChangeTime;
		m_ExportDB.GetChannelConfigChangeTime(channelID, tNewChannelChangeTime, sErrorMessage);
		if (tNewChannelChangeTime > pChannel->m_configchangetime) {
			m_ExportDB.LoadNewChannel(pChannel->m_channelID, sErrorMessage);
			g_util.LogprintfExport(sChannelName, "INFO: Reloaded config data..");
			pChannel = g_prefs.GetChannelStruct(m_ExportDB, channelID);
			pChannel->m_configchangetime = tNewChannelChangeTime;
		}

		if (g_BreakSignal)
			break;

		bIsStopped = FALSE;
		CString sInputFolder = g_prefs.m_workfolder + _T("\\") + g_util.Int2String(channelID);
		CString outputFolder = pChannel->m_outputuseftp == NETWORKTYPE_SHARE ? pChannel->m_outputfolder : "ftp://" + pChannel->m_outputftpserver + "/" +   pChannel->m_outputftpfolder;

		// Check input folder access

		if (g_util.CheckFolderWithPing(sInputFolder) == FALSE) {
			if (g_prefs.m_logToFile2 ) 
				g_util.LogprintfExport(sChannelName, "NET INFO: Input CheckFolderWithPing() failed..");
			BOOL bOK = FALSE;
			
			if (bOK == FALSE) {
				sErrorMessage.Format("Unable to locate LAN work folder %s. Please check configuration", (LPCTSTR)sInputFolder);
				if (g_prefs.m_logToFile2)
					g_util.LogprintfExport(sChannelName, "NET INFO: Input Reconnect() failed..");
				g_util.LogprintfExport(sChannelName, "%s",sErrorMessage);
				bReConnect = TRUE;
				g_util.SendMail(MAILTYPE_FOLDERERROR, sErrorMessage);
				sErrorMessage2 = sErrorMessage;
				m_ExportDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), sErrorMessage2, sErrorMessage);
				::Sleep(1000);
				continue;
			}
		} 

		// Check output folder access (SMB output only)
		BOOL bOutputAccessOK = TRUE;
		
		if (pChannel->m_outputuseftp == NETWORKTYPE_SHARE) {
			if (g_util.CheckFolderWithPing(pChannel->m_outputfolder) == FALSE) {
				if (g_prefs.m_logToFile2)
					g_util.LogprintfExport(sChannelName, "NET INFO: Output CheckFolderWithPing() failed..");
				BOOL bOK = FALSE;
		
				if (!pChannel->m_outputusecurrentuser)
					bOK = g_util.Reconnect(pChannel->m_outputfolder, pChannel->m_outputusername, pChannel->m_outputpassword);
						
				if (bOK == FALSE) {
					if (g_prefs.m_logToFile2)
						g_util.LogprintfExport(sChannelName, "NET INFO: Output Reconnect() failed..");
					
					sErrorMessage2.Format("Unable to locate output folder %s. Please check configuration", (LPCSTR)pChannel->m_outputfolder);
					g_util.LogprintfExport(sChannelName, "%s",sErrorMessage2);
					bReConnect = TRUE;
					bOutputAccessOK = FALSE;
					
					m_ExportDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), sErrorMessage2, sErrorMessage);
					::Sleep(1000);

					continue;
				}
			} else { 
				if (g_prefs.m_filebuildupseparatefolder) {		
					s.Format("%s\\tmp", (LPCTSTR)pChannel->m_outputfolder);
					::CreateDirectory(s, NULL);
				}

			}
		}

		////////////////////////////////////////////
		// Folder initialization done
		////////////////////////////////////////////
	
		if (bOutputAccessOK == FALSE && pChannel->m_breakifnooutputaccess) {			
			Sleep(1000);
			continue;
		}			
			
		for (int tm=0; tm<100; tm++) {
			if (g_BreakSignal)
				break;				
			::Sleep(10 * g_prefs.m_defaultpolltime);
		}
	
		if (g_BreakSignal)
			break;

		if (pChannel->m_useprecommand && pChannel->m_precommand != "")
			g_util.DoCreateProcess(pChannel->m_precommand);

		g_util.AlivePoll();

		//if (g_prefs.m_logToFile2 && bDebugMe)
		//	g_util.Logprintf("INFO: Scanning folder %s for files...", sInputFolder);
		BOOL bHasLock = FALSE;
		while (bHasLock == FALSE) {

			int nRet = m_ExportDB.AcuireExportLock(pChannel->m_channelID, sErrorMessage);
			if (nRet == 0) {
				//g_util.LogprintfExport(sChannelName, "INFO: Waiting for lock...");
				::Sleep(1000);
				continue;
			}
			if (nRet == -1)
				g_util.LogprintfExport(sChannelName, "ERROR: AcuireExportLock() - %s", sErrorMessage);

			bHasLock = TRUE;		
		}
		//if (bHasLock && g_prefs.m_logToFile2)
		//	g_util.LogprintfExport(sChannelName, "INFO: Got lock!");

		nFiles = g_util.ScanDir(sInputFolder, "*.*", aFilesReady, MAXFILESPERTHREAD);
		//if (g_prefs.m_logToFile2 && bDebugMe)
		//	g_util.Logprintf("INFO: Scan folder %s done (%d) ", sInputFolder, nFiles);

		for (int f1 = 0; f1 < nFiles; f1++) 
			aFilesReady[f1].sFolder = _T("");
		
		if (nFiles < 0) {

			sErrorMessage2.Format("Failed scanning folder %s", (LPCTSTR)sInputFolder);
			g_util.LogprintfExport(sChannelName, "ERROR: Failed scanning folder %s", (LPCTSTR)sInputFolder);

			g_util.SendMail(MAILTYPE_FOLDERERROR, sErrorMessage2);

			m_ExportDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), sErrorMessage2, sErrorMessage);
		}

		if (nFiles <= 0) {
			::Sleep(500);

			// Because we have exclusive access to channel AND no jobs found - we can reset any 'EXPORTING' status (bad status)
			if (pChannel->m_mergePDF == FALSE && nFiles == 0) {
			//	g_util.LogprintfExport(sChannelName, "DEBUG: Testing for files in status 5..");
				int nHasStatus5 = m_ExportDB.HasExportingState(pChannel->m_channelID, sErrorMessage);
				if (nHasStatus5 == -1)
					g_util.LogprintfExport(sChannelName, "ERROR: db.HasExportingState() - %s", sErrorMessage);
				if (nHasStatus5 > 0) {
					g_util.LogprintfExport(sChannelName, "DEBUG: Found files in status 5..(channelID %d)", pChannel->m_channelID);
					if (m_ExportDB.ResetExportStatus(pChannel->m_channelID, sErrorMessage) == FALSE)
						g_util.LogprintfExport(sChannelName, "ERROR: db.ResetExportStatus() - %s", sErrorMessage);
				}
			}

			if (m_ExportDB.ReleaseExportLock(pChannel->m_channelID, sErrorMessage) == -1)
				g_util.LogprintfExport(sChannelName, "ERROR: db.ReleaseExportLock() - %s", sErrorMessage);

			//g_util.LogprintfExport(sChannelName, "Lock released!");
			continue;
		}

		if (g_prefs.m_logToFile2)
			g_util.LogprintfExport(sChannelName, "INFO: Found %d files (setup %s)..", nFiles, pChannel->m_channelname);
		CString sFullOutputPath;
		CString sFullInputPath;

		// START OF OUTPUT PART

		CString sInfo;
		sInfo.Format("%d file(s) found in input folder", nFiles);

		for (int it=0; it < nFiles; it++) {

			sInfo = _T("");

			FILEINFOSTRUCT fi = aFilesReady[it];
			fi.nMasterCopySeparationSet = 0;
			
			if (g_prefs.m_logToFile2)
				g_util.LogprintfExport(sChannelName, "INFO: Setup %s - Processing %d of %d files..", pChannel->m_channelname, it + 1, nFiles);

			if (fi.sFileName == _T(".") || fi.sFileName == _T(".."))
				continue;
		
			if (g_BreakSignal)
				break;
			
			sFullInputPath = sInputFolder + "\\" + fi.sFileName;
			
			//////////////////////////////////////
			// Perform usual file checks..
			//////////////////////////////////////

			if (!g_util.FileExist(sFullInputPath)) {
				if (g_prefs.m_logToFile2)
					g_util.LogprintfExport(sChannelName, "INFO: Secondary FileExist failed for file %s", fi.sFileName);
				continue;
			}

			DWORD dwInitialSize = fi.nFileSize;
			CTime tInitialWriteTime = fi.tWriteTime;
			// Still file growth??
			if (g_prefs.m_defaultstabletime > 0 && it == 0)
				::Sleep(g_prefs.m_defaultstabletime);

			if (g_prefs.m_defaultstabletime > 0) {
				if (g_util.GetFileSize(sFullInputPath) != dwInitialSize) {
					if (g_prefs.m_logToFile2)
						g_util.LogprintfExport(sChannelName, "INFO: Stable size check failed for file %s", fi.sFileName);
					continue;
				}
				if (g_util.GetWriteTime(sFullInputPath) != tInitialWriteTime) {
					if (g_prefs.m_logToFile2)
						g_util.LogprintfExport(sChannelName, "INFO: Stable time check failed for file %s", fi.sFileName);
					continue;
				}
			}

			if (!g_util.LockCheck(sFullInputPath)) {
				if (g_prefs.m_logToFile2)
					g_util.LogprintfExport(sChannelName, "INFO: Initial lock check failed for file %s", fi.sFileName);
				continue;
			}
		
			fi.sFileNameStripped = fi.sFileName;
			fi.sFolder = "";
			//@@xyz@@abcd#masterno.pdf
			//012345678901234567890123
			// m1 = 0
			// m2 = 5
			// m1 = 11
			// m2 = 20
			int m1 = fi.sFileName.Find("@@");
			if (m1 != -1) {
				int m2 = fi.sFileName.Find("@@", m1 + 2);
				if (m2 != -1) {
					fi.sFolder = fi.sFileName.Mid(m1 + 2, m2 - m1 - 2);

					fi.sFileNameStripped.Replace("@@" + fi.sFolder + "@@", "");
				}
			}

			m1 = fi.sFileName.ReverseFind('#');
			if (m1 != -1) {
				int m2 = fi.sFileName.ReverseFind('.');
				if (m2 != -1) {
					CString sMasterNumber = fi.sFileName.Mid(m1 + 1, m2 - m1 - 1);
					fi.nMasterCopySeparationSet = atoi(sMasterNumber);
					fi.sFileNameStripped.Replace("#" + sMasterNumber, "");
				}
			}

			fi.sFolder.Replace("@", "/");

			CString sFullErrorPath = pChannel->m_errorfolder + "\\" + fi.sFileNameStripped;
			CString sOutputFileTitle = fi.sFileNameStripped;
			sOutputFileTitle.Replace("?","");
			sOutputFileTitle.Replace("*","");

			if (fi.sFolder != "")
				sFullOutputPath = pChannel->m_outputfolder + _T("\\") + fi.sFolder + _T("\\") + sOutputFileTitle;
			else
				sFullOutputPath = pChannel->m_outputfolder + "\\" + sOutputFileTitle;

			//////////////////////////////////////
			// process this job
			//////////////////////////////////////
			
			if (g_prefs.m_logToFile2)
				g_util.LogprintfExport(sChannelName, "INFO: Stable time and size OK for file %s", fi.sFileName);

			if (pChannel->m_archivefiles && pChannel->m_archivefolder != "") {
				BOOL bDoCopy = TRUE;
				if (pChannel->m_archiveconditionally && pChannel->m_copymatchexpression != _T("")) {
					if (g_util.TryMatchExpression(pChannel->m_copymatchexpression, fi.sFileNameStripped, FALSE) == FALSE)
						bDoCopy = FALSE;
				}

				if (bDoCopy) {

					if (::CopyFile(sFullInputPath, pChannel->m_archivefolder + _T("\\") + fi.sFileNameStripped, FALSE) == FALSE) {
						::Sleep(1000);
						if (g_prefs.m_firecommandonfolderreconnect && g_prefs.m_sFolderReconnectCommand != "") {
							g_util.DoCreateProcess(g_prefs.m_sFolderReconnectCommand);
							::Sleep(500);
						}
						if (::CopyFile(sFullInputPath, pChannel->m_archivefolder + _T("\\") + fi.sFileNameStripped, FALSE) == FALSE) {
							g_util.Logprintf("ERROR: Unable to copy file %s to archive folder %s", sFullInputPath, pChannel->m_archivefolder);
						}
					}
				}
			}

			if (pChannel->m_archivefiles2 && pChannel->m_archivefolder2 != "") {
				if (::CopyFile(sFullInputPath, pChannel->m_archivefolder2 + _T("\\") + fi.sFileNameStripped, FALSE) == FALSE) {
					::Sleep(1000);
					if (g_prefs.m_firecommandonfolderreconnect && g_prefs.m_sFolderReconnectCommand != "") {
						g_util.DoCreateProcess(g_prefs.m_sFolderReconnectCommand);
						::Sleep(500);
					}
					if (::CopyFile(sFullInputPath, pChannel->m_archivefolder2 + _T("\\") + fi.sFileNameStripped, FALSE) == FALSE) {
						g_util.Logprintf("ERROR: Unable to copy file %s to archive folder %s", sFullInputPath, pChannel->m_archivefolder2);
					}
				}
			}
			int nCRC = 0;

			if (g_prefs.m_logToFile2)
				g_util.LogprintfExport(sChannelName, "INFO: Got file - ready for processing..  %s", fi.sFileName);

		/*	if (pChannel->m_zipoutputfile) {
				g_util.Logprintf("INFO: Zipping %s..",fi.sFileName);
				CString sZipName = sFullInputPath + _T(".zip");
				DeleteFile(sZipName);
				m_zip.Open(sZipName,CZipArchive::create);
				//m_zip.SetPassword("");
				if (m_zip.AddNewFile(sFullInputPath, 5, true)) {
					strcat(szInfo,"Zipped  ");
					DeleteFile(sFullInputPath);
					sFullInputPath = sZipName;
					fi.sFileName += _T(".zip");
					sOutputFileTitle += _T(".zip");
					sFullOutputPath += _T(".zip");
				}
				m_zip.Close();
			}
		*/

			CString sMovingText = _T("Moving file.. ");
			if (pChannel->m_outputuseftp == NETWORKTYPE_FTP)
				sMovingText = _T("Sending file.. ");
			
			BOOL bFileCopyOK = FALSE;
			DWORD bFileSizeSource = g_util.GetFileSize(sFullInputPath);

			DWORD dwCRCbefore = 0;
			if (pChannel->m_outputuseftp == NETWORKTYPE_FTP &&  pChannel->m_FTPpostcheck >= REMOTEFILECHECK_READBACK) {
				if (g_prefs.m_logToFile2)
					g_util.LogprintfExport(sChannelName, "INFO: Calculating CRC32 for file %s", fi.sFileName);
				dwCRCbefore = g_util.CRC32(sFullInputPath, TRUE);

			}

			//sOutputFileTitle = fi.sFileNameStripped;
						
			if (pChannel->m_useregexp) {
				CString sTranslatedName = _T("");

				for (int ex=0; ex<pChannel->m_NumberOfRegExps; ex++) {
					if (g_util.TryMatchExpression(pChannel->m_RegExpList[ex].m_matchExpression, sOutputFileTitle, pChannel->m_RegExpList[ex].m_partialmatch)) {
						sTranslatedName = g_util.FormatExpression(pChannel->m_RegExpList[ex].m_matchExpression, pChannel->m_RegExpList[ex].m_formatExpression, sOutputFileTitle, pChannel->m_RegExpList[ex].m_partialmatch);
						if (sTranslatedName != _T("")) {
							sOutputFileTitle = sTranslatedName;

							if (sOutputFileTitle.Find("%D1[") != -1 && sOutputFileTitle.Find("]")) {
								int n1 = sOutputFileTitle.Find("%D1[");
								int n2 = sOutputFileTitle.Find("]");

								CString sDateInFileToReplace = sOutputFileTitle.Mid(n1, n2 - n1 + 1);
								CString sDateInFile = sDateInFileToReplace;
								sDateInFile.Replace("%D1[", "");
								sDateInFile.Replace("]", "");
								CTime dt = g_util.ParseDate(sDateInFile, g_prefs.m_sDateFormat);
								dt = g_util.GetNextDay(dt);
								sDateInFile = g_util.Date2String(dt, g_prefs.m_sDateFormat, FALSE, 0);
								sOutputFileTitle.Replace(sDateInFileToReplace, sDateInFile);

							}
							else if (sOutputFileTitle.Find("%D") != -1) {
								CString sDateString = g_util.Date2String(CTime::GetCurrentTime(), g_prefs.m_sDateFormat, TRUE, 0);
								sOutputFileTitle.Replace("%D", sDateString);
							}

							g_util.LogprintfExport(sChannelName, "INFO: Translated %s to %s using regexp.", sOutputFileTitle, sTranslatedName);
							sInfo += _T("Renamed to: ") + sOutputFileTitle + _T(" using regular expression");
							break;	// Success!

						} else {
							g_util.LogprintfExport(sChannelName, "INFO: No regular expression matches for file name %s", sOutputFileTitle);
						}
					}
				}
			}
			
			sFullOutputPath = pChannel->m_outputfolder + "\\" + sOutputFileTitle;

			CString sSubFolder = fi.sFolder;
			// Create final subfolder (share version)
			if (sSubFolder != _T("") && pChannel->m_outputuseftp == NETWORKTYPE_SHARE) {
				
				// regex for subfolder..
				if (pChannel->m_useregexp) {
					for (int ex = 0; ex < pChannel->m_NumberOfRegExps; ex++) {
						if (pChannel->m_RegExpList[ex].m_matchExpression.Find("SUBFOLDER") != -1) {
							CString sMatch = pChannel->m_RegExpList[ex].m_matchExpression;
							sMatch.Replace("SUBFOLDER", "");
							if (g_util.TryMatchExpression(sMatch, sSubFolder, pChannel->m_RegExpList[ex].m_partialmatch)) {
								CString sTranslatedSubName = g_util.FormatExpression(sMatch, pChannel->m_RegExpList[ex].m_formatExpression, sSubFolder, pChannel->m_RegExpList[ex].m_partialmatch);
								if (sTranslatedSubName != _T(""))
									sSubFolder = sTranslatedSubName;
							}
						}
					}
				}

				sFullOutputPath = pChannel->m_outputfolder + _T("\\") + sSubFolder + _T("\\") + sOutputFileTitle;
				::CreateDirectory(g_util.GetFilePath(sFullOutputPath), NULL);
			}

			bFileCopyOK = FALSE;

			int nRetries = pChannel->m_retries;
			if (nRetries < 1)
				nRetries = 1;

			CTime tStart = CTime::GetCurrentTime();
			DWORD dwGUID = g_util.GenerateGUID(channelID, nTick);

			if (pChannel->m_outputuseftp == NETWORKTYPE_SHARE) {

				// Windows target folder 1

				bFileCopyOK = FALSE;
				g_util.LogTX("Sending", pChannel->m_channelname, fi.sFileName,"");
				
				CString sExtraInfo = _T("");

				while (--nRetries >= 0 && bFileCopyOK == FALSE) {

					CString sFinalOutputPath = sFullOutputPath;
					
					CString sTempOutputPath = sFullOutputPath + _T("-") + g_util.GenerateTimeStamp() + ".tmp";
					if (pChannel->m_filebuildupmethod == TEMPFILE_NONE) 
						sTempOutputPath = sFullOutputPath;
					
					if (pChannel->m_filebuildupmethod == TEMPFILE_FOLDER) {
						sTempOutputPath = sSubFolder != _T("") ? pChannel->m_outputfolder + _T("\\") + sSubFolder + _T("\\tmp\\") + sOutputFileTitle :
							pChannel->m_outputfolder + _T("\\tmp\\") + sOutputFileTitle;
						::CreateDirectory(g_util.GetFilePath(sTempOutputPath), NULL);
					}
					BOOL bExists = g_util.FileExist(sFinalOutputPath);

					if (bExists && pChannel->m_ensureuniquefilename) {
						CString sNewname = g_util.GetFileName(sOutputFileTitle, TRUE);
						int nX = 1;
						while (bExists) {
							sNewname.Format("%s-%d.%s", g_util.GetFileName(sOutputFileTitle, TRUE), nX, g_util.GetExtension(sOutputFileTitle));
							bExists = g_util.FileExist(pChannel->m_outputfolder + _T("\\") + sNewname);
							if (bExists == FALSE) {
								sOutputFileTitle = sNewname;
								sFinalOutputPath = pChannel->m_outputfolder + _T("\\") + sNewname;
								sFullOutputPath = sFinalOutputPath;

								sTempOutputPath = sFullOutputPath + _T("-") + g_util.GenerateTimeStamp() + _T(".tmp");
								if (pChannel->m_filebuildupmethod == TEMPFILE_NONE)
									sTempOutputPath = sFullOutputPath;

								if (pChannel->m_filebuildupmethod == TEMPFILE_FOLDER)
									sTempOutputPath = pChannel->m_outputfolder + _T("\\tmp\\") + sOutputFileTitle; 
								break;
							}
							nX++;
						}
						sInfo.Format("Unique filename generated %s ", (LPCTSTR)sNewname);
					}

					sExtraInfo = _T("");
					if (bExists && pChannel->m_nooverwrite) {
						bFileCopyOK = TRUE;
						sInfo.Format("File exists in output folder %s - no overwrite allowed", (LPCTSTR)pChannel->m_outputfolder);
					} else {

						if (g_prefs.m_logToFile2)
							g_util.LogprintfExport(sChannelName, "INFO: Starting copy of file %s -> %s", sFullInputPath, sTempOutputPath);
							
						bFileCopyOK = ::CopyFile(sFullInputPath, sTempOutputPath, FALSE);

						if (bFileCopyOK && g_prefs.m_logToFile2)
							g_util.LogprintfExport(sChannelName, "INFO: Copy completed for file %s -> %s", sFullInputPath, sTempOutputPath);

						if (bFileCopyOK && g_prefs.m_setlocalfiletime)
							g_util.SetFileToCurrentTime(sTempOutputPath);

						if (bFileCopyOK == FALSE) {
							sExtraInfo = g_util.GetLastWin32Error();
							g_util.LogprintfExport(sChannelName, "WARNING: System message: %s", sExtraInfo);
							g_util.LogprintfExport(sChannelName, "WARNING: Copy returned FALSE for file %s -> %s .. retrying", sFullInputPath, sTempOutputPath);
								
							::Sleep(1000);
							continue;
						}
						// Test filesize of copied file agaisnt original
						
						DWORD dwSourceSize = g_util.GetFileSize(sFullInputPath);
						DWORD dwDestSize = g_util.GetFileSize(sTempOutputPath);
						if (dwSourceSize > 0 && dwDestSize> 0 && dwSourceSize != dwDestSize) {
							g_util.LogprintfExport(sChannelName, "WARNING: File size mismatch between %s (%d bytes) and %s (%d bytes)", sFullInputPath, dwSourceSize, sTempOutputPath, dwDestSize);
							bFileCopyOK = FALSE;
							::Sleep(1000);
							continue;
						} 
						if (g_prefs.m_logToFile2)
							g_util.LogprintfExport(sChannelName, "INFO: Output remote size check OK for file %s -> %s", sFullInputPath, sTempOutputPath);


						// Rename temp file to final name
						if (sTempOutputPath != sFullOutputPath) {
							int nRetriesMove = pChannel->m_retries;
							if (nRetriesMove < 1)
								nRetriesMove = 1;

							BOOL bFileMoveOK = FALSE;

							while (--nRetriesMove >= 0 && bFileMoveOK == FALSE) {
								bFileMoveOK = ::MoveFileEx(sTempOutputPath, sFinalOutputPath, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
								if (bFileMoveOK == FALSE) {
									sExtraInfo = g_util.GetLastWin32Error();
									g_util.LogprintfExport(sChannelName, "WARNING: System message: %s", sExtraInfo);
									::Sleep(1000);
								}
							}

							if (bFileMoveOK == FALSE) 
								g_util.LogprintfExport(sChannelName, "ERROR: Unable to rename temp file %s to %s", sTempOutputPath, sFinalOutputPath);
								
							if (g_prefs.m_logToFile2)
								g_util.LogprintfExport(sChannelName, "INFO: Output move OK for file %s -> %s", sTempOutputPath, sFinalOutputPath);

							bFileCopyOK = bFileMoveOK;
						}
					}

				} // end whil retries..
					
				if (bFileCopyOK == FALSE) {
					
					g_util.LogTX("Error", pChannel->m_channelname, fi.sFileName, "File move error - " + sExtraInfo);
					if (pChannel->m_moveonerror)
						::MoveFileEx(sFullInputPath, sFullErrorPath, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);

					::DeleteFile(sFullInputPath);	

					sErrorMessage.Format("Error moving file %s to output folder %s", (LPCTSTR)fi.sFileName, (LPCTSTR)sFullOutputPath);
					g_util.SendMail(MAILTYPE_FILEERROR, sErrorMessage);
					g_util.LogprintfExport(sChannelName, "ERROR: Error copying file %s - all attempts failed", sFullInputPath);
				} 

				if (bFileCopyOK && pChannel->m_usescript) {
					CString sCommand = pChannel->m_script;
					sCommand.Replace("%f", "\""+sFullOutputPath+"\"");
					sCommand.Replace("%F", "\""+sFullOutputPath+"\"");
					g_util.DoCreateProcess(sCommand);
				}

			} else if (pChannel->m_outputuseftp == NETWORKTYPE_FTP) {

				// FTP retry loop

				CString sFinalOutputFileTitle;
				CString sTempOutputFileTitle;
				DWORD dwFileSizeAfter = 0;
				CString sErrorDetail = "";
				BOOL bUsingSubDir = FALSE;
				int nRetryNumber = 0;
				CString sTryNumber;

				CString sFTPSubFolder = fi.sFolder;

				// regex for subfolder..
				if (pChannel->m_useregexp) {
					for (int ex = 0; ex < pChannel->m_NumberOfRegExps; ex++) {
						if (pChannel->m_RegExpList[ex].m_matchExpression.Find("SUBFOLDER") != -1) {
							CString sMatch = pChannel->m_RegExpList[ex].m_matchExpression;
							sMatch.Replace("SUBFOLDER", "");
							if (g_util.TryMatchExpression(sMatch, sSubFolder, pChannel->m_RegExpList[ex].m_partialmatch)) {
								CString sTranslatedSubName = g_util.FormatExpression(sMatch, pChannel->m_RegExpList[ex].m_formatExpression, sSubFolder, pChannel->m_RegExpList[ex].m_partialmatch);
								if (sTranslatedSubName != _T(""))
									sFTPSubFolder = sTranslatedSubName;
							}
						}
					}
				}


				while (--nRetries >= 0 && bFileCopyOK == FALSE) {
					++nRetryNumber;
					if (nRetryNumber > 1)
						::Sleep(3000);
						
					sTryNumber.Format("Try number %d", nRetryNumber);
					g_util.LogTX("Queuing", pChannel->m_channelname, fi.sFileName, sTryNumber);

					g_util.LogprintfExport(sChannelName, "INFO: Retry number %d of %d for FTP transmission of %s",nRetryNumber, pChannel->m_retries, sOutputFileTitle);

					if (pChannel->m_outputfpttls != FTPTYPE_SFTP) {
						// non SFTP init stuff
						ftp_out.InitSession();
						if (pChannel->m_outputftpXCRC)
							ftp_out.SetAutoXCRC(TRUE);

						if (pChannel->m_outputfpttls == FTPTYPE_FTPES) {
							ftp_out.SetSSL(FALSE);
							ftp_out.SetAuthTLS(TRUE);
						}
						else if (pChannel->m_outputfpttls == FTPTYPE_FTPS) {
							ftp_out.SetSSL(TRUE);
							ftp_out.SetAuthTLS(FALSE);
						}
						// Validate that connection is still up..
						if (bIsConnected) {
							if (ftp_out.EnsureConnected(sErrorMessage) == FALSE) {
								bIsConnected = FALSE;
								g_util.LogprintfExport(sChannelName, "RESPONSE: FTP EnsureConnected() %s", sErrorMessage);
							}
						}
						if (bIsConnected == FALSE) {

							if (pChannel->m_outputftpbuffersize > 0) {
								g_util.LogprintfExport(sChannelName, "INFO: Setting BlockSize = %d..", pChannel->m_outputftpbuffersize);
								ftp_out.SetSendBufferSize(pChannel->m_outputftpbuffersize);
							}
							bIsConnected = ftp_out.OpenConnection(pChannel->m_outputftpserver, pChannel->m_outputftpuser,
								pChannel->m_outputftppw, pChannel->m_outputftppasv, pChannel->m_outputftpfolder, pChannel->m_outputftpport,
								pChannel->m_ftptimeout, sErrorMessage, sConnectFailReason);

							//util.Logprintf("FTP Reply - OpenConnection() %s", ftp_out.GetLastReply()); 
						}
					}
					else {

						// SFTP init stuff
						sftp_out.InitSession();
						// Validate that connection is still up..
						if (bIsConnected) {
							if (sftp_out.EnsureConnected(sErrorMessage) == FALSE) {
								bIsConnected = FALSE;
								g_util.LogprintfExport(sChannelName, "RESPONSE: SFTP EnsureConnected() %s", sErrorMessage);
							}
						}

						if (bIsConnected == FALSE) {

							bIsConnected = sftp_out.OpenConnection(pChannel->m_outputftpserver, pChannel->m_outputftpuser,
								pChannel->m_outputftppw, pChannel->m_outputftppasv, pChannel->m_outputftpfolder, pChannel->m_outputftpport,
								pChannel->m_ftptimeout, sErrorMessage, sConnectFailReason);

							//util.Logprintf("FTP Reply - OpenConnection() %s", ftp_out.GetLastReply()); 
						}
					}
					
					if (bIsConnected == FALSE) {
						CString sFolder;
						sErrorDetail = "No connection";
						sFolder.Format(_T("ftp://%s:%d/%s"),pChannel->m_outputftpserver,pChannel->m_outputftpport,pChannel->m_outputftpfolder);
						if (pChannel->m_outputfpttls == FTPTYPE_SFTP)
							sFolder = _T("s") + sFolder;
					
						g_util.LogprintfExport(sChannelName, "ERROR: FTP Connection error");
						g_util.LogprintfExport(sChannelName, "ERROR: FTP Connection reason: %s", sConnectFailReason);
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
							g_util.LogprintfExport(sChannelName, "ERROR: FTP Connection reply: %s", g_util.LimitErrorMessage(ftp_out.GetLastReply()));
						else
							g_util.LogprintfExport(sChannelName, "ERROR: SFTP Connection reply: %s", g_util.LimitErrorMessage(sftp_out.GetLastReply()));
						g_util.LogprintfExport(sChannelName, "ERROR: FTP Connection error: %s", sErrorMessage);
						if (sConnectFailReason != "") {
							s = "Error connecting to FTP server - " + sConnectFailReason;
							sErrorDetail = "No connection " + sConnectFailReason;
						}

						sInfo =  s;

						bReConnect = TRUE;
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
							ftp_out.Disconnect();
						else
							sftp_out.Disconnect();
						continue; // retry
					}		

					BOOL bExists;
					BOOL bChDirOK;

					if (pChannel->m_outputftpfolder != "")
					{
						CString sCurrentDir = _T("");
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP) {
							if (ftp_out.GetCurrentDirectory(sCurrentDir, sErrorMessage) == FALSE)
								g_util.LogprintfExport(sChannelName, "ERROR: GetCurrentDirectory() - %s",sErrorMessage);
							if (sCurrentDir.CompareNoCase(pChannel->m_outputftpfolder) != 0)
								if (ftp_out.ChangeDirectory(pChannel->m_outputftpfolder, sErrorMessage) == FALSE)
									g_util.LogprintfExport(sChannelName, "ERROR: ChangeDirectory(%s) - %s", pChannel->m_outputftpfolder,sErrorMessage);
						}
						else {
							if (sftp_out.GetCurrentDirectory(sCurrentDir, sErrorMessage) == FALSE)
								g_util.LogprintfExport(sChannelName, "ERROR: GetCurrentDirectory() %s ",sErrorMessage);
							if (sCurrentDir.CompareNoCase(pChannel->m_outputftpfolder) != 0)
								if (sftp_out.ChangeDirectory(pChannel->m_outputftpfolder, sErrorMessage) == FALSE)
									g_util.LogprintfExport(sChannelName, "ERROR: ChangeDirectory(%s) - %s", pChannel->m_outputftpfolder, sErrorMessage);
						}
					}
					

					if (sFTPSubFolder != "") {
						
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP) 
							bExists = ftp_out.DirectoryExists(sFTPSubFolder, sErrorMessage);
						else
							bExists = sftp_out.DirectoryExists(sFTPSubFolder, sErrorMessage);
						if (bExists == FALSE) {
							
							if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
								bChDirOK = ftp_out.CreateDirectory(sFTPSubFolder, sErrorMessage);
							else
								bChDirOK = sftp_out.CreateDirectory(sFTPSubFolder, sErrorMessage);
							if (bChDirOK == FALSE) {

								CString sFolder;
								sErrorDetail = "FTP CreateDirectory failed";
								sFolder.Format(_T("ftp://%s:%d/%s/%s"), pChannel->m_outputftpserver, pChannel->m_outputftpport, pChannel->m_outputftpfolder, sFTPSubFolder);
								if (pChannel->m_outputfpttls == FTPTYPE_SFTP)
									sFolder = _T("s") + sFolder;

								g_util.LogprintfExport(sChannelName, "ERROR: FTP CreateDirectory(%s) failed - %s", sFTPSubFolder, sErrorMessage);

								if (sErrorMessage != "") {
									s = "Error connecting to FTP server - " + sErrorMessage;
									sErrorDetail = "No connection " + sErrorMessage;
								}

								sInfo = s;

								bReConnect = TRUE;
								if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
									ftp_out.Disconnect();
								else
									sftp_out.Disconnect();
								continue; // retry
							}
						}

					/*	if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
							bChDirOK = ftp_out.ChangeDirectory(sFTPSubFolder, sErrorMessage);
						else
							bChDirOK = sftp_out.ChangeDirectory(sFTPSubFolder, sErrorMessage);
				
						if (bChDirOK == FALSE) {
							CString sFolder;
							sErrorDetail = "FTP ChangeDirectory failed";
							if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
								sFolder.Format(_T("ftp://%s:%d/%s/%s"), pChannel->m_outputftpserver, pChannel->m_outputftpport, pChannel->m_outputftpfolder, sFTPSubFolder);
							else
								sFolder.Format(_T("sftp://%s:%d/%s/%s"), pChannel->m_outputftpserver, pChannel->m_outputftpport, pChannel->m_outputftpfolder, sFTPSubFolder);

							g_util.LogprintfExport(sChannelName,"ERROR: FTP ChangeDirectory(%s) failed", sFTPSubFolder);


							if (sErrorMessage != "") {
								s = "Error connecting to FTP server - " + sErrorMessage;
								sErrorDetail = "No connection " + sErrorMessage;
							}

							sInfo = s;

							bReConnect = TRUE;
							if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
								ftp_out.Disconnect();
							else
								sftp_out.Disconnect();
							continue; // retry
						} */

					}

					sFinalOutputFileTitle =  sOutputFileTitle;
					
					if (sFTPSubFolder != "") {

						sFinalOutputFileTitle = "/" + ftp_out.TrimSlashes(pChannel->m_outputftpfolder) + "/" + ftp_out.TrimSlashes(sFTPSubFolder + "/" + sOutputFileTitle);
						sTempOutputFileTitle = "/" + ftp_out.TrimSlashes(pChannel->m_outputftpfolder) + "/" + ftp_out.TrimSlashes(sFTPSubFolder + "/" + sFinalOutputFileTitle);
					} 

					
					sTempOutputFileTitle = sFinalOutputFileTitle;
										
					if (pChannel->m_outputftpusetempfolder == TEMPFILE_NAME)
						sTempOutputFileTitle += _T("-") + g_util.GenerateTimeStamp() + _T(".tmp");
					/*else if (pChannel->m_outputftpusetempfolder == TEMPFILE_FOLDER) {
						if (pChannel->m_outputftptempfolder != "") {
							sTempOutputFileTitle = pChannel->m_outputftptempfolder + _T("/") + sOutputFileTitle;
						}
					}*/

					if (pChannel->m_nooverwrite == FALSE) {
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP) {
							ftp_out.DeleteFile(sTempOutputFileTitle, FALSE, sErrorMessage);						
							ftp_out.DeleteFile(sFinalOutputFileTitle, FALSE, sErrorMessage);
						}
						else {
							sftp_out.DeleteFile(sTempOutputFileTitle, FALSE, sErrorMessage);
							sftp_out.DeleteFile(sFinalOutputFileTitle, FALSE, sErrorMessage);
						}
					} else {
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP) 
							bExists = ftp_out.FileExists(sFinalOutputFileTitle, sErrorMessage);
						else
							bExists = sftp_out.FileExists(sFinalOutputFileTitle, sErrorMessage);
						if (bExists) {
							g_util.LogTX("Warning", pChannel->m_channelname, fi.sFileName, "Existing file on FTP server");
							bFileCopyOK = TRUE;
							::DeleteFile(sFullInputPath);

							if (pChannel->m_keepoutputconnection == FALSE) {
								if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
									ftp_out.Disconnect();
								else
									sftp_out.Disconnect();
							}								
							
							continue;
						}
						//util.Logprintf("RESPONSE: FileExists() %s", ftp_out.GetLastReply());
					}

					g_util.LogTX("Connected", pChannel->m_channelname, fi.sFileName, sTryNumber);
					
					g_util.LogTX("Sending", pChannel->m_channelname, fi.sFileName, sTryNumber);
			
					if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
						bFileCopyOK = ftp_out.PutFile(sFullInputPath, sTempOutputFileTitle, FALSE, sErrorMessage);
					else
						bFileCopyOK = sftp_out.PutFile(sFullInputPath, sTempOutputFileTitle, FALSE, sErrorMessage);
					g_util.LogprintfExport(sChannelName, "RESPONSE: PutFile(%s,%s) %s", sFullInputPath, sTempOutputFileTitle, ftp_out.GetLastReply());
						
					if (bFileCopyOK == FALSE) {
						s = "Error uploading file to FTP " + sErrorMessage;
							
						sInfo = s;
						bFileCopyOK = FALSE;
						::Sleep(2000);
						sErrorDetail.Format("FTP Send command failed - %s", sErrorMessage);

						// Force close 
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
							ftp_out.Disconnect();
						else
							sftp_out.Disconnect();
						continue; // retry

					}


					// File uploaded with TMP extension..
					if (sTempOutputFileTitle != sFinalOutputFileTitle) {
						BOOL bRenameOK;
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
							bRenameOK = ftp_out.RenameFile(sTempOutputFileTitle, sFinalOutputFileTitle, sErrorMessage);
						else
							bRenameOK = sftp_out.RenameFile(sTempOutputFileTitle, sFinalOutputFileTitle, sErrorMessage);
						if (bRenameOK == FALSE) {

							s = "Error renameing file on FTP " + sErrorMessage;
							sInfo = sErrorMessage;
							bFileCopyOK = FALSE;
							::Sleep(2000);
							sErrorDetail = "FTP Rename command failed";

							// Force close 
							if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
								ftp_out.Disconnect();
							else
								sftp_out.Disconnect();
							continue; // retry
						}
						//util.Logprintf("RESPONSE: FTPRenameFile() %s", ftp_out.GetLastReply());
					}

					
					int nFileSizeAfter = 0;
					if (pChannel->m_FTPpostcheck >= REMOTEFILECHECK_SIZE) {

						if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
							nFileSizeAfter = ftp_out.GetFileSize(sFinalOutputFileTitle, sErrorMessage);
						else
							nFileSizeAfter = sftp_out.GetFileSize(sFinalOutputFileTitle, sErrorMessage);
						if (nFileSizeAfter == -1)
							g_util.Logprintf("ERROR: FTP GetFileSize(%s) - %s", sFinalOutputFileTitle, sErrorMessage);
					}

					dwFileSizeAfter = nFileSizeAfter >= 0 ? nFileSizeAfter : 0;
					
					// File not timed out - perform remote test on file

					if (bFileCopyOK && pChannel->m_FTPpostcheck >= REMOTEFILECHECK_EXIST) {

						if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
							bFileCopyOK = ftp_out.FileExists(sFinalOutputFileTitle, sErrorMessage);
						else
							bFileCopyOK = sftp_out.FileExists(sFinalOutputFileTitle, sErrorMessage);

						if (bFileCopyOK == FALSE) {
							sErrorDetail = "File exists check failed";
							g_util.LogprintfExport(sChannelName, "ERROR: FTP Postcheck FileExists() returned %s", sErrorMessage);
							if (sErrorMessage != "")
								sErrorDetail += " " + sErrorMessage;

							// Force close 
							if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
								ftp_out.Disconnect();
							else
								sftp_out.Disconnect();
							continue; // retry..
						} else
							g_util.LogprintfExport(sChannelName, "INFO: FTP Postcheck 1 OK");

						if (bFileCopyOK && pChannel->m_FTPpostcheck >= REMOTEFILECHECK_SIZE && dwFileSizeAfter > 0) {
							if (dwFileSizeAfter != bFileSizeSource) {
								bFileCopyOK = FALSE;
								g_util.LogprintfExport(sChannelName, "ERROR: FTP File size check mismatch: Source %d, Destination %d", bFileSizeSource, dwFileSizeAfter);
								sErrorDetail = "File size check failed";
								// Force close 
								if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
									ftp_out.Disconnect();
								else
									sftp_out.Disconnect();
								continue; // retry..

							} else
								g_util.LogprintfExport(sChannelName, "INFO: FTP Postcheck 2 OK");
						}

						if (bFileCopyOK && pChannel->m_FTPpostcheck == REMOTEFILECHECK_READBACK) {

							// Retrieve file and compare against transmitted file
							g_util.LogTX("Readback test", pChannel->m_channelname, fi.sFileName, "");
								
							CString sCopyBackFile = g_prefs.m_workfolder2 + _T("\\") + sOutputFileTitle;
							::DeleteFile(sCopyBackFile);
							g_util.LogprintfExport(sChannelName, "INFO: Initialing FTP Postcheck 3.. (FTPReceiveFile)");
							if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
								bFileCopyOK = ftp_out.GetFile(sFinalOutputFileTitle, sCopyBackFile, FALSE, sErrorMessage);
							else
								bFileCopyOK = sftp_out.GetFile(sFinalOutputFileTitle, sCopyBackFile, FALSE, sErrorMessage);
							if (bFileCopyOK == FALSE) {
								g_util.LogprintfExport(sChannelName, "ERROR: Readback test GetFile() - %s", sErrorMessage);
								if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
									ftp_out.Disconnect();
								else
									sftp_out.Disconnect();
								continue; // retry..
							}
						
							g_util.LogprintfExport(sChannelName, "INFO: FTP Postcheck 3 finalized");
							DWORD dwCRCafter = 0;
							if (bFileCopyOK) {
								dwCRCafter = g_util.CRC32(sCopyBackFile, TRUE);
								::DeleteFile(sCopyBackFile);

								if (dwCRCbefore > 0 && dwCRCafter > 0) {
									if (dwCRCafter == dwCRCbefore)
										g_util.LogprintfExport(sChannelName, "INFO: FTP Postcheck 3 OK");
									else {
										g_util.LogprintfExport(sChannelName, "ERROR: FTP Read back check mismatch - CRC before:%d CRC after: %d",dwCRCbefore,dwCRCafter);
										sErrorDetail = "File readback check failed";
										bFileCopyOK = FALSE;
										// Force close 
										if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
											ftp_out.Disconnect();
										else
											sftp_out.Disconnect();
										continue; // retry..
									}
								}
							}
								
						}
					}

					if (sFTPSubFolder != _T("")) {
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
							bChDirOK = ftp_out.ChangeDirectory(_T(".."), sErrorMessage);
						else
							bChDirOK = sftp_out.ChangeDirectory(_T(".."), sErrorMessage);
						if (bChDirOK == FALSE) {

							CString sFolder;
							sErrorDetail = "FTP ChangeDirectory .. failed";
							if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
								sFolder.Format(_T("ftp://%s:%d/%s/%s"), pChannel->m_outputftpserver, pChannel->m_outputftpport, pChannel->m_outputftpfolder, sFTPSubFolder);
							else
								sFolder.Format(_T("sftp://%s:%d/%s/%s"), pChannel->m_outputftpserver, pChannel->m_outputftpport, pChannel->m_outputftpfolder, sFTPSubFolder);

							g_util.LogprintfExport(sChannelName, "ERROR: FTP ChangeDirectory(..) failed");

							if (sErrorMessage != "") {
								s = "Error changing FTP directory - " + sErrorMessage;
								sErrorDetail = "No connection " + sErrorMessage;
							}

							g_util.LogprintfExport(sChannelName, "%s", s);
							sInfo =  s;

							bReConnect = TRUE;
							if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
								ftp_out.Disconnect();
							else
								sftp_out.Disconnect();
							continue; // retry
						}
					}

				} //end while retries (per file)
				
				// All retries failed..
				if (bFileCopyOK == FALSE) {
					s = _T("Error in FTP transfer - all retries failed - ") +  sErrorDetail;
					g_util.LogTX("Error", pChannel->m_channelname, fi.sFileName, s);
					g_util.LogprintfExport(sChannelName, "Unable to move ftp file %s - %s",fi.sFileName,sErrorDetail);
					sInfo.Format("%s", (LPCTSTR)s);

					if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
						ftp_out.Disconnect();
					else
						sftp_out.Disconnect();
				} 

				// File transferred OK!

				if (bFileCopyOK) {
					CTime tEnd = CTime::GetCurrentTime();
					CTimeSpan ts = tEnd - tStart;
					double f = (double)ts.GetTotalSeconds();
					if (f != 0) {
						f = (bFileSizeSource/1024.0) / f;
						sInfo.Format( "%d sec %.1f KB/sec", (int)ts.GetTotalSeconds(), f);
					}
				}
					
				if (pChannel->m_keepoutputconnection == FALSE) {
					if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
						ftp_out.Disconnect();
					else
						sftp_out.Disconnect();
					g_util.Logprintf("INFO:  FTP Disconnected");
				}

				if (bFileCopyOK && pChannel->m_usescript) {
					CString sCommand = pChannel->m_script;
					sCommand.Replace("%f", "\""+fi.sFileName+"\"");
					sCommand.Replace("%F", "\""+fi.sFileName+"\"");
					g_util.DoCreateProcess(sCommand);
				}

			} else if (pChannel->m_outputuseftp == NETWORKTYPE_EMAIL) {

				email_out.m_SMTPuseSSL = pChannel->m_emailusessl;
				email_out.m_SMTPuseStartTLS = pChannel->m_emailusestartTLS;

				CString sSubject = pChannel->m_emailsubject;
				sSubject.Replace("%F", fi.sFileName);
				sSubject.Replace("%T", g_util.GenerateTimeStamp());
				sSubject.Replace("%f", fi.sFileName);
				sSubject.Replace("%t", g_util.GenerateTimeStamp());

				CString sBody = pChannel->m_emailbody;
				sBody.Replace("%F", fi.sFileName);
				sBody.Replace("%T", g_util.GenerateTimeStamp());
				sBody.Replace("%f", fi.sFileName);
				sBody.Replace("%t", g_util.GenerateTimeStamp());
				if (pChannel->m_emailtimeout > 0)
					email_out.SetSMTPConnectoinTimeout(pChannel->m_emailtimeout);

				CString sAttachmentFileName = sFullInputPath;
				m1 = sFullInputPath.ReverseFind('#');
				if (m1 != -1) {
					int m2 = sFullInputPath.ReverseFind('.');
					if (m2 != -1) {
						CString sMasterNumber = fi.sFileName.Mid(m1 + 1, m2 - m1 - 1);
						sAttachmentFileName.Replace("#" + sMasterNumber, "");
					}
				}
				::CopyFile(sFullInputPath, sAttachmentFileName, FALSE);
				BOOL bEmailRet = email_out.SendMailAttachment(  pChannel->m_emailsmtpserver, pChannel->m_emailsmtpport,
																pChannel->m_emailsmtpserverusername, pChannel->m_emailsmtpserverpassword,
																pChannel->m_emailfrom,
																pChannel->m_emailto, pChannel->m_emailcc, sSubject,
																sBody, pChannel->m_emailusehtml, sAttachmentFileName);
				if (bEmailRet == FALSE)
					g_util.LogprintfExport(sChannelName, "ERROR: Unable to send email attachment %s - %s", sAttachmentFileName, email_out.GetLastError());
				::DeleteFile(sAttachmentFileName);

				bFileCopyOK = bEmailRet;
			}

			//////////////////////////////////////
			// Finish off
			//////////////////////////////////////

			/*if (pChannel->m_useack && pChannel->m_statusfolder != _T("")) {
				CString sAckFileName = pChannel->m_statusfolder + _T("\\") + fi.sFileName + _T(".ack");
				if (g_prefs.m_addsetupnametoackfile)
					sAckFileName = pChannel->m_statusfolder + _T("\\") + fi.sFileName + _T("#") + pChannel->m_channelname + _T(".ack");
				if (bFileCopyOK == FALSE) {
					sAckFileName = pChannel->m_statusfolder + _T("\\") + fi.sFileName + _T(".nack");
					if (g_prefs.m_addsetupnametoackfile)
						sAckFileName = pChannel->m_statusfolder + _T("\\") + fi.sFileName + _T("#") + pChannel->m_channelname + _T(".nack");

				}

				g_util.SaveFile(sAckFileName, pChannel->m_channelname +_T("\r\n"));
			}*/

			if (bFileCopyOK) {

		 		if (pChannel->m_inputemailnotification && pChannel->m_inputemailnotificationTo != _T("")) {
					CString sSubject = pChannel->m_inputemailnotificationSubject;
					sSubject.Replace("%f", fi.sFileName);
					sSubject.Replace("%F", fi.sFileName);
					sSubject.Replace("%t", g_util.CurrentTime2String());
					sSubject.Replace("%T", g_util.CurrentTime2String());

					CString sBody = pChannel->m_inputemailnotificationSubject;
					sBody.Replace("%f", fi.sFileName);
					sBody.Replace("%F", fi.sFileName);
					sBody.Replace("%t", g_util.CurrentTime2String());
					sBody.Replace("%T", g_util.CurrentTime2String());
					sBody += "\r\n";
					CString sCC = _T("");
					CString sTo = pChannel->m_inputemailnotificationTo;
					int nx = sTo.Find(",");
					if (nx != -1) {
						sCC = pChannel->m_inputemailnotificationTo.Mid(nx + 1).Trim();
						sTo = pChannel->m_inputemailnotificationTo.Left(nx);
					}

					CEmailClient email;

					email.m_SMTPuseSSL = g_prefs.m_mailusessl;
					email.m_SMTPuseStartTLS = g_prefs.m_mailusestarttls;
					email.SendMail(g_prefs.m_emailsmtpserver, g_prefs.m_mailsmtpport, g_prefs.m_mailsmtpserverusername, g_prefs.m_mailsmtpserverpassword,g_prefs.m_emailfrom, sTo, sCC, sSubject, sBody, false);
				}

				if (g_prefs.m_generateJMF && g_prefs.m_jmferroronly == FALSE)
					WriteXmlLogFile(fi.sFileName, JMFSTATUS_OK, pChannel->m_channelname, "", CTime::GetCurrentTime(), nCRC, pChannel->m_channelname);
				m_ExportDB.InsertLogEntry(EVENTCODE_EXPORTED, pChannel->m_channelname, fi.sFileName, sInfo, fi.nMasterCopySeparationSet, 0, 0, "", sErrorMessage);

				if (fi.sFileName != sOutputFileTitle)
					sInfo.Format("Final name: %s", (LPCTSTR)sOutputFileTitle);

				// Update all pages in merged doc to DONE
				if (pChannel->m_mergePDF) {
					CUIntArray aMasterCopySeparationSetList;
					int nProductionID = 0;
					int nEditionID = 0;
					m_ExportDB.GetProductionID(fi.nMasterCopySeparationSet, nProductionID, nEditionID, sErrorMessage);
					if (nProductionID > 0) {
						if (m_ExportDB.GetMasterCopySeparationListPDF(TRUE, pChannel->m_pdftype, nProductionID, nEditionID, aMasterCopySeparationSetList, sErrorMessage) == FALSE) {
							g_util.LogprintfTransmit("ERROR: GetMasterCopySeparationListPDF(ProductionID %d,EditionID %d)  failed - %s", nProductionID, nEditionID, sErrorMessage);
						}
						else {
							for (int pg = 0; pg < aMasterCopySeparationSetList.GetCount(); pg++) {							
								m_ExportDB.UpdateChannelStatus(2, aMasterCopySeparationSetList[pg], 0, pChannel->m_channelID, EXPORTSTATUSNUMBER_EXPORTED, sInfo, fi.sFileNameStripped, sErrorMessage);
							}
						}
					}
				}
				else
					m_ExportDB.UpdateChannelStatus(2, fi.nMasterCopySeparationSet, 0, pChannel->m_channelID, EXPORTSTATUSNUMBER_EXPORTED, sInfo, fi.sFileNameStripped, sErrorMessage);

				g_util.LogTX("Done", pChannel->m_channelname, fi.sFileName, sInfo);

				// Status message generation

				if (g_prefs.m_generatestatusmessage && g_prefs.m_messageonsuccess) {
					int nTypeNumber = 0;
					if (pChannel->m_pdftype == POLLINPUTTYPE_PRINTPDF) 
						nTypeNumber = 1010;
					
					else if (pChannel->m_pdftype == POLLINPUTTYPE_HIRESPDF) 
						nTypeNumber = 1011;
					else if (pChannel->m_pdftype == POLLINPUTTYPE_LOWRESPDF && pChannel->m_channelnamealias == "VI") // VisioLink only!! {
						nTypeNumber = 1012;

					if (nTypeNumber > 0) {
						CString sOrgFileName = _T("");
						m_ExportDB.GetOriginalFileName(fi.nMasterCopySeparationSet, sOrgFileName, sErrorMessage);
						if (sOrgFileName != _T("")) {
							int nn = sOrgFileName.Find("#");
							if (nn != -1)
								sOrgFileName = sOrgFileName.Left(nn) + g_util.GetExtension(sOrgFileName);
							g_util.IssueMessage(sOrgFileName, EXPORTSTATUSNUMBER_EXPORTED, g_prefs.m_messageoutputfolder, nTypeNumber, sErrorMessage);
						}
					}
				} 


			} else {
				if (pChannel->m_moveonerror)
					::MoveFileEx(sFullInputPath, sFullErrorPath, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);

				if (g_prefs.m_generateJMF)
					WriteXmlLogFile(fi.sFileName, JMFSTATUS_ERROR, pChannel->m_channelname, sInfo, CTime::GetCurrentTime(), nCRC, pChannel->m_channelname);
						
				// Update all pages in merged doc to ERROR
				if (pChannel->m_mergePDF) {
					CUIntArray aMasterCopySeparationSetList;
					int nProductionID = 0;
					int nEditionID = 0;
					m_ExportDB.GetProductionID(fi.nMasterCopySeparationSet, nProductionID, nEditionID, sErrorMessage);
					if (nProductionID > 0) {
						if (m_ExportDB.GetMasterCopySeparationListPDF(TRUE, pChannel->m_pdftype, nProductionID, nEditionID, aMasterCopySeparationSetList, sErrorMessage) == FALSE) {
							g_util.LogprintfTransmit("ERROR: GetMasterCopySeparationListPDF(ProductionID %d,EditionID %d)  failed - %s", nProductionID, nEditionID, sErrorMessage);
						}
						else {
							for (int pg = 0; pg < aMasterCopySeparationSetList.GetCount(); pg++) {
								m_ExportDB.UpdateChannelStatus(2,aMasterCopySeparationSetList[pg], 0, pChannel->m_channelID, EXPORTSTATUSNUMBER_EXPORTERROR, sInfo, fi.sFileNameStripped, sErrorMessage);
							}
						}

					}
				}
				else
					m_ExportDB.UpdateChannelStatus(2,fi.nMasterCopySeparationSet, 0, pChannel->m_channelID, EXPORTSTATUSNUMBER_EXPORTERROR, sInfo, fi.sFileNameStripped, sErrorMessage);
				
				m_ExportDB.InsertLogEntry(EVENTCODE_EXPORTERROR, pChannel->m_channelname, fi.sFileName, sInfo, fi.nMasterCopySeparationSet, 0, 0, "", sErrorMessage);

				g_util.LogTX("Error", pChannel->m_channelname, fi.sFileName, sInfo);

				if (g_prefs.m_generatestatusmessage && g_prefs.m_messageonerror) {
					int nTypeNumber = 0;
					if (pChannel->m_pdftype == POLLINPUTTYPE_PRINTPDF)
						nTypeNumber = 1014;
					else if (pChannel->m_pdftype == POLLINPUTTYPE_HIRESPDF)
						nTypeNumber = 1015;
					else if (pChannel->m_pdftype == POLLINPUTTYPE_LOWRESPDF && pChannel->m_channelnamealias == "VI") // VisioLink only!! 
						nTypeNumber = 1013;

					if (nTypeNumber > 0) {
						CString sOrgFileName = _T("");
						m_ExportDB.GetOriginalFileName(fi.nMasterCopySeparationSet, sOrgFileName, sErrorMessage);
						if (sOrgFileName != _T("")) {
							int nn = sOrgFileName.Find("#");
							if (nn != -1)
								sOrgFileName = sOrgFileName.Left(nn) + g_util.GetExtension(sOrgFileName);
							g_util.IssueMessage(sOrgFileName, EXPORTSTATUSNUMBER_EXPORTERROR, g_prefs.m_messageoutputfolder, nTypeNumber, sErrorMessage);
						}
					}
				}
			}

			::DeleteFile(sFullInputPath);
			
		} // for nFiles...

		g_util.LogprintfExport(sChannelName, "Releasing lock..");
		if (m_ExportDB.ReleaseExportLock(pChannel->m_channelID, sErrorMessage) == -1)
			g_util.LogprintfExport(sChannelName, "ERROR: db.ReleaseExportLock() - %s", sErrorMessage);
		g_util.LogprintfExport(sChannelName, "Lock released!");
		

	} while (g_BreakSignal == FALSE);

	ftp_out.Disconnect();
	sftp_out.Disconnect();
	if (pChannel != NULL)
		m_ExportDB.ReleaseExportLock(pChannel->m_channelID, sErrorMessage);
	
	m_ExportDB.ExitDB();

	::Sleep(500);
	g_exportthreadrunning = FALSE;

	return TRUE;
}

