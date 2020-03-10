#include "StdAfx.h"
#include "Defs.h"
#include "Utils.h"
#include "GoogleDriveClient.h"
#include <string.h>
#include <direct.h>
#include <winnetwk.h>
#include <CkOAuth2.h>
#include <CkJsonObject.h>
#include <CkRest.h>
#include <CkFileAccess.h>
#include <CkAuthGoogle.h>
#include <CkGlobal.h>
#include <CkStream.h>
#include <CkBinData.h>
#include <CkJsonArray.h>

extern CUtils g_util;

CGoogleDriveClient::CGoogleDriveClient(void)
{
	m_OAuth2ListenPort = 55568;
	m_AuthorizationEndpoint = _T("https://accounts.google.com/o/oauth2/v2/auth");
	m_TokenEndpoint = _T("https://www.googleapis.com/oauth2/v4/token");
	m_GooleClientID = _T("");
	m_GoogleClientSecret = _T("");
	m_GoogleApiScopeUrl = _T("https://www.googleapis.com/auth/drive");
	m_accessToken = _T("");
}

CGoogleDriveClient::~CGoogleDriveClient(void)
{
}


BOOL CGoogleDriveClient::GetToken(CString &sErrorMessage) 
{
	CkGlobal glob;

	sErrorMessage = _T("");

	bool success = glob.UnlockBundle(_T("INFRAL.CB1092019_4czEsHZS615x"));
	if (success != true) {
		sErrorMessage = CString(glob.lastErrorText());
		return FALSE;
	}

	CkOAuth2 oauth2;

	m_accessToken = _T("");
	oauth2.put_ListenPort(m_OAuth2ListenPort);
	oauth2.put_AuthorizationEndpoint(m_AuthorizationEndpoint);
	oauth2.put_TokenEndpoint(m_TokenEndpoint);
	oauth2.put_ClientId(m_GooleClientID);
	oauth2.put_ClientSecret(m_GoogleClientSecret);
	oauth2.put_CodeChallenge(true);
	oauth2.put_CodeChallengeMethod("S256");
	oauth2.put_Scope(m_GoogleApiScopeUrl);

	//  Begin the OAuth2 three-legged flow.  This returns a URL that should be loaded in a browser.
	const char *url = oauth2.startAuth();
	if (oauth2.get_LastMethodSuccess() != true) {
		sErrorMessage = oauth2.lastErrorText();
		return FALSE;
	}

	::ShellExecute(NULL, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);

	//  Now wait for the authorization.
	//  We'll wait for a max of 30 seconds.
	int numMsWaited = 0;
	while ((numMsWaited < 30000) && (oauth2.get_AuthFlowState() < 3)) {
		oauth2.SleepMs(100);
		numMsWaited = numMsWaited + 100;
	}

	if (oauth2.get_AuthFlowState() < 3) {
		oauth2.Cancel();
		sErrorMessage = _T("No response from the browser!");
		return FALSE;
	}

	if (oauth2.get_AuthFlowState() == 5) {
		CString s(oauth2.failureInfo());
		sErrorMessage = _T("OAuth2 failed to complete - ") + s;
		return FALSE;
	}

	if (oauth2.get_AuthFlowState() == 4) {
		CString s(oauth2.accessTokenResponse());
		sErrorMessage = _T("OAuth2 authorization was denied - ") + s;
		return FALSE;
	}

	if (oauth2.get_AuthFlowState() != 3) {
		sErrorMessage = _T("Unexpected AuthFlowState - ") + g_util.Int2String(oauth2.get_AuthFlowState());
		return FALSE;
	}

	//  Save the full JSON access token response to a file.
	//m_sbJson = CString(oauth2.accessTokenResponse());

//	sbJson.WriteFile("qa_data/tokens/googleDrive.json", "utf-8", false);

	//  The saved JSON response looks like this:

	//  	{
	//  	 "access_token": "ya39.Ci-XA_C5bGgRDC3UaD-h0_NeL-DVIQnI2gHtBBBHkZzrwlARkwX6R3O0PCDEzRlfaQ",
	//  	 "token_type": "Bearer",
	//  	 "expires_in": 3600,
	//  	 "refresh_token": "1/r_2c_7jddspcdfesrrfKqfXtqo08D6Q-gUU0DsdfVMsx0c"
	//  	}
	// 
	m_accessToken = oauth2.accessToken();
	m_refreshToken = oauth2.refreshToken();
	return TRUE;
}

