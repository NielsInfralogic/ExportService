#pragma once
#include "stdafx.h"

/////////////////////////////////////////
//
// Generel definitions
//
/////////////////////////////////////////


#define MAXFILES 10000
#define MAXFILESPERTHREAD 500
#define MAXCHANNELS 500
#define MAX_ERRMSG 16384
#define MAXREGEXP 100
#define MAXTRANSMITQUEUE 1000
#define MAXCHANNELSPERPUBLICATION 100

#define REC_BUFFER_SIZE 65535
#define TOP_BUFFER_SIZE 2048

#define MESSAGETIMEOUT 1000*5

#define SERVICETYPE_PLANIMPORT 1
#define SERVICETYPE_FILEIMPORT 2
#define SERVICETYPE_PROCESSING 3
#define SERVICETYPE_EXPORT     4
#define SERVICETYPE_DATABASE   5
#define SERVICETYPE_FILESERVER 6
#define SERVICETYPE_MAINTENANCE 7

#define SERVICESTATUS_STOPPED 0
#define SERVICESTATUS_RUNNING 1
#define SERVICESTATUS_HASERROR 2

#define POLLINPUTTYPE_LOWRESPDF 0
#define POLLINPUTTYPE_HIRESPDF 1
#define POLLINPUTTYPE_PRINTPDF 2

#define CHANNELSTATUSTYPE_INPUT 0
#define CHANNELSTATUSTYPE_PROCESSING 1
#define CHANNELSTATUSTYPE_EXPORT 2 
#define CHANNELSTATUSTYPE_MERGE 3

#define EXPORTSTATUSNUMBER_EXPORTING   5
#define EXPORTSTATUSNUMBER_EXPORTERROR 6
#define EXPORTSTATUSNUMBER_EXPORTINGSENT   9
#define EXPORTSTATUSNUMBER_EXPORTED   10

#define EVENTCODE_EXPORTQUEUED		30
#define EVENTCODE_EXPORTQUEUED_ERROR		26
#define EVENTCODE_EXPORTQUEUING		25

#define EVENTCODE_EXPORTED		180
#define EVENTCODE_EXPORTERROR		186

#define SCHEDULE_ALWAYS	0
#define SCHEDULE_WINDOW 1

#define MAILTYPE_DBERROR 0
#define MAILTYPE_FILEERROR	1
#define MAILTYPE_FOLDERERROR 2

#define TEMPFILE_NONE	0
#define TEMPFILE_NAME	1
#define TEMPFILE_FOLDER	2

#define LOCKCHECKMODE_NONE	0
#define LOCKCHECKMODE_READ	1
#define LOCKCHECKMODE_READWRITE	2
#define LOCKCHECKMODE_RANGELOCK	3

#define ALIASTYPE_NONE		0
#define ALIASTYPE_INPUT		1
#define ALIASTYPE_OUTPUT	2
#define ALIASTYPE_EXTENDED	3




#define JMFSTATUS_OK 0
#define JMFSTATUS_ERROR 1
#define JMFSTATUS_WARNING 2
#define JMFSTATUS_UNKNOWN -1

#define JMFNETWORK_SHARE	0
#define JMFNETWORK_FTP		1

#define NETWORKTYPE_SHARE	0
#define NETWORKTYPE_FTP		1
#define NETWORKTYPE_EMAIL	2

#define COPYTHREAD_STARTING	            10000
#define COPYTHREAD_STARTED              10001
#define COPYTHREAD_INPROGRESS           10002
#define COPYTHREAD_DONE                 10003
#define COPYTHREAD_ABORT                10004
#define COPYTHREAD_ERROR				10005
#define COPYTHREAD_TIMEOUT				10006

#define REMOTEFILECHECK_NONE		0
#define REMOTEFILECHECK_EXIST		1
#define REMOTEFILECHECK_SIZE		2
#define REMOTEFILECHECK_READBACK	3

#define FTPTYPE_NORMAL	0
#define FTPTYPE_FTPES	1
#define FTPTYPE_FTPS	2
#define FTPTYPE_SFTP	3

/////////////////////////////////////////
// Page table definitions
/////////////////////////////////////////



#define TXNAMEOPTION_REMOVESPACES		1
#define TXNAMEOPTION_ALLSMALLCPAS		2
#define TXNAMEOPTION_ZIPPAGES			4
//#define TXNAMEOPTION_MERGEPAGES			8


