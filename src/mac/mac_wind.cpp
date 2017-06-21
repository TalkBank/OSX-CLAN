#include "ced.h"
#include "my_ctype.h"
#include "MMedia.h"

#include "mac_text_bullet.h"

MACWINDOWS *RootWind;

#ifdef _UNICODE
#define DEFSYSFSIZE 9

static char fontName[256];
static short fontID = 0;
static short fontSize = 0;

char *defFontName(void) {
	if (fontID == 0 || strcmp(fontName, defUniFontName) || fontSize != defUniFontSize) {
		fontSize = defUniFontSize;
		strcpy(fontName, defUniFontName);
		if (!GetFontNumber(fontName, &fontID)) {
			fontSize = DEFSYSFSIZE;
			fontID = 4;
			strcpy(fontName, "Monaco");
		}
	}
	return(fontName);
}

short defFontID(void) {
	if (fontID == 0 || strcmp(fontName, defUniFontName) || fontSize != defUniFontSize) {
		fontSize = defUniFontSize;
		strcpy(fontName, defUniFontName);
		if (!GetFontNumber(fontName, &fontID)) {
			fontSize = DEFSYSFSIZE;
			fontID = 4;
			strcpy(fontName, "Monaco");
		}
	}
	return(fontID);
}

short defFontSize(void) {
	if (fontID == 0 || strcmp(fontName, defUniFontName) || fontSize != defUniFontSize) {
		fontSize = defUniFontSize;
		strcpy(fontName, defUniFontName);
		if (!GetFontNumber(fontName, &fontID)) {
			fontSize = DEFSYSFSIZE;
			fontID = 4;
			strcpy(fontName, "Monaco");
		}
	}
	return(fontSize);
}
#endif

short GetFontHeight(FONTINFO *fontInfo, NewFontInfo *finfo, WindowPtr wind) {
	short	  scriptId;
	FontInfo  fi;
	GrafPtr   oldPort;

	GetPort(&oldPort);
	SetPortWindowPort(wind);
	if (fontInfo != NULL) {
		TextFont(fontInfo->FName);
		TextSize(fontInfo->FSize);
	} else if (finfo != NULL) {
		TextFont(finfo->fontId);
		TextSize((short)finfo->fontSize);
	}
	GetFontInfo(&fi);
	scriptId = FontScript();
	SetPort(oldPort);
#ifndef _UNICODE
	if (scriptId == smJapanese)
		return(fi.ascent+fi.descent+fi.leading+FLINESPACE+1);
	else
#endif
		return(fi.ascent+fi.descent+fi.leading+FLINESPACE);
}

void GetWindTopLeft(WindowPtr in_wind, short *left, short *top) {
	WindowPtr	wind;
	GrafPtr		port;
	Point		pt;
	Rect		box;
		
	if (!in_wind) {
		GetPort(&port);
		wind = GetWindowFromPort(port);
	} else	
		wind = in_wind;	
		
	if (IsWindowVisible(wind)) {	
		/* Use window's content region (faster than method below): */
		
#if (TARGET_API_MAC_CARBON == 1)
		RgnHandle ioWinRgn = NewRgn();
		GetWindowRegion(wind, kWindowContentRgn, ioWinRgn);
		GetRegionBounds(ioWinRgn, &box);
		DisposeRgn(ioWinRgn);
#else
		box = (**((WindowPeek)wind)->contRgn).rgnBBox;	
#endif
		*left = box.left;	
		*top = box.top;	
		return;	
	}	
		
	/* Set port and use LocalToGlobal: */	
	if (in_wind) {	
		GetPort(&port);	
		SetPortWindowPort(wind);	
	}	
	
	pt.h = pt.v = 0;	
	LocalToGlobal(&pt);	
		
	*left = pt.h;	
	*top = pt.v;	
		
	if (in_wind)	
		SetPort(port);	
}	

void InitMyWindows(void) {
	RootWind = NULL;
}
/*
char isClosedAllWindows(void) {
	return(RootWind == NULL);
}
*/
WindowPtr FindAWindowNamed(FNType *name) {
	MACWINDOWS *twind;
	
	for (twind=RootWind; twind != NULL; twind=twind->nextWind) {
		if (!strcmp(twind->windRec->wname, name))
			return(twind->wind);
	}
	return(NULL);
}

WindowPtr FindAWindowProc(WindowProcRec *windProc) {
	MACWINDOWS *twind;
	
	for (twind=RootWind; twind != NULL; twind=twind->nextWind) {
		if (twind->windRec == windProc)
			return(twind->wind);
	}
	return(NULL);
}

