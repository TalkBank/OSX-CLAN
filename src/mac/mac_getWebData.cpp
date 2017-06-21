#include "ced.h"
#include "cu.h"
#include "MMedia.h"

#define TOPROWOFFSET 35
#define NUMWEBITEMS  3000L
#define PREVDIRLABLE ":Previous Directory:"

#define CHILDES_URL   "http://childes.talkbank.org/data-orig/"
#define TALKBANK_URL   "http://www.talkbank.org/data-orig/"


static const char *ROOTWEBDIR = NULL;
static int   local_web_num_rows, char_height;
static char  DoubleClickCount = 1;
static char  *WebItems[NUMWEBITEMS+3];
static char  fulURLPath[2000];
static long  lastWhen = 0;
static short webItemsIndex = -1, maxWebItems = 0, topWebItemIndex = 0;
static Str255 pascalUsername;
static Str255 pascalPassword;
static EventHandlerRef WebDEventHandler = NULL;
static Point lastWhere = {0,0};

Boolean isHaveURLAccess;

static int getURLData(char *URLPath, FNType *gFile, Boolean *isDirectory, FNType *location) {
	int				res = FALSE;
	OSStatus		err;
//	URLOpenFlags 	openFlags = 0;
//	URLState		dlState = kURLNullState;
	FNType			fname[FILENAME_MAX];
//	URLReference	gURLRef;
	FSRef			tRef;
//	FSSpec			fss;
//	FILE			*fp;

	if (!isHaveURLAccess)
		return(FALSE);

	res = strlen(URLPath) - 1;
	if (URLPath[res] == '/')
		*isDirectory = TRUE;
	else
		*isDirectory = FALSE;
	for (res--; URLPath[res] != '/' && res >= 0; res--) ;
	strcpy(fname, URLPath+res+1);
	if (*isDirectory)
		fname[strlen(fname)-1] = EOS;


	if (location != NULL) {
		strcpy(gFile, location);
		addFilename2Path(gFile, fname);
	} else {
		err = FSFindFolder(kOnSystemDisk,kTemporaryFolderType,kDontCreateFolder,&tRef);
		if (err != noErr) {
			err = FSFindFolder(kUserDomain,kTemporaryFolderType,kDontCreateFolder,&tRef);
			if (err != noErr) {
				strcpy(gFile, webDownLoadDir);
				addFilename2Path(gFile, fname);
			}
		}
		if (err == noErr) {
			my_FSRefMakePath(&tRef, gFile, FNSize);
			addFilename2Path(gFile, fname);
		}
	}
	res = DownloadURL(URLPath, 0L, NULL, 0, gFile, FALSE, FALSE);
/*
	// The URLNewReference function creates a URL reference that you can use in subsequent calls to the
	// URL Access Manager. When you no longer need a URL reference, you should dispose of its memory by
	// calling the function URLDisposeReference. 
	err = URLNewReference(URLPath, &gURLRef);

	if (err == noErr) {
		// The URLSetProperty function enables you to set those property values identified by the following
		// constants: kURLPassword, kURLUserName, kURLPassword, kURLHTTPRequestMethod, kURLHTTPRequestHeader,
		// kURLHTTPRequestBody, and kURLHTTPUserAgent.
		URLSetProperty(gURLRef, kURLUserName, pascalUsername, pascalUsername[0] + 1);
		URLSetProperty(gURLRef, kURLPassword, pascalPassword, pascalPassword[0] + 1);
		if (location != NULL) {
			strcpy(gFile, location);
			addFilename2Path(gFile, fname);
			if ((fp=fopen(gFile, "w")) != NULL)
				fclose(fp);
			my_FSPathMakeRef(gFile, &tRef);
		} else {
			err = FSFindFolder(kOnSystemDisk,kTemporaryFolderType,kDontCreateFolder,&tRef);
			if (err != noErr) {
				err = FSFindFolder(kUserDomain,kTemporaryFolderType,kDontCreateFolder,&tRef);
				if (err != noErr) {
					strcpy(gFile, webDownLoadDir);
					addFilename2Path(gFile, fname);
					if ((fp=fopen(gFile, "w")) != NULL)
						fclose(fp);
					my_FSPathMakeRef(gFile, &tRef);
				}
			}
			
			if (err == noErr) {
				my_FSRefMakePath(&tRef, gFile, FNSize);
				addFilename2Path(gFile, fname);
				if ((fp=fopen(gFile, "w")) != NULL)
					fclose(fp);
				my_FSPathMakeRef(gFile, &tRef); 
			}
		}
		FSGetCatalogInfo(&tRef, kFSCatInfoNone, NULL, NULL, &fss, NULL);

		// The URLOpenFlags enumeration defines masks you can use to identify the data transfer options you
		// want used when performing data transfer operations. You pass this mask in the openFlags parameter
		// of the functions URLSimpleDownload, URLDownload, URLSimpleUpload, URLUpload, and URLOpen.
		openFlags |= kURLReplaceExistingFlag | kURLDisplayAuthFlag; // | kURLDisplayProgressFlag;

		// The kURLIsDirectoryHintFlag flag only works with URLDownload and only works with FTP.  If the URL
		// ended with a "/", I set the kURLIsDirectoryHintFlag flag and attempt to downlod the directory.
		if (*isDirectory)
			openFlags |= kURLIsDirectoryHintFlag;

		// The URLDownload function downloads data from a URL specified by a URL reference to a file, directory,
		// or memory. It does not return until the download is complete.
		err = URLDownload(gURLRef, &fss, NULL, openFlags, NULL, 0);

		err = URLGetCurrentState(gURLRef, &dlState);
		if (err == noErr) {
			switch (dlState) {
				case kURLErrorOccurredState:
					unlink(gFile);
					res = FALSE;
					URLGetError(gURLRef, &err);
					if (err == 403)
						sprintf(fname, "Error accessing \"%s\", permission denied. #%ld", URLPath, err);
					else
						sprintf(fname, "Error accessing \"%s\", error number: %ld", URLPath, err);
					do_warning(fname, 0);
	
					break;

			    case kURLCompletedState:
					URLGetProperty(gURLRef, kURLUserName, pascalUsername, 255);
					URLGetProperty(gURLRef, kURLPassword, pascalPassword, 255);
					res = TRUE;
					break;

			    default:
			        // This may happen if running on Mac OS 9 because of a bug in URL Access.
					res = FALSE;
					unlink(gFile);
					break;
			}
		}
		URLDisposeReference(gURLRef);
	}
	if (err != noErr)
		res = FALSE;
*/
	return(res);
}