#define TXNAMEOPTION_GENERATEXML		4
#define TXNAMEOPTION_SENDCOMMONPAGES	8

#define PAGETYPE_NORMAL					0
#define PAGETYPE_PANORAMA				1
#define PAGETYPE_ANTIPANORAMA			2
#define PAGETYPE_DUMMY					3



/////////////////////////////////////////
// Internal status definitions
/////////////////////////////////////////



#define STATE_RELEASED				0
#define STATE_LOCKED				1
#define STATE_INPROGRESS			2


/////////////////////////////////////////
// Status codes
/////////////////////////////////////////

#define STATUSNUMBER_MISSING			0
#define STATUSNUMBER_POLLING			5
#define STATUSNUMBER_POLLINGERROR		6
#define STATUSNUMBER_POLLED				10
#define STATUSNUMBER_RESAMPLING			15
#define STATUSNUMBER_RESAMPLINGERROR	16
#define STATUSNUMBER_READY				20
#define STATUSNUMBER_TRANSMITTING		25
#define STATUSNUMBER_TRANSMISSIONERROR	26
#define STATUSNUMBER_REMOTETRANSMITTED	29
#define STATUSNUMBER_TRANSMITTED		30
#define STATUSNUMBER_ASSEMBLING			35
#define STATUSNUMBER_ASSEMBLINGERROR	36
#define STATUSNUMBER_ASSEMBLED			40
#define STATUSNUMBER_IMAGING			45
#define STATUSNUMBER_IMAGINGERROR		46
#define STATUSNUMBER_IMAGED				50
#define STATUSNUMBER_VERIFYING			55
#define STATUSNUMBER_VERIFYERROR		56
#define STATUSNUMBER_VERIFIED			60
#define STATUSNUMBER_KILLED				99



typedef struct {
	CString sStatus;
	CString	sFolder;
	CString	sFileName;
	CString	sMessage;
	int     nChannelID;
	DWORD	dwGUID;
} LOGLISTITEM;

typedef struct {
	int nTick;
	CString	sFileName;
	int nSetupIndex;
	int nStatus;
	CString sMessage;
	BOOL bEnabled;
} PROGRESSLOGITEM;

typedef struct  {
	CString	sFileName;
	CString	sFolder;
	CTime	tJobTime;
	CTime	tWriteTime;
	DWORD	nFileSize;
	CString sFileNameStripped;
	int		nMasterCopySeparationSet;
} FILEINFOSTRUCT;

typedef CList <FILEINFOSTRUCT,FILEINFOSTRUCT&> FILELISTTYPE;

typedef struct {
	CString	sSrcFileName;
	CString	sDstFileName;
	int		nStatus;
	CString sErrorMessage;
	int		nTimeOut;
} FILECOPYINFO;

class ALIASSTRUCT {
public:
	ALIASSTRUCT() { sType = _T("Color"); sLongName = _T(""); sShortName = _T(""); };
	CString	sType;
	CString sLongName;
	CString sShortName;
};
typedef CArray <ALIASSTRUCT, ALIASSTRUCT&> ALIASLIST;

class REGEXPSTRUCT {
	public:
	REGEXPSTRUCT() {	m_useExpression = FALSE; 
						m_matchExpression = _T("");
						m_formatExpression = _T("");
						m_partialmatch = FALSE;};

	BOOL		m_useExpression;
	CString		m_matchExpression;
	CString		m_formatExpression;
	BOOL		m_partialmatch;
};


typedef struct {
	int m_ID;
	CString m_name;
} ITEMSTRUCT;

typedef CArray <ITEMSTRUCT,ITEMSTRUCT&> ITEMLIST;

typedef struct {
	int		m_mastercopyseparationset;
	int		m_copyseparationset;
	int		m_approved;
	CString m_jobname;
	int		m_inputID;
	int     m_status;
	int		m_version;
	int		m_channelID;
	CString m_filename;
	int     m_pdftype;
	int		m_publicationID;
	BOOL	m_testfileexists;
	int		m_productionID;
} NextTransmitJob;




class STATUSSTRUCT {
public:
	STATUSSTRUCT() { m_statusnumber = 0; m_statusname = _T(""); m_statuscolor = _T("FFFFFF"); };
	int	m_statusnumber;
	CString m_statusname;
	CString m_statuscolor;
};

