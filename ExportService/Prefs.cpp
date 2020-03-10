#include "stdafx.h"
#include <afxmt.h>

#include "Defs.h"
#include "Prefs.h"
#include "Utils.h"
#include "Markup.h"
#include "ExportService.h"
#include "Registry.h"


extern BOOL		g_BreakSignal;
extern CTime	g_lastSetupFileTime;
extern BOOL		g_connected;
extern CUtils g_util;

CCriticalSection g_dbAccess;


CPrefs::CPrefs(void)
{

	g_dbAccess.Unlock();


	m_instancenumber = 1;

	m_FieldExists_Log_FileName = FALSE;
	m_FieldExists_Log_MiscString = FALSE;


	m_emailtimeout = 60;
	m_workfolder = _T("c:\\temp");
	m_workfolder2 = _T("c:\\temp2");

	m_bypasspingtest = TRUE;
	m_bypassreconnect = FALSE;
	m_lockcheckmode = LOCKCHECKMODE_READWRITE;
	m_autostart = FALSE;
	m_autostartmin = 0;

	m_logtodatabase = FALSE;
	m_DBserver = _T("");
	m_Database = _T("ControlCenterPDF") ;
	m_DBuser = _T("sa");
	m_DBpassword = _T("infra");

	m_logToFile = FALSE;
	m_logToFile2 = FALSE;

	m_scripttimeout = 60;

	m_updatelog = FALSE;

	m_maxlogfilesize = 10 * 1024 * 1024;

	m_generateJMF = FALSE;
	m_generateJMFstatus = FALSE;
	m_deleteerrorfilesdays = 0;
	m_emailsmtpserver = _T("");
	m_emailfrom = _T("");
	m_emailto = _T("");
	m_emailcc = _T("");
	m_emailsubject = _T("");
	m_emailpreventflooding = TRUE;
	m_emailpreventfloodingdelay = 10;
	m_mailsmtpport = 25;


	m_ftptimeout = 30;
	m_bSortOnCreateTime = TRUE;
	m_defaultpolltime = 1;
	m_bypassdiskfreeteste = FALSE;

	m_addsetupnametoackfile = TRUE;
	m_ignorelockcheck = FALSE;
	m_filebuildupseparatefolder = FALSE;

	m_sDateFormat = "DDMMYYYY";

	m_generatestatusmessage = FALSE;
	m_messageonsuccess = TRUE;
	m_messageonerror = FALSE;
	m_messageoutputfolder = _T("");
	m_messagetemplateerror = _T("");
	m_messagetemplatesuccess = _T("");

}


CPrefs::~CPrefs(void)
{
}




