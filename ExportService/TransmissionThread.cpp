#include "stdafx.h"
#include <afxmt.h>
#include <afxtempl.h>
#include <direct.h>
#include "Defs.h"
#include "DatabaseManager.h"
#include "ExportThread.h"
#include "TransmissionThread.h"
#include "Utils.h"
#include "Prefs.h"
#include "PDFutils.h"

extern CPrefs g_prefs;
extern CUtils g_util;
extern BOOL g_BreakSignal;
extern BOOL g_transmitthreadrunning;

NextTransmitJob TxJob[MAXTRANSMITQUEUE];

UINT TransmissionThread(LPVOID param)
{
	CDatabaseManager m_TransmitDB;
	CString sMsg, sErrorMessage = _T("");
	BOOL bExcludeMergePDF = FALSE;

	// name caches already loaded!!
	BOOL m_connected = m_TransmitDB.InitDB(g_prefs.m_DBserver, g_prefs.m_Database, g_prefs.m_DBuser, g_prefs.m_DBpassword,
			g_prefs.m_IntegratedSecurity, sErrorMessage);

	if (m_connected) {
		if (m_TransmitDB.RegisterService(sErrorMessage) == FALSE) {
			g_util.LogprintfTransmit("ERROR: RegisterService() returned - %s", sErrorMessage);
		}
	} else
		g_util.LogprintfTransmit("ERROR: InitDB() returned - %s", sErrorMessage);

	BOOL bReconnect = FALSE;

	int nWaitingJobs = 0;
	BOOL bReRunQueryAfterSuccess = TRUE;
	BOOL bUseBackup = FALSE;
	g_util.LogprintfTransmit("Initialized transmit thread.");
	int nTick = 0;

	while (g_BreakSignal == FALSE) {
		BOOL bHasWaited = FALSE;
		nTick++;

		// Only look for merge jobs every fourth time... (low priority).

		bExcludeMergePDF = (nTick % 8) > 0;
		
		if (bReconnect) {
			m_TransmitDB.ExitDB();
			for (int i = 0; i < 20; i++) {
				Sleep(100);
				if (g_BreakSignal)
					break;
			}
			if (g_BreakSignal)
				break;

			bReconnect = FALSE;
			BOOL m_connected = m_TransmitDB.InitDB(g_prefs.m_DBserver, g_prefs.m_Database, g_prefs.m_DBuser, g_prefs.m_DBpassword,
				g_prefs.m_IntegratedSecurity, sErrorMessage);

			if (m_connected == FALSE) {
				g_util.LogprintfTransmit("ERROR: Reconnect InitDB() returned - %s", sErrorMessage);

				bReconnect = TRUE;
				::Sleep(1000);
				continue;
			}
		}


		g_transmitthreadrunning = TRUE;
		if (nTick % 10 == 0)
			m_TransmitDB.UpdateService(SERVICESTATUS_RUNNING, _T(""), _T(""), _T(""), sErrorMessage);

		if (g_prefs.m_bypassdiskfreeteste == FALSE) {
			//if (g_prefs.m_logToFile2)
			//	g_util.LogprintfTransmit("INFO:  Scanning for free disk..");
			int nFreeMB = g_util.FreeDiskSpaceMB(g_prefs.m_workfolder);
			if (nFreeMB > 0 && nFreeMB < 100) {
				g_util.LogprintfTransmit("WARNING: work folder disk too low on free space");
				::Sleep(1000);
				continue;
			}
		}

		if (nWaitingJobs == 0) {
			for (int i = 0; i < 10; i++) {
				::Sleep(100);
				if (g_BreakSignal)
					break;
			}
			
			if (g_BreakSignal)
				break;
		}
		else
			::Sleep(100);

		if (g_BreakSignal)
			break;
		g_util.AliveTX();

		if (m_TransmitDB.AcuireTransmitLock(sErrorMessage) == 0) {
			::Sleep(1000);
			// Did not get exclusive lock on - try again later
			continue;
		}

		if (g_prefs.m_logToFile > 2)
			g_util.LogprintfTransmit("INFO: Got transmission lock");
		::Sleep(500);
		
		nWaitingJobs = m_TransmitDB.LookupTransmitJob(TRUE, FALSE, TRUE, bExcludeMergePDF, sErrorMessage);
		if (nWaitingJobs < 0) {
			g_util.LogprintfTransmit("ERROR: DB error looking for jobs - %s", sErrorMessage);
			bReconnect = TRUE;
			//			g_util.SendMail(MAILTYPE_DBERROR, IDS_DB_TESTERROR);

			CString sErrorMessage2 = sErrorMessage;
			m_TransmitDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), sErrorMessage2, _T(""),sErrorMessage);
			m_TransmitDB.ReleaseTransmitLock(sErrorMessage);
			continue;
		}

		if (nWaitingJobs == 0) {
			//g_util.LogprintfTransmit("No jobs found..");
			m_TransmitDB.ReleaseTransmitLock(sErrorMessage);
			continue;
		}


		BOOL bAllErrorJobs = TRUE;
		for (int i = 0; i < nWaitingJobs; i++) {
			if (TxJob[i].m_status != 26 && TxJob[i].m_status != 16) {
				bAllErrorJobs = FALSE;
				break;
			}
		}
		if (nWaitingJobs > 0 && bAllErrorJobs)
			g_util.LogprintfTransmit("INFO: All waiting jobs have TX-error...!!");

		BOOL bAnyReady = FALSE;


		CUIntArray recoveredMasters;
		for (int i = 0; i < nWaitingJobs; i++) {

			if (TxJob[i].m_testfileexists == FALSE && (TxJob[i].m_pdftype == POLLINPUTTYPE_LOWRESPDF || TxJob[i].m_pdftype == POLLINPUTTYPE_PRINTPDF)) {
				// UPS - missing converted file.
				CString sPDFFileNameHighRes = g_prefs.FormCCFilesName(POLLINPUTTYPE_HIRESPDF, TxJob[i].m_mastercopyseparationset, TxJob[i].m_filename);
				g_util.LogprintfTransmit("WARNING: Attempting to recover missing convert file from %s..", sPDFFileNameHighRes);
				if (g_util.FileExist(sPDFFileNameHighRes)) {

					// Only reset convert status once for a mastercopy-page
					BOOL bFoundElement = FALSE;
					for (int xx = 0; xx < recoveredMasters.GetCount(); xx++) {
						if (recoveredMasters[xx] == TxJob[i].m_mastercopyseparationset) {
							bFoundElement = TRUE;
							break;
						}
					}

					if (bFoundElement)
						continue;

					// Highres file is there - trigger reprocessing (3 times max)
					m_TransmitDB.ResetConvertStatus(TxJob[i].m_pdftype, TxJob[i].m_mastercopyseparationset, sErrorMessage);
					g_util.LogprintfTransmit("WARNING: Triggered a new convertion of file %s (channelID %d)", sPDFFileNameHighRes, TxJob[i].m_channelID);
					recoveredMasters.Add(TxJob[i].m_mastercopyseparationset);
				}
			}
		}

		for (int i = 0; i < nWaitingJobs; i++) {
			
			if (TxJob[i].m_testfileexists == FALSE)
				continue;
			CString sJobName = TxJob[i].m_jobname;
			int nApproved = TxJob[i].m_approved == TRUE ? STATE_RELEASED : STATE_LOCKED;

			bAnyReady = TRUE;

			g_util.LogprintfTransmit("INFO: Transmit job detected2: Mastercopyseparationset %d for ChannelID %d - %s", TxJob[i].m_mastercopyseparationset, TxJob[i].m_channelID, sErrorMessage);

			CString sDestName = _T("");
			CString sTxErrorMessage = _T("");
			bReRunQueryAfterSuccess = FALSE;
			CString sChannelWorkFolder = g_prefs.m_workfolder + _T("\\") + g_util.Int2String(TxJob[i].m_channelID);
			::CreateDirectory(sChannelWorkFolder, NULL);

			int nMergeProductionIDToIgnore = 0;
		
			int nSuccessTX = TransmitJobChannels(TxJob[i], m_TransmitDB, sChannelWorkFolder, sDestName, bReRunQueryAfterSuccess, nMergeProductionIDToIgnore, sTxErrorMessage);

		
			g_util.LogprintfTransmit("INFO: TransmitJobChannels(Mastercopyseparationset %d ,ChannelID %d) = %d", TxJob[i].m_mastercopyseparationset, TxJob[i].m_channelID, nSuccessTX);
			CTime t = CTime::GetCurrentTime();

			//nSuccessTX=0 means merged files not yet ready - not an error..
			if (nSuccessTX > 0) {
				m_TransmitDB.InsertLogEntry(EVENTCODE_EXPORTQUEUED, g_prefs.GetChannelName(TxJob[i].m_channelID), sDestName,_T("Queued for export"),
						TxJob[i].m_mastercopyseparationset, TxJob[i].m_version,0,_T(""), sErrorMessage);

				m_TransmitDB.UpdateService(SERVICESTATUS_RUNNING, sJobName,_T(""), _T(""), sErrorMessage);
			}
			else if (nSuccessTX == -1) {

				m_TransmitDB.InsertLogEntry(EVENTCODE_EXPORTQUEUED_ERROR, g_prefs.GetChannelName(TxJob[i].m_channelID), sDestName, sTxErrorMessage,
					TxJob[i].m_mastercopyseparationset, TxJob[i].m_version, 0, _T(""), sErrorMessage);

				m_TransmitDB.UpdateService(SERVICESTATUS_HASERROR, sJobName, sTxErrorMessage, _T(""), sErrorMessage);
				
			}
			// Flag other merge files for same production just registered as not conmplete
			/*else if (nSuccessTX == 0 && nMergeProductionIDToIgnore > 0)
			{
				for (int j = i; j < nWaitingJobs; j++) {
					CHANNELSTRUCT *pChannel = g_prefs.GetChannelStruct(m_TransmitDB, TxJob[j].m_channelID);
					if (pChannel != NULL) {

						if (TxJob[j].m_productionID == nMergeProductionIDToIgnore && pChannel->m_mergePDF)
							TxJob[j].m_testfileexists = FALSE;
					}
				}
			}*/

			if (bReRunQueryAfterSuccess && nSuccessTX > 0)
				break; // Break now and re-run TX-query for next job..

		} //for .. all waiting jobs..
		if (bAnyReady == FALSE)
			nWaitingJobs = 0;

		m_TransmitDB.ReleaseTransmitLock(sErrorMessage);

		g_transmitthreadrunning = FALSE;

	}

	if (!bReconnect) {
		m_TransmitDB.ReleaseTransmitLock(sErrorMessage);
		m_TransmitDB.UpdateService(0, _T(""), _T(""), _T(""), sErrorMessage);
	}
	m_TransmitDB.ExitDB();

	g_util.LogprintfTransmit("INFO: TransmitThread() terminated");

	return 0;

}