static void freeWebItem(void) {
	int i;

	for (i=0; i < maxWebItems; i++) {
		if (WebItems[i] != NULL)
			free(WebItems[i]);
	}
	maxWebItems = 0;
	topWebItemIndex = 0;
	webItemsIndex = 0;
}

static void CleanupWeb(WindowPtr wind) {
	c2pstrcpy(pascalUsername, "");
	c2pstrcpy(pascalPassword, "");
	ROOTWEBDIR = NULL;
	freeWebItem();
	if (WebDEventHandler)
		RemoveEventHandler(WebDEventHandler);
	WebDEventHandler = NULL;
}

static void DrawWebHighlight(WindowPtr win) {
	register short i;
    register int print_row;
	Rect		theRect;
	Rect		rect;
	GrafPtr		oldPort;
	extern Boolean IsColorMonitor;

	GetPort(&oldPort);
	SetPortWindowPort(win);
	PenMode(((IsColorMonitor) ? 50 : notPatCopy));
	print_row = TOPROWOFFSET;
	for (i=topWebItemIndex; i < maxWebItems; i++) {
		print_row += char_height;
		if (webItemsIndex == i)
			break;
	}
	if (webItemsIndex != i || i >= local_web_num_rows+topWebItemIndex) {
		SetPort(oldPort);
		return;
	}
	theRect.left  = LEFTMARGIN;
	GetWindowPortBounds(win, &rect);
	theRect.right = rect.right - SCROLL_BAR_SIZE;
	theRect.bottom = print_row;
	theRect.top = theRect.bottom - char_height + FLINESPACE;
	theRect.bottom += FLINESPACE;
	PaintRect(&theRect);
	SetPort(oldPort);
}