void CPrefs::LoadIniFile(CString sIniFile)
{
	TCHAR Tmp[MAX_PATH];
	TCHAR Tmp2[MAX_PATH];

	::GetPrivateProfileString("System", "CcdataFilenameMode", "0", Tmp, 255, sIniFile);
	m_ccdatafilenamemode = atoi(Tmp);
	

	::GetPrivateProfileString("System", "DebugSetup", "", Tmp, 255, sIniFile);
	m_debugsetup = Tmp;



	GetPrivateProfileString("System", "LockCheckMode", "2", Tmp, 255, sIniFile);
	m_lockcheckmode = _tstoi(Tmp);

	GetPrivateProfileString("System", "IgnoreLockCheck", "0", Tmp, 255, sIniFile);
	if (_tstoi(Tmp) > 0)
		m_lockcheckmode = 0;

	GetPrivateProfileString("System", "AutoStartMin", "0", Tmp, 255, sIniFile);
	m_autostartmin = _tstoi(Tmp);

	GetPrivateProfileString("System", "LogToFile", "1", Tmp, 255, sIniFile);
	m_logToFile = _tstoi(Tmp);

	GetPrivateProfileString("System", "LogToFile2", "1", Tmp, 255, sIniFile);
	m_logToFile2 = _tstoi(Tmp);

	GetPrivateProfileString("System", "SetLocalFileTime", "0", Tmp, 255, sIniFile);
	m_setlocalfiletime = _tstoi(Tmp);


	GetPrivateProfileString("System", "GenerateJMF", "0", Tmp, 255, sIniFile);
	m_generateJMF = _tstoi(Tmp);

	GetPrivateProfileString("System", "GenerateJMFstatus", "0", Tmp, 255, sIniFile);
	m_generateJMFstatus = _tstoi(Tmp);

	GetPrivateProfileString("System", "JMFFolder", "", m_jmffolder.GetBuffer(MAX_PATH), 255, sIniFile);
	m_jmffolder.ReleaseBuffer();

	GetPrivateProfileString("System", "JMFDeviceName", "", m_jmfdevicename.GetBuffer(MAX_PATH), 255, sIniFile);
	m_jmfdevicename.ReleaseBuffer();
	GetPrivateProfileString("System", "JMFNetworkMethod", "0", Tmp, 255, sIniFile);
	m_jmfnetworkmethod = _tstoi(Tmp);
	GetPrivateProfileString("System", "JMFFTPServer", "", m_jmfftpserver.GetBuffer(MAX_PATH), 255, sIniFile);
	m_jmfftpserver.ReleaseBuffer();
	GetPrivateProfileString("System", "JMFFTPUsername", "", m_jmfftpusername.GetBuffer(MAX_PATH), 255, sIniFile);
	m_jmfftpusername.ReleaseBuffer();
	GetPrivateProfileString("System", "JMFFTPPassword", "", m_jmfftppassword.GetBuffer(MAX_PATH), 255, sIniFile);
	m_jmfftppassword.ReleaseBuffer();
	GetPrivateProfileString("System", "JMFFTPDir", "", m_jmfftpdir.GetBuffer(MAX_PATH), 255, sIniFile);
	m_jmfftpdir.ReleaseBuffer();
	GetPrivateProfileString("System", "JMFFTPAccount", "", m_jmfftpaccount.GetBuffer(MAX_PATH), 255, sIniFile);
	m_jmfftpaccount.ReleaseBuffer();
	GetPrivateProfileString("System", "JMFFTPPort", "21", Tmp, 255, sIniFile);
	m_jmfftpport = _tstoi(Tmp);

	GetPrivateProfileString("System", "JMFFTPPASV", "0", Tmp, 255, sIniFile);
	m_jmfftppasv = _tstoi(Tmp);

	GetPrivateProfileString("System", "JMFErrorsOnly", "0", Tmp, 255, sIniFile);
	m_jmferroronly = _tstoi(Tmp);

	
	GetPrivateProfileString("System", "BypassPingTest", "1", Tmp, 255, sIniFile);
	m_bypasspingtest = _tstoi(Tmp);

	GetPrivateProfileString("System", "BypassReconnect", "0", Tmp, 255, sIniFile);
	m_bypassreconnect = _tstoi(Tmp);

	GetPrivateProfileString("System", "BypassDiskFreeTest", "0", Tmp, 255, sIniFile);
	m_bypassdiskfreeteste = _tstoi(Tmp);

	GetPrivateProfileString("System", "DefaultPolltime", "1", Tmp, 255, sIniFile);
	m_defaultpolltime = _tstoi(Tmp);
	if (m_defaultpolltime == 0)
		m_defaultpolltime = 1;

	GetPrivateProfileString("System", "DefaultStabletime", "3", Tmp, 255, sIniFile);
	m_defaultstabletime = _tstoi(Tmp);
	if (m_defaultstabletime == 0)
		m_defaultstabletime = 2;

	GetPrivateProfileString("System", "DeleteErrorFilesDays", "0", Tmp, 255, sIniFile);
	m_deleteerrorfilesdays = _tstoi(Tmp);

	
	GetPrivateProfileString("System", "KeepConnection", "1", Tmp, 255, sIniFile);
	m_persistentconnection = _tstoi(Tmp);

	GetPrivateProfileString("System", "EventCodeOK", "180", Tmp, 255, sIniFile);
	m_eventcodeOk = _tstoi(Tmp);

	GetPrivateProfileString("System", "EventCodeFail", "186", Tmp, 255, sIniFile);
	m_eventcodeFail = _tstoi(Tmp);

	GetPrivateProfileString("System", "EventCodeWarning", "187", Tmp, 255, sIniFile);
	m_eventcodeWarning = _tstoi(Tmp);

	GetPrivateProfileString("System", "UpdateLog", "0", Tmp, 255, sIniFile);
	m_updatelog = _tstoi(Tmp);



	::GetPrivateProfileString("System", "FireCommandOnFolderReconnect", "0", Tmp, 255, sIniFile);
	m_firecommandonfolderreconnect = _tstoi(Tmp);

	::GetPrivateProfileString("System", "FolderReconnectCommand", "", Tmp, 255, sIniFile);
	m_sFolderReconnectCommand = Tmp;

	::GetPrivateProfileString("System", "FileBuildupSeparateFolder", "0", Tmp, 255, sIniFile);
	m_filebuildupseparatefolder = _tstoi(Tmp);


	::GetPrivateProfileString("System", "WorkFolder", "c:\\temp", Tmp, 255, sIniFile);
	m_workfolder = Tmp;

	::GetPrivateProfileString("System", "WorkFolder2", "c:\\temp2", Tmp, 255, sIniFile);
	m_workfolder2 = Tmp;

	::GetPrivateProfileString("System", "MailOnFileError", "0", Tmp, 255, sIniFile);
	m_emailonfileerror = _tstoi(Tmp);

	::GetPrivateProfileString("System", "MailOnFolderError", "0", Tmp, 255, sIniFile);
	m_emailonfoldererror = _tstoi(Tmp);

	::GetPrivateProfileString("System", "MailSmtpServer", "", Tmp, 255, sIniFile);
	m_emailsmtpserver = Tmp;
	::GetPrivateProfileString("System", "MailSmtpPort", "25", Tmp, 255, sIniFile);
	m_mailsmtpport = _tstoi(Tmp);

	::GetPrivateProfileString("System", "MailSmtpUsername", "", Tmp, 255, sIniFile);
	m_mailsmtpserverusername = Tmp;

	::GetPrivateProfileString("System", "MailSmtpPassword", "", Tmp, 255, sIniFile);
	m_mailsmtpserverpassword = Tmp;

	::GetPrivateProfileString("System", "MailUseSSL", "0", Tmp, 255, sIniFile);
	m_mailusessl = _tstoi(Tmp);

	::GetPrivateProfileString("System", "MailUseStartTLS", "0", Tmp, 255, sIniFile);
	m_mailusestarttls = _tstoi(Tmp);
	
	::GetPrivateProfileString("System", "MailFrom", "", Tmp, 255, sIniFile);
	m_emailfrom = Tmp;
	::GetPrivateProfileString("System", "MailTo", "", Tmp, 255, sIniFile);
	m_emailto = Tmp;
	::GetPrivateProfileString("System", "MailCc", "", Tmp, 255, sIniFile);
	m_emailcc = Tmp;
	::GetPrivateProfileString("System", "MailSubject", "", Tmp, 255, sIniFile);
	m_emailsubject = Tmp;

	::GetPrivateProfileString("System", "MailPreventFlooding", "0", Tmp, 255, sIniFile);
	m_emailpreventflooding = _tstoi(Tmp);

	::GetPrivateProfileString("System", "MainPreventFloodingDelay", "10", Tmp, 255, sIniFile);
	m_emailpreventfloodingdelay = _tstoi(Tmp);

	::GetPrivateProfileString("System", "MaxLogFileSize", "10485760", Tmp, 255, sIniFile);
	m_maxlogfilesize = _tstoi(Tmp);

	::GetPrivateProfileString("System", "FtpTimeout", "30", Tmp, 255, sIniFile);
	m_ftptimeout = _tstoi(Tmp);


	::GetPrivateProfileString("System", "DateFormat", "DDMMYYYY", Tmp, 255, sIniFile);
	m_sDateFormat = Tmp;

	::GetPrivateProfileString("System", "AddSetupNameToAckFile", "1", Tmp, 255, sIniFile);
	m_addsetupnametoackfile = _tstoi(Tmp);

	::GetPrivateProfileString("System", "HeartBeat", "0", Tmp, 255, sIniFile);
	m_heartbeatMS = _tstoi(Tmp);

	::GetPrivateProfileString("System", "WeekDays", "Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday", Tmp, 255, sIniFile);
	m_WeekDaysList = Tmp;


	::GetPrivateProfileString("System", "RecalculateCheckSum", "1", Tmp, 255, sIniFile);
	m_recalculateCheckSum = _tstoi(Tmp);

	::GetPrivateProfileString("System", "MaxTimeExportingMin", "5", Tmp, 255, sIniFile);
	m_maxexportingtimemin = _tstoi(Tmp);

	::GetPrivateProfileString("Message", "GenerateMessage", "0", Tmp, 255, sIniFile);
	m_generatestatusmessage = atoi(Tmp);

	::GetPrivateProfileString("Message", "MessageOnSuccess", "1", Tmp, 255, sIniFile);
	m_messageonsuccess = atoi(Tmp);

	::GetPrivateProfileString("Message", "MessageOnError", "0", Tmp, 255, sIniFile);
	m_messageonerror = atoi(Tmp);


	::GetPrivateProfileString("Message", "OutputFolder", "c:\\temp", Tmp, 255, sIniFile);
	m_messageoutputfolder = Tmp;

	sprintf(Tmp2, "%s\\messagetemplates\\Success.jfm", (LPCSTR)m_apppath);
	::GetPrivateProfileString("Message", "TemplateOnSuccess", Tmp2, Tmp, 255, sIniFile);
	m_messagetemplatesuccess = Tmp;

	sprintf(Tmp2, "%s\\messagetemplates\\Error.jfm", (LPCSTR)m_apppath);
	::GetPrivateProfileString("Message", "TemplateOnError", Tmp2, Tmp, 255, sIniFile);
	m_messagetemplateerror = Tmp;
	
}