typedef CArray <STATUSSTRUCT, STATUSSTRUCT&> STATUSLIST;

class ALIASTABLE {
public:
	ALIASTABLE() { sType = _T("Color"); sLongName = _T(""); sShortName = _T(""); };
	CString	sType;
	CString sLongName;
	CString sShortName;
};


typedef struct {
	int m_channelID;
	int m_pushtrigger;
	int m_pubdatemovedays;
	int m_releasedelay;
	BOOL m_sendplan;
} PUBLICATIONCHANNELSTRUCT;



typedef struct {
	int m_packageID;
	CString m_name;
	int m_publicationID;
	int m_sectionIndex;
	int m_condition;
	CString m_comment;
	CTime m_configchangetime;
} PACKAGESTRUCT;

typedef CArray <PACKAGESTRUCT, PACKAGESTRUCT&> PACKAGELIST;


class PUBLICATIONSTRUCT {
public:
	PUBLICATIONSTRUCT() {
		m_name = _T("");
		m_PageFormatID = 0;
		m_TrimToFormat = FALSE;
		m_LatestHour = 0.0;
		m_ProofID = 1;
		m_Approve = FALSE;
		m_autoPurgeKeepDays = 0;
		m_emailrecipient = _T("");		
		m_emailCC = _T("");
		m_emailSubject = _T("");
		m_emailBody = _T("");
		m_customerID = 0;
		m_uploadfolder = _T("");
		m_deadline = CTime(1975, 1, 1);
	
		m_allowunplanned = TRUE;
		m_priority = 50;
		m_annumtext = _T("");
		m_releasedays = 0;
		m_releasetimehour = 0;
		m_releasetimeminute = 0;
		m_pubdatemove = FALSE;
		m_pubdatemovedays = 0;
		m_alias = _T("");
		m_outputalias = _T("");
		m_extendedalias = _T("");
		m_extendedalias2 = _T("");
		m_publisherID = 0;
		m_numberOfChannels = 0;
		for (int i = 0; i < MAXCHANNELSPERPUBLICATION; i++)
			m_channels[i].m_channelID = 0;

		m_configchangetime = CTime(1975, 1, 1);
	};

	int			m_ID;
	CString		m_name;
	int			m_PageFormatID;
	int			m_TrimToFormat;
	double		m_LatestHour;
	int			m_ProofID;

	int			m_Approve;

	int			m_autoPurgeKeepDays;
	CString		m_emailrecipient;
	CString		m_emailCC;
	CString		m_emailSubject;
	CString		m_emailBody;
	int			m_customerID;
	CString		m_uploadfolder;
	CTime		m_deadline;

//	CString		m_channelIDList;

	BOOL		m_allowunplanned;
	int			m_priority;

	CString		m_annumtext;

	int			m_releasedays;
	int			m_releasetimehour;
	int			m_releasetimeminute;


	BOOL		m_pubdatemove;
	int  		m_pubdatemovedays;
	CString		m_alias;
	CString		m_outputalias;
	CString		m_extendedalias;
	CString		m_extendedalias2;
	int			m_publisherID;

	int			m_numberOfChannels;
	PUBLICATIONCHANNELSTRUCT m_channels[MAXCHANNELSPERPUBLICATION];

	CTime		m_configchangetime;
};

typedef CArray <PUBLICATIONSTRUCT, PUBLICATIONSTRUCT&> PUBLICATIONLIST;