static void SetWebScrollControl(WindowPtr win) {
	GrafPtr			savePort;
	WindowProcRec	*windProc;

	GetPort(&savePort);
 	SetPortWindowPort(win);
 	windProc = WindowProcs(win);
 	if (windProc != NULL && windProc->VScrollHnd != NULL) {
		if (topWebItemIndex == 0 && local_web_num_rows >= maxWebItems) {
			HiliteControl(windProc->VScrollHnd, 255);
		} else {
			HiliteControl(windProc->VScrollHnd,0);
			SetControlMaximum(windProc->VScrollHnd, maxWebItems-1);
			SetControlValue(windProc->VScrollHnd, topWebItemIndex);
		}
	}
	SetPort(savePort);
}

static void UpdateWeb(WindowPtr win) {
    register int i;
    register int print_row;
    Rect theRect;
	Rect rect;
    GrafPtr oldPort;
	Point p = { 1, 1 };

    GetPort(&oldPort);
    SetPortWindowPort(win);

	TextFont(4);
	TextSize(9);
	TextFace(0);

	GetWindowPortBounds(win, &rect);
	theRect.bottom = rect.bottom;
	theRect.top = rect.top;
	theRect.left = rect.left;
	theRect.right = rect.right - SCROLL_BAR_SIZE;

	local_web_num_rows = NumOfRowsInAWindow(win, &char_height, 0);
	local_web_num_rows = local_web_num_rows - (TOPROWOFFSET / char_height);
	EraseRect(&theRect);
	print_row = char_height + TOPROWOFFSET;
	for (i=topWebItemIndex; i < maxWebItems && i < local_web_num_rows+topWebItemIndex; i++) {
		MoveTo(LEFTMARGIN, print_row);
		DrawJustified(WebItems[i], strlen(WebItems[i]), 0L, onlyStyleRun, p, p);
		print_row += char_height;
	}

	DrawWebHighlight(win);
	SetWebScrollControl(win);
	DrawGrowIcon(win);
	DrawControls(win);
    SetPort(oldPort);
}

void  HandleWebVScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    long  MaxTick;

	do {
		HiliteControl(theControl, code);
		switch (code) {
			case kControlUpButtonPart:
				if (topWebItemIndex > 0) {
					topWebItemIndex--;
					UpdateWeb(win);
				}
				break;

			case kControlDownButtonPart:
				if (topWebItemIndex < maxWebItems-1) {
					topWebItemIndex++;
					UpdateWeb(win);
				}
				break;

			case kControlPageUpPart:
				topWebItemIndex -= (local_web_num_rows / 2);
				if (topWebItemIndex < 0)
					topWebItemIndex = 0;
				UpdateWeb(win);
				break;

			case kControlPageDownPart:
				topWebItemIndex += (local_web_num_rows / 2);
				if (topWebItemIndex >= maxWebItems) 
					topWebItemIndex = maxWebItems - 1;
				UpdateWeb(win);
				break;

			case kControlIndicatorPart:
				code = TrackControl(theControl, myPt, NULL);
				topWebItemIndex = GetControlValue(theControl);
				if (topWebItemIndex >= maxWebItems)
					topWebItemIndex = maxWebItems - 1;
				UpdateWeb(win);
				break;	
		}

		SetWebScrollControl(win);
//		if (code != kControlUpButtonPart && code != kControlDownButtonPart) {
			MaxTick = TickCount() + 10;
			do {  
				if (TickCount() > MaxTick) break;		 
			} while ( Button() == TRUE) ;
//		}
	} while (StillDown() == TRUE) ;
}

static void Do_Web_ScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    short RefCon;

	RefCon = GetControlReference(theControl);
	switch  (RefCon) {
		case I_VScroll_bar:
			HandleWebVScrollBar(win,code,theControl,myPt);
			break;
		default:
			break;
	}
}