BOOL CPrefs::ConnectToDB(BOOL bLoadDBNames, CString &sErrorMessage)
{

	g_connected = m_DBmaint.InitDB(m_DBserver, m_Database, m_DBuser, m_DBpassword, m_IntegratedSecurity, sErrorMessage);

	if (g_connected) {
		if (m_DBmaint.RegisterService(sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: RegisterService() returned - %s", sErrorMessage);
		}
	}
	else
		g_util.Logprintf("ERROR: InitDB() returned - %s", sErrorMessage);

	if (bLoadDBNames) {
		if (m_DBmaint.LoadAllPrefs(sErrorMessage) == FALSE)
			g_util.Logprintf("ERROR: LoadAllPrefs() returned - %s", sErrorMessage);

	}

	return g_connected;
}


BOOL CPrefs::UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError,  CString &sErrorMessage)
{
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.UpdateService(nCurrentState, sCurrentJob, sLastError, sErrorMessage);
	g_dbAccess.Unlock();

	return bSuccess;
}

/*
BOOL CPrefs::UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString sLogData, CString &sErrorMessage)
{
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.UpdateService(nCurrentState, sCurrentJob, sLastError, sLogData, sErrorMessage);
	g_dbAccess.Unlock();

	return bSuccess;
}

BOOL CPrefs::UpdateChannelStatus(int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage)
{
	g_util.Logprintf("INFO: UpdateChannelStatus(MasterNumber=%d,ChannelID=%d,Status=%d,FileName=%s) ", nMasterCopySeparationSet, nChannelID, nStatus, sFileName);
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.UpdateChannelStatus(2, nMasterCopySeparationSet, nProductionID, nChannelID, nStatus, sMessage, sFileName, sErrorMessage);

	g_dbAccess.Unlock();
	g_util.Logprintf("INFO: Done - UpdateChannelStatus(MasterNumber=%d,ChannelID=%d,Status=%d,FileName=%s) ", nMasterCopySeparationSet, nChannelID, nStatus, sFileName);

	return bSuccess;
}

BOOL CPrefs::InsertLogEntry(int nEvent, CString sSetupName, CString sFileName, CString sMessage, int nMasterCopySeparationSet, int nVersion, int nMiscInt, CString sMiscString, CString &sErrorMessage)
{
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.InsertLogEntry(nEvent, sSetupName, sFileName, sMessage, nMasterCopySeparationSet, nVersion, nMiscInt, sMiscString, sErrorMessage);
	g_dbAccess.Unlock();

	return bSuccess;
}

int CPrefs::HasExportingState(int nChannelID, CString &sErrorMessage)
{
	g_dbAccess.Lock();
	int nRet = m_DBmaint.HasExportingState(nChannelID, sErrorMessage);
	g_dbAccess.Unlock();

	return nRet;
}

BOOL CPrefs::ResetExportStatus(int nChannelID, CString &sErrorMessage)
{
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.ResetExportStatus(nChannelID, sErrorMessage);
	g_dbAccess.Unlock();

	return bSuccess;
}


BOOL CPrefs::ReLoadChannelOnOff(int nChannelID, CString &sErrorMessage)
{
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.ReLoadChannelOnOff(nChannelID, sErrorMessage);
	g_dbAccess.Unlock();

	return bSuccess;
}

int	CPrefs::AcuireExportLock(int nChannelID, CString &sErrorMessage)
{
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.AcuireExportLock(nChannelID, sErrorMessage);
	g_dbAccess.Unlock();

	return bSuccess;
}


BOOL CPrefs::ReleaseExportLock(int nChannelID, CString &sErrorMessage)
{
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.ReleaseExportLock(nChannelID, sErrorMessage);
	g_dbAccess.Unlock();
	
	return bSuccess;

}

BOOL CPrefs::GetProductionID(int nMasterCopySeparationSet, int &nProductionID, int &nEditionID, CString &sErrorMessage)
{
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.GetProductionID(nMasterCopySeparationSet, nProductionID, nEditionID, sErrorMessage);
	g_dbAccess.Unlock();

	return bSuccess;
}

BOOL CPrefs::GetMasterCopySeparationListPDF(BOOL bClearList, int nPdfType, int nProductionID, int nEditionID, CUIntArray &aArrMasterCopySeparationSet, CString &sErrorMessage)
{
	g_dbAccess.Lock();
	BOOL bSuccess = m_DBmaint.GetMasterCopySeparationListPDF(bClearList, nPdfType, nProductionID, nEditionID, aArrMasterCopySeparationSet, sErrorMessage);
	g_dbAccess.Unlock();

	return bSuccess;
}
*/

