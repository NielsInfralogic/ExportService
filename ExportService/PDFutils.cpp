#include "stdafx.h"
#include "Defs.h"
#include "Utils.h"
#include "Prefs.h"
#include "math.h"
#include <iostream>
#include "pdflib.h"
#include "PDFutils.h"
#include "DatabaseManager.h"

extern CPrefs g_prefs;
extern CUtils g_util;

//#define PDFLIB_LICENSE  "W900202-010080-132518-UR4KH2-WDXFD2"
#define PDFLIB_LICENSE  "W900202-010092-132518-5CDVH2-68MH72" //; - 29. Aug 2019


BOOL IsPDF(CString sFileName)
{
	FILE* fp;
	char data[8];

	TCHAR filename[MAX_PATH];
	strcpy(filename, sFileName);
	
	if ((fp = fopen( filename, "rb")) == 0) {
		return FALSE;
    }

//	%PDF-1.xx	

	if (fread(data, 1, 7, fp) != 7) {
		fclose(fp);
		return FALSE;
	}

	data[7] = 0;

	fclose(fp);

	if (!strcmp("%PDF-1.", data)) {
		return TRUE;
    }

	return FALSE;
}




BOOL GetPDFinfo(CString sInputFileName, int &nPageCount, double &fSizeX, double &fSizeY)
{
	CUtils util;
	BOOL bRet = TRUE;
	CString sErrorMessage;

	TCHAR szFeature[200];
	_tcscpy(szFeature, "");

	int doc = -1;
	int page = -1;
	char *searchpath = (char *) "../data";

	PDF *p = PDF_new();
	if (p == (PDF *)0) {
		sErrorMessage.Format( "ERROR: Couldn't create PDFlib object (out of memory)!");
		util.Logprintf("ERROR: PDFInfo() %s", sErrorMessage);
		return FALSE;
	}
	PDF_TRY(p) {

		PDF_set_parameter(p, "errorpolicy", "return");
		PDF_set_parameter(p, "license", PDFLIB_LICENSE);


		if (PDF_begin_document(p, g_util.GetTempFolder() + _T("\\dummy.pdf"), 0, "") == -1) {
			//util.LogprintfResample("ERROR: %s", szErrorMessage);
			PDF_delete(p);
			return FALSE;
		}


		doc = PDF_open_pdi_document(p, sInputFileName, 0, "");
		if (doc == -1) {
			sErrorMessage.Format( "Couldn't open PDF input file '%s'", sInputFileName);
			util.Logprintf("ERROR: PDFInfo() %s", sErrorMessage);
			PDF_delete(p);
			return FALSE;
		}

		int  numberofpages = (int)PDF_pcos_get_number(p, doc, "/Root/Pages/Count");
		nPageCount = numberofpages;

		// Get documnet information from input file
	//	util.Logprintf("INFO: PDFlib PDF_open_pdi called");

		page = PDF_open_pdi_page(p, doc, 1, "");
		if (page == -1) {
			sErrorMessage.Format( "Couldn't open page 1 of PDF file '%s'", sInputFileName);
			util.Logprintf("ERROR: PDFInfo() %s", sErrorMessage);
			PDF_delete(p);
			return FALSE;
		}
		//	util.Logprintf("INFO: PDFlib PDF_open_pdi_page called");

		double width = PDF_pcos_get_number(p, doc, "pages[0]/width");
		double height = PDF_pcos_get_number(p, doc, "pages[0]/height");

		fSizeX = width * 25.4 / 72.0;
		fSizeY = height * 25.4 / 72.0;
		//	util.Logprintf("INFO: PDF size %.4f,%.4f", width, height);


		PDF_begin_page_ext(p, width, height, "");

		PDF_fit_pdi_page(p, page, 0, 0, "");
		PDF_close_pdi_page(p, page);

		PDF_end_page_ext(p, "");
		PDF_end_document(p, "");
		DeleteFile("c:\\temp\\dummy.pdf");

	}
	PDF_CATCH(p) {
		sErrorMessage.Format("PDFlib exception occurred: [%d] %s: %s",
			PDF_get_errnum(p), PDF_get_apiname(p), PDF_get_errmsg(p));
		PDF_delete(p);
		util.Logprintf("ERROR: PDFInfo() %s", sErrorMessage);
		return FALSE;
	}
	PDF_delete(p);
	return TRUE;

}