WindowPtr FindAWindowID(short id) {
	MACWINDOWS *twind;
	
	for (twind=RootWind; twind != NULL; twind=twind->nextWind) {
		if (twind->windRec->id == id)
			return(twind->wind);
	}
	return(NULL);
}

extern WindowProcRec *FindAProcRecID(short id);

WindowProcRec *FindAProcRecID(short id) {
	MACWINDOWS *twind;
	
	for (twind=RootWind; twind != NULL; twind=twind->nextWind) {
		if (twind->windRec->id == id)
			return(twind->windRec);
	}
	return(NULL);
}

WindowProcRec *WindowProcs(WindowPtr wind) {
	MACWINDOWS *twind;
	
	for (twind=RootWind; twind != NULL; twind=twind->nextWind) {
		if (twind->wind == wind)
			return(twind->windRec);
	}
	return(NULL);
}

myFInfo *WindowFromGlobal_df(myFInfo *df) {
	MACWINDOWS *twind;

	if (df == NULL)
		return(NULL);

	for (twind=RootWind; twind != NULL; twind=twind->nextWind) {
		if (twind->windRec->FileInfo == df) 
			return(df);
	}
	return(NULL);
}

static MACWINDOWS *RegisterWindow(WindowPtr wind, WindowProcRec *rec) {
	MACWINDOWS *w;

	if ((w=NEW(MACWINDOWS)) == NULL)
		ProgExit("Out of memory");
	w->nextWind = RootWind;
	w->wind = wind;
	w->windRec = rec;
	RootWind = w;
	return(w);
}

#if (TARGET_API_MAC_CARBON == 1)
static pascal OSStatus MouseWheelHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* userData ) {
	Point					LocalPtr;
	OSStatus				result = eventNotHandledErr;
	EventMouseWheelAxis		axis;
	SInt32					delta;
	MACWINDOWS				*mWin = (MACWINDOWS *)userData;

	if (mWin == NULL)
		return(result);

	if (mWin->wind != FrontWindow())
		return(result);
		
	GetMouse(&LocalPtr);
	GetEventParameter( inEvent, kEventParamMouseWheelAxis, typeMouseWheelAxis,
		NULL, sizeof( EventMouseWheelAxis ), NULL, &axis );	
	GetEventParameter( inEvent, kEventParamMouseWheelDelta, typeLongInteger,
		NULL, sizeof( SInt32 ), NULL, &delta );	

	if ( axis == kEventMouseWheelAxisY ) {
		short code = ((delta > 0) ? kControlUpButtonPart : kControlDownButtonPart);
		Point   myPt = {0,0};

		if (global_df != NULL) {
			if (global_df->VScrollHnd != NULL) {
				UInt16 val;
				val = GetControlHilite(global_df->VScrollHnd);
				if (val == 255) {
					result = noErr;
					return result;
				}
			}
			if (global_df->SoundWin && LocalPtr.v > global_df->SoundWin->LT_row) {
				result = noErr;
				return result;
			}
		}
#ifndef TEXT_BULLETS_PICTS // text_bullet
		if (mWin->windRec->id == 1962 || mWin->windRec->id == 1964 || mWin->windRec->id == 1966)
#else
		if (mWin->windRec->id == 1962 || mWin->windRec->id == 1964)
#endif //TEXT_BULLETS_PICTS
		{
			DrawCursor(0);
			HandleVScrollBar(code, mWin->windRec->VScrollHnd, myPt);
			DrawCursor(1);
		} else if (mWin->windRec->id == 1965)
			HandleThumbVScrollBar(mWin->wind, code, mWin->windRec->VScrollHnd,myPt);
#ifdef TEXT_BULLETS_PICTS // text_bullet
		else if (mWin->windRec->id == 1966)
			HandleTextVScrollBar(mWin->wind, code, mWin->windRec->VScrollHnd,myPt);
#endif //TEXT_BULLETS_PICTS
		else if (mWin->windRec->id == 2000)
			HandleRecallVScrollBar(mWin->wind, code, mWin->windRec->VScrollHnd,myPt);
		else if (mWin->windRec->id == 2001)
			HandleProgsVScrollBar(mWin->wind, code, mWin->windRec->VScrollHnd,myPt);
		else if (mWin->windRec->id == 2002)
			HandleHlpVScrollBar(mWin->wind, code, mWin->windRec->VScrollHnd,myPt);
		else if (mWin->windRec->id == 2007)
			HandleCharsVScrollBar(mWin->wind, code, mWin->windRec->VScrollHnd,myPt);
		else if (mWin->windRec->id == 2009)
			HandleWebVScrollBar(mWin->wind, code, mWin->windRec->VScrollHnd,myPt);
		else if (mWin->windRec->id == 2010)
			HandleFoldersVScrollBar(mWin->wind, code, mWin->windRec->VScrollHnd,myPt);
		result = noErr;
	} else if ( axis == kEventMouseWheelAxisX ) {
		short code = ((delta > 0) ? kControlUpButtonPart : kControlDownButtonPart);
		Point   myPt = {0,0};

		if (global_df != NULL) {
			if (global_df->HScrollHnd != NULL) {
				UInt16 val;
				val = GetControlHilite(global_df->HScrollHnd);
				if (val == 255) {
					result = noErr;
					return result;
				}
			}
			if (global_df->SoundWin && LocalPtr.v < global_df->SoundWin->LT_row) {
				result = noErr;
				return result;
			}
		}
#ifndef TEXT_BULLETS_PICTS // text_bullet
		if (mWin->windRec->id == 1962 || mWin->windRec->id == 1964 || mWin->windRec->id == 1966)
#else
		if (mWin->windRec->id == 1962 || mWin->windRec->id == 1964)
#endif //TEXT_BULLETS_PICTS
		{
			DrawCursor(0);
			HandleHScrollBar(code, mWin->windRec->VScrollHnd, myPt);
			DrawCursor(1);
		}
		result = noErr;
	}
	return result;
}