class CHANNELSTRUCT {
public:
	CHANNELSTRUCT() {
		m_ownerinstance = -1;
		m_channelID = 0;
		m_channelname = _T("");
		m_channelnamealias = _T("");
		m_enabled = TRUE;

		m_usereleasetime = FALSE;
		m_releasetimehour = 0;
		m_releasetimeminute = 0;

		m_transmitnameconv = _T("");
		m_transmitdateformat = _T("");
		m_transmitnameoptions = 0;
		m_transmitnameuseabbr = TRUE;
		m_subfoldernameconv = _T("");

		m_configdata = _T("");

		m_pdftype = POLLINPUTTYPE_LOWRESPDF;
		m_mergePDF = FALSE;

		m_precommand = _T("");
		m_useprecommand = FALSE;

		m_archivefolder = _T("");
		m_archivefiles = FALSE;
		m_archivefolder2 = _T("");
		m_archivefiles2 = FALSE;

		m_moveonerror = FALSE;
		m_errorfolder = _T("");

		m_outputfolder = _T("");
		m_outputusecurrentuser = TRUE;
		m_outputusername = _T("");
		m_outputpassword = _T("");

		m_outputuseftp = NETWORKTYPE_SHARE;
		m_outputftpserver = _T("");
		m_outputftpuser = _T("");
		m_outputftppw = _T("");
		m_outputftpfolder = _T("");
		m_outputftpport = 21;
		m_outputftpbuffersize = 4096;
		m_outputftpXCRC = FALSE;
		//m_useack = FALSE;
	//	m_statusfolder = _T("");
		m_outputftppasv = TRUE;

		m_useregexp = FALSE;

		m_NumberOfRegExps = 0;
		for (int j = 0; j < MAXREGEXP; j++) {
			m_RegExpList[j].m_matchExpression = _T("");
			m_RegExpList[j].m_formatExpression = _T("");
			m_RegExpList[j].m_partialmatch = FALSE;
		}

		m_usescript = FALSE;
		m_script = _T("");
		m_nooverwrite = FALSE;

		m_retries = 3;
		m_keepoutputconnection = TRUE;
		m_timeout = 60;

		m_outputftpusetempfolder = FALSE;
		m_outputftpaccount = _T("");
		m_outputftptempfolder = _T("");
		m_outputftppasv = FALSE;
		m_FTPpostcheck = REMOTEFILECHECK_SIZE;
		m_outputfpttls = FTPTYPE_NORMAL;
		m_ftptimeout = 120;

		m_ensureuniquefilename = FALSE;

	//	m_archivedays = 0;

		m_breakifnooutputaccess = TRUE;

		m_filebuildupmethod = TEMPFILE_NAME;

		m_copymatchexpression = _T("");
		m_archiveconditionally = FALSE;

		m_inputemailnotification = FALSE;
		m_inputemailnotificationTo = _T("");
		m_inputemailnotificationSubject = _T("");

		m_emailsmtpserver = _T("");
		m_emailsmtpport = 25;
		m_emailsmtpserverusername = _T("");
		m_emailsmtpserverpassword = _T("");
		m_emailfrom = _T("");
		m_emailto = _T("");
		m_emailcc = _T("");
		m_emailbody = _T("");
		m_emailsubject = _T("");
		m_emailusehtml = FALSE;
		m_emailusessl = FALSE;
		m_emailusestartTLS = FALSE;
		m_emailtimeout = 60;

		m_editionstogenerate = 0;
		m_subfoldernameconv = _T("");

		m_configchangetime = CTime(1975, 1, 1);

		m_pdfprocessID = 0;

		m_usepackagenames = FALSE;
		
	};



	int		m_channelID;
	CString m_channelname;
	int     m_ownerinstance;
	BOOL	m_enabled;

//	int		m_channelgroupID;
//	int     m_publisherID;
	BOOL	m_usereleasetime;
	int		m_releasetimehour;
	int		m_releasetimeminute;

	CString m_transmitnameconv;
	CString m_transmitdateformat;
	int		m_transmitnameoptions;
	int		m_transmitnameuseabbr;
	CString m_subfoldernameconv;

	int		m_miscint;
	CString m_miscstring;

	int     m_pdftype;
	BOOL	m_mergePDF;


	CString m_configdata;

	CString m_archivefolder;
	BOOL	m_archivefiles;
	CString m_archivefolder2;
	BOOL	m_archivefiles2;
//	int		m_archivedays;

	CString m_precommand;
	BOOL m_useprecommand;


	BOOL	m_moveonerror;
	CString m_errorfolder;
	CString m_outputfolder;

	BOOL	m_outputusecurrentuser;
	CString m_outputusername;
	CString m_outputpassword;

	int		m_outputuseftp;

	CString m_outputftpserver;
	CString	m_outputftpuser;
	CString	m_outputftppw;
	CString m_outputftpfolder;