int MakeOriginalPdfBook(CDatabaseManager &db, int nProductionID, int nEditionID, int nPDFType, CString sOutputFileName, int nSectionID, BOOL bIgnoreMissing, int nPageToProcess, CString &sErrorMessage)
{
	CUtils util;
	sErrorMessage = _T("");

/*	TCHAR szOutputFileName[MAX_PATH];
	strcpy(szOutputFileName, sOutputFileName);
	CString sFileTitle = szOutputFileName;
	sFileTitle = util.GetFileName(sFileTitle);
	int n = sFileTitle.ReverseFind('.');
	if (n != -1)
		sFileTitle = sFileTitle.Left(n);
		*/
	CUIntArray sArrMasterCopySeparationSets;
	CStringArray sArrFileNames;

	if (nSectionID == 0) {
		if (db.GetMasterCopySeparationListPDF(TRUE, nPDFType, nProductionID, nEditionID, sArrMasterCopySeparationSets, sArrFileNames, sErrorMessage) == FALSE)
			return FALSE;
	}
	else {
		if (db.GetMasterCopySeparationListPDF(TRUE, nPDFType, nProductionID, nEditionID, nSectionID, sArrMasterCopySeparationSets, sArrFileNames, sErrorMessage) == FALSE)
			return FALSE;
	}
	
	return MakeOriginalPdfFromBookMasterSets(nPDFType, sArrMasterCopySeparationSets, sArrFileNames, sOutputFileName, bIgnoreMissing, nPageToProcess, sErrorMessage);

/*	::DeleteFile(szOutputFileName);

	if (sArrMasterCopySeparationSets.GetCount() == 0) {
		sErrorMessage = _T("No previews ready for product");
		return FALSE;
	}

	if (sArrFileNames.GetCount() < nPageToProcess) {
		sErrorMessage.Format("Only %d pages ready - expected %d", sArrFileNames.GetCount(), nPageToProcess);
		return FALSE;
	}

	BOOL bHasFiles = FALSE;
	for (int i = 0; i < sArrMasterCopySeparationSets.GetCount(); i++) {

		CString sFileName = g_prefs.FormCCFilesName(nPDFType, sArrMasterCopySeparationSets[i], sArrFileNames[i]);

		if (util.FileExist(sFileName) == FALSE) {
			sErrorMessage.Format("Unable to load PDF file %s", sFileName);
			if (bIgnoreMissing)
				sArrMasterCopySeparationSets[i] = 0;
			else {
				::DeleteFile(szOutputFileName);
				return FALSE;
			}
		}
		else
			bHasFiles = TRUE;
	}

	if (bHasFiles == FALSE) {
		sErrorMessage =  _T("Not all files present for PDF merge generator");
		return FALSE;
	}

	PDF *p = PDF_new();
	if (p == (PDF *)0) {
		sErrorMessage = _T("ERROR: Couldn't create PDFlib object");
		util.Logprintf("ERROR: %s", sErrorMessage);
		return FALSE;
	}
	PDF_TRY(p) {

		PDF_set_parameter(p, "errorpolicy", "return");
		PDF_set_parameter(p, "license", PDFLIB_LICENSE);


		if (PDF_begin_document(p, szOutputFileName, 0, "") == -1) {
			sErrorMessage.Format("%s", PDF_get_errmsg(p));
			PDF_delete(p);
			::DeleteFile(szOutputFileName);
			return FALSE;
		}

		PDF_set_info(p, "Creator", "InfraLogic");
		PDF_set_info(p, "Author", "PlanCenter");
		PDF_set_info(p, "Title", sFileTitle);

		for (int i = 0; i < sArrMasterCopySeparationSets.GetCount(); i++) {
			if (sArrMasterCopySeparationSets[i] == 0)
				continue;
			CString sPDFinputFile = g_prefs.FormCCFilesName(nPDFType, sArrMasterCopySeparationSets[i], sArrFileNames[i]);


			//int image = PDF_load_image(p, "jpeg", szImageFile, 0, "");
			int indoc = PDF_open_pdi_document(p, sPDFinputFile, 0, "");

			if (indoc == -1)
			{
				sErrorMessage.Format( "PDF error (load PDF page %s) - %s", sPDFinputFile, PDF_get_errmsg(p));
				PDF_delete(p);
				::DeleteFile(szOutputFileName);
				return FALSE;
			}

			int page = PDF_open_pdi_page(p, indoc, 1, "");

			PDF_begin_page_ext(p, 10, 10, "");

			PDF_fit_pdi_page(p, page, 0, 0, "adjustpage");
			PDF_close_pdi_page(p, page);
			PDF_end_page_ext(p, "");
			PDF_close_pdi_document(p, indoc);
		}
		PDF_end_document(p, "");
	}

	PDF_CATCH(p) {
		sErrorMessage.Format("PDFlib problem: %s", PDF_get_errmsg(p));
		PDF_delete(p);
		::DeleteFile(szOutputFileName);
		return FALSE;
	}

	PDF_delete(p);
	return TRUE;
	*/
}