CString CPrefs::GetPublicationName(int nID)
{
	return GetPublicationName(m_DBmaint, nID);
}

PUBLICATIONCHANNELSTRUCT *CPrefs::GetPublicationChannelStruct(int nPublicationID, int nChannelID)
{
	PUBLICATIONSTRUCT *pPub = GetPublicationStruct(nPublicationID);
	if (pPub == NULL)
		return NULL;
	for (int i = 0; i < pPub->m_numberOfChannels; i++) {
		if (pPub->m_channels[i].m_channelID == nChannelID)
			return &pPub->m_channels[i];
	}

	return NULL;
}

CString CPrefs::GetPublicationName(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return m_PublicationList[i].m_name;
	}

	g_dbAccess.Lock();
	CString sPubName = DB.LoadNewPublicationName(nID, sErrorMessage);

	if (sPubName != "") 
		DB.LoadSpecificAlias("Publication", sPubName, sErrorMessage);
	
	g_dbAccess.Unlock();

	return sPubName;
}


void CPrefs::FlushPublicationName(CString sName)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (sName.CompareNoCase(m_PublicationList[i].m_name) == 0) {
			m_PublicationList.RemoveAt(i);
			return;
		}
	}
}

PUBLICATIONSTRUCT *CPrefs::GetPublicationStruct(int nID)
{
	return GetPublicationStruct(m_DBmaint, nID);
}