static char *SelectWebPosition(WindowPtr win, EventRecord *event) {
    register short i;
    register int print_row;
	register int h = event->where.h;
	register int v = event->where.v;
    Point LocalPtr;
	Rect rect;

	GetWindowPortBounds(win, &rect);
	if (v < TOPROWOFFSET || h > rect.right - rect.left - SCROLL_BAR_SIZE)
		return(NULL);
	LocalPtr.h = h;
	LocalPtr.v = v;
	if (((event->when - lastWhen) < GetDblTime()) &&
	    (abs(LocalPtr.h - lastWhere.h) < 5) &&
	    (abs(LocalPtr.v - lastWhere.v) < 5)) {
	    if (++DoubleClickCount > 2)
	    	DoubleClickCount = 2;
	    while (StillDown() == TRUE) ;

		print_row = TOPROWOFFSET;
		for (i=topWebItemIndex; i < maxWebItems; i++) {
			print_row += char_height;
			if (v < print_row)
				break;
		}
		if (i >= local_web_num_rows+topWebItemIndex || i >= maxWebItems)
			return(NULL);

		DrawWebHighlight(win);
		webItemsIndex = i;
		DrawWebHighlight(win);
		return(WebItems[i]);
	}
	DoubleClickCount = 1;
	lastWhen  = event->when;
	lastWhere = LocalPtr;

	print_row = TOPROWOFFSET;
	for (i=topWebItemIndex; i < maxWebItems; i++) {
		print_row += char_height;
		if (v < print_row)
			break;
	}
	if (i >= local_web_num_rows+topWebItemIndex || i >= maxWebItems)
		return(NULL);

	DrawWebHighlight(win);
	webItemsIndex = i;
	DrawWebHighlight(win);
	return(NULL);
}

static char *mkStr(const char *st, long beg) {
	char *newSt;

	newSt = (char *)malloc(strlen(st+beg)+1);
	if (newSt != NULL)
		strcpy(newSt, st+beg);
	return(newSt);
}

static long extractNames(char *st) {
	long i, beg;

	for (i=0; st[i] != EOS; i++) {
		if (uS.mStrnicmp(st+i, "<A HREF=\"", 9) == 0) {
			beg = i;
			for (; st[i] != EOS && st[i] != '"'; i++) ;
			if (st[i] == '"')
				i++;
			if (st[i] == EOS) {
				strcpy(st, st+beg-1);
				beg = strlen(st);
				return(beg);
			}
			beg = i;
			for (; st[i] != '"' && st[i] != EOS; i++) ;
			if (st[i] == '"') {
				st[i] = EOS;
				if (maxWebItems < NUMWEBITEMS && st[beg] != '?') {
					if (st[beg] == '/')
						WebItems[maxWebItems++] = mkStr(PREVDIRLABLE, 0L);
					else if (st[i-1] == '/' || strchr(st+beg, '.') == NULL || uS.fpatmat(st+beg, "*.cha") || uS.fpatmat(st+beg, "*.cdc"))
						WebItems[maxWebItems++] = mkStr(st, beg);
				}
				st[i] = '"';
			} else if (st[i] == EOS) {
				strcpy(st, st+beg-1);
				beg = strlen(st);
				return(beg);
			}
		}
	}
	return(0L);
}

static void handleWebDirFile(FNType *gFile) {
	long  max, count, offset;
	FILE *fp;

	if ((fp=fopen(gFile, "r")) != NULL) {
		offset = 0L;
		freeWebItem();
		while (1) {
			max = UTTLINELEN - offset;
			if ((count=fread(templineC+offset, 1, max, fp)) != max) {
				if (feof(fp)) {
					templineC[offset+count] = EOS;
					extractNames(templineC);
				}
				break;
			}
			templineC[offset+count] = EOS;
			offset = extractNames(templineC);
		}
		fclose(fp);
		unlink(gFile);
	}
}