static pascal OSStatus TextInputHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* userData ) {
	EventRef myInEvent;
	EventRecord	 myEvent;
	unsigned long cLen, nLen;
	UnicodeInput *UniInputBuf = (UnicodeInput *)userData;

	GetEventParameter( inEvent, kEventParamTextInputSendKeyboardEvent, typeEventRef, NULL, sizeof( EventRef ), NULL, &myInEvent );
	ConvertEventRefToEventRecord(myInEvent, &myEvent);
	if (myEvent.modifiers & cmdKey)
		return eventNotHandledErr;
	UniInputBuf->inEvent = myEvent;

	cLen = UniInputBuf->len;
	GetEventParameter(inEvent, kEventParamTextInputSendText, typeUnicodeText, NULL, 
						sizeof( short )*(kMaxUnicodeInputStringLength-cLen), &nLen, UniInputBuf->unicodeInputString+cLen);
	UniInputBuf->len += nLen / 2;
	UniInputBuf->numDel = 0;
//	if (UniInputBuf->len <= 0)
		return eventNotHandledErr;
//	else
//		return noErr;
}

static void makeWinRecTSMDoc(WindowProcRec *rec, WindowPtr wind) {
	OSErr err;
	InterfaceTypeList supportedInterfaceTypes;

	supportedInterfaceTypes[0] = kUnicodeDocument; //kUnicodeTextService; //kUnicodeDocument; // kTextService;
	err = NewTSMDocument (1, supportedInterfaceTypes, &rec->idocID, (SInt32)rec);
	if (rec->idocID != nil) {
//		err = TSMSetInlineInputRegion(rec->idocID, wind, NULL /*RgnHandle inRegion*/);
		err = UseInputWindow(rec->idocID, TRUE);
	}
}

static void makeWinRecUnicode(WindowProcRec *rec, WindowPtr wind) {
	OSErr err;

	makeWinRecTSMDoc(rec, wind);

	EventTypeSpec textInputEvent = { kEventClassTextInput, kEventTextInputUnicodeForKeyEvent };
	rec->textHandler = NewEventHandlerUPP((EventHandlerProcPtr)TextInputHandler);
	err = InstallEventHandler(GetWindowEventTarget(wind),rec->textHandler,1,&textInputEvent,&UniInputBuf,&rec->textEventHandler);

}
#endif // (TARGET_API_MAC_CARBON == 1)