PUBLICATIONSTRUCT *CPrefs::GetPublicationStructNoDbLookup(int nID)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return &m_PublicationList[i];
	}

	return NULL;
}



PUBLICATIONSTRUCT *CPrefs::GetPublicationStruct(CDatabaseManager &DB, int nID)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return &m_PublicationList[i];
	}

	CString sErrorMessage;

	g_dbAccess.Lock();
	CString sPubName = DB.LoadNewPublicationName(nID, sErrorMessage);

	if (sPubName != "")
		DB.LoadSpecificAlias("Publication", sPubName, sErrorMessage);
	g_dbAccess.Unlock();

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return &m_PublicationList[i];
	}

	return NULL;
}

CString CPrefs::GetExtendedAlias(int nPublicationID)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nPublicationID)
			return m_PublicationList[i].m_extendedalias;
	}

	return GetPublicationName(nPublicationID);
}

CString CPrefs::GetExtendedAlias2(int nPublicationID)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nPublicationID)
			return m_PublicationList[i].m_extendedalias2;
	}

	return GetPublicationName(nPublicationID);
}



void CPrefs::FlushAlias(CString sType, CString sLongName)
{
	for (int i = 0; i < m_AliasList.GetCount(); i++) {
		if (m_AliasList[i].sType == sType && m_AliasList[i].sLongName.CompareNoCase(sLongName) == 0) {
			m_AliasList.RemoveAt(i);
			return;
		}
	}
}


CString CPrefs::GetEditionName(int nID)
{
	return GetEditionName(m_DBmaint, nID);
}

CString CPrefs::GetEditionName(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_EditionList.GetCount(); i++) {
		if (m_EditionList[i].m_ID == nID)
			return m_EditionList[i].m_name;
	}

	g_dbAccess.Lock();

	CString sEdName = DB.LoadNewEditionName(nID, sErrorMessage);

	if (sEdName != "")
		DB.LoadSpecificAlias("Edition", sEdName, sErrorMessage);

	g_dbAccess.Unlock();

	return sEdName;
}

CString CPrefs::GetSectionName(int nID)
{
	return GetSectionName(m_DBmaint, nID);
}

