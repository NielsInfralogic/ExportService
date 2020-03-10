#include "StdAfx.h"
#include "Defs.h"
#include "DatabaseManager.h"
#include "Utils.h"
#include "Prefs.h"
#include "ODBCRecordset.h"

extern CPrefs g_prefs;
extern CUtils g_util;


CDatabaseManager::CDatabaseManager(void)
{
	m_DBopen = FALSE;
	m_pDB = NULL;

	m_DBserver = _T(".");
	m_Database = _T("PDFHUB");
	m_DBuser = "saxxxxxxxxx";
	m_DBpassword = "xxxxxx";
	m_IntegratedSecurity = FALSE;
	m_PersistentConnection = FALSE;

}

CDatabaseManager::~CDatabaseManager(void)
{
	ExitDB();
	if (m_pDB != NULL)
		delete m_pDB;
}

BOOL CDatabaseManager::InitDB(CString sDBserver, CString sDatabase, CString sDBuser, CString sDBpassword, BOOL bIntegratedSecurity, CString &sErrorMessage)
{
	m_DBserver = sDBserver;
	m_Database = sDatabase;
	m_DBuser = sDBuser;
	m_DBpassword = sDBpassword;
	m_IntegratedSecurity = bIntegratedSecurity;

	return InitDB(sErrorMessage);
}


int CDatabaseManager::InitDB(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (m_pDB) {
		if (m_pDB->IsOpen() == FALSE) {
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
		}
	}

	if (!m_PersistentConnection)
		ExitDB();

	if (m_DBopen)
		return TRUE;

	if (m_DBserver == _T("") || m_Database == _T("") || m_DBuser == _T("")) {
		sErrorMessage = _T("Empty server, database or username not allowed");
		return FALSE;
	}

	if (m_pDB == NULL)
		m_pDB = new CDatabase;

	m_pDB->SetLoginTimeout(g_prefs.m_databaselogintimeout);
	m_pDB->SetQueryTimeout(g_prefs.m_databasequerytimeout);

	CString sConnectStr = _T("Driver={SQL Server}; Server=") + m_DBserver + _T("; ") +
		_T("Database=") + m_Database + _T("; ");

	if (m_IntegratedSecurity)
		sConnectStr += _T(" Integrated Security=True;");
	else
		sConnectStr += _T("USER=") + m_DBuser + _T("; PASSWORD=") + m_DBpassword + _T(";");

	try {
		if (!m_pDB->OpenEx((LPCTSTR)sConnectStr, CDatabase::noOdbcDialog)) {
			sErrorMessage.Format(_T("Error connecting to database with connection string '%s'"), (LPCSTR)sConnectStr);
			return FALSE;
		}
	}
	catch (CDBException* e) {
		sErrorMessage.Format(_T("Error connecting to database - %s (%s)"), (LPCSTR)e->m_strError, (LPCSTR)sConnectStr);
		e->Delete();
		return FALSE;
	}

	m_DBopen = TRUE;
	return TRUE;
}

void CDatabaseManager::ExitDB()
{
	if (!m_DBopen)
		return;

	if (m_pDB)
		m_pDB->Close();

	m_DBopen = FALSE;

	return;
}

BOOL CDatabaseManager::IsOpen()
{
	return m_DBopen;
}

//
// SERVICE RELATED METHODS
//

BOOL CDatabaseManager::LoadConfigIniFile(int nInstanceNumber, CString &sFileName, CString &sFileName2, CString &sFileName3, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	sSQL.Format("SELECT ConfigData,ConfigData2,ConfigData3 FROM ServiceConfigurations WHERE ServiceName='ExportService' AND InstanceNumber=%d", nInstanceNumber);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format(_T("Query failed - %s"), (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			sFileName = s.Trim();

			Rs.GetFieldValue((short)1, s);
			sFileName2 = s.Trim();
			
			Rs.GetFieldValue((short)2, s);
			sFileName3 = s.Trim();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format(_T("ERROR (DATABASEMGR): Query failed - %s"), (LPCSTR)e->m_strError);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;

}

BOOL CDatabaseManager::RegisterService(CString &sErrorMessage)
{
	CString sSQL, s;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);

	CString sComputerName = g_util.GetComputerName();

	sSQL.Format("{CALL spRegisterService ('ExportService', %d, 4, '%s',-1,'','','')}", g_prefs.m_instancenumber, sComputerName);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::InsertLogEntry(int nEvent, CString sSource,  CString sFileName, CString sMessage, int nMasterCopySeparationSet, int nVersion, int nMiscInt, CString sMiscString, CString &sErrorMessage)
{
	CString sSQL;
	
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	
	if (sMessage == "")
		sMessage = "Ok";

	sSQL.Format("{CALL [spAddServiceLogEntry] ('%s',%d,%d, '%s','%s', '%s',%d,%d,%d,'%s')}",
		_T("ExportService"), g_prefs.m_instancenumber, nEvent, sSource, sFileName, sMessage, nMasterCopySeparationSet, nVersion, nMiscInt, sMiscString);

	try {
		m_pDB->ExecuteSQL( sSQL );
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString &sErrorMessage)
{
	return UpdateService(nCurrentState, sCurrentJob, sLastError, _T(""), sErrorMessage);
}

BOOL CDatabaseManager::UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage)
{
	CString sSQL;
	CString sComputerName = g_util.GetComputerName();
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	sSQL.Format("{CALL spRegisterService ('ExportService', %d, 4, '%s',%d,'%s','%s','%s')}", g_prefs.m_instancenumber, sComputerName, nCurrentState, sCurrentJob, sLastError, sAddedLogData);

	try {
		m_pDB->ExecuteSQL( sSQL );
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return FALSE;
	}
 
	return TRUE;
}

BOOL CDatabaseManager::LoadSTMPSetup(CString &sErrorMessage)
{
	sErrorMessage = _T("");

	CString sSQL = _T("SELECT TOP 1 SMTPServer,SMTPPort, SMTPUserName,SMTPPassword,UseSSL,SMTPConnectionTimeout,SMTPFrom,SMTPCC,SMTPTo,SMTPSubject FROM SMTPPreferences");
	CString s;
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailsmtpserver);
			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_mailsmtpport = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_mailsmtpserverusername);
			Rs.GetFieldValue((short)fld++, g_prefs.m_mailsmtpserverpassword);
			Rs.GetFieldValue((short)fld++, s);
			int n = atoi(s);
			g_prefs.m_mailusessl = (n & 1) > 0;
			g_prefs.m_mailusestarttls = (n & 2) > 0;

			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_emailtimeout = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_emailfrom);
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailcc);
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailto);
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailsubject);

		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;

}

BOOL CDatabaseManager::LoadAllPrefs(CString &sErrorMessage)
{
	if (LoadGeneralPreferences(sErrorMessage) == FALSE)
		return FALSE;
	
	LoadSTMPSetup(sErrorMessage);

	if (LoadPublicationList(sErrorMessage) == FALSE)
		return FALSE;

	if (LoadSectionNameList(sErrorMessage) == FALSE)
		return FALSE;

	if (LoadEditionNameList(sErrorMessage) == FALSE)
		return FALSE;


	if (LoadFileServerList(sErrorMessage) == FALSE)
		return FALSE;


	if (LoadAliasList(sErrorMessage) == FALSE)
		return FALSE;


	g_prefs.m_StatusList.RemoveAll();
	if (LoadStatusList(_T("StatusCodes"), g_prefs.m_StatusList, sErrorMessage) == FALSE)
		return FALSE;

	g_prefs.m_ExternalStatusList.RemoveAll();
	if (LoadStatusList(_T("ExternalStatusCodes"), g_prefs.m_ExternalStatusList, sErrorMessage) == FALSE)
		return FALSE;


	if (LoadPublisherList(sErrorMessage) == FALSE)
		return FALSE;
	
	if (LoadChannelList(sErrorMessage) == FALSE)
		return FALSE;

	if (LoadPackageList(sErrorMessage) == FALSE)
		return FALSE;


	g_util.Logprintf("INFO: Loaded all name preferences");
	return TRUE;
}