static webMFile *FindMediaFileName(unCH *line, int *oIndex, char *mvFName, FNType *oFile, webMFile *webMFiles, FNType *tMediaFName) {
	int		i, orgIndex, index;
	char	*s, isPict;
	FNType	buf[FILENAME_MAX];
	wchar_t	wbuf[FILENAME_MAX];
	FNType	gFile[FNSize];
	Boolean isDirectory;
	webMFile *p;

	index = *oIndex;
	orgIndex = index;
	isPict = FALSE;
	for (i=0, index=orgIndex+1; line[index]; i++, index++) {
		if (line[index] != PICTTIER[i])
			break;
	}
	if (PICTTIER[i] != EOS) {
		if (tMediaFName[0] == EOS) {
			for (i=0, index=orgIndex+1; line[index]; i++, index++) {
				if (line[index] != SOUNDTIER[i])
					break;
			}
			if (SOUNDTIER[i] != EOS) {
				for (i=0, index=orgIndex+1; line[index]; i++, index++) {
					if (line[index] != REMOVEMOVIETAG[i])
						break;
				}
				if (REMOVEMOVIETAG[i] != EOS) {
					*oIndex = orgIndex;
					return(webMFiles);
				}
			}
		}
	} else
		isPict = TRUE;
	
	if (isPict || tMediaFName[0] == EOS) {
		for (; line[index] && (isSpace(line[index]) || line[index] == '_'); index++) ;
		if (line[index] != '"') {
			*oIndex = orgIndex;
			return(webMFiles);
		}
		index++;
		if (line[index] == EOS) {
			*oIndex = orgIndex;
			return(webMFiles);
		}
		for (i=0; line[index] && line[index] != '"'; index++)
			wbuf[i++] = line[index];
		wbuf[i] = EOS;
		u_strcpy(buf, wbuf, FILENAME_MAX);
	} else if (tMediaFName[0] != EOS)
		strcpy(buf, tMediaFName);

	if (uS.mStricmp(buf, mvFName) != 0) {
		strcpy(mvFName, fulURLPath);
		addFilename2Path(mvFName, "media/");
		s = strrchr(buf, '.');
		addFilename2Path(mvFName, buf);
		if (s == NULL) {
			if (isPict)
				strcat(mvFName, ".jpg");
			else
				strcat(mvFName, ".mov");
		}
		if (getURLData(mvFName, gFile, &isDirectory, oFile)) {
			if (webMFiles == NULL) {
				webMFiles = NEW(webMFile);
				p = webMFiles;
			} else {
				for (p=webMFiles; p->nextFile != NULL; p=p->nextFile) ;
				p->nextFile = NEW(webMFile);
				p = p->nextFile;
			}
			if (p != NULL) {
				if (s == NULL)
					p->name = (FNType *)malloc((strlen(buf)+5)*sizeof(FNType));
				else
					p->name = (FNType *)malloc((strlen(buf)+1)*sizeof(FNType));
				if (p->name != NULL) {
					uS.str2FNType(p->name, 0L, buf);
					if (s == NULL) {
						if (isPict)
							uS.str2FNType(p->name, strlen(p->name), ".jpg");
						else
							uS.str2FNType(p->name, strlen(p->name), ".mov");
					}
				}
				p->nextFile = NULL;
			}
		}
		strcpy(mvFName, buf);
	}
	*oIndex = index;
	return(webMFiles);
}

static webMFile *getMediaFiles(FNType *oFile, myFInfo *df, webMFile *webMFiles) {
	int  i;
	FNType mvFName[FNSize];
	FNType tMediaFName[FNSize];
	ROWS *curRow;
	
	tMediaFName[0] = EOS;
	mvFName[0] = EOS;
	curRow = df->head_text->next_row;
	while (!AtBotEnd(curRow, df->tail_text, FALSE)) {
		if (uS.partcmp(curRow->line, MEDIAHEADER, FALSE, FALSE)) {
			u_strcpy(tMediaFName, curRow->line+strlen(MEDIAHEADER), FILENAME_MAX-1);
			tMediaFName[FILENAME_MAX-1] = EOS;
			getMediaName(NULL, tMediaFName, FNSize);
		}
		for (i=0; curRow->line[i]; i++) {
			if (curRow->line[i] == HIDEN_C) {
				webMFiles = FindMediaFileName(curRow->line, &i, mvFName, oFile, webMFiles, tMediaFName);
			}
		}
		curRow = ToNextRow(curRow, FALSE);
	}
	return(webMFiles);
}