WindowPtr isWinExists(short id, FNType *name, char checkForDups) {
	UInt32		 	dateValue;
	WindowPtr		wind;	
	WindowProcRec	*rec;	
	PrepareStruct	saveRec;

	if ((wind=FindAWindowNamed(name)) != NULL) {
		rec = NULL;	
		if (wind != FrontWindow() && (id != 500 || PlayingContMovie)) {	
			if ((rec=WindowProcs(wind)) != NULL) {	
				global_df = rec->FileInfo;	
				ChangeWindowsMenuItem(rec->wname, TRUE);	
			} else	
				global_df = NULL;	
			SelectWindow(wind);	
			SetTextKeyScript(FALSE);	
			ControlAllControls('\001', id);	
			PrepWindow(id);	
			ChangeSpeakerMenuItem();
			SetTextWinMenus(TRUE);	
			if (global_df != NULL)	
				SetCurrentFontParams(global_df->row_txt->Font.FName, global_df->row_txt->Font.FSize);	
			else	
				SetCurrentFontParams(dFnt.fontId, dFnt.fontSize);	
		} else
			ShowWindow(wind);

		if (checkForDups) {
			if (uS.FNTypecmp(name, NEWFILENAME, 0L) || name[0] == '/') {
				if (rec == NULL)
					rec = WindowProcs(wind);
				dateValue = 0L;
				if (rec != NULL) {
					PrepareWindA4(wind, &saveRec);
					if (rec->UpdateProc)
						rec->UpdateProc(wind);
					RestoreWindA4(&saveRec);
					getFileDate(name, &dateValue);
				}
				if (rec->fileDate != dateValue && dateValue != 0L) {
					if (QueryDialog("This file is already open. Do you want to replace it with the one on the hard disk?", 147) == 1) {
						PrepareWindA4(wind, &saveRec);
						mCloseWindow(wind);
						RestoreWindA4(&saveRec);
						wind = NULL;
					}
				}
			}
		}
	}
	return(wind);
}


static WindowPtr OpenADialog(short orgId, FNType *name, char isMakeScrollBar, WindowProcRec *rec) {	
	WindowPtr		wind;
	Rect			box;
	short 			id;
	long			width, height, top, left;

	id = orgId;
	if (id == 139)
		wind = getNibWindow(CFSTR("Warning"));
	else if (id == 142)
		wind = getNibWindow(CFSTR("Input File SF"));
	else if (id == 145)
		wind = getNibWindow(CFSTR("Progress"));
	else if (id == 155)
		wind = getNibWindow(CFSTR("Warning-Quit"));
	else if (id == 500)
		wind = getNibWindow(CFSTR("Movie"));
	else if (id == 501)
		wind = getNibWindow(CFSTR("Commands"));
	else if (id == 504)
		wind = getNibWindow(CFSTR("Picture"));
	else if (id == 505)
		wind = getNibWindow(CFSTR("Movie Help"));
	else if (id == 506)
		wind = getNibWindow(CFSTR("Walker Conroller"));
	else if (id == 1962)
		wind = getNibWindow(CFSTR("text"));
	else if (id == 1964)
		wind = getNibWindow(CFSTR("clan output"));
	else if (id == 1965)
		wind = getNibWindow(CFSTR("Movie ThumbNail"));
	else if (id == 1966)
		wind = getNibWindow(CFSTR("%Txt"));
	else if (id == 2000)
		wind = getNibWindow(CFSTR("recall"));
	else if (id == 2001)
		wind = getNibWindow(CFSTR("Clan Progs"));
	else if (id == 2002)
		wind = getNibWindow(CFSTR("Help Commands"));
	else if (id == 2007)
		wind = getNibWindow(CFSTR("Special Chars"));
	else if (id == 2009)
		wind = getNibWindow(CFSTR("Web Data"));
	else if (id == 2010)
		wind = getNibWindow(CFSTR("Clan Folders"));
	else
		wind = NULL;

	if (wind == NULL)
		return(NULL);

	if (orgId == 500) {
		MoveWindow(wind, 18, 50, FALSE);
	}
	if (GetPrefSize(orgId, name, &height, &width, &top, &left)) {	
		if (top < 40)
			top = 40;
		if (isInsideScreen(top, left, &box)) {	
			MoveWindow(wind, (short)left, (short)top, FALSE);	
		}	
		if (orgId >= 1000 && isInsideScreen((short)height, (short)width, &box))	
			SizeWindow(wind, (short)width, (short)height, 0);	
	}	

	MACWINDOWS *mWin = RegisterWindow(wind, rec);	
	GetWindowPortBounds(wind, &box);
	HIViewID viewID;
	ControlRef itemCtrl;

	viewID.signature = 'vctr';
	viewID.id = 0;
	HIViewFindByID(HIViewGetRoot(wind), viewID, &itemCtrl);
	if (itemCtrl != NULL) {
		rec->VScrollHnd = itemCtrl;
		HLock((Handle)itemCtrl);/* Lock handle while we use it */
		HideControl(itemCtrl); /* Hide it during size and move */
		SizeControl(itemCtrl, SCROLL_BAR_SIZE, box.bottom-box.top-SCROLL_BAR_SIZE);
		MoveControl(itemCtrl, box.right-box.left-SCROLL_BAR_SIZE, -1);
		ShowControl(itemCtrl);   /* Safe to show it now */ 
		HUnlock((Handle)itemCtrl);/* Let it float again */
//			SetControlAction(itemCtrl, VScrollHandler);
//			SetControlReference(itemCtrl, (SInt32)mWin);
		SetControlReference(itemCtrl, (SInt32)I_VScroll_bar);

		EventTypeSpec mouseWheelEvent = { kEventClassMouse, kEventMouseWheelMoved };
		InstallEventHandler(GetWindowEventTarget(wind),MouseWheelHandler,1,&mouseWheelEvent,mWin,&rec->mouseEventHandler);
	}

	viewID.signature = 'hctr';
	viewID.id = 0;
	HIViewFindByID(HIViewGetRoot(wind), viewID, &itemCtrl);
	if (itemCtrl != NULL) {
		rec->HScrollHnd = itemCtrl;
		HLock((Handle)itemCtrl);/* Lock handle while we use it */
		HideControl(itemCtrl); /* Hide it during size and move */
		SizeControl(itemCtrl, box.right-box.left-SCROLL_BAR_SIZE, SCROLL_BAR_SIZE);
		MoveControl(itemCtrl, -1, box.bottom-SCROLL_BAR_SIZE);
		ShowControl(itemCtrl);   /* Safe to show it now */ 
		HUnlock((Handle)itemCtrl);/* Let it float again */
		SetControlReference(itemCtrl, (SInt32)I_HScroll_bar);
	}

	if (orgId != 500)
		ShowWindow(wind);
	return wind;
}	