// Returns : -1 error
//			  1 found data
//            0 No data (fatal)
BOOL	CDatabaseManager::LoadGeneralPreferences(CString &sErrorMessage)
{
	CUtils util;
	BOOL foundData = FALSE;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	CString sSQL = _T("SELECT TOP 1 ServerName,ServerShare,ServerFilePath,ServerPreviewPath,ServerThumbnailPath,ServerConfigPath,ServerErrorPath,CopyProofToWeb,WebProofPath,ServerUseCurrentUser,ServerUserName,ServerPassword FROM GeneralPreferences");
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			Rs.GetFieldValue((short)fld++, g_prefs.m_serverName);
			Rs.GetFieldValue((short)fld++, g_prefs.m_serverShare);

			Rs.GetFieldValue((short)fld++, g_prefs.m_hiresPDFPath);
			if (g_prefs.m_hiresPDFPath.Right(1) != "\\")
				g_prefs.m_hiresPDFPath += _T("\\");

			g_prefs.m_lowresPDFPath = g_prefs.m_hiresPDFPath;
			g_prefs.m_lowresPDFPath.MakeUpper();
			g_prefs.m_lowresPDFPath.Replace("CCFILESHIRES", "CCFILESLOWRES");

			g_prefs.m_printPDFPath = g_prefs.m_hiresPDFPath;
			g_prefs.m_printPDFPath.MakeUpper();
			g_prefs.m_printPDFPath.Replace("CCFILESHIRES", "CCFILESPRINT");


			Rs.GetFieldValue((short)fld++, g_prefs.m_previewPath);
			if (g_prefs.m_previewPath.Right(1) != "\\")
				g_prefs.m_previewPath += _T("\\");

			g_prefs.m_readviewpreviewPath = g_prefs.m_previewPath;
			g_prefs.m_readviewpreviewPath.MakeUpper();
			g_prefs.m_readviewpreviewPath.Replace("CCPREVIEWS", "CCreadviewpreviews");

			Rs.GetFieldValue((short)fld++, g_prefs.m_thumbnailPath);
			if (g_prefs.m_thumbnailPath.Right(1) != "\\")
				g_prefs.m_thumbnailPath += _T("\\");

			Rs.GetFieldValue((short)fld++, g_prefs.m_configPath);
			if (g_prefs.m_configPath.Right(1) != "\\")
				g_prefs.m_configPath += _T("\\");
			Rs.GetFieldValue((short)fld++, g_prefs.m_errorPath);
			if (g_prefs.m_errorPath.Right(1) != "\\")
				g_prefs.m_errorPath += _T("\\");


			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_copyToWeb = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_webPath);
			if (g_prefs.m_webPath.Right(1) != "\\")
				g_prefs.m_webPath += _T("\\");

			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_mainserverusecussrentuser = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_mainserverusername);
			Rs.GetFieldValue((short)fld++, g_prefs.m_mainserverpassword);



			foundData = TRUE;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	if (foundData == 0)
		sErrorMessage = _T("FATAL ERROR: No data found in GeneralPrefences table");
	return TRUE;
}

BOOL CDatabaseManager::LoadPackageList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CDBVariant DBvar;
	CString sSQL, s;
	g_prefs.m_PackageList.RemoveAll();

	sSQL.Format("SELECT DISTINCT PackageID,[Name],PublicationID,SectionIndex,Condition,Comment,ConfigChangeTime FROM PackageNames ORDER BY [Name],SectionIndex");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			LogprintfDB("Query failed : %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			int f = 0;
			PACKAGESTRUCT *pItem = new PACKAGESTRUCT;
			Rs.GetFieldValue((short)f++, s);
			pItem->m_packageID = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_name);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_publicationID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_sectionIndex = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_condition = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_comment);
			
			pItem->m_configchangetime = CTime(1975, 1, 1);

			try {
				Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_configchangetime = t;
			}
			catch (...)
			{
			}

			g_prefs.m_PackageList.Add(*pItem);

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}


	return TRUE;

}

BOOL CDatabaseManager::LoadPublicationList(CString &sErrorMessage)
{
	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CDBVariant DBvar;
	CString sSQL, s;
	g_prefs.m_PublicationList.RemoveAll();

	sSQL.Format("SELECT DISTINCT PublicationID,[Name],PageFormatID,TrimToFormat,LatestHour,DefaultProofID,DefaultApprove,DefaultPriority,CustomerID,AutoPurgeKeepDays,EmailRecipient,EmailCC,EmailSubject,EmailBody,UploadFolder,Deadline,AnnumText,AllowUnplanned,ReleaseDays,ReleaseTime,PubdateMove,PubdateMoveDays,InputAlias,OutputALias,ExtendedAlias,PublisherID,ConfigChangeTime,ExtendedAlias2 FROM PublicationNames ORDER BY [Name]");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			LogprintfDB("Query failed : %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			int f = 0;
			PUBLICATIONSTRUCT *pItem = new PUBLICATIONSTRUCT;
			Rs.GetFieldValue((short)f++, s);
			pItem->m_ID = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_name);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_PageFormatID = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_TrimToFormat = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_LatestHour = atof(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_ProofID = atoi(s);
			
			Rs.GetFieldValue((short)f++, s);
			pItem->m_Approve = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_priority = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_customerID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_autoPurgeKeepDays = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_emailrecipient);
			Rs.GetFieldValue((short)f++, pItem->m_emailCC);
			Rs.GetFieldValue((short)f++, pItem->m_emailSubject);
			Rs.GetFieldValue((short)f++, pItem->m_emailBody);
			Rs.GetFieldValue((short)f++, pItem->m_uploadfolder);

			pItem->m_deadline = CTime(1975, 1, 1);

			try {
				Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_deadline = t;
			}
			catch (...)
			{}

			Rs.GetFieldValue((short)f++, pItem->m_annumtext);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_allowunplanned = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_releasedays = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			int n= atoi(s);
			pItem->m_releasetimehour = n / 100;
			pItem->m_releasetimeminute = n - pItem->m_releasetimehour;

			Rs.GetFieldValue((short)f++, s);
			pItem->m_pubdatemove = atoi(s) > 0;

			Rs.GetFieldValue((short)f++, s);
			pItem->m_pubdatemovedays = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_alias);
			Rs.GetFieldValue((short)f++, pItem->m_outputalias);
			Rs.GetFieldValue((short)f++, pItem->m_extendedalias);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_publisherID = atoi(s);

			pItem->m_configchangetime = CTime(1975, 1, 1);

			try {
				Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_configchangetime = t;
			}
			catch (...)
			{
			}
			Rs.GetFieldValue((short)f++, pItem->m_extendedalias2);
			g_prefs.m_PublicationList.Add(*pItem);

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}



	for (int i = 0; i < g_prefs.m_PublicationList.GetCount(); i++)
		if (LoadPublicationChannels(g_prefs.m_PublicationList[i].m_ID, sErrorMessage) == FALSE)
			return FALSE;

	return TRUE;
}

BOOL CDatabaseManager::LoadChannelList(CString &sErrorMessage)
{

	int foundJob = 0;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	g_prefs.m_ChannelList.RemoveAll();
	CRecordset Rs(m_pDB);
	CDBVariant DBvar;

	CString sSQL = _T("SELECT ChannelID,Name,Enabled,OwnerInstance,UseReleaseTime,ReleaseTime,TransmitNameFormat,TransmitNameDateFormat,TransmitNameUseAbbr,TransmitNameOptions,OutputType,FTPServer,FTPPort,FTPUserName,FTPPassword,FTPfolder,FTPEncryption,FTPPasv,FTPXCRC,FTPTimeout,FTPBlockSize,FTPUseTmpFolder,FTPPostCheck,EmailServer,EmailPort,EmailUserName,EmailPassword,EmailFrom,EmailTo,EmailCC,EmailUseSSL,EmailSubject,EmailBody,EmailHTML,EmailTimeout,OutputFolder,UseSpecificUser,UserName,Password,SubFolderNamingConvension,ChannelNameAlias,PDFType,MergedPDF,EditionsToGenerate,SendCommonPages,MiscInt,MiscString,ConfigChangeTime,PDFProcessID,UsePackageNames FROM ChannelNames WHERE ChannelID>0 AND Name<>''  ORDER BY Name");
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return -1;
		}

		while (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			CHANNELSTRUCT *pItem = new CHANNELSTRUCT();

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_channelID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_channelname);

			Rs.GetFieldValue((short)fld++, s); // Enabled
			pItem->m_enabled = atoi(s);

			Rs.GetFieldValue((short)fld++, s); // OwnerInstance
			pItem->m_ownerinstance = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_usereleasetime = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			int n = atoi(s);
			pItem->m_releasetimehour = n / 100;
			pItem->m_releasetimeminute = n - pItem->m_releasetimehour;

		//	Rs.GetFieldValue((short)fld++, s);
	//		pItem->m_channelgroupID = atoi(s);

		//	Rs.GetFieldValue((short)fld++, s);
		//	pItem->m_publisherID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_transmitnameconv);

			Rs.GetFieldValue((short)fld++, pItem->m_transmitdateformat);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_transmitnameuseabbr = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_transmitnameoptions = atoi(s);

			Rs.GetFieldValue((short)fld++, s); // OutputType
			pItem->m_outputuseftp = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpserver);
			Rs.GetFieldValue((short)fld++, s); // FTPPort
			pItem->m_outputftpport = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpuser);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftppw);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpfolder);
			Rs.GetFieldValue((short)fld++, s); // FTPEncryption
			pItem->m_outputfpttls = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPPasv
			pItem->m_outputftppasv = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPXCRC
			pItem->m_outputftpXCRC = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPTimeout
			pItem->m_ftptimeout = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPBlockSize
			pItem->m_outputftpbuffersize = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftptempfolder);
			Rs.GetFieldValue((short)fld++, s); // FTPPostcheck
			pItem->m_FTPpostcheck = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserver);
			Rs.GetFieldValue((short)fld++, s); // EmailPort
			pItem->m_emailsmtpport = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserverusername);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserverpassword);
			Rs.GetFieldValue((short)fld++, pItem->m_emailfrom);
			Rs.GetFieldValue((short)fld++, pItem->m_emailto);
			Rs.GetFieldValue((short)fld++, pItem->m_emailcc);
			Rs.GetFieldValue((short)fld++, s); // EmailUseSSL
			n = atoi(s);
			pItem->m_emailusessl = (n & 1) > 0 ;
			pItem->m_emailusestartTLS = (n & 2) > 0;
			Rs.GetFieldValue((short)fld++, pItem->m_emailsubject);
			Rs.GetFieldValue((short)fld++, pItem->m_emailbody);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_emailusehtml = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // emailtimeout
			pItem->m_emailtimeout = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputfolder);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_outputusecurrentuser = (atoi(s) ? FALSE : TRUE);
			Rs.GetFieldValue((short)fld++, pItem->m_outputusername);
			Rs.GetFieldValue((short)fld++, pItem->m_outputpassword);

			Rs.GetFieldValue((short)fld++, pItem->m_subfoldernameconv);
			Rs.GetFieldValue((short)fld++, pItem->m_channelnamealias);

			Rs.GetFieldValue((short)fld++, s); 
			pItem->m_pdftype = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_mergePDF = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_editionstogenerate = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_sendCommonPages = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_miscint = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_miscstring);

			pItem->m_configchangetime = CTime(1975, 1, 1);

			try {
				Rs.GetFieldValue((short)fld++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_configchangetime = t;
			}
			catch (...)
			{
			}

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_pdfprocessID = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_usepackagenames = atoi(s);

			g_prefs.m_ChannelList.Add(*pItem);

			foundJob++;

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	for (int i = 0; i < g_prefs.m_ChannelList.GetCount(); i++) {
		CString sConfigData = _T("");
		g_prefs.m_ChannelList[i].m_configdata = _T("");
		if (LoadChannelConfigData(g_prefs.m_ChannelList[i].m_channelID, sConfigData, sErrorMessage) == FALSE)
			return FALSE;
		g_prefs.m_ChannelList[i].m_configdata = sConfigData;

		if (LoadExpressionList(&g_prefs.m_ChannelList[i], sErrorMessage) == FALSE)
			return FALSE;

	}
	return TRUE;
}