// -1 Error
// 1 Transmitted
// 0 Channel disabled og file already sent (merged)

int TransmitJobChannels(NextTransmitJob &job, CDatabaseManager &db, CString sDestinationFolder, 
												CString &sFinalName, BOOL &bEmptyQueue, int &nMergeProductionIDToIgnore, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	bEmptyQueue = FALSE;
	nMergeProductionIDToIgnore = 0;
	BOOL bFileCopyOK = TRUE;
	CJobAttributes jobAttrib;

	CString sSourcePath = _T("");

	CString sLowresSourcePath = g_prefs.FormCCFilesName(POLLINPUTTYPE_LOWRESPDF, job.m_mastercopyseparationset, g_util.GetFileName(job.m_filename, TRUE));
	CString sHighresSourcePath = g_prefs.FormCCFilesName(POLLINPUTTYPE_HIRESPDF, job.m_mastercopyseparationset, g_util.GetFileName(job.m_filename, TRUE));
	CString sPrintSourcePath = g_prefs.FormCCFilesName(POLLINPUTTYPE_PRINTPDF, job.m_mastercopyseparationset, g_util.GetFileName(job.m_filename, TRUE));

	jobAttrib.m_publicationID = 0;

	if (db.GetJobDetails(&jobAttrib, job.m_copyseparationset, sErrorMessage) == FALSE) {
		g_util.LogprintfTransmit("ERROR: GetJobDetails() returned error %s", sErrorMessage);
		return -1;
	}

	int nPubDateAsNumber = jobAttrib.m_pubdate.GetYear() * 10000 + jobAttrib.m_pubdate.GetMonth() * 100 + jobAttrib.m_pubdate.GetDay();
	if (jobAttrib.m_pubdate.GetYear() < 2000)
		nPubDateAsNumber = 0;
	CTime tNow = CTime::GetCurrentTime();
	int nNowDateAsNumber = tNow.GetYear() * 10000 + tNow.GetMonth() * 100 + tNow.GetDay();

	PUBLICATIONSTRUCT *pPub = g_prefs.GetPublicationStruct(jobAttrib.m_publicationID);
	if (pPub == NULL) {
		g_util.LogprintfTransmit("ERROR: GetJobDetails returned PublicationID = %d!!", jobAttrib.m_publicationID);
		return -1;
	}

	CHANNELSTRUCT *pChannel = g_prefs.GetChannelStruct(db,job.m_channelID);
	if (pChannel == NULL) {
		g_util.LogprintfTransmit("ERROR: GetChannelStruct failed ID=%d", job.m_channelID);
		return -1;
	}

	// Reload changed config?

	CTime tNewChannelChangeTime;
	db.GetChannelConfigChangeTime(job.m_channelID, tNewChannelChangeTime, sErrorMessage);
	if (tNewChannelChangeTime > pChannel->m_configchangetime) {
		db.LoadNewChannel(job.m_channelID, sErrorMessage);
		g_util.LogprintfTransmit("INFO: Reloaded config data for channel-ID %d..", job.m_channelID);
		pChannel = g_prefs.GetChannelStruct(db, job.m_channelID);
		pChannel->m_configchangetime = tNewChannelChangeTime;
	}


	PUBLICATIONCHANNELSTRUCT *pPublicationChannel = g_prefs.GetPublicationChannelStruct(jobAttrib.m_publicationID, job.m_channelID);

	// RE-READ ON/OFF CHANNEL STATE
	if (db.ReLoadChannelOnOff(job.m_channelID, sErrorMessage) == FALSE)
		g_util.LogprintfTransmit("ERROR: Channel %s disabled!", pChannel->m_channelname);

	if (pChannel->m_enabled == FALSE) {
		g_util.LogprintfTransmit("WARNING: Channel %s disabled!", pChannel->m_channelname);
		return 0;
	}

	jobAttrib.m_publisherID = pPub->m_publisherID;

	// Edition bypass
	CString sThisEdition = g_prefs.GetEditionName(jobAttrib.m_editionID);
	BOOL bIsFirstEdition = sThisEdition == "1" || sThisEdition == "1_1";
	BOOL bIsSecondEdition = sThisEdition == "2" || sThisEdition == "2_1" || sThisEdition == "1_2" || sThisEdition == "2_2";
	BOOL bIsThirdEdition = sThisEdition == "3" || sThisEdition == "3_1" || sThisEdition == "3_2" || sThisEdition == "1_3" || sThisEdition == "2_3" || sThisEdition == "3_3";

	int nFirstEditionID = db.GetLowestEdition(jobAttrib.m_productionID, sErrorMessage);
	if (nFirstEditionID > 0) {
		CString sFirstEdition = g_prefs.GetEditionName(nFirstEditionID);
		if (sThisEdition == sFirstEdition)
			bIsFirstEdition = TRUE;
	}
	else
		g_util.LogprintfTransmit("ERROR: GetLowestEdition() - %s ..", sErrorMessage);

	if (pChannel->m_editionstogenerate == 1 && bIsFirstEdition == FALSE) {
		g_util.LogprintfTransmit("INFO:  Skipping edition page %s (not required for channel) - 1", sThisEdition);
		return 0;
	}
	if (pChannel->m_editionstogenerate == 2 && bIsFirstEdition == FALSE && bIsSecondEdition == FALSE) {
		g_util.LogprintfTransmit("INFO:  Skipping edition page %s (not required for channel) - 2", sThisEdition);
		return 0;
	}
	if (pChannel->m_editionstogenerate == 3 && bIsFirstEdition == FALSE && bIsSecondEdition == FALSE && bIsThirdEdition == FALSE) {
		g_util.LogprintfTransmit("INFO:  Skipping edition page %s (not required for channel) - 3", sThisEdition);
		return 0;
	}

	g_util.LogprintfTransmit("INFO:  Channel %s ..mode %d", pChannel->m_channelname, pChannel->m_pdftype);

	if (pChannel->m_pdftype == POLLINPUTTYPE_LOWRESPDF) {
	// 	if (g_prefs.m_logToFile > 2)
			g_util.LogprintfTransmit("INFO:  Looking for low-res file %s ..", sLowresSourcePath);

		if (g_util.FileExist(sLowresSourcePath) == FALSE) {
			sErrorMessage = _T("Source file does not exist in system storage folder");
			g_util.LogprintfTransmit("ERROR: Lowres source file '%s' does not exist in system storage folder", sLowresSourcePath);
			return -1;
		}
		DWORD dwFileSizeSource = g_util.GetFileSize(sLowresSourcePath);
		int nCRCBefore = db.GetChannelCRC(job.m_mastercopyseparationset, POLLINPUTTYPE_LOWRESPDF, sErrorMessage);
		if (nCRCBefore == -1)
			g_util.LogprintfTransmit("ERROR:  db.GetChannelCRC() - %s", sErrorMessage);
		int nCRCAfter = g_util.CRC32(sLowresSourcePath);
		g_util.LogprintfTransmit("INFO: File %s has checksum %d", sLowresSourcePath, nCRCAfter);

		if (nCRCBefore == 0) {
			g_util.LogprintfTransmit("ERROR: No checksum registered on lowres file %s", sLowresSourcePath);
			return -1;
		}

		if (nCRCAfter != 0 && nCRCBefore != 0 && nCRCAfter != nCRCBefore) {
			g_util.LogprintfTransmit("WARNING: CRC mismatch for file %s (before %d after %d)", sLowresSourcePath, nCRCBefore, nCRCAfter);
			if (g_prefs.m_recalculateCheckSum)
				if (db.ResetConvertStatus(POLLINPUTTYPE_LOWRESPDF, job.m_mastercopyseparationset, sErrorMessage) == FALSE)
					g_util.LogprintfTransmit("ERROR:  db.ResetConvertStatus() - %s", sErrorMessage);


			return -1;
		}
		sSourcePath = sLowresSourcePath;
	}
	else if (pChannel->m_pdftype == POLLINPUTTYPE_HIRESPDF) {
	//	if (g_prefs.m_logToFile > 2)
			g_util.LogprintfTransmit("INFO:  Looking for high-res file %s ..", sHighresSourcePath);

		if (g_util.FileExist(sHighresSourcePath) == FALSE) {
			sErrorMessage = _T("Source file does not exist in system storage folder");
			g_util.LogprintfTransmit("ERROR: Highres source file '%s' does not exist in system storage folder", sHighresSourcePath);
			return -1;
		}
		DWORD dwFileSizeSource = g_util.GetFileSize(sHighresSourcePath);
		int nCRCBefore = db.GetChannelCRC(job.m_mastercopyseparationset, POLLINPUTTYPE_HIRESPDF, sErrorMessage);
		if (nCRCBefore == -1)
			g_util.LogprintfTransmit("ERROR:  db.GetChannelCRC() - %s", sErrorMessage);
		int nCRCAfter = g_util.CRC32(sHighresSourcePath);
		g_util.LogprintfTransmit("INFO: File %s has checksum %d", sHighresSourcePath, nCRCAfter);
		if (nCRCBefore == 0) {
			g_util.LogprintfTransmit("ERROR: No checksum registered on highres file %s", sHighresSourcePath);
			return -1;
		}

		if (nCRCAfter != 0 && nCRCBefore != 0 && nCRCAfter != nCRCBefore) {
			g_util.LogprintfTransmit("WARNING: CRC mismatch for file %s (before %d after %d)", sHighresSourcePath, nCRCBefore, nCRCAfter);
			return -1;
		}
		sSourcePath = sHighresSourcePath;
	}
	else if (pChannel->m_pdftype == POLLINPUTTYPE_PRINTPDF) {
		//if (g_prefs.m_logToFile > 2)
			g_util.LogprintfTransmit("INFO:  Looking for print file %s ..", sPrintSourcePath);

		if (g_util.FileExist(sPrintSourcePath) == FALSE) {
			sErrorMessage = _T("Source file does not exist in system storage folder");
			g_util.LogprintfTransmit("ERROR: Print source file '%s' does not exist in system storage folder", sPrintSourcePath);
			return -1;
		}
		DWORD dwFileSizeSource = g_util.GetFileSize(sPrintSourcePath);
		int nCRCBefore = db.GetChannelCRC(job.m_mastercopyseparationset, POLLINPUTTYPE_PRINTPDF, sErrorMessage);
		if (nCRCBefore == -1)
			g_util.LogprintfTransmit("ERROR:  db.GetChannelCRC() - %s", sErrorMessage);
		int nCRCAfter = g_util.CRC32(sPrintSourcePath);

		if (nCRCBefore == 0) {
			g_util.LogprintfTransmit("ERROR: No checksum registered on print file %s", sPrintSourcePath);
			return -1;
		}

		if (nCRCAfter != 0 && nCRCBefore != 0 && nCRCAfter != nCRCBefore) {
			g_util.LogprintfTransmit("WARNING: CRC mismatch for file %s (before %d after %d)", sHighresSourcePath, nCRCBefore, nCRCAfter);

			if (g_prefs.m_recalculateCheckSum)
				if (db.ResetConvertStatus(POLLINPUTTYPE_PRINTPDF, job.m_mastercopyseparationset, sErrorMessage) == FALSE)
					g_util.LogprintfTransmit("ERROR:  db.ResetConvertStatus() - %s", sErrorMessage);


			return -1;
		}
		sSourcePath = sPrintSourcePath;
	}
	else
		return 0;

	CString sDestName = job.m_filename;
	CString sSubFolder = _T("");

	// zippped or merged..
	CString sDestZipName = sDestName;


	int nPubDateMove = 0;
	if (pChannel->m_pdftype == POLLINPUTTYPE_LOWRESPDF && pPub->m_pubdatemove && pPub->m_pubdatemovedays != 0) {
		nPubDateMove = pPub->m_pubdatemovedays;

		// Overrule if set per channel definition on this publication
		if (pPublicationChannel != NULL)
			if (pPublicationChannel->m_pubdatemovedays > 0)
				nPubDateMove = pPublicationChannel->m_pubdatemovedays;

		g_util.LogprintfTransmit("INFO:  Moving pubdate %d days in output name", nPubDateMove);
	}

	if (pChannel->m_transmitnameconv.Trim() != "") {

		sDestName = g_util.FormNameChannel(&db, jobAttrib, *pChannel, nPubDateMove);
		if (g_prefs.m_transmitcleanfilename)
			sDestName.Replace(' ', '_');

		if (sDestName == "")
			sDestName = jobAttrib.m_filename;

		CString sZipNameConv = pChannel->m_transmitnameconv;
		sZipNameConv.Replace("-%S", "");
		sZipNameConv.Replace("-S%S", "");
		sZipNameConv.Replace("_%S", "");
		sZipNameConv.Replace("_S%S", "");
		sZipNameConv.Replace("-%N", "");
		sZipNameConv.Replace("-%2N", "");
		sZipNameConv.Replace("-%3N", "");
		sZipNameConv.Replace("_%N", "");
		sZipNameConv.Replace("_%2N", "");
		sZipNameConv.Replace("_%3N", "");

		sZipNameConv.Replace("-%M", "");
		sZipNameConv.Replace("-%2M", "");
		sZipNameConv.Replace("-%3M", "");
		sZipNameConv.Replace("_%M", "");
		sZipNameConv.Replace("_%2M", "");
		sZipNameConv.Replace("_%3M", "");

		sDestZipName = g_util.FormName(&db, sZipNameConv, pChannel->m_transmitdateformat, "", jobAttrib, pChannel->m_transmitnameuseabbr, nPubDateMove, pChannel->m_mergePDF, pChannel->m_usepackagenames);
	}

	if (pChannel->m_subfoldernameconv.Find("%") != -1) {
		sSubFolder = pChannel->m_subfoldernameconv;
		CString sFinalsRemoteFolder = g_util.FormName(&db, pChannel->m_subfoldernameconv, pChannel->m_transmitdateformat, _T(""), jobAttrib, pChannel->m_transmitnameuseabbr, nPubDateMove, pChannel->m_mergePDF, pChannel->m_usepackagenames);
		if (sFinalsRemoteFolder != "") 
			sSubFolder = sFinalsRemoteFolder;
	}

	// Prefix name with destination subfolder when trenamissing
	sSubFolder.Replace("\\", "@");
	sSubFolder.Replace("/", "@");
	if (sSubFolder != "") {
		sDestZipName = _T("@@") + sSubFolder + _T("@@") + sDestZipName;
		sDestName = _T("@@") + sSubFolder + _T("@@") + sDestName;
	}

	if (job.m_mastercopyseparationset > 0) {
		sDestName = g_util.GetFileName(sDestName, TRUE) + _T("#") + g_util.Int2String(job.m_mastercopyseparationset) + _T(".") + g_util.GetExtension(sDestName);
	}

	g_util.LogprintfTransmit("INFO:  Output name (incl. masterref) %s  Subfolder name %s", sDestName, sSubFolder);

	//CString sOutputFileTitle = sDestName;
	sFinalName = sDestName;

	DWORD dwCRCbefore = 0;
	//if (pChannel->m_useftp && g_prefs.m_FTPpostcheck > 2)
	//	dwCRCbefore = util.CRC32(sSourcePath);
	BOOL bMergeOk = TRUE;
	if (pChannel->m_mergePDF) {

		// Already done??
		int nChannelStatus = db.GetChannelExportStatus(job.m_mastercopyseparationset, job.m_channelID, sErrorMessage);
		if (nChannelStatus == -1)
			g_util.LogprintfTransmit("ERROR: GetChannelExportStatus() - %s", sErrorMessage);

		g_util.LogprintfTransmit("INFO: Current Export status = %d", nChannelStatus);
		if (nChannelStatus == 10 || nChannelStatus == 9)
			return 0;


		CUIntArray aMasterCopySeparationSetList;
		CStringArray aFileNameList;

		// Find all productions contained in package..
		if (pChannel->m_usepackagenames) {
			CUIntArray aPackagePublicationIDList;
			CUIntArray aPackageSectionIndexList; 
			CString sPackageName = _T("");
			if (db.GetPackagePublications(jobAttrib.m_publicationID, aPackagePublicationIDList, aPackageSectionIndexList, sPackageName, sErrorMessage) == FALSE) {
				g_util.LogprintfTransmit("ERROR: GetPackagePublications() - %s", sErrorMessage);
			}

			// Collect all involved  in order of assembly..
			CUIntArray aProductionList;
			CUIntArray aEditionList;
			int nPagesReadyTotal = 0;

			BOOL bHasAllPages = FALSE;
			for (int ii = 0; ii < aPackagePublicationIDList.GetCount(); ii++) {
				int nThisProductionID = 0;
				int nThisEditionID = 0;
				if (db.GetProductionID(aPackagePublicationIDList[ii], jobAttrib.m_pubdate, nThisProductionID, nThisEditionID, sErrorMessage) == FALSE) {
					g_util.LogprintfTransmit("ERROR: GetProductionID() - %s", sErrorMessage);
				}

				if (nThisProductionID == 0)
					continue;

				// is this produt ready?
				int nPagesReadyInProduction = 0;
				int nPagesInProduction = db.GetPageCountInProduction(nThisProductionID, nThisEditionID, STATUSNUMBER_MISSING, sErrorMessage);
				if (nPagesInProduction < 0) {
					g_util.LogprintfTransmit("ERROR: GetPageCountInProduction(%d) failed - %s", nThisProductionID, sErrorMessage);
					return -1;
				}
				nPagesReadyInProduction = db.GetPageCountInProduction(nThisProductionID, nThisEditionID, STATUSNUMBER_POLLED, sErrorMessage);
				if (nPagesReadyInProduction < 0) {
					g_util.LogprintfTransmit("ERROR: GetPageCountInProduction(%d) 2 failed - %s", nThisProductionID, sErrorMessage);
					return -1;
				}
				if (nPagesInProduction == nPagesReadyInProduction && nPagesReadyInProduction > 0) {
					aProductionList.Add(nThisProductionID);
					aEditionList.Add(nThisEditionID);
					nPagesReadyTotal += nPagesReadyInProduction;
				} 
				else {
					bHasAllPages = FALSE;
					break;
				}
			}

			if (bHasAllPages == FALSE || aProductionList.GetCount() == 0)
				return 0;

			for (int ii = 0; ii < aProductionList.GetCount(); ii++) {
				BOOL bTestProofsReady = db.GetMasterCopySeparationListPDF(FALSE, pChannel->m_pdftype, aProductionList[ii], aEditionList[ii], aMasterCopySeparationSetList, aFileNameList, sErrorMessage);
				if (bTestProofsReady == FALSE) {
					g_util.LogprintfTransmit("ERROR: GetMasterCopySeparationListPDF(%d,%d)  failed - %s", aProductionList[ii], aEditionList[ii], sErrorMessage);
					return -1;
				}

			}

			if (aMasterCopySeparationSetList.GetCount() != nPagesReadyTotal) {
				g_util.LogprintfTransmit("ERROR: Accumulated GetMasterCopySeparationListPDF() returned only %d pages. Expected %d", aMasterCopySeparationSetList.GetCount(), nPagesReadyTotal);
				return 0;
			}

			// Do the package merge!!
			
			CString sTempZipFolder = sDestinationFolder + _T("\\tmp");
			::CreateDirectory(sTempZipFolder, NULL);
			CString sTempDestPath = sTempZipFolder + _T("\\") + sDestZipName + g_util.GenerateTimeStamp() + ".tmp";
			g_util.LogprintfTransmit("INFO: Generating package PDF book %s", sTempDestPath);

			bMergeOk = MakeOriginalPdfFromBookMasterSets(pChannel->m_pdftype, aMasterCopySeparationSetList, aFileNameList, sTempDestPath, FALSE, nPagesReadyTotal, sErrorMessage);
			if (bMergeOk == FALSE) {
				::Sleep(2000);
				bMergeOk = MakeOriginalPdfFromBookMasterSets(pChannel->m_pdftype, aMasterCopySeparationSetList, aFileNameList, sTempDestPath, FALSE, nPagesReadyTotal, sErrorMessage);
			}

			if (bMergeOk) {
				if (g_util.MoveFileWithRetry(sTempDestPath, sDestinationFolder + _T("\\") + sDestZipName, sErrorMessage) == FALSE) {
					g_util.LogprintfTransmit("ERROR: MoveFileWithRetry(%s,%s)  failed - %s",
						sTempDestPath, sDestinationFolder + _T("\\") + sDestZipName, sErrorMessage);
					bMergeOk = FALSE;
					::DeleteFile(sTempDestPath);
				}

				if (bMergeOk) {
					sFinalName = sDestZipName;

					for (int pg = 0; pg < aMasterCopySeparationSetList.GetCount(); pg++) {
						db.UpdateChannelStatus(2, aMasterCopySeparationSetList[pg], 0, job.m_channelID, 10, "Merged", sDestZipName, sErrorMessage);
					}

					// Make sure the top level query to run again..
					bEmptyQueue = TRUE;
				}
			}
			else {
				g_util.LogprintfTransmit("ERROR: GetMasterCopySeparationListPDF()  failed - %s", sErrorMessage);
				bMergeOk = FALSE;
				::DeleteFile(sTempDestPath);
			}



		}
		else // Normal merge..!
		{

			int nPagesReadyInProduction = 0;
			int nPagesInProduction = db.GetPageCountInProduction(jobAttrib.m_productionID, jobAttrib.m_editionID, STATUSNUMBER_MISSING, sErrorMessage);
			if (nPagesInProduction < 0) {
				g_util.LogprintfTransmit("ERROR: GetPageCountInProduction(%d) failed - %s", jobAttrib.m_productionID, sErrorMessage);
				return -1;
			}
			g_util.LogprintfTransmit("INFO: Testing for PDF merge of %s %.2d-%.2d - pages total %d", g_prefs.GetPublicationName(jobAttrib.m_publicationID), jobAttrib.m_pubdate.GetDay(), jobAttrib.m_pubdate.GetMonth(), nPagesInProduction);

			nPagesReadyInProduction = db.GetPageCountInProduction(jobAttrib.m_productionID, jobAttrib.m_editionID, STATUSNUMBER_POLLED, sErrorMessage);
			if (nPagesReadyInProduction < 0) {
				g_util.LogprintfTransmit("ERROR: GetPageCountInProduction(%d) 2 failed - %s", jobAttrib.m_productionID, sErrorMessage);
				return -1;
			}
			g_util.LogprintfTransmit("INFO: Testing for PDF merge of %s %.2d-%.2d - pages ready %d (ProdID %d, Edition %d)", g_prefs.GetPublicationName(jobAttrib.m_publicationID), jobAttrib.m_pubdate.GetDay(), jobAttrib.m_pubdate.GetMonth(), nPagesReadyInProduction, jobAttrib.m_productionID, jobAttrib.m_editionID);

			if (nPagesInProduction == nPagesReadyInProduction && nPagesReadyInProduction > 0) { // && (nPagesReadyInProduction%2) == 0) {
				// Do the merge...
				BOOL bTestProofsReady = db.GetMasterCopySeparationListPDF(TRUE, pChannel->m_pdftype, jobAttrib.m_productionID, jobAttrib.m_editionID, aMasterCopySeparationSetList, aFileNameList, sErrorMessage);
				if (bTestProofsReady == FALSE) {
					g_util.LogprintfTransmit("ERROR: GetMasterCopySeparationListPDF(%d,%d)  failed - %s", jobAttrib.m_productionID, jobAttrib.m_editionID, sErrorMessage);
					return -1;
				}
				if (aMasterCopySeparationSetList.GetCount() != nPagesReadyInProduction) {
					g_util.LogprintfTransmit("ERROR: GetMasterCopySeparationListPDF(%d,%d)  returned only %d pages. Expected %d", jobAttrib.m_productionID, jobAttrib.m_editionID, aMasterCopySeparationSetList.GetCount(), nPagesReadyInProduction);
					return 0;
				}
				CString sTempZipFolder = sDestinationFolder + _T("\\tmp");
				::CreateDirectory(sTempZipFolder, NULL);
				CString sTempDestPath = sTempZipFolder + _T("\\") + sDestZipName + g_util.GenerateTimeStamp() + ".tmp";
				g_util.LogprintfTransmit("INFO: Generating PDF book %s", sTempDestPath);
				bMergeOk = MakeOriginalPdfBook(db, jobAttrib.m_productionID, jobAttrib.m_editionID, pChannel->m_pdftype, sTempDestPath, 0, FALSE, nPagesReadyInProduction, sErrorMessage);
				if (bMergeOk == FALSE) {
					::Sleep(2000);
					bMergeOk = MakeOriginalPdfBook(db, jobAttrib.m_productionID, jobAttrib.m_editionID, pChannel->m_pdftype, sTempDestPath, 0, FALSE, nPagesReadyInProduction, sErrorMessage);
				}

				if (bMergeOk) {
					if (g_util.MoveFileWithRetry(sTempDestPath, sDestinationFolder + _T("\\") + sDestZipName, sErrorMessage) == FALSE) {
						g_util.LogprintfTransmit("ERROR: MoveFileWithRetry(%s,%s)  failed - %s",
							sTempDestPath, sDestinationFolder + _T("\\") + sDestZipName, sErrorMessage);
						bMergeOk = FALSE;
						::DeleteFile(sTempDestPath);

					}

					if (bMergeOk) {
						sFinalName = sDestZipName;

						for (int pg = 0; pg < aMasterCopySeparationSetList.GetCount(); pg++) {
							if (db.UpdateChannelStatus(2, aMasterCopySeparationSetList[pg], jobAttrib.m_productionID, job.m_channelID, 10, "Merged", sDestZipName, sErrorMessage) == FALSE)
								g_util.LogprintfTransmit("ERROR db.UpdateChannelStatus() - %s", sErrorMessage);

						}

						// Make sure the top level query to run again..
						bEmptyQueue = TRUE;
					}
				}
				else {
					g_util.LogprintfTransmit("ERROR: GetMasterCopySeparationListPDF(%d,%d)  failed - %s", jobAttrib.m_productionID, jobAttrib.m_editionID, sErrorMessage);
					bMergeOk = FALSE;
					::DeleteFile(sTempDestPath);
				}
			}
			else if (nPagesReadyInProduction > 0) {
				nMergeProductionIDToIgnore = jobAttrib.m_productionID;
			}
		}
	}	// end merge

	if (pChannel->m_mergePDF)
		return bMergeOk ? 1 : -1;

	// Non-merged file..

	
	CString sFullDestPath = sDestinationFolder + _T("\\") + sDestName;

	int nRetries = g_prefs.m_transmitretries;
	if (nRetries <= 1)
		nRetries = 2;

	// Delete existing fil
	if (g_util.FileExist(sFullDestPath)) {
		BOOL bDeleteSuccess = FALSE;
		while (--nRetries >= 0 && bDeleteSuccess == FALSE) {
			bDeleteSuccess = ::DeleteFile(sFullDestPath);

			if (bDeleteSuccess)
				if (g_util.FileExist(sFullDestPath))
					bDeleteSuccess = FALSE;

			if (bDeleteSuccess)
				break;

			Sleep(500);
		}
		if (bDeleteSuccess == FALSE)
			g_util.LogprintfTransmit("ERROR deleting %s in internal folder prior to transmission", sFullDestPath);
	}


	int bCopySuccess = g_util.CopyFileWithRetry(sSourcePath, sFullDestPath, sErrorMessage);
	if (bCopySuccess == FALSE) {

		sErrorMessage.Format("ERROR copying %s to internal folder %s - %s", sSourcePath, sFullDestPath, sErrorMessage);
		 
		if (sErrorMessage.Find("Not enough storage is available to process this command") != -1) {
			g_util.LogprintfTransmit("ERROR FATAL memory error - forcing quit!");
			exit(0);
		}
		g_util.LogprintfTransmit("%s", sErrorMessage);
		CString sErrorMessage2 = sErrorMessage;
		if (db.UpdateChannelStatus(2, job.m_mastercopyseparationset, 0, job.m_channelID, EXPORTSTATUSNUMBER_EXPORTERROR, sErrorMessage2, sDestName, sErrorMessage) == FALSE)
			g_util.LogprintfTransmit("ERROR db.UpdateChannelStatus() - %s", sErrorMessage);

		//g_util.SendMail(MAILTYPE_TXERROR, sMsg);
		bFileCopyOK = bCopySuccess;
	}
	else {
		g_util.LogprintfTransmit("INFO: Updating channel status Master=%d,ChannelID=%d,FileName=%s", job.m_mastercopyseparationset, job.m_channelID, sFinalName);
		if (db.UpdateChannelStatus(2, job.m_mastercopyseparationset, 0, job.m_channelID, EXPORTSTATUSNUMBER_EXPORTING, "", sDestName, sErrorMessage) == FALSE)
			g_util.LogprintfTransmit("ERROR db.UpdateChannelStatus() - %s", sErrorMessage);

	}

		

	// Send additional common pages for channels with higres option
	BOOL bSendCommonPages = (pChannel->m_transmitnameoptions & TXNAMEOPTION_SENDCOMMONPAGES) > 0 ? TRUE : FALSE;

	if (bSendCommonPages) {

		CUIntArray aCopySeparationSets;
		db.GetCommonPages(job.m_mastercopyseparationset, aCopySeparationSets, sErrorMessage);
		for (int k = 0; k < aCopySeparationSets.GetCount(); k++) {

			// Only send if one the the pages in the run is unique
			if (db.HasUniquePages(aCopySeparationSets[k], sErrorMessage) == TRUE) {
				CJobAttributes jobAttribCommonPage;
				jobAttribCommonPage.m_publicationID = 0;
				CString sDestNameCommon = "";

				db.GetJobDetails(&jobAttribCommonPage, aCopySeparationSets[k],  sErrorMessage);

				if (pChannel->m_transmitnameconv.Trim() != "") {
					sDestNameCommon = g_util.FormNameChannel(&db, jobAttribCommonPage, *pChannel, nPubDateMove);
					if (g_prefs.m_transmitcleanfilename)
						sDestNameCommon.Replace(' ', '_');

					if (sDestNameCommon == "")
						continue;
				}

				// Prefix name with destination subfolder when trenamissing
				if (sSubFolder != "") 
					sDestNameCommon = _T("@@") + sSubFolder + _T("@@") + sDestNameCommon;

				CString sFullDestPathCommon = sDestinationFolder + _T("\\") + sDestNameCommon;
				g_util.CopyFileWithRetry(sSourcePath, sFullDestPathCommon, sErrorMessage);
				g_util.LogprintfTransmit("INFO:  Sending common page %s to channel %s", sDestNameCommon, pChannel->m_channelname);
			}
		}
	}

	return bFileCopyOK ? 1 : -1;

}
