#pragma once
#include <afxdb.h>

class CDatabaseManager
{
public:
	CDatabaseManager(void);
	virtual ~CDatabaseManager(void);

	BOOL	InitDB(CString sDBserver, CString sDatabase, CString sDBuser, CString sDBpassword, BOOL bIntegratedSecurity, CString &sErrorMessage);
	BOOL	InitDB(CString &sErrorMessage);
	void	ExitDB();

	BOOL	IsOpen();
	BOOL	LoadConfigIniFile(int nInstanceNumber, CString &sFileName, CString &sFileName2, CString &sFileName3, CString &sErrorMessage);
	BOOL	RegisterService(CString &sErrorMessage);
	BOOL	UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage);
	BOOL	UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString &sErrorMessage);
	BOOL	InsertLogEntry(int nEvent, CString sSource, CString sFileName, CString sMessage, int nMasterCopySeparationSet, int nVersion, int nMiscInt, CString sMiscString, CString &sErrorMessage);


	BOOL    LoadPublicationList(CString &sErrorMessage);
	BOOL	LoadEditionNameList(CString &sErrorMessage);
	BOOL	LoadSectionNameList(CString &sErrorMessage);
	BOOL	LoadAliasList(CString &sErrorMessage);
	BOOL    LoadStatusList(CString sIDtable, STATUSLIST &v, CString &sErrorMessage);
	BOOL	LoadPublicationChannels(int nID, CString &sErrorMessage);
	BOOL	LoadFileServerList(CString &sErrorMessage);
	BOOL    LoadPublisherList(CString &sErrorMessage);

	BOOL	LoadChannelList(CString &sErrorMessage);
	BOOL	LoadExpressionList(CHANNELSTRUCT *pChannel, CString  &sErrorMessage);

	BOOL	LoadPackageList(CString &sErrorMessage);

	BOOL	LoadAllPrefs(CString &sErrorMessage);
	BOOL	LoadGeneralPreferences(CString &sErrorMessage);

	//int		LoadNewPublicationID(CString sName, CString &sErrorMessage);
	CString LoadNewPublicationName(int nID, CString &sErrorMessage);
	CString LoadNewEditionName(int nID, CString &sErrorMessage);
	CString LoadNewSectionName(int nID, CString &sErrorMessage);
	BOOL    LoadNewChannel(int nChannelID, CString &sErrorMessage);

	CString LoadNewPublisherName(int nID, CString &sErrorMessage);

	BOOL	LoadSTMPSetup(CString &sErrorMessage);

	CString  LoadSpecificAlias(CString sType, CString sLongName, CString &sErrorMessage);

	int		LookupTransmitJob(BOOL bTransmitAfterApproval, BOOL bTransmitAfterProof, BOOL bIncludeErrorRetry, BOOL bExcludeMergePDF, CString &sErrorMessage);
	BOOL	GetJobDetails(CJobAttributes *pJob, int nCopySeparationSet, CString &sErrorMessage);
	int     GetLowestEdition(int nProductionID, CString &sErrorMessage);
	//BOOL	UpdateStatusTransmit(int nCopySeparationSet, int nStatus, int nChannelID, CString sErr, CString &sErrorMessage);
	
	int     GetChannelCRC(int nMasterCopySeparationSet, int nInputMode, CString &sErrorMessage);
	BOOL	ReLoadChannelOnOff(int nChannelID, CString &sErrorMessage);
	int		GetChannelExportStatus(int nMasterCopySeparationSet, int nChannelID, CString &sErrorMessage);

	BOOL	ResetConvertStatus(int nPDFType, int nMasterCopySeparationSet, CString &sErrorMessage);


	BOOL	LookupPackageName(int &nPublicationID, int &nSectionID, CTime tPubDate, CString &sErrorMessage);
	BOOL	UpdateChannelStatus(int nStatusType, int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage);
	BOOL	UpdateChannelStatus2(int nStatusType, int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage);
	BOOL	LoadChannelConfigData(int nChannelID, CString &sConfigData, CString &sErrorMessage);

	int		GetPageCountInProduction(int nProductionID, int nEditionID, int nMinStatus, CString &sErrorMessage);
	int		GetPageCountInProduction(int nProductionID, int nEditionID, int nMinStatus, int nSectionID, CString &sErrorMessage);
	BOOL	GetMasterCopySeparationListPDF(BOOL bClearList, int nPdfType, int nProductionID, int nEditionID, CUIntArray &aArrMasterCopySeparationSet, CString &sErrorMessage);
	BOOL	GetMasterCopySeparationListPDF(BOOL bClearList, int nPdfType, int nProductionID, int nEditionID, int nSectionID, CUIntArray &aArrMasterCopySeparationSet, CStringArray &aArrFileName, CString &sErrorMessage);
	BOOL	GetMasterCopySeparationListPDF(BOOL bClearList, int nPdfType, int nProductionID, int nEditionID, CUIntArray &aArrMasterCopySeparationSet, CStringArray &aArrFileName, CString &sErrorMessage);
	int		HasUniquePages(int nCopySeparationSet, CString &sErrorMessage);
	int		GetCommonPages(int nMasterCopySeparationSet, CUIntArray &sCopySeparationSetList, CString &sErrorMessage);
	BOOL	GetPackagePublications(int nPublicationID, CUIntArray &aPackagePublicationIDList, CUIntArray &aPackageSectionIndexList, CString &sPackageName, CString &sErrorMessage);
	BOOL	GetProductionID(int nPublicationID, CTime tPubDate, int &nProductionID, int &nEditionID, CString &sErrorMessage);

	BOOL	GetChannelConfigChangeTime(int nChannelID, CTime &tChangeTime, CString &sErrorMessage);
	BOOL	GetPublicationConfigChangeTime(int nPublicationID, CTime &tChangeTime, CString &sErrorMessage);

	int		ReleaseExportLock(int nChannelID, CString &sErrorMessage);
	int		AcuireExportLock(int nChannelID, CString &sErrorMessage);
	int		SetExportLock(int nChannelID, int nLockType, CString &sErrorMessage);
	int		HasExportingState(int nChannelID, CString &sErrorMessage);
	int		ReleaseTransmitLock(CString &sErrorMessage);
	int		AcuireTransmitLock(CString &sErrorMessage);
	int		SetTransmitLock(int nLockType, CString &sErrorMessage);
	BOOL	ResetExportStatus(int nChannelID, CString &sErrorMessage);
	BOOL	GetProductionID(int nMasterCopySeparationSet, int &nProductionID, int &nEditionID, CString &sErrorMessage);
	int		StoredProcParameterExists(CString sSPname, CString sParameterName, CString &sErrorMessage);
	int		TableExists(CString sTableName, CString &sErrorMessage);
	int		FieldExists(CString sTableName, CString sFieldName , CString &sErrorMessage);

	BOOL	GetOriginalFileName(int nMasterCopySeparationSet, CString &sFileName, CString &sErrorMessage);


	CString TranslateDate(CTime tDate);
	CString TranslateTime(CTime tDate);
	void	LogprintfDB(const TCHAR *msg, ...);


private:
	BOOL	m_DBopen;
	CDatabase *m_pDB;

	BOOL	m_IntegratedSecurity;
	CString m_DBserver;
	CString m_Database;
	CString m_DBuser;
	CString m_DBpassword;
	BOOL	m_PersistentConnection;
};