BOOL CDatabaseManager::LoadExpressionList(CHANNELSTRUCT *pChannel, CString  &sErrorMessage)
{
	CString sSQL;

	int foundJob = 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	if (pChannel == NULL)
		return FALSE;

	sSQL.Format("SELECT UseExpression,MatchExpression,FormatExpression,PartialMatch,Rank FROM ChannelRegularExpressions WHERE ChannelID=%d ORDER BY Rank", pChannel->m_channelID);

	pChannel->m_useregexp = FALSE;
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}
		pChannel->m_NumberOfRegExps = 0;
		while (!Rs.IsEOF()) {
			CString s;
			int fld = 0;

			Rs.GetFieldValue((short)fld++, s);
			pChannel->m_RegExpList[pChannel->m_NumberOfRegExps].m_useExpression = atoi(s);
			if (pChannel->m_RegExpList[pChannel->m_NumberOfRegExps].m_useExpression)
				pChannel->m_useregexp = TRUE;
			Rs.GetFieldValue((short)fld++, pChannel->m_RegExpList[pChannel->m_NumberOfRegExps].m_matchExpression);
			Rs.GetFieldValue((short)fld++, pChannel->m_RegExpList[pChannel->m_NumberOfRegExps].m_formatExpression);
			Rs.GetFieldValue((short)fld++, s);
			pChannel->m_RegExpList[pChannel->m_NumberOfRegExps].m_partialmatch = atoi(s);
			pChannel->m_NumberOfRegExps++;
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::LoadChannelConfigData(int nChannelID, CString &sConfigData, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CODBCRecordset Rs(m_pDB);
	int nSize = 0;
	TCHAR *szData = NULL;

	sConfigData = _T("");

	CString sSQL;
	sSQL.Format("SELECT TOP 1 ConfigFile FROM ChannelNames WHERE ChannelID=%d", nChannelID);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CLongBinary *d1 = Rs.GetBinary(0);
			if (d1->m_dwDataLength > 0) {
				szData = new TCHAR[d1->m_dwDataLength + 2];
				HGLOBAL hGlobal = d1->m_hData;
				LPVOID pData = GlobalLock(hGlobal);
				::memcpy(szData, (TCHAR*)pData, d1->m_dwDataLength);
				::GlobalUnlock(hGlobal);
				szData[d1->m_dwDataLength] = 0;
				sConfigData = szData;
				delete szData;
			}
		}
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::LoadPublisherList(CString &sErrorMessage)
{
	int foundJob = 0;

	//if (g_prefs.m_DBcapabilities_ChannelGroupNames == FALSE)
	//	return 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	g_prefs.m_PublisherList.RemoveAll();
	CRecordset Rs(m_pDB);

	CString sSQL = _T("SELECT PublisherID,PublisherName FROM PublisherNames WHERE PublisherID>0 AND PublisherName<>''  ORDER BY PublisherName");
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			ITEMSTRUCT *pItem = new ITEMSTRUCT();

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_ID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_name);

			g_prefs.m_PublisherList.Add(*pItem);
			foundJob++;

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return -1;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::LoadPublicationChannels(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	PUBLICATIONSTRUCT *pItem = g_prefs.GetPublicationStructNoDbLookup(nID);
	if (pItem == NULL)
		return FALSE;

	pItem->m_numberOfChannels = 0;

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	CString sSQL, s;

	CString sList = _T("");
	sSQL.Format("SELECT DISTINCT ChannelID,ISNULL(PushTrigger,0),ISNULL(PubDateMoveDays,0),ISNULL(ReleaseDelay,0),ISNULL(SendPlan,0) FROM PublicationChannels WHERE PublicationID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			pItem->m_channels[pItem->m_numberOfChannels].m_channelID = atoi(s);
			Rs.GetFieldValue((short)1, s);
			pItem->m_channels[pItem->m_numberOfChannels].m_pushtrigger = atoi(s);
			Rs.GetFieldValue((short)2, s);
			pItem->m_channels[pItem->m_numberOfChannels].m_pubdatemovedays = atoi(s);
			Rs.GetFieldValue((short)3, s);
			pItem->m_channels[pItem->m_numberOfChannels].m_releasedelay = atoi(s);
			Rs.GetFieldValue((short)4, s);
			pItem->m_channels[pItem->m_numberOfChannels].m_sendplan = atoi(s);
			
			Rs.MoveNext();
		}
		
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

CString CDatabaseManager::LoadNewPublicationName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return 0;

	CRecordset Rs(m_pDB);
	CDBVariant DBvar;

	CString sSQL, s;
	CString sName = _T("");
	PUBLICATIONSTRUCT *pItem = g_prefs.GetPublicationStructNoDbLookup(nID);
	BOOL bNewEntry = pItem == NULL;

	sSQL.Format("SELECT DISTINCT [Name], PageFormatID, TrimToFormat, LatestHour, DefaultProofID, DefaultApprove, DefaultPriority, CustomerID, AutoPurgeKeepDays, EmailRecipient, EmailCC, EmailSubject, EmailBody, UploadFolder, Deadline, AnnumText, AllowUnplanned, ReleaseDays, ReleaseTime, PubdateMove, PubdateMoveDays, InputAlias, OutputALias,ExtendedAlias,PublisherID,ConfigChangeTime,ExtendedAlias2 FROM PublicationNames WHERE PublicationID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return _T("");
		}

		if (!Rs.IsEOF()) {
			int f = 0;
			if (bNewEntry)
				pItem = new PUBLICATIONSTRUCT();
			pItem->m_ID = nID;

			Rs.GetFieldValue((short)f++, pItem->m_name);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_PageFormatID = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_TrimToFormat = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_LatestHour = atof(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_ProofID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_Approve = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_priority = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_customerID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_autoPurgeKeepDays = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_emailrecipient);
			Rs.GetFieldValue((short)f++, pItem->m_emailCC);
			Rs.GetFieldValue((short)f++, pItem->m_emailSubject);
			Rs.GetFieldValue((short)f++, pItem->m_emailBody);
			Rs.GetFieldValue((short)f++, pItem->m_uploadfolder);

			pItem->m_deadline = CTime(1975, 1, 1);

			try {
				Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_deadline = t;
			}
			catch (...)
			{
			}

			Rs.GetFieldValue((short)f++, pItem->m_annumtext);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_allowunplanned = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_releasedays = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			int n = atoi(s);
			pItem->m_releasetimehour = n / 100;
			pItem->m_releasetimeminute = n - pItem->m_releasetimehour;

			Rs.GetFieldValue((short)f++, s);
			pItem->m_pubdatemove = atoi(s) > 0;

			Rs.GetFieldValue((short)f++, s);
			pItem->m_pubdatemovedays = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_alias);
			Rs.GetFieldValue((short)f++, pItem->m_outputalias);
			Rs.GetFieldValue((short)f++, pItem->m_extendedalias);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_publisherID = atoi(s);

			pItem->m_configchangetime = CTime(1975, 1, 1);

			try {
				Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_configchangetime = t;
			}
			catch (...)
			{
			}
			Rs.GetFieldValue((short)f++, pItem->m_extendedalias2);
			
			if (bNewEntry)
				g_prefs.m_PublicationList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return _T("");
	}

	LoadPublicationChannels(nID, sErrorMessage);

	return sName;
}