	BOOL	m_outputftpusetempfolder;
	CString	m_outputftpaccount;
	CString	m_outputftptempfolder;
	BOOL	m_outputftppasv;
	int		m_FTPpostcheck;
	int		m_outputftpbuffersize;
	int		m_outputftpport;
	BOOL	m_outputftpXCRC;
	int		m_outputfpttls;
	int		m_ftptimeout;

//	BOOL	m_useack;
//	CString m_statusfolder;
	BOOL	m_ensureuniquefilename;
	BOOL	m_useregexp;
	int		m_NumberOfRegExps;
	REGEXPSTRUCT	m_RegExpList[MAXREGEXP];

	BOOL	m_usescript;
	CString	m_script;

	BOOL	m_nooverwrite;
	int		m_retries;
	BOOL	m_keepoutputconnection;
	int		m_timeout;
	
	BOOL	m_breakifnooutputaccess;

	int m_filebuildupmethod;

	CString m_copymatchexpression;
	BOOL m_archiveconditionally;

	BOOL m_inputemailnotification;
	CString m_inputemailnotificationTo;
	CString m_inputemailnotificationSubject;

	CString m_emailsmtpserver;
	int m_emailsmtpport;
	CString m_emailsmtpserverusername;
	CString m_emailsmtpserverpassword;
	CString m_emailfrom;
	CString m_emailto;
	CString m_emailcc;
	CString m_emailbody;
	CString m_emailsubject;
	BOOL m_emailusehtml;
	BOOL m_emailusessl;
	BOOL m_emailusestartTLS;
	int m_emailtimeout;

	CString m_channelnamealias;
	int		m_editionstogenerate;
	BOOL	m_sendCommonPages;

	CTime m_configchangetime;


	int m_pdfprocessID;

	BOOL m_usepackagenames;

};
typedef CArray <CHANNELSTRUCT, CHANNELSTRUCT&> CHANNELLIST;




typedef struct  {
	CString m_servername;
	CString m_sharename;
	int	m_servertype;
	CString m_IP;
	CString m_username;
	CString m_password;
	int	m_locationID;
	BOOL m_uselocaluser;
} FILESERVERSTRUCT;

typedef CArray <FILESERVERSTRUCT, FILESERVERSTRUCT&> FILESERVERLIST;

class CJobAttributes {
public:
	CJobAttributes() {};

	//~CJobAttributes() {
		//m_pagename.Empty();
	//};

	void InitJobAttributes() {
		Init();
		m_ccdatafilename = _T("");
		m_inputID = 0;
		m_sectionID = 0;
		m_editionID =0;
		m_pressrunID = 0;
		m_locationID = 1;
		m_commoneditionID = 0;
		m_pressID = 1;
		m_pressgroupID = 0;
		m_publicationID = 0;
		m_templateID = 0;
		m_prooftemplateID = 0;

		m_copies = 1;
		CTime tm = CTime(1975, 1, 1, 0, 0, 0);
		//			if (tm.GetHour() <= 6)
		//	tm += CTimeSpan( 1, 0, 0, 0 );
		m_pubdate = tm;
		m_originalfilename = _T("");
		m_previousstatus = 0;
		m_previousactive = 1;
		m_weekreference = 0;
		m_version = 0;
		m_locationgroupIDList = _T("");
		m_locationgroup = _T("");
		m_miscstring1 = _T("");
		m_miscstring2 = _T("");
		m_miscstring3 = _T("");
	}

