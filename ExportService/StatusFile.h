#pragma once

BOOL	WriteXmlLogFile(CString sJobName, int nStatus, CString sSourceFolder, CString sMsg, CTime tEventTime, int nCRC, CString sSetupName);
BOOL	WriteXmlStatusFile(int nStatus);
BOOL	SendXMLfileFTP(CString sInputFile) ;

