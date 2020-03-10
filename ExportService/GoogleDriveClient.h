#pragma once
#include "stdafx.h"
#include <CkOAuth2.h>
#include <CkStringBuilder.h>

typedef struct
{
	CString fileName;
	CString ID;
	CString mimeType;
} GOOGLEFILEINFOSTRUCT;

class CGoogleDriveClient
{
public:
	CGoogleDriveClient(void);
	~CGoogleDriveClient(void);

	CString m_accessToken;
	CString m_refreshToken;

	int m_OAuth2ListenPort;
	CString m_AuthorizationEndpoint;
	CString m_TokenEndpoint;
	CString m_GooleClientID;
	CString m_GoogleClientSecret;
	CString m_GoogleApiScopeUrl;

	BOOL	GetToken(CString &sErrorMessage);
	BOOL	RefreshToken(CString &sErrorMessage);
	int		SearchForFiles(CString sMask, GOOGLEFILEINFOSTRUCT aFileList[], CString &sErrorMessage);
	BOOL	GetFile(GOOGLEFILEINFOSTRUCT googleFileID, CString sLocalFileName, CString &sErrorMessage);
	BOOL	DeleteFile(GOOGLEFILEINFOSTRUCT googleFileID, CString &sErrorMessage);
	BOOL	PutFile(CString sLocalFile, CString sGoogleFolderID, CString &sErrorMessage);
	BOOL	GetFolderID(CString sFolderName, CString &sID, CString &sErrorMessage);

};