CString CDatabaseManager::LoadNewEditionName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T( "");
	if (InitDB(sErrorMessage) == FALSE)
		return "";

	CRecordset Rs(m_pDB);

	CString sSQL;
	CString sEd = "";
	sSQL.Format("SELECT Name FROM EditionNames WHERE EditionID=%d", nID);


	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return "";
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, sEd);
			pItem->m_ID = nID;
			pItem->m_name = sEd;		
			g_prefs.m_EditionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return "";
	}

	return sEd;
}

CString CDatabaseManager::LoadNewSectionName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return "";

	CRecordset Rs(m_pDB);

	CString sSQL;
	CString sEd = "";
	sSQL.Format("SELECT Name FROM SectionNames WHERE SectionID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return "";
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, sEd);
			pItem->m_ID = nID;
			pItem->m_name = sEd;
			g_prefs.m_SectionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return "";
	}

	return sEd;
}

CString CDatabaseManager::LoadNewPublisherName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return "";

	CRecordset Rs(m_pDB);

	CString sSQL;
	CString sName = "";
	sSQL.Format("SELECT PublisherName FROM PublisherNames WHERE PublisherID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return "";
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, sName);
			pItem->m_ID = nID;
			pItem->m_name = sName;
			g_prefs.m_PublisherList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return "";
	}

	return sName;
}

BOOL CDatabaseManager::LoadEditionNameList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_EditionList.RemoveAll();
	CString sSQL;
	CString s;

	sSQL = _T("SELECT EditionID,Name FROM EditionNames");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT;
			Rs.GetFieldValue((short)0, s);
			pItem->m_ID = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_name);
			g_prefs.m_EditionList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::LoadSectionNameList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_SectionList.RemoveAll();
	CString sSQL;
	CString s;

	sSQL = _T("SELECT SectionID,Name FROM SectionNames");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT;
			Rs.GetFieldValue((short)0, s);
			pItem->m_ID = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_name);
			g_prefs.m_SectionList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::LoadFileServerList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	CString sSQL, s;

	sSQL.Format("SELECT [Name],ServerType,CCdatashare,Username,password,IP,Locationid,uselocaluser FROM FileServers ORDER BY ServerType");

	g_prefs.m_FileServerList.RemoveAll();
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			FILESERVERSTRUCT *pItem = new FILESERVERSTRUCT();

			Rs.GetFieldValue((short)0, pItem->m_servername);

			Rs.GetFieldValue((short)1, s);
			pItem->m_servertype = atoi(s);

			Rs.GetFieldValue((short)2, pItem->m_sharename);

			Rs.GetFieldValue((short)3, pItem->m_username);

			Rs.GetFieldValue((short)4, pItem->m_password);

			Rs.GetFieldValue((short)5, pItem->m_IP);

			pItem->m_IP.Trim();

			Rs.GetFieldValue((short)6, s);
			pItem->m_locationID = atoi(s);

			Rs.GetFieldValue((short)7, s);
			pItem->m_uselocaluser = atoi(s);

			g_prefs.m_FileServerList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::LoadStatusList(CString sIDtable, STATUSLIST &v, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	sSQL.Format("SELECT * FROM %s", sIDtable);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			STATUSSTRUCT *pItem = new STATUSSTRUCT;
			CString s;
			Rs.GetFieldValue((short)0, s);
			pItem->m_statusnumber = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_statusname);
			Rs.GetFieldValue((short)2, pItem->m_statuscolor);
			v.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::LoadAliasList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_AliasList.RemoveAll();

	CString sSQL;
	sSQL = _T("SELECT Type,Longname,Shortname FROM InputAliases");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			ALIASSTRUCT *pItem = new ALIASSTRUCT();
			CString s;
			Rs.GetFieldValue((short)0, s);
			pItem->sType = s;
			Rs.GetFieldValue((short)1, s);
			pItem->sLongName = s;
			Rs.GetFieldValue((short)2, s);
			pItem->sShortName = s;
			g_prefs.m_AliasList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

CString CDatabaseManager::LoadSpecificAlias(CString sType, CString sLongName, CString &sErrorMessage)
{

	CString sShortName = sLongName;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return sShortName;

	CRecordset Rs(m_pDB);

	CString sSQL;
	sSQL.Format("SELECT Shortname FROM InputAliases WHERE Type='%s' AND LongName='%s'", sType, sLongName);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, sShortName);

			ALIASSTRUCT *pItem = new ALIASSTRUCT();
			pItem->sType = sType;
			pItem->sLongName = sLongName;
			pItem->sShortName = sShortName;
			g_prefs.FlushAlias(sType, sLongName);
			g_prefs.m_AliasList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return sShortName;
	}
	return sShortName;
}

BOOL CDatabaseManager::LoadNewChannel(int nChannelID, CString &sErrorMessage)
{
	
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CHANNELSTRUCT *pItem = g_prefs.GetChannelStructNoDbLookup(nChannelID);

	BOOL bNewChannel = (pItem == NULL);

	if (bNewChannel) {
		pItem = new CHANNELSTRUCT();
		pItem->m_channelID = nChannelID;
	}


	CRecordset Rs(m_pDB);
	CString sSQL, s;
	CDBVariant DBvar;

	sSQL.Format("SELECT ChannelID,Name,Enabled,OwnerInstance,UseReleaseTime,ReleaseTime,TransmitNameFormat,TransmitNameDateFormat,TransmitNameUseAbbr,TransmitNameOptions,OutputType,FTPServer,FTPPort,FTPUserName,FTPPassword,FTPfolder,FTPEncryption,FTPPasv,FTPXCRC,FTPTimeout,FTPBlockSize,FTPUseTmpFolder,FTPPostCheck,EmailServer,EmailPort,EmailUserName,EmailPassword,EmailFrom,EmailTo,EmailCC,EmailUseSSL,EmailSubject,EmailBody,EmailHTML,EmailTimeout,OutputFolder,UseSpecificUser,UserName,Password,SubFolderNamingConvension,ChannelNameAlias,PDFType,MergedPDF,EditionsToGenerate,SendCommonPages,MiscInt,MiscString,ConfigChangeTime,PDFProcessID,UsePackageNames FROM ChannelNames WHERE ChannelID=%d", nChannelID);
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			
			int fld = 0;

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_channelID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_channelname);

			Rs.GetFieldValue((short)fld++, s); // Enabled
			pItem->m_enabled = atoi(s);

			Rs.GetFieldValue((short)fld++, s); // OwnerInstance
			pItem->m_ownerinstance = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_usereleasetime = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			int n = atoi(s);
			pItem->m_releasetimehour = n / 100;
			pItem->m_releasetimeminute = n - pItem->m_releasetimehour;

			//Rs.GetFieldValue((short)fld++, s);
			//pItem->m_channelgroupID = atoi(s);

			//Rs.GetFieldValue((short)fld++, s);
			//pItem->m_publisherID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_transmitnameconv);

			Rs.GetFieldValue((short)fld++, pItem->m_transmitdateformat);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_transmitnameuseabbr = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_transmitnameoptions = atoi(s);

			Rs.GetFieldValue((short)fld++, s); // OutputType
			pItem->m_outputuseftp = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpserver);
			Rs.GetFieldValue((short)fld++, s); // FTPPort
			pItem->m_outputftpport = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpuser);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftppw);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpfolder);
			Rs.GetFieldValue((short)fld++, s); // FTPEncryption
			pItem->m_outputfpttls = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPPasv
			pItem->m_outputftppasv = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPXCRC
			pItem->m_outputftpXCRC = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPTimeout
			pItem->m_ftptimeout = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPBlockSize
			pItem->m_outputftpbuffersize = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftptempfolder);
			Rs.GetFieldValue((short)fld++, s); // FTPPostcheck
			pItem->m_FTPpostcheck = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserver);
			Rs.GetFieldValue((short)fld++, s); // EmailPort
			pItem->m_emailsmtpport = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserverusername);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserverpassword);
			Rs.GetFieldValue((short)fld++, pItem->m_emailfrom);
			Rs.GetFieldValue((short)fld++, pItem->m_emailto);
			Rs.GetFieldValue((short)fld++, pItem->m_emailcc);
			Rs.GetFieldValue((short)fld++, s); // EmailUseSSL
			pItem->m_emailusessl = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsubject);
			Rs.GetFieldValue((short)fld++, pItem->m_emailbody);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_emailusehtml = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_emailtimeout = atoi(s);
		
			Rs.GetFieldValue((short)fld++, pItem->m_outputfolder);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_outputusecurrentuser = (atoi(s) ? FALSE : TRUE);
			Rs.GetFieldValue((short)fld++, pItem->m_outputusername);
			Rs.GetFieldValue((short)fld++, pItem->m_outputpassword);

			Rs.GetFieldValue((short)fld++, pItem->m_subfoldernameconv);
			Rs.GetFieldValue((short)fld++, pItem->m_channelnamealias);

			Rs.GetFieldValue((short)fld++, s); 
			pItem->m_pdftype = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_editionstogenerate = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_sendCommonPages = atoi(s);

			Rs.GetFieldValue((short)fld++, s); 
			pItem->m_mergePDF = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_miscint = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_miscstring);

			pItem->m_configchangetime = CTime(1975, 1, 1);

			try {
				Rs.GetFieldValue((short)fld++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_configchangetime = t;
			}
			catch (...)
			{
			}

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_pdfprocessID = atoi(s); // PDFProcessID

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_usepackagenames = atoi(s); 


		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return -1;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	if (pItem->m_channelID > 0) {
		if (LoadExpressionList(pItem, sErrorMessage) == FALSE)
			return FALSE;
		CString sConfigData = _T("");
		if (LoadChannelConfigData(nChannelID, sConfigData, sErrorMessage) == FALSE)
			return FALSE;
		pItem->m_configdata = sConfigData;
		if (bNewChannel) 
			g_prefs.m_ChannelList.Add(*pItem);
	}

	

	return TRUE;
}