int OpenWindow(
				short id,
				FNType *name,
				long limit,
				char checkForDups,
				char isMakeScrollBar,
				short (*EP)(WindowPtr,EventRecord *),
				void (*UP)(WindowPtr),
				void (*CL)(WindowPtr)) {
	WindowPtr	  wind;	
	WindowProcRec *rec;	
	char		  isUTF8Header;
	CFStringRef	  theStr;
	extern short DOSdata;
	extern char DOSdataErr[];

	isUTF8Header = FALSE;
	if (isWinExists(id, name, checkForDups) != NULL)
		return(0);
	
	if ((rec=NEW(WindowProcRec)) == NULL)	
		ProgExit("Out of memory");	
	
	strcpy(rec->wname, name);	
	rec->idocID = nil;
	getFileDate(name, &rec->fileDate);
	rec->Offset = 0;	
	rec->id = id;	
	rec->CloseProc = CL;	
	rec->UpdateProc = UP;	
	rec->EventProc = EP;	
	rec->VScrollHnd = nil;
	rec->HScrollHnd = nil;
	rec->mouseEventHandler = NULL;
	rec->textHandler = NULL;
	rec->textEventHandler = NULL;
	if ((wind=OpenADialog(id, name, isMakeScrollBar, rec)) == NULL) {	
		free(rec);	
		return(1);	
	}	
	theStr = my_CFStringCreateWithBytes(name);
	if (theStr != NULL) {
		SetWindowTitleWithCFString(wind, theStr);
		CFRelease(theStr);
	}
	
	if (id != 500 || PlayingContMovie) {	
		ChangeWindowsMenuItem(name, TRUE);	
		SelectWindow(wind);	
	} else if (id == 500)
		ChangeWindowsMenuItem(name, '\002');	

	if (id == 501) {	
		makeWinRecTSMDoc(rec, wind);
	}

ReadAgain:
#ifndef TEXT_BULLETS_PICTS // text_bullet
	if (id == 1962 || id == 1964 || id == 1966)
#else
	if (id == 1962 || id == 1964)
#endif //TEXT_BULLETS_PICTS // text_bullet
	{	
		rec->FileInfo = InitFileInfo(name, limit, wind, id, &isUTF8Header);
		if (rec->FileInfo != NULL) {
			rec->FileInfo->VScrollHnd = rec->VScrollHnd;
			rec->FileInfo->HScrollHnd = rec->HScrollHnd;
		}
		if (rec->FileInfo == NULL) {
			mCloseWindow(wind);
			return(1);
		} else if (id == 1962 && isAjustCursor)
			ajustCursorPos(name);
		else
			wrefresh(rec->FileInfo->w1);
		SetTextWinMenus(TRUE);
		if (id == 1962)
			SetPBCglobal_df(false, 0L);
		if (rec->FileInfo != NULL) {
			makeWinRecUnicode(rec, wind);
		}
	} else
		rec->FileInfo = NULL;
		
	global_df = rec->FileInfo;	
	PrepWindow(id);	
	ControlAllControls('\001', id);	
	if (global_df == NULL) {	
		SetCurrentFontParams(dFnt.fontId, dFnt.fontSize);	
		SetTextKeyScript(FALSE);	
	}	
	SetScrollControl();	

	if (global_df != NULL) {	
#ifndef TEXT_BULLETS_PICTS // text_bullet
		if (isUTF8Header == FALSE && (id == 1962 || id == 1966) && !global_df->dontAskDontTell)
#else
		if (isUTF8Header == FALSE && id == 1962 && !global_df->dontAskDontTell)
#endif //TEXT_BULLETS_PICTS // text_bullet
		{
			if (QueryDialog("This file is missing '@UTF8'. Do you believe it is a 'UTF8' file?", 147) == 1) {
				isUTF8Header = TRUE;
			    FreeFileInfo(global_df);
			    global_df = rec->FileInfo = NULL;

				if (rec->textHandler)
					DisposeEventHandlerUPP(rec->textHandler);
				if (rec->textEventHandler)
					RemoveEventHandler(rec->textEventHandler);
				rec->textHandler = NULL;
				rec->textEventHandler = NULL;

				goto ReadAgain;
			}
		}

		if (DOSdata != 0) {
			if (DOSdata == -1) {
				*templineC = EOS;
			} else if (DOSdata == 1)
				sprintf(templineC, "File \"%s\" is in DOS format", global_df->fileName);	
			else if (DOSdata == 2)
				sprintf(templineC, "File \"%s\" is in Macintosh format", global_df->fileName);	
			else if (DOSdata == 3)
				*templineC = EOS; // sprintf(templineC, "File \"%s\" is in Windows 9x or 2000 format", global_df->fileName);	
			else if (DOSdata == 4)
				sprintf(templineC, "File \"%s\" is in UNICODE format", global_df->fileName);	
			else if (DOSdata == 5)
				*templineC = EOS;//	sprintf(templineC, "File \"%s\" has PC style line returns", global_df->fileName);
			else if (DOSdata == 6)
				strcpy(templineC, DOSdataErr);
			else if (DOSdata == 200)
				strcpy(templineC, DOSdataErr);
			else
				*templineC = EOS;

			if (*templineC != EOS)
				do_warning(templineC, 0);	
		}	
		DOSdata = -1;
	}	
	return(0);	
}	

