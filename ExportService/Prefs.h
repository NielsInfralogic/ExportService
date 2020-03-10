#pragma once
#include "Defs.h"
#include "DatabaseManager.h"

class CPrefs
{
public:
	CPrefs(void);
	~CPrefs(void);

	void	LoadIniFile(CString sIniFile);
	BOOL	ConnectToDB(BOOL bLoadDBNames, CString &sErrorMessage);

	
	BOOL  LoadSetup(CHANNELSTRUCT *pChannel);
	BOOL UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString &sErrorMessage);
	BOOL UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage);
	BOOL ReLoadChannelOnOff(int nChannelID, CString &sErrorMessage);

	BOOL LoadPreferencesFromRegistry();

	CString GetPublicationName(CDatabaseManager &DB, int nID);
	CString GetPublicationName(int nID);

	PUBLICATIONSTRUCT *GetPublicationStruct(int nID);
	PUBLICATIONSTRUCT *GetPublicationStruct(CDatabaseManager &DB, int nID);
	PUBLICATIONSTRUCT *GetPublicationStructNoDbLookup(int nID);

	CString  GetExtendedAlias(int nPublicationID);
	CString  GetExtendedAlias2(int nPublicationID);
	void FlushPublicationName(CString sName);

	void FlushAlias(CString sType, CString sLongName);

	CString GetEditionName(int nID);
	CString GetEditionName(CDatabaseManager &DB, int nID);
	CString GetSectionName(int nID);
	CString GetSectionName(CDatabaseManager &DB, int nID);

	CString GetPublisherName(int nID);
	CString GetPublisherName(CDatabaseManager &DB, int nID);

	CString		LookupOutputAliasFromName(CString sLongName);
	CString		LookupExtendedAliasFromName(CString sLongName);
	CString  LookupAbbreviationFromName(CString sType, CString sLongName);
	CString  LookupNameFromAbbreviation(CString sType, CString sShortName);
	CString		GetChannelName(int nID);
	CHANNELSTRUCT	*GetChannelStruct(int nID);
	CHANNELSTRUCT	*GetChannelStruct(CDatabaseManager &DB, int nID);
	CHANNELSTRUCT	*GetChannelStructNoDbLookup(int nID);
	PUBLICATIONCHANNELSTRUCT *CPrefs::GetPublicationChannelStruct(int nPublicationID, int nChannelID);



	CString FormCCFilesName(int nPDFtype, int nMasterCOpySeparationSet, CString sFileName);

	BOOL UpdateChannelStatus(int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage);
	BOOL InsertLogEntry(int nEvent, CString sSetupName, CString sFileName, CString sMessage, int nMasterCopySeparationSet, int nVersion, int nMiscInt, CString sMiscString, CString &sErrorMessage);

	BOOL	ReleaseExportLock(int nChannelID, CString &sErrorMessage);
	int		AcuireExportLock(int nChannelID, CString &sErrorMessage);

	int		HasExportingState(int nChannelID, CString &sErrorMessage);
	BOOL	ResetExportStatus(int nChannelID, CString &sErrorMessage);
	BOOL	GetProductionID(int nMasterCopySeparationSet, int &nProductionID, int &nEditionID, CString &sErrorMessage);
	BOOL	GetMasterCopySeparationListPDF(BOOL bClearList, int nPdfType, int nProductionID, int nEditionID, CUIntArray &aArrMasterCopySeparationSet, CString &sErrorMessage);

	BOOL m_FieldExists_Log_FileName;
	BOOL m_FieldExists_Log_MiscString;

	CString	m_apppath;

	CString     m_WeekDaysList;


	CString m_workfolder;
	CString m_workfolder2;
	BOOL m_bypasspingtest;
	BOOL m_bypassreconnect;
	int m_lockcheckmode;
	DWORD m_maxlogfilesize;

	BOOL	m_autostart;
	int		m_autostartmin;

	BOOL		m_logtodatabase;
	CString		m_DBserver;
	CString		m_Database;
	CString		m_DBuser;
	CString		m_DBpassword;
	BOOL		m_IntegratedSecurity;
	int			m_databaselogintimeout;
	int			m_databasequerytimeout;
	int			m_nQueryRetries;
	int			m_QueryBackoffTime;



	BOOL		m_persistentconnection;
	int		m_logToFile;
	int		m_logToFile2;
	CString m_logFilePath;

	int			m_scripttimeout;

	BOOL		m_updatelog;
	int			m_eventcodeOk;
	int			m_eventcodeFail;
	int			m_eventcodeWarning;



	BOOL		m_setlocalfiletime;



	int	m_instancenumber;

	CString m_setupenableXMLpath;
	CString m_debugsetup;

	int			m_deleteerrorfilesdays;
	BOOL		m_generateJMF;
	BOOL		m_generateJMFstatus;
	CString m_jmfdevicename;
 	int m_jmfnetworkmethod;
	CString m_jmfftpserver;
	CString m_jmfftpusername;
	CString m_jmfftpdir;
	CString m_jmfftppassword;
	int m_jmfftpport;
	CString m_jmfftpaccount;
	CString m_jmffolder;
	BOOL m_jmfftppasv;
	BOOL m_jmferroronly;

	BOOL m_emailonfoldererror;
	BOOL m_emailonfileerror;


	CString m_emailsmtpserver;
	int m_mailsmtpport;
	CString m_mailsmtpserverusername;
	CString m_mailsmtpserverpassword;
	BOOL m_mailusessl;
	BOOL m_mailusestarttls;
	CString m_emailfrom;
	CString m_emailto;
	CString m_emailcc;
	CString m_emailsubject;
	int		m_emailtimeout;
	BOOL	m_emailpreventflooding;
	int		m_emailpreventfloodingdelay;


	int m_ftptimeout;
	BOOL m_bSortOnCreateTime;
	CString m_alivelogFilePath;
	int m_defaultpolltime;
	int m_defaultstabletime;
	BOOL m_bypassdiskfreeteste;



	BOOL m_addsetupnametoackfile;
	CString m_extlogfilename;
	BOOL m_ignorelockcheck;
	BOOL m_filebuildupseparatefolder;


	BOOL m_firecommandonfolderreconnect;
	CString m_sFolderReconnectCommand;
	CString m_sDateFormat;


	int m_heartbeatMS;


	// Transmitter properties..
	CString m_lowresPDFPath;
	CString m_hiresPDFPath;		// == CCFiles ServerShare
	CString m_printPDFPath;

	CString m_previewPath;
	CString m_thumbnailPath;
	CString m_serverName;
	CString m_serverShare;
	CString m_readviewpreviewPath;
	CString m_configPath;
	CString m_errorPath;
	CString m_webPath;
	BOOL   m_copyToWeb;
	int		m_mainserverusecussrentuser;
	CString m_mainserverusername;
	CString m_mainserverpassword;

	PUBLICATIONLIST		m_PublicationList;
	ITEMLIST			m_EditionList;
	ITEMLIST			m_SectionList;
	FILESERVERLIST		m_FileServerList;
	ALIASLIST			m_AliasList;
	STATUSLIST			m_StatusList;
	STATUSLIST			m_ExternalStatusList;

	CHANNELLIST			m_ChannelList;
	//CHANNELGROUPLIST	m_ChannelGroupList;
	ITEMLIST			m_PublisherList;

	PACKAGELIST			m_PackageList;


	BOOL m_ccdatafilenamemode;
	BOOL m_transmitcleanfilename;
	int  m_transmitretries;

	BOOL m_transmitafterproof;

	BOOL m_recalculateCheckSum;

	CDatabaseManager m_DBmaint;

	int  m_maxexportingtimemin;

	BOOL m_generatestatusmessage;
	BOOL m_messageonsuccess;
	BOOL m_messageonerror;
	CString m_messageoutputfolder;
	CString m_messagetemplatesuccess;
	CString m_messagetemplateerror;
};