CString CPrefs::GetSectionName(CDatabaseManager &db, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_SectionList.GetCount(); i++) {
		if (m_SectionList[i].m_ID == nID)
			return m_SectionList[i].m_name;
	}

	g_dbAccess.Lock();

	CString sSecName = db.LoadNewSectionName(nID, sErrorMessage);

	if (sSecName != "")
		db.LoadSpecificAlias("Section", sSecName, sErrorMessage);
	g_dbAccess.Unlock();

	return sSecName;
}


CString CPrefs::GetPublisherName(int nID)
{
	return GetPublisherName(m_DBmaint, nID);
}

CString CPrefs::GetPublisherName(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_PublisherList.GetCount(); i++) {
		if (m_PublisherList[i].m_ID == nID)
			return m_PublisherList[i].m_name;
	}
	g_dbAccess.Lock();

	CString sPublisherName = DB.LoadNewPublisherName(nID, sErrorMessage);
	g_dbAccess.Unlock();

	return sPublisherName;
}




CString CPrefs::GetChannelName(int nID)
{
	for (int i = 0; i < m_ChannelList.GetCount(); i++) {
		if (m_ChannelList[i].m_channelID == nID) {
			return m_ChannelList[i].m_channelname;
		}
	}

	CHANNELSTRUCT *p = GetChannelStruct(nID);
	if (p != NULL)
		return p->m_channelname;

	return _T("");
}

CHANNELSTRUCT	*CPrefs::GetChannelStruct(int nID)
{
	return GetChannelStruct(m_DBmaint, nID);
}

CHANNELSTRUCT	*CPrefs::GetChannelStructNoDbLookup(int nID)
{
	for (int i = 0; i < m_ChannelList.GetCount(); i++) {
		if (m_ChannelList[i].m_channelID == nID) {
			return &m_ChannelList[i];
		}
	}

	return NULL;
}

CHANNELSTRUCT	*CPrefs::GetChannelStruct(CDatabaseManager &db, int nID)
{
	CString sErrorMessage = _T("");
	int nIndex = 0;
	for (int i = 0; i < m_ChannelList.GetCount(); i++) {
		if (m_ChannelList[i].m_channelID == nID) {
			return &m_ChannelList[i];
		}
	}

//	g_dbAccess.Lock();

	db.LoadNewChannel(nID, sErrorMessage);
//	g_dbAccess.Unlock();

	for (int i = 0; i < m_ChannelList.GetCount(); i++) {
		if (m_ChannelList[i].m_channelID == nID)
			return &m_ChannelList[i];
	}

	return NULL;
}

/*
CHANNELGROUPSTRUCT	*CPrefs::GetChannelGroupStruct(int nID)
{
	return GetChannelGroupStruct(m_DBmaint, nID);
}
*/

/*
CHANNELGROUPSTRUCT	*CPrefs::GetChannelGroupStruct(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage = _T("");
	int nIndex = 0;
	for (int i = 0; i < m_ChannelGroupList.GetCount(); i++) {
		if (m_ChannelGroupList[i].m_channelgroupID == nID) {
			return &m_ChannelGroupList[i];
		}
	}

	g_dbAccess.Lock();

	DB.LoadNewChannelGroup(nID, sErrorMessage);

	g_dbAccess.Unlock();

	for (int i = 0; i < m_ChannelGroupList.GetCount(); i++) {
		if (m_ChannelGroupList[i].m_channelgroupID == nID) {
			return &m_ChannelGroupList[i];
		}
	}
	return NULL;
}
*/

CString CPrefs::LookupExtendedAliasFromName(CString sLongName)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (sLongName.CompareNoCase(m_PublicationList[i].m_name) == 0 && m_PublicationList[i].m_extendedalias)
			return m_PublicationList[i].m_extendedalias;
	}

	return sLongName;
}

CString CPrefs::LookupOutputAliasFromName(CString sLongName)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (sLongName.CompareNoCase(m_PublicationList[i].m_name) == 0 && m_PublicationList[i].m_outputalias)
			return m_PublicationList[i].m_outputalias;
	}
	
	return sLongName;
}

CString CPrefs::LookupAbbreviationFromName(CString sType, CString sLongName)
{
	if (sType == "Publication")
	{
		for (int i = 0; i < m_PublicationList.GetCount(); i++) {
			if (sLongName.CompareNoCase(m_PublicationList[i].m_name) == 0 && m_PublicationList[i].m_alias)
				return m_PublicationList[i].m_alias;
		}
	}

	for (int i = 0; i < m_AliasList.GetCount(); i++) {
		ALIASSTRUCT a =  m_AliasList[i];
		if (a.sType.CompareNoCase(sType) == 0 && a.sLongName.CompareNoCase(sLongName) == 0)
			return a.sShortName;
	}

	return sLongName;
}