static void takeCareOfSuccess(Boolean isDirectory, FNType *gFile, int t, WindowPtr wind) {
	WindowProcRec	*windProc;
	webMFile		*webMFiles;
	FNType			oFile[FNSize];

	if (isDirectory) {
		handleWebDirFile(gFile);
		if (wind != NULL)
			UpdateWeb(wind);
	} else {
		fulURLPath[t] = EOS;
		isAjustCursor = TRUE;
		OpenAnyFile(gFile, 1962, FALSE);
		windProc = WindowProcs(FindAWindowNamed(gFile));
		if (windProc != NULL && windProc->FileInfo != NULL) {
			webMFiles = NULL;
			extractPath(oFile, gFile);
			webMFiles = getMediaFiles(oFile, windProc->FileInfo, webMFiles);
			windProc->FileInfo->isTempFile = 1;
			windProc->FileInfo->webMFiles = webMFiles;
		}
	}
}

static short WebEvent(WindowPtr win, EventRecord *event) {
	int				code, t;
	char			*line, alreadyHighlighted, tchar;
	short			key;
	FNType			gFile[FNSize];
	Boolean			isDirectory;
	ControlRef		theControl;
	WindowProcRec	*windProc;

	windProc = WindowProcs(win);
	if (windProc == NULL)
		return(-1);
	
	alreadyHighlighted = FALSE;
	if (event->what == keyDown || event->what == autoKey) {
		key = (event->message & keyCodeMask) / 256;
		if (key == up_key) {
			DrawWebHighlight(win);
			webItemsIndex--;
			if (webItemsIndex < 0)
				webItemsIndex = maxWebItems - 1;
			if (webItemsIndex < topWebItemIndex) {
				topWebItemIndex--;
				UpdateWeb(win);
				alreadyHighlighted = TRUE;
			}
			if (webItemsIndex >= local_web_num_rows+topWebItemIndex) {
				topWebItemIndex = webItemsIndex - local_web_num_rows + 1;
				UpdateWeb(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawWebHighlight(win);
		} else if (key == down_key) {
			DrawWebHighlight(win);
			webItemsIndex++;
			if (webItemsIndex >= maxWebItems)
				webItemsIndex = 0;
			if (webItemsIndex < topWebItemIndex) {
				topWebItemIndex = 0;
				UpdateWeb(win);
				alreadyHighlighted = TRUE;
			}
			if (webItemsIndex >= local_web_num_rows+topWebItemIndex) {
				topWebItemIndex++;
				UpdateWeb(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawWebHighlight(win);
		} else {
			key = event->message & charCodeMask;
			if (key == RETURN_CHAR || key == ENTER_CHAR) {
				if (uS.mStricmp(WebItems[webItemsIndex], PREVDIRLABLE)) {
					t = strlen(fulURLPath);
					tchar = 0;
					strcat(fulURLPath, WebItems[webItemsIndex]);
				} else if (uS.mStricmp(fulURLPath, ROOTWEBDIR)) {
					tchar = 0;
					t = strlen(fulURLPath) - 1;
					for (t--; t > 0 && fulURLPath[t] != '/'; t--) ;
					if (t > 0) {
						tchar = fulURLPath[t+1];
						fulURLPath[t+1] = EOS;
					} else
						t = strlen(fulURLPath);
				}

				if (getURLData(fulURLPath, gFile, &isDirectory, NULL)) {
					takeCareOfSuccess(isDirectory, gFile, t, win);
				} else if (tchar != 0) {
					fulURLPath[t+1] = tchar;
					t = strlen(fulURLPath);
					if (getURLData(fulURLPath, gFile, &isDirectory, NULL)) {
						takeCareOfSuccess(isDirectory, gFile, t, win);
					} else
						fulURLPath[t] = EOS;
				} else
					fulURLPath[t] = EOS;
			}
		}
	} else if (event->what == mouseDown) {
		GlobalToLocal(&event->where);
		code = FindControl(event->where, win, &theControl);
		if (code == kControlButtonPart) {
			ControlID outID;
			if (theControl != NULL && GetControlID(theControl, &outID) == noErr) {
				if (outID.id == 1) {
					if (URL_Address[0] != EOS)
						ROOTWEBDIR = URL_Address;
					else
						ROOTWEBDIR = CHILDES_URL;
				} else if (outID.id == 2)
					ROOTWEBDIR = TALKBANK_URL;
				strcpy(fulURLPath, ROOTWEBDIR);
				if (getURLData(fulURLPath, gFile, &isDirectory, NULL)) {
					takeCareOfSuccess(isDirectory, gFile, strlen(fulURLPath), win);
				} else
					fulURLPath[0] = EOS;
			}
		} else if (code != 0) {
			if (code == kControlUpButtonPart || code == kControlDownButtonPart || 
				code == kControlIndicatorPart || code == kControlPageDownPart || 
				code == kControlPageUpPart) {
				Do_Web_ScrollBar(win,code,theControl,event->where);
			}
		} else {
			line = SelectWebPosition(win, event);
			if (line != NULL) {
				if (uS.mStricmp(line, PREVDIRLABLE)) {
					t = strlen(fulURLPath);
					tchar = 0;
					strcat(fulURLPath, line);
				} else if (uS.mStricmp(fulURLPath, ROOTWEBDIR)) {
					tchar = 0;
					t = strlen(fulURLPath) - 1;
					for (t--; t > 0 && fulURLPath[t] != '/'; t--) ;
					if (t > 0) {
						tchar = fulURLPath[t+1];
						fulURLPath[t+1] = EOS;
					} else
						t = strlen(fulURLPath);
				}
				if (getURLData(fulURLPath, gFile, &isDirectory, NULL)) {
					takeCareOfSuccess(isDirectory, gFile, t, win);
				} else if (tchar != 0) {
					fulURLPath[t+1] = tchar;
					t = strlen(fulURLPath);
					if (getURLData(fulURLPath, gFile, &isDirectory, NULL)) {
						takeCareOfSuccess(isDirectory, gFile, t, win);
					} else
						fulURLPath[t] = EOS;
				} else
					fulURLPath[t] = EOS;
			}
		}
		LocalToGlobal(&event->where);
	}
	return(-1);
}

void OpenWebWindow(void) {
	FNType	gFile[FNSize];
	Boolean	isDirectory;
	WindowPtr WebWinCreated;

	WebWinCreated = FindAWindowNamed(WEB_Dirs_str);
	if (OpenWindow(2009,WEB_Dirs_str,0L,false,1,WebEvent,UpdateWeb,CleanupWeb)) {
		do_warning("Can't open Web Directories window", 0);
		return;
	}
	if (WebWinCreated == NULL) {
		c2pstrcpy(pascalUsername, "");
		c2pstrcpy(pascalPassword, "");
		if (URL_Address[0] != EOS)
			ROOTWEBDIR = URL_Address;
		else
			ROOTWEBDIR = CHILDES_URL;
		strcpy(fulURLPath, ROOTWEBDIR);
		if (getURLData(fulURLPath, gFile, &isDirectory, NULL)) {
			if (isDirectory) {
				handleWebDirFile(gFile);
				if (webItemsIndex == -1)
					webItemsIndex = 0;
			} else {
				fulURLPath[0] = EOS;
			}
		} else
			fulURLPath[0] = EOS;
		WebWinCreated = FindAWindowNamed(WEB_Dirs_str);
		if (WebWinCreated)
			UpdateWeb(WebWinCreated);
	}
}