void CloseAllWindows(void) {
	MACWINDOWS *t;
	
	while (RootWind) {
		t = RootWind;

		global_df = t->windRec->FileInfo;
		SetPBCglobal_df(true, 0L);
		FreeFileInfo(t->windRec->FileInfo);
		if (t->windRec->VScrollHnd != NULL)
			DisposeControl(t->windRec->VScrollHnd);
		if (t->windRec->HScrollHnd != NULL)
			DisposeControl(t->windRec->HScrollHnd);
#if (TARGET_API_MAC_CARBON == 1)
		if (t->windRec->idocID) {
//			FixTSMDocument(t->windRec->idocID);
			DeactivateTSMDocument(t->windRec->idocID);
			DeleteTSMDocument(t->windRec->idocID);
		}
		if (t->windRec->mouseEventHandler)
			RemoveEventHandler(t->windRec->mouseEventHandler);
		if (t->windRec->textHandler)
			DisposeEventHandlerUPP(t->windRec->textHandler);
		if (t->windRec->textEventHandler)
			RemoveEventHandler(t->windRec->textEventHandler);
#endif
		ChangeWindowsMenuItem(t->windRec->wname, FALSE);
		if (t->windRec->CloseProc)
			t->windRec->CloseProc(t->wind);
		DisposeWindow(t->wind);
		RootWind = RootWind->nextWind;
		free(t->windRec);
		free(t);
	}
}