CString CPrefs::LookupNameFromAbbreviation(CString sType, CString sShortName)
{
	//	TCHAR szErrorMessage[MAX_ERRMSG];

	

	for (int i = 0; i < m_AliasList.GetCount(); i++) {
		ALIASSTRUCT a = m_AliasList[i];
		if (a.sType.CompareNoCase(sType) == 0 && a.sShortName.CompareNoCase(sShortName) == 0)
			return a.sLongName;
	}

	return sShortName;
}




BOOL CPrefs::LoadSetup(CHANNELSTRUCT *pChannel)
{
	CString csText = pChannel->m_configdata;

	// Parse
#ifdef MARKUP_MSXML
	CMarkupMSXML xml;
#elif defined( MARKUP_STL )
	CMarkupSTL xml;
#else
	CMarkup xml;
#endif

	// Display results
	if (xml.SetDoc(csText) == FALSE) {
#ifdef MARKUP_STL
		CString csError = xml.GetError().c_str();
#else
		CString csError = xml.GetError();
#endif
		//AfxMessageBox("Error parsing file " + m_setupXMLpath + " - " + csError);
		g_util.Logprintf("Error parsing data %s - %s", csText, csError);
		return FALSE;
	}

	if (xml.FindElem("Configuration") == false) {
		//AfxMessageBox("Error parsing setup definition file " + m_setupXMLpath + " - cannot find 'Configurations' element", MB_ICONSTOP);
		g_util.Logprintf("Error parsing config XML data - 'Configuration' not found");
		return FALSE;
	}	

	CString s = xml.GetAttrib("SetupName");
	if (s == _T("")) {
		g_util.Logprintf("Error parsing config XML dat - 'SetupName' attribute now found");
		return FALSE;
	}

	//pChannel->m_breakifnooutputaccess = _tstoi(xml.GetAttrib("OutputBreakIfNoAccess"));

	//pChannel->m_archivefolder = xml.GetAttrib("ArchiveFolder");
	//pChannel->m_archivefiles = _tstoi(xml.GetAttrib("ArchiveFiles"));
	//pChannel->m_archivefolder2 = xml.GetAttrib("ArchiveFolder2");
	//pChannel->m_archivefiles2 = _tstoi(xml.GetAttrib("ArchiveFiles2"));

	//pChannel->m_copymatchexpression = xml.GetAttrib("ArchiveMatchExpression");
	//pChannel->m_archiveconditionally = _tstoi(xml.GetAttrib("ArchiveConditionally"));


	//pChannel->m_precommand = xml.GetAttrib("PreCommand");
	//pChannel->m_useprecommand = _tstoi(xml.GetAttrib("UsePreCommand"));



	//pChannel->m_moveonerror = _tstoi(xml.GetAttrib("MoveOnError"));
	//pChannel->m_errorfolder = xml.GetAttrib("ErrorFolder");

	//pChannel->m_archivedays = _tstoi(xml.GetAttrib("ArchiveKeepDays"));


	//pChannel->m_useack = _tstoi(xml.GetAttrib("UseStatusAck"));
	//pChannel->m_statusfolder = xml.GetAttrib("StatusFolder");

//	pChannel->m_useregexp = _tstoi(xml.GetAttrib("UseRegularExpression"));
/*	pChannel->m_NumberOfRegExps = _tstoi(xml.GetAttrib("NumberOfRegularExpressions"));
	if (pChannel->m_NumberOfRegExps > MAXREGEXP)
		pChannel->m_NumberOfRegExps = MAXREGEXP;

	for (int i = 0; i< pChannel->m_NumberOfRegExps; i++) {
		CString ss;
		ss.Format("MatchExpression_%d", i + 1);
		pChannel->m_RegExpList[i].m_matchExpression = xml.GetAttrib(ss);
		ss.Format("FormatExpression_%d", i + 1);
		pChannel->m_RegExpList[i].m_formatExpression = xml.GetAttrib(ss);
		ss.Format("PartialExpression_%d", i + 1);
		pChannel->m_RegExpList[i].m_partialmatch = _tstoi(xml.GetAttrib(ss));
	}
*/
	//pChannel->m_usescript = _tstoi(xml.GetAttrib("UseScript"));
	//pChannel->m_script = xml.GetAttrib("Script");


//	pChannel->m_nooverwrite = _tstoi(xml.GetAttrib("NoOverwrite"));
	//pChannel->m_retries = _tstoi(xml.GetAttrib("Retries"));
	//pChannel->m_timeout = _tstoi(xml.GetAttrib("Timeout"));

	//pChannel->m_ensureuniquefilename = _tstoi(xml.GetAttrib("EnsureUniqueFileName"));

//	pChannel->m_filebuildupmethod = _tstoi(xml.GetAttrib("FileBuildupMethod"));

	//pChannel->m_inputemailnotification = _tstoi(xml.GetAttrib("UseEmailNotification"));
	//pChannel->m_inputemailnotificationTo = xml.GetAttrib("EmailNotificationTo");
//	pChannel->m_inputemailnotificationSubject = xml.GetAttrib("EmailNotificationSubject");



	//xml.OutOfElem(); // </Configuration>


	g_lastSetupFileTime = CTime::GetCurrentTime();

	return TRUE;
}