BOOL CGoogleDriveClient::RefreshToken(CString &sErrorMessage)
{
	CkOAuth2 oauth2;
	CkRest rest;

	sErrorMessage = _T("");

	oauth2.put_AccessToken(m_accessToken);
	oauth2.put_RefreshToken(m_refreshToken);

	//  Connect using TLS.
	bool bAutoReconnect = true;
	bool success = rest.Connect("www.googleapis.com", 443, true, bAutoReconnect);

	//  Provide the authentication credentials (i.e. the access token)
	rest.SetAuthOAuth2(oauth2);

	// Test 
	const char *jsonResponse = rest.fullRequestNoBody("DELETE", "/drive/v3/files/trash");
	if (rest.get_LastMethodSuccess() != true) {
		sErrorMessage = CString(rest.lastErrorText());
		return FALSE;
	}

	// Response 401 means expired..
	if (rest.get_ResponseStatusCode() == 401) {
		oauth2.put_AuthorizationEndpoint(m_AuthorizationEndpoint);
		oauth2.put_TokenEndpoint(m_TokenEndpoint);
		oauth2.put_ClientId(m_GooleClientID);
		oauth2.put_ClientSecret(m_GoogleClientSecret);
		oauth2.put_Scope(m_GoogleApiScopeUrl);

		//  Use OAuth2 to refresh the access token.
		success = oauth2.RefreshAccessToken();
		if (success != true) {
			sErrorMessage = CString(oauth2.lastErrorText());
			return FALSE;
		}

		m_accessToken = oauth2.accessToken();
		m_refreshToken = oauth2.refreshToken();
		return TRUE;

		//  Retry the request with the new access token..
		jsonResponse = rest.fullRequestNoBody("DELETE", "/drive/v3/files/trash");
		if (rest.get_LastMethodSuccess() != true) {
			sErrorMessage = CString(rest.lastErrorText());
			return FALSE;
		}

	}

	//  A successful response will have a status code equal to 204 and the response body is empty.
	//  (If not successful, then there should be a JSON response body with information..)
	if (rest.get_ResponseStatusCode() != 204) {
		sErrorMessage = CString(rest.responseStatusText());
		return FALSE;
	}

	return TRUE;

}

int CGoogleDriveClient::SearchForFiles(CString sMask, GOOGLEFILEINFOSTRUCT aFileList[], CString &sErrorMessage)
{
	CkAuthGoogle gAuth;
	CkGlobal glob;
	CkRest rest;
	CkJsonObject json;
	int i;
	int numFiles = 0;

	sErrorMessage = _T("");

	bool success = glob.UnlockBundle(_T("INFRAL.CB1092019_4czEsHZS615x"));
	if (success != true) {
		sErrorMessage = CString(glob.lastErrorText());
		return -1;
	}

	gAuth.put_AccessToken(m_accessToken);

	//  Connect using TLS.
	bool bAutoReconnect = true;
	success = rest.Connect("www.googleapis.com", 443, true, bAutoReconnect);
	//  Provide the authentication credentials (i.e. the access token)
	rest.SetAuthGoogle(gAuth);

	//  Get 5 results per page for testing.  (The default page size is 100, with a max of 1000.
	rest.AddQueryParam("pageSize", "5");
	//  Our search filter is to list all files containing ".jpg" (i.e. all JPG image files)
	CString query;

	if (sMask[0] == _T('*'))
		sMask = sMask.Mid(1);
	query.Format("name contains '%s'", sMask);
	rest.AddQueryParam("q", query);

	//  Send the request for the 1st page.
	const char *jsonResponse = rest.fullRequestNoBody("GET", "/drive/v3/files");


	int pageNumber = 1;
	const char *pageToken = 0;
	bool bContinueLoop = rest.get_LastMethodSuccess() && (rest.get_ResponseStatusCode() == 200);

	while (bContinueLoop == true) {

		//  Iterate over each file in the response and show the name, id, and mimeType.
		json.Load(jsonResponse);

		numFiles = json.SizeOfArray("files");
		i = 0;
		while (i < numFiles) {
			json.put_I(i);
			aFileList[numFiles].fileName = json.stringOf("files[i].name");
			aFileList[numFiles].ID = json.stringOf("files[i].id");
			aFileList[numFiles++].mimeType = json.stringOf("files[i].mimeType");
			i = i + 1;
		}

		//  Get the next page of files.
		//  If the "nextPageToken" is present in the JSON response, then use it in the "pageToken" parameter
		//  for the next request.   If no "nextPageToken" was present, then this was the last page of files.
		pageToken = json.stringOf("nextPageToken");
		bContinueLoop = false;
		bool bHasMorePages = json.get_LastMethodSuccess();
		if (bHasMorePages == true) {
			rest.ClearAllQueryParams();
			rest.AddQueryParam("pageSize", "5");
			rest.AddQueryParam("pageToken", pageToken);
			query.Format("name contains '%s'", sMask);
			rest.AddQueryParam("q", query);

			jsonResponse = rest.fullRequestNoBody("GET", "/drive/v3/files");
			bContinueLoop = rest.get_LastMethodSuccess() && (rest.get_ResponseStatusCode() == 200);
			pageNumber = pageNumber + 1;
		}

	}

	if (rest.get_LastMethodSuccess() != true) {
		sErrorMessage = CString(rest.lastErrorText());
		return FALSE;
	}

	//  A successful response will have a status code equal to 200.
	if (rest.get_ResponseStatusCode() != 200) {
		sErrorMessage = CString(rest.responseStatusText());
		return FALSE;
	}

	return TRUE;
}