int MakeOriginalPdfFromBookMasterSets(int nPDFType, CUIntArray &sArrMasterCopySeparationSets, CStringArray &sArrFileNames, CString sOutputFileName, BOOL bIgnoreMissing, int nPageToProcess, CString &sErrorMessage)
{
	CUtils util;
	sErrorMessage = _T("");

	TCHAR szOutputFileName[MAX_PATH];
	strcpy(szOutputFileName, sOutputFileName);
	CString sFileTitle = szOutputFileName;
	sFileTitle = util.GetFileName(sFileTitle);
	int n = sFileTitle.ReverseFind('.');
	if (n != -1)
		sFileTitle = sFileTitle.Left(n);

	::DeleteFile(szOutputFileName);

	if (sArrMasterCopySeparationSets.GetCount() == 0) {
		sErrorMessage = _T("No previews ready for product");
		return FALSE;
	}

	if (sArrFileNames.GetCount() < nPageToProcess) {
		sErrorMessage.Format("Only %d pages ready - expected %d", sArrFileNames.GetCount(), nPageToProcess);
		return FALSE;
	}

	BOOL bHasFiles = FALSE;
	for (int i = 0; i < sArrMasterCopySeparationSets.GetCount(); i++) {

		CString sFileName = g_prefs.FormCCFilesName(nPDFType, sArrMasterCopySeparationSets[i], sArrFileNames[i]);

		if (util.FileExist(sFileName) == FALSE) {
			sErrorMessage.Format("Unable to load PDF file %s", sFileName);
			if (bIgnoreMissing)
				sArrMasterCopySeparationSets[i] = 0;
			else {
				::DeleteFile(szOutputFileName);
				return FALSE;
			}
		}
		else
			bHasFiles = TRUE;
	}

	if (bHasFiles == FALSE) {
		sErrorMessage = _T("Not all files present for PDF merge generator");
		return FALSE;
	}

	PDF *p = PDF_new();
	if (p == (PDF *)0) {
		sErrorMessage = _T("ERROR: Couldn't create PDFlib object");
		util.Logprintf("ERROR: %s", sErrorMessage);
		return FALSE;
	}
	PDF_TRY(p) {

		PDF_set_parameter(p, "errorpolicy", "return");
		PDF_set_parameter(p, "license", PDFLIB_LICENSE);

		if (PDF_begin_document(p, szOutputFileName, 0, "") == -1) {
			sErrorMessage.Format("%s", PDF_get_errmsg(p));
			PDF_delete(p);
			::DeleteFile(szOutputFileName);
			return FALSE;
		}

		PDF_set_info(p, "Creator", "InfraLogic");
		PDF_set_info(p, "Author", "PlanCenter");
		PDF_set_info(p, "Title", sFileTitle);

		for (int i = 0; i < sArrMasterCopySeparationSets.GetCount(); i++) {
			if (sArrMasterCopySeparationSets[i] == 0)
				continue;
			CString sPDFinputFile = g_prefs.FormCCFilesName(nPDFType, sArrMasterCopySeparationSets[i], sArrFileNames[i]);


			//int image = PDF_load_image(p, "jpeg", szImageFile, 0, "");
			int indoc = PDF_open_pdi_document(p, sPDFinputFile, 0, "");

			if (indoc == -1)
			{
				sErrorMessage.Format("PDF error (load PDF page %s) - %s", sPDFinputFile, PDF_get_errmsg(p));
				PDF_delete(p);
				::DeleteFile(szOutputFileName);
				return FALSE;
			}

			/* dummy page size, will be adjusted by PDF_fit_image() */
			int page = PDF_open_pdi_page(p, indoc, 1, "");

			PDF_begin_page_ext(p, 10, 10, "");

			PDF_fit_pdi_page(p, page, 0, 0, "adjustpage");
			PDF_close_pdi_page(p, page);
			PDF_end_page_ext(p, "");
			PDF_close_pdi_document(p, indoc);
		}
		PDF_end_document(p, "");
	}

	PDF_CATCH(p) {
		sErrorMessage.Format("PDFlib problem: %s", PDF_get_errmsg(p));
		PDF_delete(p);
		::DeleteFile(szOutputFileName);
		return FALSE;
	}

	PDF_delete(p);
	return TRUE;

}