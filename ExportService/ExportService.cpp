
#include "stdafx.h"
#include "Winsvc.h"
#include "Defs.h"
#include "Utils.h"
#include "Prefs.h"
#include "DatabaseManager.h"
#include "ServiceUtils.h"
#include "ExportService.h"



CUtils	g_util;
CPrefs	g_prefs;

extern TCHAR g_szServiceName[MAX_PATH];

BOOL		g_BreakSignal = FALSE;
BOOL		g_exportthreadrunning = FALSE;
BOOL		 g_transmitthreadrunning = FALSE;
BOOL		g_connected = FALSE;

CTime		g_lastSetupFileTime;
BOOL g_breaktransfer = FALSE;
CWinApp theApp;

using namespace std;
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		return 1;
	}
	if (argc > 1) {
		if (_tcscmp(argv[1], _T("-i")) == 0) {

			if (InstallService())
				_tprintf(_T("\n\nService Installed Sucessfully\n"));
			else
				_tprintf(_T("\n\nError Installing Service\n"));
		}
		else if (strcmp(argv[1], "-d") == 0) {
			if (DeleteService())
				_tprintf(_T("\n\nService UnInstalled Sucessfully\n"));
			else
				_tprintf(_T("\n\nError UnInstalling Service\n"));
		}
		else {
			_tprintf(_T("\n\nUnknown Switch Usage\n\nFor Install use %s.exe -i\n\nFor UnInstall use %s.exe -d\n"), g_szServiceName, g_szServiceName);
		}
	}
	else {
#ifndef EMULATE
		SERVICE_TABLE_ENTRY DispatchTable[] = { {g_szServiceName,ServiceMain},{NULL,NULL} };
		StartServiceCtrlDispatcher(DispatchTable);
#else
		ServiceMain(0, NULL);
#endif
	}

	return 0;
}