void mCloseWindow(WindowPtr wind) {
	char res;
	MACWINDOWS *w, *t = RootWind;
	WindowPtr front;
	WindowProcRec *windProc;

	if (t == NULL || wind == NULL)
		return;
	
	res = 'y';
	if (t->wind == wind) {
		w = t;
		if ((global_df=w->windRec->FileInfo) != NULL && global_df->DataChanged == '\001') {
			res = QuitDialog(global_df->fileName);
			if (res == 'n') {
				strcpy(global_df->err_message, DASHES);
				return;
			}
		}
	} else {
		while (t->nextWind != NULL) {
			if (t->nextWind->wind == wind)
				break;
			t = t->nextWind;
		}
		if (t->nextWind == NULL)
			return;
		w = t->nextWind;
		if ((global_df=w->windRec->FileInfo) != NULL && global_df->DataChanged == '\001') {
			res = QuitDialog(global_df->fileName);
			if (res == 'n') {
				strcpy(global_df->err_message, DASHES);
				return;
			}
		}
	}

	if (res == 'y')
		setCursorPos(w->wind);
	global_df = w->windRec->FileInfo;
	SetPBCglobal_df(true, 0L);
	FreeFileInfo(w->windRec->FileInfo);
	if (w->windRec->VScrollHnd != NULL)
		DisposeControl(w->windRec->VScrollHnd);
	if (w->windRec->HScrollHnd != NULL)
		DisposeControl(w->windRec->HScrollHnd);
#if (TARGET_API_MAC_CARBON == 1)
	if (w->windRec->idocID) {
//		FixTSMDocument(w->windRec->idocID);
		DeactivateTSMDocument(w->windRec->idocID);
		DeleteTSMDocument(w->windRec->idocID);
	}
	if (w->windRec->mouseEventHandler)
		RemoveEventHandler(w->windRec->mouseEventHandler);
	if (w->windRec->textHandler)
		DisposeEventHandlerUPP(w->windRec->textHandler);
	if (w->windRec->textEventHandler)
		RemoveEventHandler(w->windRec->textEventHandler);
#endif
	ChangeWindowsMenuItem(w->windRec->wname, FALSE);
	if (w->windRec->CloseProc)
		w->windRec->CloseProc(w->wind);
	DisposeWindow(w->wind);
	if (RootWind->wind == wind)
		RootWind = t->nextWind;
	else
		t->nextWind = w->nextWind;
	free(w->windRec);
	free(w);
	front = FrontWindow();
	if (front == NULL) {
		global_df = NULL;
		windProc = NULL;
		ControlAllControls(FALSE, 0);
	} else {
		windProc = WindowProcs(front);
		if (windProc == NULL) {
			global_df = NULL;
			ControlAllControls(FALSE, 0);
		} else {
			global_df = windProc->FileInfo;
			ChangeWindowsMenuItem(windProc->wname, TRUE);
		}
	}
	SetPBCglobal_df(false, 0L);
	if (windProc != NULL) {
		ControlAllControls('\001', windProc->id);
		PrepWindow(windProc->id);
	}
	ChangeSpeakerMenuItem();
	SetTextWinMenus(TRUE);
	if (global_df != NULL)
		SetCurrentFontParams(global_df->row_txt->Font.FName, global_df->row_txt->Font.FSize);
	else
		SetCurrentFontParams(dFnt.fontId, dFnt.fontSize);
}

int getNumberOffset(void) {
	GrafPtr savePort;	
    FONTINFO *font;
    int width, nDigs;

	GetPort(&savePort);	
	if (!isShowLineNums)
		return(0);
	font = GetTextFont(global_df);
	if (font == NULL)
		return(0);
	SetPortWindowPort(global_df->wind);	
	TextFont(4); // font->FName);	lxs2
	TextSize(font->FSize);
	if (LineNumberDigitSize != 0)
		nDigs = LineNumberDigitSize;
	else
		nDigs = 6;
	if (nDigs > 20)
		nDigs = 20;
	if (nDigs < 0)
		nDigs = 6;
	width = TextWidthInPix(cl_T("555555555555555555555"), 0, nDigs, font, 0);
	SetPort(savePort);	
	return(width);	
}

int WindowPageWidth(void) {	
	int char_width,check_pgwid;
//	FontInfo fontInfo;	
    FONTINFO *font;
	GrafPtr cPort;	
	GrafPtr savePort;	
	Rect rect;

	GetPort(&savePort);	
	font = GetTextFont(global_df);
	if (font == NULL)
		return(300);
	SetPortWindowPort(global_df->wind);	
	cPort = GetWindowPort(global_df->wind);	
	TextFont(font->FName);	
	TextSize(font->FSize);
	char_width = TextWidthInPix(cl_T("iiiiiiiiiii"), 0, 10, font, 0);
	char_width /= 10;
#if (TARGET_API_MAC_CARBON == 1)
	GetPortBounds(cPort, &rect);
#else
	rect = cPort->portRect;	
#endif
	check_pgwid = (rect.right - rect.left - SCROLL_BAR_SIZE - LEFTMARGIN) / char_width;	
	SetPort(savePort);	
/*
	GetPort(&savePort);	
	cPort = GetWindowPort(FrontWindow());	
	SetPort(cPort);	
	GetFontInfo(&fontInfo);	
	char_width = fontInfo.widMax;	
#if (TARGET_API_MAC_CARBON == 1)
	GetPortBounds(cPort, &rect);
#else
	rect = cPort->portRect;	
#endif
	check_pgwid = (rect.right - rect.left - 	
						SCROLL_BAR_SIZE - LEFTMARGIN) / char_width;	
	SetPort(savePort);	
*/
	return(check_pgwid);	
}	
	
