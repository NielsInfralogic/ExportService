#pragma once
#include "DatabaseManager.h"

BOOL IsPDF(CString sFileName);
BOOL GetPDFinfo(CString sInputFileName, int &nPageCount, double &fSizeX, double &fSizeY);
int MakeOriginalPdfFromBookMasterSets(int nPDFType, CUIntArray &sArrMasterCopySeparationSets, CStringArray &sArrFileNames, CString sOutputFileName, BOOL bIgnoreMissing, int nPageToProcess, CString &sErrorMessage);

int MakeOriginalPdfBook(CDatabaseManager &db, int nProductionID, int nEditionID, int nPDFType, CString sOutputFileName, int nSectionID, BOOL bIgnoreMissing, int nPageToProcess, CString &sErrorMessage);