BOOL	CGoogleDriveClient::GetFile(GOOGLEFILEINFOSTRUCT googleFileID, CString sLocalFileName, CString &sErrorMessage)
{
	CkAuthGoogle gAuth;
	CkGlobal glob;
	CkRest rest;
	CkJsonObject json;

	sErrorMessage = _T("");

	bool success = glob.UnlockBundle(_T("INFRAL.CB1092019_4czEsHZS615x"));
	if (success != true) {
		sErrorMessage = CString(glob.lastErrorText());
		return FALSE;
	}

	gAuth.put_AccessToken(m_accessToken);

	//  Connect using TLS.
	bool bAutoReconnect = true;
	success = rest.Connect("www.googleapis.com", 443, true, bAutoReconnect);
	//  Provide the authentication credentials (i.e. the access token)
	rest.SetAuthGoogle(gAuth);

	//  We need to send a GET request like this:
   //  GET https://www.googleapis.com/drive/v3/files/fileId?alt=media
   //  The fileId is part of the path.
	CkStringBuilder sbPath;
	sbPath.Append("/drive/v3/files/");
	sbPath.Append(googleFileID.ID);
	rest.AddQueryParam("alt", "media");

	//  Create a stream object with a SinkFile set to the path where the downloaded file will go.
	CkStream fileStream;
	fileStream.put_SinkFile(sLocalFileName);
	//  Indicate that the response body should stream directly to fileStream,
	//  but only if the response status is 200.
	rest.SetResponseBodyStream(200, true, fileStream);

	//  Download the file, streaming the content to the SinkFile.
	const char *jsonResponse = rest.fullRequestNoBody("GET", sbPath.getAsString());
	if (rest.get_LastMethodSuccess() != true) {
		sErrorMessage = CString(rest.lastErrorText());
		return FALSE;
	}

	//  A successful response will have a status code equal to 200.
	if (rest.get_ResponseStatusCode() != 200) {
		sErrorMessage = CString(rest.responseStatusText());
		return FALSE;
	}

	return TRUE;
}

BOOL	CGoogleDriveClient::DeleteFile(GOOGLEFILEINFOSTRUCT googleFileID, CString &sErrorMessage)
{
	CkAuthGoogle gAuth;
	CkGlobal glob;
	CkRest rest;
	CkJsonObject json;

	sErrorMessage = _T("");

	bool success = glob.UnlockBundle(_T("INFRAL.CB1092019_4czEsHZS615x"));
	if (success != true) {
		sErrorMessage = CString(glob.lastErrorText());
		return FALSE;
	}

	gAuth.put_AccessToken(m_accessToken);

	//  Connect using TLS.
	bool bAutoReconnect = true;
	success = rest.Connect("www.googleapis.com", 443, true, bAutoReconnect);
	//  Provide the authentication credentials (i.e. the access token)
	rest.SetAuthGoogle(gAuth);

	//  We need to send a GET request like this:
   //  GET https://www.googleapis.com/drive/v3/files/fileId?alt=media
   //  The fileId is part of the path.
	CkStringBuilder sbPath;
	sbPath.Append("/drive/v3/files/");
	sbPath.Append(googleFileID.ID);

	const char *jsonResponse = rest.fullRequestNoBody("DELETE", sbPath.getAsString());
	if (rest.get_LastMethodSuccess() != true) {
		sErrorMessage = CString(rest.lastErrorText());
		return FALSE;
	}

	//  A successful response will have a status code equal to 200.
	if (rest.get_ResponseStatusCode() != 200) {
		sErrorMessage = CString(rest.responseStatusText());
		return FALSE;
	}

	return TRUE;
}