int WindowPageLength(int *size) {	
	int char_height, check_pglen, wOffset;
	FontInfo fontInfo;	
	GrafPtr cPort;	
	GrafPtr savePort;	
	Rect rect;
	
	GetPort(&savePort);	
	wOffset = SpecialWindowOffset(global_df->winID);
	cPort = GetWindowPort(global_df->wind);
	SetPort(cPort);	
	TextFont(DEFAULT_ID);	
	TextSize(DEFAULT_SIZE);	
	GetFontInfo(&fontInfo);	
	char_height = fontInfo.ascent + fontInfo.descent + fontInfo.leading + FLINESPACE;	

#if (TARGET_API_MAC_CARBON == 1)
	GetPortBounds(cPort, &rect);
#else
	rect = cPort->portRect;	
#endif
	check_pglen = (rect.bottom - rect.top - wOffset - 17) / char_height;	
	check_pglen = (check_pglen * char_height) - (char_height * (*size));	
	if (check_pglen < global_df->MinFontSize) {	
		check_pglen = (rect.bottom - rect.top-wOffset-17) / char_height;	
		check_pglen = (check_pglen * char_height) - (char_height * 2);	
		*size = 2;	
	}	
	global_df->TextWinSize = check_pglen;	
	check_pglen = check_pglen / global_df->MinFontSize;	
	SetPort(savePort);
	wOffset = (check_pglen+(*size)) / 3;
	check_pglen = check_pglen + (*size) + wOffset;
	return(check_pglen);	
}	
	
int NumOfRowsOfDefaultWindow(void) {	
	int char_height, check_pglen, wOffset;	
	FontInfo fontInfo;	
	GrafPtr cPort;	
	GrafPtr savePort;	
	Rect rect;

	GetPort(&savePort);	
	cPort = GetWindowPort(global_df->wind);
	wOffset = SpecialWindowOffset(global_df->winID);
	SetPort(cPort);	
	TextFont(DEFAULT_ID);	
	TextSize(DEFAULT_SIZE);	
	GetFontInfo(&fontInfo);	
	char_height = fontInfo.ascent + fontInfo.descent + fontInfo.leading + FLINESPACE;	
#if (TARGET_API_MAC_CARBON == 1)
	GetPortBounds(cPort, &rect);
#else
	rect = global_df->wind->portRect;	
#endif
	check_pglen = (rect.bottom - rect.top - wOffset - 17) / char_height;	
	SetPort(savePort);	
	return(check_pglen);	
}	
	
int NumOfRowsInAWindow(WindowPtr wp, int *char_height, int offset) {	
	int check_pglen;	
	FontInfo fontInfo;	

	GetFontInfo(&fontInfo);	
	*char_height = fontInfo.ascent + fontInfo.descent + fontInfo.leading + FLINESPACE;	
#if (TARGET_API_MAC_CARBON == 1)
	Rect rect;
	GetPortBounds(GetWindowPort(wp), &rect);
	check_pglen = (rect.bottom - rect.top - (offset + 18)) / *char_height;	
#else
	check_pglen = (wp->portRect.bottom - wp->portRect.top - (offset + 18)) / *char_height;	
#endif
	return(check_pglen);					             	
}

void RefreshAllTextWindows(char isInitWindow) {
	MACWINDOWS *twind;
	myFInfo *tGlobal_df;

	for (twind=RootWind; twind != NULL; twind=twind->nextWind) {
		if (twind->windRec != NULL) {
			if (twind->windRec->FileInfo != NULL) {
				tGlobal_df = global_df;
				global_df = twind->windRec->FileInfo;
				if (isInitWindow) {
					if (global_df->SoundWin) {
						if (doMixedSTWave)
							global_df->total_num_rows = 5;
						else
							global_df->total_num_rows = global_df->SnTr.SNDchan * 5;
						global_df->total_num_rows += 1;
						global_df->CodeWinStart = 0;
					} else {
						global_df->total_num_rows = 1;
						global_df->CodeWinStart = 0;
					}
					init_windows(true, 1, true);
				}
				DisplayTextWindow(NULL, 1);
				global_df = tGlobal_df;
			}
		}
	}
}