BOOL CPrefs::LoadPreferencesFromRegistry()
{
	CRegistry pReg;

	// Set defaults
	m_logFilePath = _T("c:\\Temp\\");
	m_DBserver = _T(".");
	m_Database = _T("PDFHUB");
	m_DBuser = _T("sa");
	m_DBpassword = _T("infra");
	m_IntegratedSecurity = FALSE;
	m_logToFile = 1;
	m_logToFile2 = 1;
	m_databaselogintimeout = 20;
	m_databasequerytimeout = 10;
	m_nQueryRetries = 20;
	m_QueryBackoffTime = 500;

	if (pReg.OpenKey(CRegistry::localMachine, "Software\\InfraLogic\\ExportService\\Parameters")) {
		CString sVal = _T("");
		DWORD nValue;

		if (pReg.GetValue("InstanceNumber", nValue))
			m_instancenumber = nValue;

		if (pReg.GetValue("IntegratedSecurity", nValue))
			m_IntegratedSecurity = nValue;


		if (pReg.GetValue("DBServer", sVal))
			m_DBserver = sVal;

		if (pReg.GetValue("Database", sVal))
			m_Database = sVal;

		if (pReg.GetValue("DBUser", sVal))
			m_DBuser = sVal;

		if (pReg.GetValue("DBpassword", sVal))
			m_DBpassword = sVal;

		if (pReg.GetValue("DBLoginTimeout", nValue))
			m_databaselogintimeout = nValue;

		if (pReg.GetValue("DBQueryTimeout", nValue))
			m_databasequerytimeout = nValue;

		if (pReg.GetValue("DBQueryRetries", nValue))
			m_nQueryRetries = nValue > 0 ? nValue : 5;

		if (pReg.GetValue("DBQueryBackoffTime", nValue))
			m_QueryBackoffTime = nValue >= 500 ? nValue : 500;

		if (pReg.GetValue("Logging", nValue))
			m_logToFile = nValue;

		if (pReg.GetValue("Logging2", nValue))
			m_logToFile2 = nValue;

		if (pReg.GetValue("LogFileFolder", sVal))
			m_logFilePath = sVal;

		pReg.CloseKey();

		return TRUE;
	}

	return FALSE;
}

CString CPrefs::FormCCFilesName(int nPDFtype, int nMasterCOpySeparationSet,CString sFileName)
{
	CString sPath = _T("");
	if (m_ccdatafilenamemode) {
		if (nPDFtype == POLLINPUTTYPE_LOWRESPDF)
			sPath.Format("%s%s#%d.pdf", m_lowresPDFPath, g_util.GetFileName(sFileName,TRUE), nMasterCOpySeparationSet);
		else if (nPDFtype == POLLINPUTTYPE_HIRESPDF)
			sPath.Format("%s%s#%d.pdf", m_hiresPDFPath, g_util.GetFileName(sFileName, TRUE), nMasterCOpySeparationSet);
		else
			sPath.Format("%s%s#%d.pdf", m_printPDFPath, g_util.GetFileName(sFileName, TRUE), nMasterCOpySeparationSet);

	}
	else {
		if (nPDFtype == POLLINPUTTYPE_LOWRESPDF)
			sPath.Format("%s%d.pdf", m_lowresPDFPath, nMasterCOpySeparationSet);
		else if (nPDFtype == POLLINPUTTYPE_HIRESPDF)
			sPath.Format("%s%d.pdf", m_hiresPDFPath, nMasterCOpySeparationSet);
		else
			sPath.Format("%s%d.pdf", m_printPDFPath, nMasterCOpySeparationSet);
	}
	return sPath;
}