BOOL	CGoogleDriveClient::PutFile(CString sLocalFile, CString sGoogleFolderID, CString &sErrorMessage)
{
	CkAuthGoogle gAuth;
	CkGlobal glob;
	CkRest rest;
	CkJsonObject json;

	sErrorMessage = _T("");

	bool success = glob.UnlockBundle(_T("INFRAL.CB1092019_4czEsHZS615x"));
	if (success != true) {
		sErrorMessage = CString(glob.lastErrorText());
		return FALSE;
	}

	gAuth.put_AccessToken(m_accessToken);

	//  Connect using TLS.
	bool bAutoReconnect = true;
	success = rest.Connect("www.googleapis.com", 443, true, bAutoReconnect);
	//  Provide the authentication credentials (i.e. the access token)
	rest.SetAuthGoogle(gAuth);
	//  A multipart upload to Google Drive needs a multipart/related Content-Type
	rest.AddHeader("Content-Type", "multipart/related");

	//  The 1st part is JSON with information about the file.
	rest.put_PartSelector("1");
	rest.AddHeader("Content-Type", "application/json; charset=UTF-8");

	json.AppendString("name", g_util.GetFileName(sLocalFile));
	json.AppendString("description", g_util.GetFileName(sLocalFile));
	if (g_util.GetExtension(sLocalFile).CompareNoCase("pdf") == 0)
		json.AppendString("mimeType", "application/pdf");
	else
		json.AppendString("mimeType", "application/octet-stream");

	//  Use the folder ID we already looked up..
	CkJsonArray *parents = json.AppendArray("parents");
	parents->AddStringAt(-1, sGoogleFolderID);
	delete parents;

	rest.SetMultipartBodyString(json.emit());

	//  The 2nd part is the file content, which will contain the binary image data.
	rest.put_PartSelector("2");
	rest.AddHeader("Content-Type", "application/octet-stream");

	CkBinData jpgBytes;
	success = jpgBytes.LoadFile(sLocalFile);

	//  Add the data to our upload
	rest.SetMultipartBodyBd(jpgBytes);

	const char *jsonResponse = rest.fullRequestMultipart("POST", "/upload/drive/v3/files?uploadType=multipart");
	if (rest.get_LastMethodSuccess() != true) {
		sErrorMessage = CString(rest.lastErrorText());
		return FALSE;
	}

	
	//  A successful response will have a status code equal to 200.
	if (rest.get_ResponseStatusCode() != 200) {
		sErrorMessage = CString(rest.responseStatusText());
		return FALSE;
	}

	return TRUE;
}

BOOL	CGoogleDriveClient::GetFolderID(CString sFolderName, CString &sID, CString &sErrorMessage)
{
	CkAuthGoogle gAuth;
	CkGlobal glob;
	CkRest rest;
	CkJsonObject json;

	sErrorMessage = _T("");
	sID = _T("");

	bool success = glob.UnlockBundle(_T("INFRAL.CB1092019_4czEsHZS615x"));
	if (success != true) {
		sErrorMessage = CString(glob.lastErrorText());
		return FALSE;
	}

	gAuth.put_AccessToken(m_accessToken);

	//  Connect using TLS.
	bool bAutoReconnect = true;
	success = rest.Connect("www.googleapis.com", 443, true, bAutoReconnect);
	//  Provide the authentication credentials (i.e. the access token)
	rest.SetAuthGoogle(gAuth);

	json.put_EmitCompact(false);

	if (sFolderName[0] == _T('/'))
		sFolderName = sFolderName.Mid(1);

	CString sRoot = sFolderName;
	CString sSubFolder = _T("");
	int n = sRoot.Find("/");

	if (n != -1)
	{
		sRoot = sRoot.Left(n);
		sSubFolder = sRoot.Mid(n + 1);
	}

	//  Get the sRoot folder that is in the Google Drive root.
	CString query;
	query.Format("'root' in parents and name='%s'", sRoot);
	rest.AddQueryParam("q", query);
	const char *jsonResponse = rest.fullRequestNoBody("GET", "/drive/v3/files");
	if (rest.get_LastMethodSuccess() != true) {
		sErrorMessage = CString(rest.lastErrorText());
		return FALSE;
	}

	json.Load(jsonResponse);
	sID = json.stringOf("files[0].id");

	if (sID != _T("") && sSubFolder != _T("")) {

		//  Now that we know the ID for the sRoot directory, get the id for the sSubFolder having sRoot as the parent.
		rest.ClearAllQueryParams();
		CkStringBuilder sbQuery;
		query.Format("name='%s' and '%s' in parents", sSubFolder, sID);
		rest.AddQueryParamSb("q", sbQuery);

		jsonResponse = rest.fullRequestNoBody("GET", "/drive/v3/files");
		if (rest.get_LastMethodSuccess() != true) {
			sErrorMessage = CString(rest.lastErrorText());
			return FALSE;
		}

		json.Load(jsonResponse);
		sID = json.stringOf("files[0].id");
	}

	return TRUE;

}