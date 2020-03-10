#pragma once
#include "Defs.h"

UINT TransmissionThread(LPVOID param);
int TransmitJobChannels(NextTransmitJob &job, CDatabaseManager &db, CString sDestinationFolder,
			CString &sFinalName, BOOL &bEmptyQueue, int &nMergeProductionIDToIgnore, CString &sErrorMessage);