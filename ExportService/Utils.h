#pragma once
#include "stdafx.h"
#include "Defs.h"
#include "DatabaseManager.h"

class CUtils
{
public:
	CUtils(void);
	~CUtils(void);
	CString Int2String(int n);
	CString UInt2String(int un);
	CString GetTempFolder();

	CString LimitErrorMessage(CString s);
	CString LimitErrorMessage(TCHAR *szStr);

	CString GetComputerName();

	BOOL SetFileToCurrentTime(CString sFile);

	int		CRC32(CString sFileName);
	int		CRC32(CString sFileName, BOOL bFastMode);


	int		StringSplitter(CString sInput, CString sSeparators, CStringArray &sArr);
	BOOL	LockCheck(CString sFileToLock);
	BOOL	StableTimeCheck(CString sFileToTest, int nStableTimeSec);
	DWORD	GetFileSize(CString sFile);
	CTime	GetWriteTime(CString sFile);
	int		GetFileAgeMinutes(CString sFileName);
	int		GetFileAgeHours(CString sFileName);
	int		GetFolderList(CString sSpoolFolder, CStringArray &sList);
	CTime	Systime2CTime(SYSTEMTIME SysTime);

	BOOL	FileExist(CString sFile);
	BOOL	CheckFolder(CString sFolder);
	BOOL	CheckFolderWithPing(CString sFolder);

	BOOL	Reconnect(CString sFolder, CString sUserName, CString sPassWord);

	CString	GetExtension(CString sFileName);
	CString GetFileName(CString sFullName);
	CString GetFileName(CString sFullName, BOOL bExcludeExtension);

	CString GetFilePath(CString sFullName);
	CString	GetFirstFile(CString sSpoolFolder, CString sSearchMask);

	int		ScanDir(CString sSpoolFolder, CString sSearchMask, FILEINFOSTRUCT aFileList[], int nMaxFiles);
	int		ScanDir(CString sSpoolFolder, CString sSearchMask, FILEINFOSTRUCT aFileList [], int nMaxFiles, 
					CString sIgnoreMask, BOOL bIgnoreHidden, BOOL bKillZeroByteFiles, BOOL bUseRegex, int nRegex, REGEXPSTRUCT pRegex[] );
	int		ScanDirCount(CString sSpoolFolder, CString sSearchMask);

	int		TokenizeName(CString sName, CString sSeparators, CString sv[], CString sp[]);
	
	BOOL	TryMatchExpression(CString sMatchExpression, CString sInputString, BOOL bPartialMatch);
	CString FormatExpression(CString sMatchExpression, CString sFormatExpression, CString sInputString, BOOL bPartialMatch);

	BOOL	TryMatchExpressionEx(CString sMatchExpression, CString sInputString, BOOL bPartialMatch, CString &sErrorMessage);
	CString FormatExpressionEx(CString sMatchExpression, CString sFormatExpression, CString sInputString, BOOL bPartialMatch,  CString &sErrorMessage, BOOL *bMatched);


	BOOL IsolateReference(CString sNameFormat, CString sFileName, CString &sColor, CString &sPageNumber) ;

	CString GetRawMaskEx(CString sOrgMask);
	CString ReadTiffImageDescription(CString sFullInputPath);
	BOOL	DoCreateProcess(CString sCmdLine);
	BOOL	DoCreateProcessEx(CString sCmdLine, int nTimeout);
	int		DeleteFiles(CString sSpoolFolder, CString sSearchMask);
	int		DeleteFiles(CString sSpoolFolder, CString sSearchMask, int sMaxAgeHours);
	int		MoveFiles(CString sSpoolFolder, CString sSearchMask, CString sOutputFolder, BOOL bIgnoreZeroLengt);
	BOOL    MoveFileWithRetry(CString sSourceFile, CString sDestinationFile, CString &sErrorMessage);
	BOOL	CopyFileWithRetry(CString sSourceFile, CString sDestinationFile, CString &sErrorMessage);

	void	TruncateLogFile(CString sFile, DWORD dwMaxSize);
	void	Logprintf(const TCHAR *msg,...);
	void	LogprintfExport(CString channelName, const TCHAR *msg, ...);
	void	LogprintfTransmit(const TCHAR *msg, ...);
	int		FreeDiskSpaceMB(CString sPath);

	BOOL	SendMail(int nMailType, CString sMessage);
	BOOL	SendMail(CString sMailSubject, CString sMailBody);
	BOOL	SendMail(CString sMailSubject, CString sMailBody, CString sAttachMent);

	CString GenerateTimeStamp();

	CString LoadFile(CString sFilename);

	CString GetLastWin32Error();

	CString		Date2String(CTime tDate, CString sDateFormat, BOOL bAllowSeparators, int nWeekNumber);
	CString     Date2StringEx(CTime tDate, CString sDateFormat, BOOL bAllowSeparators, int nWeekNumber);
	CString     Time2String(CTime tDate, CString sDateFormat);
	CString     GetLocalWeekDayName(CTime tDate, BOOL bUseIniWeekNames);
	CTime		ParseDate(CString sDate, CString sDateFormat);
	CTime		GetNextDay(CTime dt);
	CString		CurrentTime2String();

	BOOL	SaveFile(CString sFileName, CString sContent);

	void AlivePoll();
	void AliveTX();


	CString EncodeBlowFish(CString sInput);
	CString DecodeBlowFish(CString sInput);

	BOOL Save(CString  csFileName, CString csDoc);
	BOOL Load(CString  csFileName, CString &csDoc);

	void LogTX(CString sStatus, CString sSetup, CString sFileName, CString sMessage);
	DWORD GenerateGUID(int nSpooler, int nTick);

	CString FormNameChannel(CDatabaseManager *pDB, CJobAttributes &job, CHANNELSTRUCT &channelGroup, int nDaysToAdvance);
	CString  FormName(CDatabaseManager *pDB, CString sOrgMask, CString sDateFormat, CString sTimeFormat, CJobAttributes &job, int bAbbr, int nDaysToAdvance, BOOL bMergePDF, BOOL bUsePackageNames);

	CString		GetModuleLoadPath();

	int CUtils::IssueMessage(CString sOrgFileName, int nStatus, CString sTargetFolder, int nTypeNumber, CString &sErrorMessage);

};