BOOL CDatabaseManager::LookupPackageName(int &nPublicationID, int &nSectionID, CTime tPubDate, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	sSQL.Format("{CALL spOutputPackageName (%d,%d,'%s')}", nPublicationID, nSectionID, TranslateDate(tPubDate));

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nPublicationID = atoi(s);
			Rs.GetFieldValue((short)1, s);
			nSectionID = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

// Transmissionthread
extern NextTransmitJob TxJob[MAXTRANSMITQUEUE];
/////////////////////////////////////////////////////////////////////////////////
// Transmit thread query functions
//
// LookupTransmitJob() calls the following SP:
//	
// IF @TransmitAfterProof=0 AND @TransmitAfterApproval=0
//		SELECT TOP 10  P.MasterCopySeparationSet, P.ColorID,P.PublicationID, P.SectionID, P.EditionID, P.IssueID, P.PageName
//			FROM PageTable P 
//			WHERE P.ACTIVE=1 AND P.Status=20 AND LocationID=@LocationID
//	ELSE IF @TransmitAfterProof=1 AND @TransmitAfterApproval=0
//		SELECT  TOP 10  P.MasterCopySeparationSet, P.ColorID,P.PublicationID, P.SectionID, P.EditionID, P.IssueID, P.PageName
//			FROM PageTable P 
//			WHERE P.ACTIVE=1 AND P.Status=20 AND LocationID=@LocationID AND ProofStatus=10
//	ELSE IF @TransmitAfterProof=0 AND @TransmitAfterApproval=1
//		SELECT  TOP 10  P.MasterCopySeparationSet, P.ColorID,P.PublicationID, P.SectionID, P.EditionID, P.IssueID, P.PageName
//			FROM PageTable P 
//			WHERE P.ACTIVE=1 AND P.Status=20 AND LocationID=@LocationID AND (Approved = 1 OR Approved = -1)
//	ELSE IF @TransmitAfterProof=1 AND @TransmitAfterApproval=1
//		SELECT  TOP 10  P.MasterCopySeparationSet, P.ColorID,P.PublicationID, P.SectionID, P.EditionID, P.IssueID, P.PageName
//			FROM PageTable P 
//			WHERE P.ACTIVE=1 AND P.Status=20 AND LocationID=@LocationID  AND ProofStatus=10 AND (Approved = 1 OR Approved = -1)
//
/////////////////////////////////////////////////////////////////////////////////

int CDatabaseManager::LookupTransmitJob(BOOL bTransmitAfterApproval, BOOL bTransmitAfterProof, BOOL bIncludeErrorRetry, BOOL bExcludeMergePDF, CString &sErrorMessage)
{
	CUtils util;
	CString sSQL, s;

	sSQL.Format("{CALL spTransmitLookupNextJob (%d,%d,%d,%d,%d)}", 1, bTransmitAfterProof, bTransmitAfterApproval, bIncludeErrorRetry, bExcludeMergePDF);

	CString sJobName;
	int foundJob = 0;
	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;

	sErrorMessage = _T("");

	int nRetries = g_prefs.m_nQueryRetries;
	if (g_prefs.m_logToFile > 2)
		util.LogprintfTransmit("INFO: Looking for transmit job using query %s...", sSQL);
	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			int nFields = Rs->GetODBCFieldCount();

			while (!Rs->IsEOF()) {
				int fld = 0;
				Rs->GetFieldValue((short)fld++, s);
				TxJob[foundJob].m_mastercopyseparationset = _tstoi(s);
				Rs->GetFieldValue((short)fld++, s);
				TxJob[foundJob].m_copyseparationset = _tstoi(s);
			//	TxJob[foundJob].m_colorID = 5;
				Rs->GetFieldValue((short)fld++, s); // Approved
				int nApp = atoi(s);
				TxJob[foundJob].m_approved = (nApp == 1 || nApp == -1) ? TRUE : FALSE;
				Rs->GetFieldValue((short)fld++, s);
				TxJob[foundJob].m_publicationID = atoi(s);
				sJobName = g_prefs.GetPublicationName(TxJob[foundJob].m_publicationID);

				Rs->GetFieldValue((short)fld++, s);
				TxJob[foundJob].m_version = atoi(s);
				
				//sJobName += _T("-") + g_prefs.GetRunName(atoi(s));
				Rs->GetFieldValue((short)fld++, s);
				sJobName += _T("-") + g_prefs.GetEditionName(atoi(s));

				Rs->GetFieldValue((short)fld++, s);
				sJobName += _T("-") + g_prefs.GetSectionName(atoi(s));

				Rs->GetFieldValue((short)fld++, s);

				sJobName += _T("-") + s;// + _T("-") + g_prefs.GetColorName(TxJob[foundJob][nLocIndex].m_colorID);
				TxJob[foundJob].m_jobname = sJobName;

				Rs->GetFieldValue((short)fld++, s);
				TxJob[foundJob].m_inputID = atoi(s);

				Rs->GetFieldValue((short)fld++, s); // Status
				TxJob[foundJob].m_status = atoi(s);
				
				Rs->GetFieldValue((short)fld++, s); // Filename
				TxJob[foundJob].m_filename = s;
				if (s != "")
					TxJob[foundJob].m_jobname = s; //Filename instead of constructed job name

				Rs->GetFieldValue((short)fld++, s); // ChannelID
				TxJob[foundJob].m_channelID = atoi(s);

				Rs->GetFieldValue((short)fld++, s); // PDFType
				TxJob[foundJob].m_pdftype = atoi(s);
				
				TxJob[foundJob].m_productionID = 0;
			//	Rs->GetFieldValue((short)fld++, s); // ProductionID
			//	TxJob[foundJob].m_productionID = atoi(s);

				CString sPDFFileName = g_prefs.FormCCFilesName(TxJob[foundJob].m_pdftype, TxJob[foundJob].m_mastercopyseparationset, TxJob[foundJob].m_filename);
				TxJob[foundJob].m_testfileexists = g_util.FileExist(sPDFFileName);
				if (TxJob[foundJob].m_testfileexists) {
					//if (g_prefs.m_logToFile > 2)
					util.LogprintfTransmit("INFO: File ready for TX: %s ChannelID %d OK (PDFType=%d, DB approve=%d, status=%d, filename=%s)", sPDFFileName, TxJob[foundJob].m_channelID, TxJob[foundJob].m_pdftype , TxJob[foundJob].m_approved, TxJob[foundJob].m_status, TxJob[foundJob].m_filename);
				}
				else {

					util.LogprintfTransmit("ERROR: File %s ChannelID %d not found (PDFType=%d, DB approve=%d, status=%d, filename=%s)", sPDFFileName, TxJob[foundJob].m_channelID, TxJob[foundJob].m_pdftype , TxJob[foundJob].m_approved, TxJob[foundJob].m_status, TxJob[foundJob].m_filename);
				}
				foundJob++;


				if (foundJob >= MAXTRANSMITQUEUE - 2)
					break;
				Rs->MoveNext();
			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}
	//	util.LogprintfTransmit("Found %d transmit job for locatioID=%d",foundJob, nLocationID);

	if (Rs != NULL)
		delete Rs;

	return bSuccess ? foundJob : -1;
}


BOOL CDatabaseManager::GetChannelConfigChangeTime(int nChannelID, CTime &tChangeTime, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	CHANNELSTRUCT *pChannel = g_prefs.GetChannelStructNoDbLookup(nChannelID);
	if (pChannel == NULL)
		return FALSE;

	tChangeTime = CTime(1975, 1, 1);

	
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CDBVariant DBvar;
	CRecordset Rs(m_pDB);
	CString sSQL;

	sSQL.Format("SELECT [ConfigChangeTime] FROM ChannelNames WHERE ChannelID=%d", nChannelID);
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {

			try {
				Rs.GetFieldValue((short)0, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				tChangeTime = t;
			}
			catch (...)
			{
			}
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::GetPublicationConfigChangeTime(int nPublicationID, CTime &tChangeTime, CString &sErrorMessage)
{

	PUBLICATIONSTRUCT *pPub = g_prefs.GetPublicationStructNoDbLookup(nPublicationID);
	if (pPub == NULL)
		return FALSE;

	tChangeTime = CTime(1975, 1, 1);

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CDBVariant DBvar;
	CRecordset Rs(m_pDB);
	CString sSQL;

	sSQL.Format("SELECT [ConfigChangeTime] FROM PublicationNames WHERE PublicationID=%d", pPub);
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {

			try {
				Rs.GetFieldValue((short)0, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				tChangeTime = t;
			}
			catch (...)
			{
			}
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;

}




BOOL CDatabaseManager::ReLoadChannelOnOff(int nChannelID, CString &sErrorMessage)
{
	int foundJob = 0;


	CHANNELSTRUCT *pChannel = g_prefs.GetChannelStructNoDbLookup(nChannelID);
	if (pChannel == NULL)
		return FALSE;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);


	CString sSQL;
	sSQL.Format("SELECT Enabled,OwnerInstance FROM ChannelNames WHERE ChannelID=%d", nChannelID);
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;

			Rs.GetFieldValue((short)0, s);
			pChannel->m_enabled = atoi(s);

			Rs.GetFieldValue((short)1, s);
			pChannel->m_ownerinstance = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::GetJobDetails(CJobAttributes *pJob, int nCopySeparationSet,  CString &sErrorMessage)
{
	CUtils util;
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 PublicationID,PubDate,EditionID,SectionID,PageName,Pagination,PageIndex,ColorID,Version,Comment,MiscString1,MiscString2,MiscString3,FileName,LocationID,ProductionID FROM PageTable WITH (NOLOCK) WHERE CopySeparationSet=%d AND Dirty=0 ORDER BY PageIndex", nCopySeparationSet);


	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;

	pJob->m_copyflatseparationset = nCopySeparationSet;

	sErrorMessage = _T("");
	int nRetries = g_prefs.m_nQueryRetries;

	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			int nFields = Rs->GetODBCFieldCount();

			if (!Rs->IsEOF()) {
				int fld = 0;
				Rs->GetFieldValue((short)fld++, s);
				pJob->m_publicationID = atoi(s);


				CDBVariant DBvar;
				try {
					Rs->GetFieldValue((short)fld++, DBvar, SQL_C_TIMESTAMP);
					CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
					pJob->m_pubdate = t;
				}
				catch (CException *ex)
				{
				}

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_editionID = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_sectionID = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_pagename = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_pagination = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_pageindex = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_colorname = _T("PDF");		// HARDCODED

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_version = atoi(s);


				Rs->GetFieldValue((short)fld++, s);
				pJob->m_comment = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_miscstring1 = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_miscstring2 = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_miscstring3 = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_filename = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_locationID = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_productionID = atoi(s);
			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}

BOOL CDatabaseManager::GetProductionID(int nMasterCopySeparationSet, int &nProductionID, int &nEditionID, CString &sErrorMessage)
{
	CString sSQL, s;

	sErrorMessage = _T("");
	nProductionID = 0;
	nEditionID = 0;

	sSQL.Format("SELECT TOP 1 ProductionID,EditionID FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d AND UniquePage=1 AND Dirty=0", nMasterCopySeparationSet);

	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;

	int nRetries = g_prefs.m_nQueryRetries;

	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				nProductionID = atoi(s);

				Rs->GetFieldValue((short)1, s);
				nEditionID = atoi(s);

			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}

BOOL CDatabaseManager::GetProductionID(int nPublicationID, CTime tPubDate,  int &nProductionID, int &nEditionID, CString &sErrorMessage)
{
	CString sSQL, s;

	sErrorMessage = _T("");
	nProductionID = 0;
	nEditionID = 0;

	sSQL.Format("SELECT TOP 1 ProductionID,EditionID FROM PageTable WITH (NOLOCK) WHERE PublicationID=%d AND PubDate='%s' AND UniquePage=1 AND Dirty=0", nPublicationID, TranslateDate(tPubDate));

	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;

	int nRetries = g_prefs.m_nQueryRetries;

	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				nProductionID = atoi(s);
				Rs->GetFieldValue((short)1, s);
				nEditionID = atoi(s);
			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}



int CDatabaseManager::GetLowestEdition(int nProductionID, CString &sErrorMessage)
{

	CString sSQL, s;

	sErrorMessage = _T("");
	int nLowestEditionID = 0;

	sSQL.Format("SELECT MIN(EditionID) FROM PageTable WITH (NOLOCK) WHERE UniquePage=1 AND Dirty=0 AND ProductionID=%d", nProductionID);

	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;

	int nRetries = g_prefs.m_nQueryRetries;

	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				::Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				nLowestEditionID = atoi(s);

			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess ? nLowestEditionID : 0;
}

int CDatabaseManager::GetChannelCRC(int nMasterCopySeparationSet, int nInputMode, CString &sErrorMessage)
{
	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	int nCRC = 0;

	sSQL.Format("SELECT TOP 1 CheckSumPdfPrint FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	
	if (nInputMode == POLLINPUTTYPE_HIRESPDF)
		sSQL.Format("SELECT TOP 1 CheckSumPdfHighRes FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	else if (nInputMode == POLLINPUTTYPE_LOWRESPDF)
		sSQL.Format("SELECT TOP 1 CheckSumPdfLowRes FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);

	

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nCRC = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nCRC;
}

int CDatabaseManager::GetChannelExportStatus(int nMasterCopySeparationSet, int nChannelID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	int nStatus = -1;

	sSQL.Format("SELECT TOP 1 ExportStatus FROM ChannelStatus WITH (NOLOCK) WHERE MasterCopySeparationSet=%d AND ChannelID=%d", nMasterCopySeparationSet, nChannelID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nStatus = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nStatus;
}

// nStatusType:  0:InputStatus, 1: ProcessingStatus, 2:ExportStatus, 3:MergeStatus
BOOL CDatabaseManager::UpdateChannelStatus(int nStatusType, int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	BOOL ret = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (ret == FALSE && nRetries-- > 0) {

		ret = UpdateChannelStatus2(nStatusType, nMasterCopySeparationSet, nProductionID, nChannelID, nStatus, sMessage, sFileName, sErrorMessage);
		if (ret == FALSE)
			Sleep(g_prefs.m_QueryBackoffTime);
	}
	return ret;
}


BOOL CDatabaseManager::UpdateChannelStatus2(int nStatusType, int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage)
{

	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("{ CALL spUpdateChannelStatus (%d,%d,%d,%d,%d,'%s','%s') }", nStatusType, nMasterCopySeparationSet, nProductionID, nChannelID, nStatus, sMessage, sFileName);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert/Update failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}



/*
BOOL CDatabaseManager::UpdateStatusTransmit(int nCopySeparationSet,  int nStatus, int nChannelID, CString sErr, CString &sErrorMessage)
{
	CString sSQL;
	sSQL.Format("{CALL spTransmitUpdateStatus (%d,%d,%d,'%s',%d,%d)}", nCopySeparationSet, 5, nStatus, sErr, g_prefs.m_instancenumber, nChannelID);

	sErrorMessage, _T("");

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}

		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}
	return bSuccess;
}
*/


int CDatabaseManager::GetPageCountInProduction(int nProductionID, int nEditionID, int nMinStatus, CString &sErrorMessage)
{
	CRecordset *Rs = NULL;
	CString sSQL, s;

	int nPages = 0;

	// Ensure all pages polled are proofed...
	if (nMinStatus >= STATUSNUMBER_POLLED) {
		if (nEditionID == 0)
			sSQL.Format("SELECT COUNT(DISTINCT CopySeparationSet) FROM PageTable WITH (NOLOCK) WHERE Status>=10 AND Active>0 AND Dirty=0 AND ((PageType<2 AND Active=1) or (PageType=2)) AND ProductionID=%d", nProductionID);
		else
			sSQL.Format("SELECT COUNT(DISTINCT CopySeparationSet) FROM PageTable WITH (NOLOCK) WHERE Status>=10 AND Active>0  AND Dirty=0 AND ((PageType<2 AND Active=1) or (PageType=2)) AND ProductionID=%d AND EditionID=%d", nProductionID, nEditionID);
	}
	else {
		if (nEditionID == 0)
			sSQL.Format("SELECT COUNT(DISTINCT CopySeparationSet) FROM PageTable WITH (NOLOCK) WHERE Status>=%d AND Active>0 AND Dirty=0 AND ((PageType<2 AND Active=1) or (PageType=2)) AND ProductionID=%d  AND Dirty=0", nMinStatus, nProductionID);
		else
			sSQL.Format("SELECT COUNT(DISTINCT CopySeparationSet) FROM PageTable WITH (NOLOCK) WHERE Status>=%d AND Active>0  AND Dirty=0 AND ((PageType<2 AND Active=1) or (PageType=2)) AND ProductionID=%d AND EditionID=%d  AND Dirty=0", nMinStatus, nProductionID, nEditionID);
	}
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	//	util.LogprintfTransmit("INFO: %s", sSQL);

	sErrorMessage = _T("");
	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				::Sleep(g_prefs.m_QueryBackoffTime);
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format( "Query failed - %s", sSQL);

				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				delete Rs;
				Rs = NULL;
				continue;
			}

			if (!Rs->IsEOF()) {

				Rs->GetFieldValue((short)0, s);
				nPages = atoi(s);

			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
				return FALSE;
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			::Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess ? nPages : -1;
}

int CDatabaseManager::GetPageCountInProduction(int nProductionID, int nEditionID, int nMinStatus, int nSectionID, CString &sErrorMessage)
{
	CRecordset *Rs = NULL;
	CString sSQL, s;

	int nPages = 0;

	if (nEditionID == 0)
		sSQL.Format("SELECT COUNT(DISTINCT CopySeparationSet) FROM PageTable WITH (NOLOCK) WHERE Status>=%d AND Dirty=0 AND ((PageType<2 AND Active=1) or (PageType=2)) AND ProductionID=%d AND SectionID=%d", nMinStatus, nProductionID, nSectionID);
	else
		sSQL.Format("SELECT COUNT(DISTINCT CopySeparationSet) FROM PageTable WITH (NOLOCK) WHERE Status>=%d AND Dirty=0 AND ((PageType<2 AND Active=1) or (PageType=2)) AND ProductionID=%d AND EditionID=%d AND SectionID=%d", nMinStatus, nProductionID, nEditionID, nSectionID);

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	sErrorMessage = _T("");
	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				::Sleep(g_prefs.m_QueryBackoffTime);
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("Query failed - %s", sSQL);

				::Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				delete Rs;
				Rs = NULL;
				continue;
			}

			if (!Rs->IsEOF()) {
				int fld = 0;

				Rs->GetFieldValue((short)fld++, s);
				nPages = atoi(s);

			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
				return FALSE;
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess ? nPages : -1;
}

BOOL CDatabaseManager::GetMasterCopySeparationListPDF(BOOL bClearList, int nPdfType, int nProductionID, int nEditionID, CUIntArray &aArrMasterCopySeparationSet, CStringArray &aArrFileName, CString &sErrorMessage)
{
	return GetMasterCopySeparationListPDF(bClearList, nPdfType, nProductionID, nEditionID, 0, aArrMasterCopySeparationSet, aArrFileName, sErrorMessage);
}

BOOL CDatabaseManager::GetMasterCopySeparationListPDF(BOOL bClearList, int nPdfType, int nProductionID, int nEditionID, CUIntArray &aArrMasterCopySeparationSet, CString &sErrorMessage)
{
	CStringArray aArrFileName;
	return GetMasterCopySeparationListPDF(bClearList, nPdfType, nProductionID, nEditionID, 0, aArrMasterCopySeparationSet, aArrFileName, sErrorMessage);
}


BOOL CDatabaseManager::GetMasterCopySeparationListPDF(BOOL bClearList,int nPdfType, int nProductionID, int nEditionID, int nSectionID, CUIntArray &aArrMasterCopySeparationSet, CStringArray &aArrFileName, CString &sErrorMessage)
{
	CRecordset *Rs = NULL;
	CString sSQL, s;

	if (bClearList) {
		aArrMasterCopySeparationSet.RemoveAll();
		aArrFileName.RemoveAll();
	}
	if (nSectionID > 0) {

		if (nPdfType == POLLINPUTTYPE_LOWRESPDF)
			sSQL.Format("SELECT DISTINCT P.MasterCopySeparationSet,P.FileName,P.PageIndex,P.EditionID,P.SectionID FROM PageTable P WITH (NOLOCK) INNER JOIN ChannelStatus CS WITH (NOLOCK) ON CS.MasterCopySeparationSet=P.MasterCopySeparationSet AND P.ProductionID=%d AND P.EditionID=%d AND SectionID=%d AND Dirty=0 AND PageType<2 AND Active>0 AND StatusPdfLowRes>=10 AND CheckSumPdfLowRes <> 0 ORDER BY EditionID,SectionID,PageIndex", nProductionID, nEditionID, nSectionID);
		else if (nPdfType == POLLINPUTTYPE_HIRESPDF)
			sSQL.Format("SELECT DISTINCT P.MasterCopySeparationSet,P.FileName,P.PageIndex,P.EditionID,P.SectionID FROM PageTable P WITH (NOLOCK) INNER JOIN ChannelStatus CS WITH (NOLOCK) ON CS.MasterCopySeparationSet=P.MasterCopySeparationSet AND P.ProductionID=%d AND P.EditionID=%d AND SectionID=%d AND Dirty=0 AND PageType<2 AND Active>0 AND StatusPdfHighRes>=10 AND CheckSumPdfHighRes <> 0  ORDER BY EditionID,SectionID,PageIndex", nProductionID, nEditionID, nSectionID);
		else
			sSQL.Format("SELECT DISTINCT P.MasterCopySeparationSet,P.FileName,P.PageIndex,P.EditionID,P.SectionID FROM PageTable P WITH (NOLOCK) INNER JOIN ChannelStatus CS WITH (NOLOCK) ON CS.MasterCopySeparationSet=P.MasterCopySeparationSet AND P.ProductionID=%d AND P.EditionID=%d AND SectionID=%d AND Dirty=0 AND PageType<2 AND Active>0 AND StatusPdfPrintRes>=10 AND CheckSumPdfPrintRes <> 0  ORDER BY EditionID,SectionID,PageIndex", nProductionID, nEditionID, nSectionID);
	}
	else
	{
		if (nPdfType == POLLINPUTTYPE_LOWRESPDF)
			sSQL.Format("SELECT DISTINCT P.MasterCopySeparationSet,P.FileName,P.PageIndex,P.EditionID,P.SectionID FROM PageTable P WITH (NOLOCK) INNER JOIN ChannelStatus CS WITH (NOLOCK) ON CS.MasterCopySeparationSet=P.MasterCopySeparationSet AND P.ProductionID=%d AND P.EditionID=%d AND Dirty=0 AND PageType<2 AND Active>0 AND StatusPdfLowRes>=10 AND CheckSumPdfLowRes <> 0 ORDER BY EditionID,SectionID,PageIndex", nProductionID, nEditionID);
		else if (nPdfType == POLLINPUTTYPE_HIRESPDF)
			sSQL.Format("SELECT DISTINCT P.MasterCopySeparationSet,P.FileName,P.PageIndex,P.EditionID,P.SectionID FROM PageTable P WITH (NOLOCK) INNER JOIN ChannelStatus CS WITH (NOLOCK) ON CS.MasterCopySeparationSet=P.MasterCopySeparationSet AND P.ProductionID=%d AND P.EditionID=%d AND Dirty=0 AND PageType<2 AND Active>0 AND StatusPdfHighRes>=10 AND CheckSumPdfHighRes <> 0 ORDER BY EditionID,SectionID,PageIndex", nProductionID, nEditionID);
		else
			sSQL.Format("SELECT DISTINCT P.MasterCopySeparationSet,P.FileName,P.PageIndex,P.EditionID,P.SectionID FROM PageTable P WITH (NOLOCK) INNER JOIN ChannelStatus CS WITH (NOLOCK) ON CS.MasterCopySeparationSet=P.MasterCopySeparationSet AND P.ProductionID=%d AND P.EditionID=%d AND Dirty=0 AND PageType<2 AND Active>0 AND StatusPdfPrintRes>=10 AND CheckSumPdfPrintRes <> 0 ORDER BY EditionID,SectionID,PageIndex", nProductionID, nEditionID);
	}
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	sErrorMessage = _T("");
	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				::Sleep(g_prefs.m_QueryBackoffTime);
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("Query failed - %s", sSQL);

				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				delete Rs;
				Rs = NULL;
				continue;
			}

			while (!Rs->IsEOF()) {

				Rs->GetFieldValue((short)0, s);
				aArrMasterCopySeparationSet.Add(atoi(s));

				Rs->GetFieldValue((short)1, s);
				aArrFileName.Add(s);

				Rs->MoveNext();
			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
				return FALSE;
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}


BOOL CDatabaseManager::GetPackagePublications(int nPublicationID, CUIntArray &aPackagePublicationIDList, CUIntArray &aPackageSectionIndexList, CString &sPackageName, CString &sErrorMessage)
{
	CUtils util;
	CString sSQL, s;

	sPackageName = _T("");
	aPackagePublicationIDList.RemoveAll();
	aPackageSectionIndexList.RemoveAll();

	sSQL.Format("SELECT DISTINCT P1.PublicationID,P1.SectionIndex,P1.Name FROM [PackageNames] P1 WITH (NOLOCK) WHERE P1.Name IN (SELECT TOP 1 Name FROM [PackageNames] P2 WITH (NOLOCK) WHERE P2.PublicationID=%d) ORDER BY P1.SectionIndex", nPublicationID);

	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;

	sErrorMessage = _T("");
	int nRetries = g_prefs.m_nQueryRetries;

	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			while (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				aPackagePublicationIDList.Add(atoi(s));

				Rs->GetFieldValue((short)1, s);
				aPackageSectionIndexList.Add(atoi(s));

				Rs->GetFieldValue((short)2, sPackageName);

				Rs->MoveNext();
			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}

int CDatabaseManager::GetCommonPages(int nMasterCopySeparationSet, CUIntArray &sCopySeparationSetList, CString &sErrorMessage)
{
	CUtils util;
	CString sSQL, s;

	sCopySeparationSetList.RemoveAll();

	sSQL.Format("SELECT DISTINCT CopySeparationSet FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d AND UniquePage=0 AND Dirty=0 AND Active>0", nMasterCopySeparationSet);

	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;

	sErrorMessage = _T("");
	int nRetries = g_prefs.m_nQueryRetries;

	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			while (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				sCopySeparationSetList.Add(atoi(s));

				Rs->MoveNext();
			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}

int CDatabaseManager::HasUniquePages(int nCopySeparationSet, CString &sErrorMessage)
{
	CUtils util;
	CString sSQL, s;

	int nHasUniquePage = FALSE;

	sSQL.Format("SELECT TOP 1 P1.UniquePage FROM PageTable P1 WITH (NOLOCK) WHERE P1.UniquePage=1 AND P1.PressRunID IN (SELECT TOP 1  P2.PressRunID  FROM PageTable P2 WITH (NOLOCK) WHERE P2.CopySeparationSet=%d)", nCopySeparationSet);

	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;


	sErrorMessage = _T("");
	int nRetries = g_prefs.m_nQueryRetries;

	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				nHasUniquePage = TRUE;

			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess ? nHasUniquePage : -1;
}



int	CDatabaseManager::AcuireTransmitLock(CString &sErrorMessage)
{
	BOOL ret = -1;
	int nRetries = g_prefs.m_nQueryRetries;

	while (ret == -1 && nRetries-- > 0) {

		ret = SetTransmitLock(1, sErrorMessage);
		if (ret == -1)
			::Sleep(g_prefs.m_QueryBackoffTime);
	}
	return ret;
}

int CDatabaseManager::SetTransmitLock(int nLockType, CString &sErrorMessage)
{
	CString sSQL;
	
	int nCurrentLock = -1;

	CString sComputerName = g_util.GetComputerName();

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);

	sSQL.Format("{CALL spTransmitLock (%d,'%s')}", nLockType, sComputerName);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;
			Rs.GetFieldValue((short)0, s);
			nCurrentLock = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}
	return nCurrentLock;
}

int CDatabaseManager::ReleaseTransmitLock(CString &sErrorMessage)
{
	BOOL ret = -1;
	int nRetries = g_prefs.m_nQueryRetries;

	while (ret == -1 && nRetries-- > 0) {

		ret = SetTransmitLock(0, sErrorMessage);
		if (ret == -1)
			::Sleep(g_prefs.m_QueryBackoffTime);
	}

	return ret;
}

int	CDatabaseManager::AcuireExportLock(int nChannelID, CString &sErrorMessage)
{
	BOOL ret = -1;
	int nRetries = g_prefs.m_nQueryRetries;

	while (ret == -1 && nRetries-- > 0) {

		ret = SetExportLock(nChannelID, 1, sErrorMessage);
		if (ret == -1)
			::Sleep(g_prefs.m_QueryBackoffTime);
	}
	return ret;
}

int CDatabaseManager::SetExportLock(int nChannelID, int nLockType, CString &sErrorMessage)
{
	CString sSQL;
	int nCurrentLock = -1;

	CString sComputerName = g_util.GetComputerName();

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);

	sSQL.Format("{CALL spExportLock (%d,%d,'%s')}", nChannelID, nLockType, sComputerName);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;
			Rs.GetFieldValue((short)0, s);
			nCurrentLock = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}
	return nCurrentLock;
}

int CDatabaseManager::ReleaseExportLock(int nChannelID, CString &sErrorMessage)
{
	BOOL ret = -1;
	int nRetries = g_prefs.m_nQueryRetries;

	while (ret == -1 && nRetries-- > 0) {

		ret = SetExportLock(nChannelID, 0, sErrorMessage);
		if (ret == -1)
			::Sleep(g_prefs.m_QueryBackoffTime);
	}

	return ret;
}

BOOL CDatabaseManager::ResetExportStatus(int nChannelID, CString &sErrorMessage)
{
	CString sSQL, s;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("UPDATE ChannelStatus SET ExportStatus=0 WHERE ChannelID=%d AND ExportStatus=5 AND DATEDIFF(minute,EventTime,GETDATE()) > %d", nChannelID, g_prefs.m_maxexportingtimemin);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert/Update failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}


}


BOOL CDatabaseManager::ResetConvertStatus(int nPDFType, int nMasterCopySeparationSet, CString &sErrorMessage)
{
	CString sSQL, s;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	if (nPDFType == POLLINPUTTYPE_LOWRESPDF)
		sSQL.Format("UPDATE PageTable SET StatusPDFLowres=0,ChecksumPdfLowres=0,InkStatus=InkStatus+1 WHERE MasterCopySeparationSet=%d AND InkStatus<=4 AND StatusPDFLowres=10", nMasterCopySeparationSet);
	else if (nPDFType == POLLINPUTTYPE_PRINTPDF)
		sSQL.Format("UPDATE PageTable SET StatusPDFPrint=0,ChecksumPdfPrint=0,InkStatus=InkStatus+1 WHERE MasterCopySeparationSet=%d AND InkStatus<=4 AND StatusPDFPrint=10", nMasterCopySeparationSet);
	else
		return TRUE;

	g_util.LogprintfTransmit("DEBUG: %s", sSQL);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert/Update failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::GetOriginalFileName(int nMasterCopySeparationSet, CString &sFileName, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	sFileName = _T("");
	CString sSQL;

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	CRecordset Rs(m_pDB);

	sSQL.Format("SELECT TOP 1 FileName FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d AND UniquePage=1 AND Dirty=0", nMasterCopySeparationSet);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}
		if (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, sFileName);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


int CDatabaseManager::HasExportingState(int nChannelID, CString &sErrorMessage)
{
	int nPagesFound = 0;
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	sSQL.Format("SELECT COUNT(MasterCopySeparationSet) FROM ChannelStatus WITH (NOLOCK) WHERE ChannelID=%d AND ExportStatus=5 AND DATEDIFF(minute,EventTime,GETDATE()) > %d", nChannelID, g_prefs.m_maxexportingtimemin);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}
		if (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			nPagesFound = atoi(s);
		
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nPagesFound;
}

int CDatabaseManager::FieldExists(CString sTableName, CString sFieldName , CString &sErrorMessage)
{
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);
	sSQL.Format("{ CALL sp_columns ('%s') }", (LPCSTR)sTableName);

	try {		
		if(!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}
		while (!Rs.IsEOF()) {
			
			Rs.GetFieldValue((short)3, s);

			if (s.CompareNoCase(sFieldName) == 0) {
				bFound = TRUE;
				break;
			}
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bFound ? 1 : 0;
}

int CDatabaseManager::TableExists(CString sTableName, CString &sErrorMessage)
{
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);
	sSQL.Format("{ CALL sp_tables ('%s') }", (LPCSTR)sTableName);

	try {		
		if(!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}
		if (!Rs.IsEOF()) {
			
			bFound = TRUE;
		}
		Rs.Close();
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bFound ? 1 : 0;
}

int CDatabaseManager::StoredProcParameterExists(CString sSPname, CString sParameterName, CString &sErrorMessage)
{
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);
	sSQL.Format("{ CALL sp_sproc_columns ('%s') }", (LPCSTR)sSPname);

	try {		
		if(!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}
		while (!Rs.IsEOF()) {
			
			Rs.GetFieldValue((short)3, s);

			if (s.CompareNoCase(sParameterName) == 0) {
				bFound = TRUE;
				break;
			}
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bFound ? 1 : 0;
}

/////////////////////////////////////////////////
// Translate CTime to SQL Server DATETIME type
/////////////////////////////////////////////////

CString CDatabaseManager::TranslateDate(CTime tDate)
{
	CString dd,mm,yy,yysmall;

	dd.Format("%.2d",tDate.GetDay());
	mm.Format("%.2d",tDate.GetMonth());
	yy.Format("%.4d",tDate.GetYear());
	yysmall.Format("%.2d",tDate.GetYear());

	return mm + "/" + dd + "/" + yy;

}

CString CDatabaseManager::TranslateTime(CTime tDate)
{
	CString dd,mm,yy,yysmall, hour, min, sec;

	dd.Format("%.2d",tDate.GetDay());
	mm.Format("%.2d",tDate.GetMonth());
	yy.Format("%.4d",tDate.GetYear());
	yysmall.Format("%.2d",tDate.GetYear());

	hour.Format("%.2d",tDate.GetHour());
	min.Format("%.2d",tDate.GetMinute());
	sec.Format("%.2d",tDate.GetSecond());

	return mm + "/" + dd + "/" + yy + " " + hour + ":" + min + ":" + sec;
}

void CDatabaseManager::LogprintfDB(const TCHAR *msg, ...)
{
	TCHAR	szLogLine[16000], szFinalLine[16000];
	va_list	ap;
	DWORD	n, nBytesWritten;
	SYSTEMTIME	ltime;


	HANDLE hFile = ::CreateFile(g_prefs.m_logFilePath + _T("\\ExportServiceDBerror.log"), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	// Seek to end of file
	::SetFilePointer(hFile, 0, NULL, FILE_END);

	va_start(ap, msg);
	n = vsprintf(szLogLine, msg, ap);
	va_end(ap);
	szLogLine[n] = '\0';

	::GetLocalTime(&ltime);
	_stprintf(szFinalLine, "[%.2d-%.2d %.2d:%.2d:%.2d.%.3d] %s\r\n", (int)ltime.wDay, (int)ltime.wMonth, (int)ltime.wHour, (int)ltime.wMinute, (int)ltime.wSecond, (int)ltime.wMilliseconds, szLogLine);

	::WriteFile(hFile, szFinalLine, (DWORD)_tcsclen(szFinalLine), &nBytesWritten, NULL);

	::CloseHandle(hFile);

#ifdef _DEBUG
	TRACE(szFinalLine);
#endif
}