	void Init()
	{
		m_planname = "";
		m_ccdatafilename = "";

		m_sectionID = 0;
		m_editionID = 0;
		m_pressrunID = 0;
		m_locationID = 0;
		m_publicationID = 0;
		m_templateID = 0;
		m_prooftemplateID = 0;
		m_copies = 1;

		m_deviceID = 0;
		m_copynumber = 1;
		m_pagename = _T("1");
		m_colorname = _T("K");
		CTime t = CTime::GetCurrentTime();	// Default to neext day
		t += CTimeSpan(1, 0, 0, 0);

		m_pubdate = t;
		m_auxpubdate = t;
		m_pubdateweekstart = t;
		m_pubdateweekend = t;
		m_hasweekpubdate = FALSE;

		m_version = 0;

		m_mastercopyseparationset = 1;
		m_copyseparationset = 1;
		m_separationset = 101;
		m_separation = 10101;
		m_copyflatseparationset = 1;
		m_flatseparationset = 101;
		m_flatseparation = 10101;

		m_numberofcolors = 1;
		m_colornumber = 1;

		m_layer = 1;
		m_filename = _T("");
		m_pagination = 0;
		m_pageindex = 0;

		m_priority = 50;
		m_comment = _T("");
		m_pageposition = 1;
		m_pagetype = PAGETYPE_NORMAL;
		m_pagesonplate = 1;
		m_hold = FALSE;
		m_approved = FALSE;
		m_active = TRUE;

		m_status = STATUSNUMBER_MISSING;
		m_externalstatus = 0;
		CTime t2;
		m_deadline = t2;
		m_pressID = 0;
		m_presssectionnumber = 0;
		m_sortnumber = 0;
		m_pubdatefound = FALSE;

		m_presstower = _T("1");
		m_presszone = _T("1");
		m_presscylinder = _T("1");
		m_presshighlow = _T("H");
		m_productionID = 0;
		m_inputID = 0;
		m_jobname = _T("");
		m_markgroup = _T("");
		m_includemarkgroup = 0;
		m_currentmarkgroup = _T("");

		m_commoneditionID = 0;

		m_originalfilename = _T("");

		m_previousstatus = 0;
		m_previousactive = TRUE;
		m_pressruncomment = "";
		m_pressruninkcomment = "";
		m_pressrunordernumber = "";

		m_customername = _T("");
		m_customeralias = _T("");
		m_customeremail = _T("");
		m_originalmastercopyseparationset = 0;
		m_panomate = _T("");
		m_miscstring1 = _T("");
		m_miscstring2 = _T("");
		m_miscstring3 = _T("");

		m_locationgroupIDList = _T("");
		m_locationgroup = _T("");

		m_publisherID = 0;
		m_annumtext = _T("");
	};

	CString m_planname;
	CString m_ccdatafilename;

	CTime	m_pubdate;
	CTime	m_auxpubdate;
	CTime	m_pubdateweekstart;
	CTime	m_pubdateweekend;
	BOOL	m_hasweekpubdate;

	CString	m_pagename;	// page number!
	CString m_colorname;
	int		m_publicationID;
	int		m_sectionID;
	int		m_editionID;
	int		m_pressrunID;
	int		m_templateID;
	int		m_deviceID;
	int		m_prooftemplateID;
	int		m_locationID;
	int		m_pressID;
	int		m_inputID;
	int		m_pressgroupID;

	int		m_copies;
	int		m_copynumber;
	int		m_version;
	int		m_layer;
	CString m_filename;
	int		m_pagination;
	int		m_pageindex;
	int		m_priority;
	CString	m_comment;

	int		m_pageposition;
	int		m_pagetype;
	int		m_pagesonplate;
	int		m_colornumber;
	int		m_numberofcolors;

	BOOL	m_hold;
	int		m_approved;
	BOOL	m_active;

	int		m_sheetnumber;
	int		m_sheetside;

	int		m_mastercopyseparationset;
	int		m_copyseparationset;
	int		m_separationset;
	int		m_separation;
	int		m_copyflatseparationset;
	int		m_flatseparationset;
	int		m_flatseparation;

	int		m_status;
	int		m_externalstatus;
	CTime	m_deadline;
	int		m_presssectionnumber;
	int		m_sortnumber;
	BOOL	m_pubdatefound;

	CString	m_presstower;
	CString	m_presszone;
	CString	m_presscylinder;
	CString	m_presshighlow;

	int		m_productionID;


	CString m_jobname;

	CString m_currentmarkgroup;
	CString m_markgroup;
	int		m_includemarkgroup;

	CString m_polledpublicationname;
	CString m_polledsectionname;
	CString m_pollededitionname;
	CString m_polledissuename;

	int		m_commoneditionID;
	CString m_originalfilename;

	int		m_previousstatus;
	BOOL	m_previousactive;

	int		m_weekreference;

	CString m_pressruncomment;
	CString m_pressruninkcomment;
	CString m_pressrunordernumber;

	CString m_customername;
	CString m_customeralias;
	CString m_customeremail;
	int		m_originalmastercopyseparationset;

	CString m_panomate;
	CString m_locationgroupIDList;
	CString m_locationgroup;

	CString m_miscstring1;
	CString m_miscstring2;
	CString	m_miscstring3;
	int m_publisherID;
	CString m_annumtext;
};

typedef CArray <CJobAttributes, CJobAttributes&> CJobAttributesList;

