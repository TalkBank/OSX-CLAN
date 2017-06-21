#include "ced.h"
/*
#undef chdir
#undef access
#undef unlink
#undef rename
#undef getcwd
*/
#include <Files.h>
#include "cu.h"
#include "MMedia.h"
#include "mac_dial.h"

extern Handle pasteStr;
extern SInt16 currentKeyScript;
extern short mScript;
extern FNType warning_str[];
extern FNType progress_str[];

static unCH *lPasteStr;
// 04/01/04 char isSpellCheckFound = false;
long pasteIndex;
static char text[256];
//static char tText[80];
static wchar_t wTempStr[UTTLINELEN+2];
FNType defaultPath[FNSize] = {0};

static int	saveIsPlayS;
static int	saveF_key;
static short ProgressFilled = 0;
static myFInfo *saveGlobal_df;

static unsigned long actualUTFCnt = 0L, actualUTFLen = 0L;
static wchar_t UTFstr[kMaxUnicodeInputStringLength];

long UnicodeToUTF8(wchar_t *UniStr, unsigned long actualStringLength, unsigned char *UTF8str, unsigned long *actualUT8Len, unsigned long MaxLen) {
	OSStatus err;
	TECObjectRef ec;
	TextEncoding utf8Encoding;
	unsigned long ail;
	unsigned long aol;

	actualStringLength *= 2;
	utf8Encoding = CreateTextEncoding( kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF8Format );
	if ((err=TECCreateConverter(&ec, kTextEncodingUnicodeDefault, utf8Encoding)) != noErr) {
		if (actualUT8Len != NULL)
			*actualUT8Len = 0L;
		UTF8str[0] = EOS;
		return(err);
	}

	if ((err=TECConvertText(ec, (ConstTextPtr)UniStr, actualStringLength, &ail, (TextPtr)UTF8str, MaxLen-2, &aol)) != noErr) {
		if (actualUT8Len != NULL)
			*actualUT8Len = 0L;
		UTF8str[aol] = EOS;
		return(err);
	}
	err = TECDisposeConverter(ec);
	UTF8str[aol] = EOS;
	if (ail < actualStringLength) {
		if (actualUT8Len != NULL)
			*actualUT8Len = 0L;
		return(1L);
	}
	if (actualUT8Len != NULL)
		*actualUT8Len = aol;
	return(0L);
}

long UTF8ToUnicode(unsigned char *UTF8str, unsigned long actualUT8Len, wchar_t *UniStr, unsigned long *actualUnicodeLength, unsigned long MaxLen) {
	OSStatus err;
	TECObjectRef ec;
	TextEncoding utf8Encoding;
	unsigned long ail;
	unsigned long aol;

	utf8Encoding = CreateTextEncoding( kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF8Format );
	if ((err=TECCreateConverter(&ec, utf8Encoding, kTextEncodingUnicodeDefault)) != noErr) {
		if (actualUnicodeLength != NULL)
			*actualUnicodeLength = 0L;
		UniStr[0] = EOS;
		return(err);
	}

	if ((err=TECConvertText(ec, (ConstTextPtr)UTF8str, actualUT8Len, &ail, (TextPtr)UniStr, (MaxLen*2)-2, &aol)) != noErr) {
		if (actualUnicodeLength != NULL)
			*actualUnicodeLength = 0L;
		aol /= 2;
		UniStr[aol] = EOS;
		return(err);
	}
	err = TECDisposeConverter(ec);
	aol /= 2;
	UniStr[aol] = EOS;
	if (ail < actualUT8Len) {
		if (actualUnicodeLength != NULL)
			*actualUnicodeLength = 0L;
		return(1L);
	}
	if (actualUnicodeLength != NULL)
		*actualUnicodeLength = aol;
	return(0L);
}

long UnicodeToANSI(wchar_t *UniStr, unsigned long actualStringLength, unsigned char *ANSIstr, unsigned long *actualANSILen, unsigned long MaxLen, short script) {
	OSStatus err;
	TECObjectRef ec;
	TextEncoding MacRomanEncoding;
	unsigned long ail;
	unsigned long aol;

	actualStringLength *= 2;
	MacRomanEncoding = CreateTextEncoding((long)script, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat);
	if ((err=TECCreateConverter(&ec, kTextEncodingUnicodeDefault, MacRomanEncoding)) != noErr) {
		if (actualANSILen != NULL)
			*actualANSILen = 0L;
		ANSIstr[0] = EOS;
		return(err);
	}

	if ((err=TECConvertText(ec, (ConstTextPtr)UniStr, actualStringLength, &ail, (TextPtr)ANSIstr, MaxLen-2, &aol)) != noErr) {
		if (actualANSILen != NULL)
			*actualANSILen = 0L;
		ANSIstr[aol] = EOS;
		return(err);
	}
	err = TECDisposeConverter(ec);
	ANSIstr[aol] = EOS;
	if (ail < actualStringLength) {
		if (actualANSILen != NULL)
			*actualANSILen = 0L;
		return(1L);
	}
	if (actualANSILen != NULL)
		*actualANSILen = aol;
	return(0L);
}

long ANSIToUnicode(unsigned char *ANSIstr, unsigned long actualANSILen, wchar_t *UniStr, unsigned long *actualStringLength, unsigned long MaxLen, short script) {
	OSStatus err;
	TECObjectRef ec;
	TextEncoding MacRomanEncoding;
	unsigned long ail;
	unsigned long aol;

	MacRomanEncoding = CreateTextEncoding((long)script, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat);
	if ((err=TECCreateConverter(&ec, MacRomanEncoding, kTextEncodingUnicodeDefault)) != noErr) {
		if (err == kTECNoConversionPathErr) {
			for (aol=0; aol < actualANSILen; aol++)
				UniStr[aol] = ANSIstr[aol];
			ail = actualANSILen;
			goto finANSIToUni;
		} else {
			if (actualStringLength != NULL)
				*actualStringLength = 0L;
			UniStr[0] = EOS;
			return(err);
		}
	}
	if ((err=TECConvertText(ec, (ConstTextPtr)ANSIstr, actualANSILen, &ail, (TextPtr)UniStr, MaxLen-2, &aol)) != noErr) {
		aol /= 2;
		if (actualStringLength != NULL)
			*actualStringLength = aol;
		UniStr[aol] = EOS;
		return(err);
	}
	err = TECDisposeConverter(ec);
	aol /= 2;
finANSIToUni:
	UniStr[aol] = EOS;
	if (ail < actualANSILen) {
		if (actualStringLength != NULL)
			*actualStringLength = 0L;
		return(1L);
	}
	if (actualStringLength != NULL)
		*actualStringLength = aol;
	return(0L);
}

long UTF8ToANSI(unsigned char *UTF8str, unsigned long actualUT8Len, unsigned char *ANSIstr, unsigned long *actualANSILen, unsigned long MaxLen, short script) {
	OSStatus err;
	TECObjectRef ec;
	TextEncoding MacRomanEncoding;
	TextEncoding utf8Encoding;
	unsigned long ail;
	unsigned long aol;

	MacRomanEncoding = CreateTextEncoding((long)script, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat);
	utf8Encoding = CreateTextEncoding( kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF8Format );
	if ((err=TECCreateConverter(&ec, utf8Encoding, MacRomanEncoding)) != noErr) {
		if (actualANSILen != NULL)
			*actualANSILen = 0L;
		ANSIstr[0] = EOS;
		return(err);
	}

	if ((err=TECConvertText(ec, (ConstTextPtr)UTF8str, actualUT8Len, &ail, (TextPtr)ANSIstr, MaxLen-2, &aol)) != noErr) {
		ANSIstr[aol] = EOS;
		if (actualANSILen != NULL)
			*actualANSILen = 0L;
		return(err);
	}
	err = TECDisposeConverter(ec);
	ANSIstr[aol] = EOS;
	if (ail < actualUT8Len) {
		if (actualANSILen != NULL)
			*actualANSILen = 0L;
		return(1L);
	}
	if (actualANSILen != NULL)
		*actualANSILen = aol;
	return(0L);
}

long ANSIToUTF8(unsigned char *ANSIstr, unsigned long actualANSILen, unsigned char *UTF8str, unsigned long *actualUT8Len, unsigned long MaxLen, short script) {
	OSStatus err;
	TECObjectRef ec;
	TextEncoding MacRomanEncoding;
	TextEncoding utf8Encoding;
	unsigned long ail;
	unsigned long aol;

	MacRomanEncoding = CreateTextEncoding((long)script, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat);
	utf8Encoding = CreateTextEncoding( kTextEncodingUnicodeDefault, kTextEncodingDefaultVariant, kUnicodeUTF8Format );
	if ((err=TECCreateConverter(&ec, MacRomanEncoding, utf8Encoding)) != noErr) {
		if (actualUT8Len != NULL)
			*actualUT8Len = 0L;
		UTF8str[0] = EOS;
		return(err);
	}

	if ((err=TECConvertText(ec, (ConstTextPtr)ANSIstr, actualANSILen, &ail, (TextPtr)UTF8str, MaxLen-2, &aol)) != noErr) {
		if (actualUT8Len != NULL)
			*actualUT8Len = 0L;
		UTF8str[aol] = EOS;
		return(err);
	}
	err = TECDisposeConverter(ec);
	UTF8str[aol] = EOS;
	if (ail < actualANSILen) {
		if (actualUT8Len != NULL)
			*actualUT8Len = 0L;
		return(1L);
	}
	if (actualUT8Len != NULL)
		*actualUT8Len = aol;
	return(0L);
}

OSStatus my_FSPathMakeRef(const FNType *path, FSRef *ref) {
	return(FSPathMakeRef((UInt8 *)path, ref, NULL));
}

OSStatus my_FSRefMakePath(const FSRef *ref, FNType *path, UInt32 maxPathSize) {
	return(FSRefMakePath(ref, (UInt8 *)path, maxPathSize));
}

CFStringRef my_CFStringCreateWithBytes(const char *bytes) {
	return(CFStringCreateWithBytes(NULL, (UInt8 *)bytes, strlen(bytes), kCFStringEncodingUTF8, false));
}

void my_CFStringGetBytes(CFStringRef theString, char *buf, CFIndex maxBufLen) {
	CFIndex	len;

	CFStringGetBytes(theString, CFRangeMake(0, CFStringGetLength(theString)), kCFStringEncodingUTF8, 0, false, (UInt8 *)buf, maxBufLen, &len);
	buf[len] = EOS;
}

int ced_getc(void) {
	int key;

	if (global_df != NULL) {
		if (!global_df->LeaveHighliteOn) {
			global_df->row_win2 = 0L;
			global_df->col_win2 = -2L;
			global_df->col_chr2 = -2L;
		}
	}
	F_key = 0;
	if (pasteStr == 0) {
		SetScrollControl();
		if (actualUTFCnt < actualUTFLen) {
			key = UTFstr[actualUTFCnt];
			actualUTFCnt++;
		} else {
			while(RealMainEvent(&key) == -1 && isPlayS == 0) {
				if (isKillProgram == 1) {
					key = '\n';
					break;
				}
			}
			actualUTFCnt = 0L;
			actualUTFLen = 0L;
			if (global_df != NULL && UniInputBuf.len > 0L) {
				if (UniInputBuf.unicodeInputString[0] > 32) {
					if (UniInputBuf.len == 1L)
						key = UniInputBuf.unicodeInputString[0];
					else {
						strcpy(UTFstr, UniInputBuf.unicodeInputString);
						actualUTFLen = UniInputBuf.len;
						if (actualUTFCnt < actualUTFLen) {
							key = (int)UTFstr[actualUTFCnt];
							actualUTFCnt++;
							if (actualUTFCnt < actualUTFLen)
								globalWhichInput = 0;
						}
					}
				}
				UniInputBuf.len = 0L;
			}
		}
	} else {
		lPasteStr = (unCH *)*pasteStr;
		key = lPasteStr[pasteIndex++];
		if ((*pasteStr)[pasteIndex] == 0) {
			pasteIndex = 0L;
			DisposeHandle(pasteStr);
			pasteStr = 0;
		}
	}
	if (global_df != NULL) {
		if (isPlayS == 0 && !global_df->isExtend)
			global_df->LeaveHighliteOn = FALSE;
	}
//	if (key == 0x3000)
//		key = 0x20;
	return(key);
}

static void AdjustPathNameLen(wchar_t *path, long pixelLen, FONTINFO *fnt) {
	wchar_t *o, *b, *e;
	
	b = path;
/*
 #ifdef _WIN32
	if (b[0] == PATHDELIMCHR && b[1] == PATHDELIMCHR) {
		b += 2;
		if (iswalpha(b[0]) && b[1] == ':' && b[2] == PATHDELIMCHR)
			b += 2;
		else
			b--;
	} else if (iswalpha(b[0]) && b[1] == ':' && b[2] == PATHDELIMCHR)
		b += 2;
#endif // _WIN32
*/
	o = b;
	e = strchr(b+1, PATHDELIMCHR);
	if (e != NULL)
		b = e;
	
	while (TextWidthInPix(path, 0L, strlen(path), fnt, 0) > pixelLen) {
		e = strchr(b+1, PATHDELIMCHR);
		if (e == NULL) {
			if (b > o)
				b = o;
			else
				break;
		} else if (e - b <= 4) {
			strcpy(b, e);
			b++;
		} else {
			strcpy(b+4, e);
			b[1] = '.';
			b[2] = '.';
			b[3] = '.';
			b++;
		}
	}
}

// val = GetControl32BitValue(itemCtrl);
// SetControl32BitValue(itemCtrl, val); // val == 1, 0
// kControlEditTextTextTag, kControlStaticTextTextTag
void SetDialogItemValue(WindowPtr myDlg, short itemID, short val) {
	ControlRef	itemCtrl;
	HIViewID	viewID;

	viewID.signature = 0;
	viewID.id = itemID;
	HIViewFindByID(HIViewGetRoot(myDlg), viewID, &itemCtrl);
	SetControl32BitValue(itemCtrl, val);
}

void SetDialogItemUTF8(const char *text, WindowPtr myDlg, short itemID, char isWinLim) {
	Rect		box;
	HIViewID	viewID;
	ControlRef	itemCtrl;
	FONTINFO	fnt;

	viewID.signature = 0;
	viewID.id = itemID;
	HIViewFindByID(HIViewGetRoot(myDlg), viewID, &itemCtrl);
	GetControlBounds(itemCtrl, &box);
	box.bottom++;
	EraseRect(&box);
	if (uS.mStricmp(dFnt.fontName, "Ascender Uni Duo") == 0) {
		TextFont(0);
		TextSize(0);
		fnt.FName = 0;
		fnt.FSize = 0;
	} else {
		fnt.FName = dFnt.fontId;
		if (dFnt.fontSize < 15)
			fnt.FSize = dFnt.fontSize;
		else
			fnt.FSize = 14;
		TextFont(fnt.FName);
		TextSize(fnt.FSize);
	}
	TextFace(0);
	if (isWinLim) {
		u_strcpy(wTempStr, text, UTTLINELEN);
		AdjustPathNameLen(wTempStr, box.right-box.left, &fnt);
	} else {
		u_strcpy(wTempStr, text, UTTLINELEN);
	}
	TXNDrawUnicodeTextBox((UniChar *)wTempStr, strlen(wTempStr), &box, NULL, NULL); 
}

short windWidth(WindowPtr wind) {
	Rect	portRect;

	GetWindowPortBounds(wind, &portRect);
	return(boxWidth(portRect));
}

short windHeight(WindowPtr wind) {
	Rect	portRect;
	
	GetWindowPortBounds(wind, &portRect);
	return(boxHeight(portRect));
}

int LocateDir(const char *prompt, FNType *currentDir, char noDefault) {
	char				DirSelected;
	int					len;
	FNType				new_path[FNSize];
	FNType 				dirname[FNSize];
	NavReplyRecord		reply;
	OSStatus			anErr = noErr;
	FSRef				ref;
	CFStringRef			CFPrompt,
						CFdirname;
	NavDialogRef		outDialog;
	NavEventUPP			eventProc = NewNavEventUPP(myNavEventProc);
	NavDialogCreationOptions Options;

	DirSelected = FALSE;
	uS.str2FNType(dirname, 0L, "Current Directory-> ");
	strcat(dirname, currentDir);
	CFPrompt = my_CFStringCreateWithBytes(prompt);
	if (CFPrompt == NULL)
		return(FALSE);
	CFdirname= my_CFStringCreateWithBytes(dirname);
	if (CFdirname == NULL) {
		CFRelease(CFPrompt);
		return(FALSE);
	}

	NavGetDefaultDialogCreationOptions(&Options);
	Options.version = kNavDialogCreationOptionsVersion;
	Options.optionFlags = kNavDefaultNavDlogOptions;
	Options.location = SFGwhere;
	Options.clientName = CFSTR("CLAN");
	Options.windowTitle = CFPrompt;
	Options.actionButtonLabel = CFSTR("Select Folder");
	Options.cancelButtonLabel = CFSTR("Cancel");
//	Options.saveFileName = CFSTR("");
	Options.message = CFdirname;
	Options.preferenceKey = 0;
//	Options.popupExtension;
//	Options.modality;
//	Options.parentWindow;
	anErr = NavCreateChooseFolderDialog(&Options, eventProc, NULL, NULL, &outDialog);
	if (anErr != noErr) {
		CFRelease(CFPrompt);
		CFRelease(CFdirname);
		return(FALSE);
	}

	if (noDefault) {
		if (!isRefEQZero(defaultPath)) {
			AEDesc defaultDir;
			
			my_FSPathMakeRef(defaultPath, &ref);
			AECreateDesc(typeFSRef, &ref, sizeof(ref),&defaultDir);
			NavCustomControl(outDialog, kNavCtlSetLocation, &defaultDir);
			AEDisposeDesc(&defaultDir);
		}
	} else if (!isRefEQZero(currentDir)) {
		AEDesc defaultDir;
		
		my_FSPathMakeRef(currentDir, &ref);
		AECreateDesc(typeFSRef, &ref, sizeof(ref),&defaultDir);
		NavCustomControl(outDialog, kNavCtlSetLocation, &defaultDir);
		AEDisposeDesc(&defaultDir);
	}
	anErr = NavDialogRun(outDialog);
	if (anErr != noErr) {
		CFRelease(CFPrompt);
		CFRelease(CFdirname);
		NavDialogDispose(outDialog);
		return(FALSE);
	}
	anErr = NavDialogGetReply(outDialog, &reply);
	if (anErr != noErr) {
		CFRelease(CFPrompt);
		CFRelease(CFdirname);
		NavDialogDispose(outDialog);
		return(FALSE);
	}
	if (reply.validRecord) {
		long count;
		anErr = AECountItems(&(reply.selection),&count);
		if (anErr == noErr) {
			long index;
			AEKeyword theKeyword;
			DescType actualType;
			Size actualSize;

			for (index=1; index <=count; index++) {
				anErr = AEGetNthPtr(&(reply.selection),index,typeFSRef,&theKeyword,&actualType,&ref,sizeof(ref),&actualSize);
				if (anErr ==noErr) {
					DirSelected = TRUE;
					my_FSRefMakePath(&ref, new_path, FNSize);
					if (noDefault) {
						strcpy(defaultPath, new_path);
						len = strlen(defaultPath);
						if (defaultPath[len-1] != PATHDELIMCHR)
							uS.str2FNType(defaultPath, len, PATHDELIMSTR);
					}
				}
			}
		}
	}
	//Dispose of NavReplyRecord,resources,descriptors
	NavDisposeReply(&reply);
	NavDialogDispose(outDialog);
	CFRelease(CFPrompt);
	CFRelease(CFdirname);

	if (DirSelected) {
		FinishMainLoop();
		strcpy(currentDir, new_path);
		return(TRUE);
	} else
		return(FALSE);
}

pascal void myNavEventProc(NavEventCallbackMessage callBackSelector,
							NavCBRecPtr callBackParms, NavCallBackUserData callBackUD)
{
#pragma unused(callBackUD)
	if (callBackSelector == kNavCBEvent) {
		UInt16			firstCustomItem, hit;
		PrepareStruct	saveRec;
		EventRecord		*myEvent = callBackParms->eventData.eventDataParms.event;
		WindowPtr whichWindow = (WindowPtr)myEvent->message;
		WindowProcRec *windProc = WindowProcs(whichWindow);

		switch (myEvent->what) {
			case mouseDown:		/* mouse event */
				ControlRef		theControl;
				ControlPartCode	code;
				ControlID outID;

				NavCustomControl(callBackParms->context, kNavCtlGetFirstControlID, &firstCustomItem);
				GlobalToLocal(&myEvent->where);
				theControl = FindControlUnderMouse(myEvent->where,whichWindow,&code);
				if (theControl != NULL && GetControlID(theControl, &outID) == noErr) {
						hit = outID.id;
						hit -= firstCustomItem;
				}
				LocalToGlobal(&myEvent->where);
				break;
			case updateEvt:
				if (windProc != NULL && global_df != NULL) {
					DrawCursor(0);
					DrawSoundCursor(0);
				}
				if (windProc != NULL) {
					if (windProc->id == 500 && theMovie != NULL) {
						MCIsPlayerEvent(theMovie->MvController,myEvent);
					}
					BeginUpdate(whichWindow);
					PrepareWindA4(whichWindow, &saveRec);
					if (windProc->UpdateProc)
						windProc->UpdateProc(whichWindow);/* Update this window */
					RestoreWindA4(&saveRec);
					EndUpdate(whichWindow);
				}
				break;
		}
	}
}

static Handle NewOpenHandle(OSType applicationSignature, short numTypes, OSType typeList[])
{
	Handle hdl = NULL;
	
	if ( numTypes > 0 ) {
		hdl = NewHandle(sizeof(NavTypeList) + numTypes * sizeof(OSType));
		if (hdl != NULL) {
			NavTypeListHandle open		= (NavTypeListHandle)hdl;
			(*open)->componentSignature = applicationSignature;
			(*open)->osTypeCount		= numTypes;
			BlockMoveData(typeList, (*open)->osType, numTypes * sizeof(OSType));
		}
	}
	return hdl;
}

char myNavGetFile(const char *prompt, short numTypes, OSType typeList[], NavObjectFilterProcPtr FilterProc, FNType *file_path) {
//	int					len;
	FNType				mDirPathName[FNSize];
	NavReplyRecord		reply;
	OSStatus			anErr = noErr;
	FSRef				ref;
	CFStringRef			CFPrompt;
	NavTypeListHandle	openList = nil;
	NavObjectFilterUPP	filterProc;
	NavDialogRef		outDialog;
	NavEventUPP			eventProc = NewNavEventUPP(myNavEventProc);
	NavDialogCreationOptions Options;

	if (prompt == nil)
		prompt = "Open File";
	if (numTypes == -1 || typeList == nil)
		openList = nil;
	else
		openList = (NavTypeListHandle)NewOpenHandle('****',numTypes,typeList);

	CFPrompt = my_CFStringCreateWithBytes(prompt);
	if (CFPrompt == NULL)
		return(FALSE);

	if (FilterProc == nil)
		filterProc = nil;
	else {
		filterProc = NewNavObjectFilterUPP(FilterProc);
	}

	NavGetDefaultDialogCreationOptions(&Options);
	Options.version = kNavDialogCreationOptionsVersion;
	Options.optionFlags = kNavDefaultNavDlogOptions;
	Options.location = SFGwhere;
	Options.clientName = CFSTR("CLAN");
	Options.windowTitle = CFPrompt;
	Options.actionButtonLabel = CFSTR("Open File");
	Options.cancelButtonLabel = CFSTR("Cancel");
//	Options.saveFileName = CFSTR("");
//	Options.message = ;
	Options.preferenceKey = 0;
//	Options.popupExtension;
//	Options.modality;
//	Options.parentWindow;

	anErr = NavCreateGetFileDialog(&Options, openList, eventProc, nil, filterProc, nil, &outDialog);
	if (anErr != noErr) {
		CFRelease(CFPrompt);
		return(FALSE);
	}
	if (!isRefEQZero(file_path)) {
		AEDesc defaultDir;
//		AEDescList defaultSelection;

		extractPath(mDirPathName, file_path);
		my_FSPathMakeRef(mDirPathName, &ref);
		AECreateDesc(typeFSRef, &ref, sizeof(ref), &defaultDir);
		NavCustomControl(outDialog, kNavCtlSetLocation, &defaultDir);
		AEDisposeDesc(&defaultDir);
/*
		AECreateList(NULL, 0, FALSE, &defaultSelection);
//		AECreateDesc(typeFSRef, &ref, sizeof(ref), &defaultSelection);
		NavCustomControl(outDialog, kNavCtlSetSelection, &defaultSelection);
		AEDisposeDesc(&defaultSelection);
*/
	} else if (!isRefEQZero(defaultPath)) {
		AEDesc defaultDir;

//		len = strlen(defaultPath);
//		if (defaultPath[len-1] == PATHDELIMCHR)
//			defaultPath[len-1] = EOS;
		my_FSPathMakeRef(defaultPath, &ref);
		AECreateDesc(typeFSRef, &ref, sizeof(ref),&defaultDir);
		NavCustomControl(outDialog, kNavCtlSetLocation, &defaultDir);
		AEDisposeDesc(&defaultDir);
	}
	anErr = NavDialogRun(outDialog);
	if (anErr != noErr) {
		CFRelease(CFPrompt);
		NavDialogDispose(outDialog);
		return(FALSE);
	}
	anErr = NavDialogGetReply(outDialog, &reply);
	if (anErr != noErr) {
		CFRelease(CFPrompt);
		NavDialogDispose(outDialog);
		return(FALSE);
	}
	if (reply.validRecord) {
		long count;
		anErr = AECountItems(&(reply.selection),&count);
		if (anErr == noErr) {
			long index;
			AEKeyword theKeyword;
			DescType actualType;
			Size actualSize;
	
			for (index=1; index <=count; index++) {
				anErr = AEGetNthPtr(&(reply.selection),index,typeFSRef,&theKeyword,&actualType,&ref,sizeof(ref),&actualSize);
				if (anErr == noErr) {
					my_FSRefMakePath(&ref, file_path, FNSize);
					extractPath(defaultPath, file_path);
					break;
				} else { 
					strcpy(global_df->err_message, DASHES);
					break;
				}
			}
		}
	}
	//Dispose of NavReplyRecord,resources,descriptors
	if (eventProc != nil)
		DisposeNavEventUPP(eventProc);
	if (filterProc != nil)
		DisposeNavObjectFilterUPP(filterProc);
	if (openList != nil)
		DisposeHandle((Handle)openList);
	NavDisposeReply(&reply);
	NavDialogDispose(outDialog);
	CFRelease(CFPrompt);
	return(anErr == noErr);
}

char myNavPutFile(FNType *fname, const char *prompt, OSType fileType, Boolean *isReplace) {
	CFStringRef			CFPrompt,
						CFfilename;
	FSRef				ref;
	NavReplyRecord		reply;
	OSStatus			anErr = noErr;
	NavDialogRef		outDialog;
	NavEventUPP			eventProc = NewNavEventUPP(myNavEventProc);
	AEDesc				defaultDir;
	FNType				mDirPathName[FNSize];
	NavDialogCreationOptions Options;

	CFPrompt = my_CFStringCreateWithBytes(prompt);
	if (CFPrompt == NULL)
		return(FALSE);

	extractFileName(mDirPathName, fname);
	CFfilename= my_CFStringCreateWithBytes(mDirPathName);
	if (CFfilename == NULL) {
		CFRelease(CFPrompt);
		return(FALSE);
	}

	NavGetDefaultDialogCreationOptions(&Options);
	Options.version = kNavDialogCreationOptionsVersion;
	Options.optionFlags = kNavDefaultNavDlogOptions;
	Options.location = SFGwhere;
	Options.clientName = CFSTR("CLAN");
	Options.windowTitle = CFPrompt;
	Options.actionButtonLabel = CFSTR("Save File");
	Options.cancelButtonLabel = CFSTR("Cancel");
	Options.saveFileName = CFfilename;
//	Options.message = ;
	Options.preferenceKey = 0;
//	Options.popupExtension;
//	Options.modality;
//	Options.parentWindow;

	anErr = NavCreatePutFileDialog(&Options, fileType, PROGCREATOR, eventProc, nil, &outDialog);
	if (anErr != noErr) {
		CFRelease(CFPrompt);
		CFRelease(CFfilename);
		return(FALSE);
	}

	extractPath(mDirPathName, fname);
	if (!isRefEQZero(mDirPathName)) {		
		my_FSPathMakeRef(mDirPathName, &ref);
		AECreateDesc(typeFSRef, &ref, sizeof(ref),&defaultDir);
		NavCustomControl(outDialog, kNavCtlSetLocation, &defaultDir);
		AEDisposeDesc(&defaultDir);
	} else if (!isRefEQZero(defaultPath)) {		
		my_FSPathMakeRef(defaultPath, &ref);
		AECreateDesc(typeFSRef, &ref, sizeof(ref),&defaultDir);
		NavCustomControl(outDialog, kNavCtlSetLocation, &defaultDir);
		AEDisposeDesc(&defaultDir);
	}
	
	anErr = NavDialogRun(outDialog);
	if (anErr != noErr) {
		CFRelease(CFPrompt);
		CFRelease(CFfilename);
		NavDialogDispose(outDialog);
		return(FALSE);
	}
	anErr = NavDialogGetReply(outDialog, &reply);
	if (anErr != noErr) {
		CFRelease(CFPrompt);
		CFRelease(CFfilename);
		NavDialogDispose(outDialog);
		return(FALSE);
	}

	if (reply.validRecord) {
		long count;
		anErr = AECountItems(&(reply.selection),&count);
		if (anErr == noErr) {
			long index;
			AEKeyword theKeyword;
			DescType actualType;
			Size actualSize;
			for (index=1; index <=count; index++) {
				anErr = AEGetNthPtr(&(reply.selection),index,typeFSRef,&theKeyword,&actualType,&ref,sizeof(ref),&actualSize);
				if (anErr == noErr) {
					my_CFStringGetBytes(reply.saveFileName, mDirPathName, 256);
					my_FSRefMakePath(&ref, fname, FNSize);
					addFilename2Path(fname, mDirPathName);
					extractPath(defaultPath, fname);
					if (isReplace != nil)
						*isReplace = reply.replacing;
					break;
				} else { 
					strcpy(global_df->err_message, DASHES);
					break;
				}
			}
		}
	}
	//Dispose of NavReplyRecord,resources,descriptors
	if (eventProc != nil)
		DisposeNavEventUPP(eventProc);
	NavDisposeReply(&reply);
	NavDialogDispose(outDialog);
	CFRelease(CFfilename);
	CFRelease(CFPrompt);
	return(anErr == noErr);
}

void SetDefKeyScript(void) {
    WindowPtr	win;
    GrafPtr oldPort;

	GetPort(&oldPort);
    if ((win=FrontWindow()) != NULL) {
		if (WindowProcs(win)) {
			SetPortWindowPort(win);
			TextFont(DEFAULT_ID);
			TextSize(DEFAULT_SIZE);
			cedDFnt.Encod = my_FontToScript(DEFAULT_ID, 0);
			if (AutoScriptSelect)
				KeyScript(cedDFnt.Encod);
			SetPort(oldPort);
		}
	}
}

short my_FontToScript(short fName, int charSet) {
	extern short ArialUnicodeFOND, SecondDefUniFOND;

	if (ArialUnicodeFOND != 0 && ArialUnicodeFOND == fName)
		return(GetScriptManagerVariable(smKeyScript));
	if (SecondDefUniFOND != 0 && SecondDefUniFOND == fName)
		return(GetScriptManagerVariable(smKeyScript));
	return(FontToScript(fName));
}

short my_CharacterByteType(const char *org, short pos, NewFontInfo *finfo) {
	short cType;

		cType = 2;
/*
		for (i=0; i <= pos; i++) {
			if (IsDBCSLeadByteEx(950,(BYTE)org[i]) == 0) {
				DWORD t = GetLastError();
				if (cType == -1) {
					cType = 1;
				} else
					cType = 0;
			} else
				cType = -1;
		}
*/
		if (UTF8_IS_SINGLE((unsigned char)org[pos]))
			cType = 0;
		else if (UTF8_IS_LEAD((unsigned char)org[pos]))
			cType = -1;
		else if (UTF8_IS_TRAIL((unsigned char)org[pos])) {
			if (!UTF8_IS_TRAIL((unsigned char)org[pos+1]))
				cType = 1;
			else
				cType = 2;
		}

		return(cType);
}

void EnableDItem(DialogPtr dial, short which, short enable) {
	short		type;
	Handle		item;
	Rect		box;

	GetDialogItem(dial, which, &type, &item, &box);
	if (enable) {
		HiliteControl((ControlRef)item, 0);
		type &= 0x7;
	} else {
		HiliteControl((ControlRef)item, 255);
		type |= 0x8;
	}
	SetDialogItem(dial, which, type, item, &box);
}

char getFileDate(FNType *fn, UInt32 *date) {
	FSCatalogInfo catalogInfo;
	FSRef  ref;
	CFAbsoluteTime oCFTime;

	*date = 0L;
	my_FSPathMakeRef(fn, &ref); 
	if (FSGetCatalogInfo(&ref, kFSCatInfoContentMod, &catalogInfo, NULL, NULL, NULL) == noErr) {
		if (UCConvertUTCDateTimeToCFAbsoluteTime(&catalogInfo.contentModDate, &oCFTime) == noErr)
			UCConvertCFAbsoluteTimeToSeconds(oCFTime, date);
		return(1);
	} else
		return(0);
}

void setFileDate(FNType *fn, UInt32 date) {
	FSCatalogInfo catalogInfo;
	FSRef  ref;
	CFAbsoluteTime oCFTime;

	my_FSPathMakeRef(fn, &ref); 
	if (FSGetCatalogInfo(&ref, kFSCatInfoContentMod, &catalogInfo, NULL, NULL, NULL) == noErr) {
		if (UCConvertSecondsToCFAbsoluteTime(date, &oCFTime) == noErr) {
			if (UCConvertCFAbsoluteTimeToUTCDateTime(oCFTime, &catalogInfo.contentModDate) == noErr)
				FSSetCatalogInfo(&ref, kFSCatInfoContentMod, &catalogInfo);
		}
	}
}

void gettyp(const FNType *fn, long *type, long *creator) {
	FSRef  ref;
	creator_type the_file_type;
	FSCatalogInfo catalogInfo;

	my_FSPathMakeRef(fn, &ref); 
	if (FSGetCatalogInfo(&ref, kFSCatInfoFinderInfo, &catalogInfo, NULL, NULL, NULL) == noErr) {
		the_file_type.in[0] = catalogInfo.finderInfo[0];
		the_file_type.in[1] = catalogInfo.finderInfo[1];
		the_file_type.in[2] = catalogInfo.finderInfo[2];
		the_file_type.in[3] = catalogInfo.finderInfo[3];
		*type = the_file_type.out;
		the_file_type.in[0] = catalogInfo.finderInfo[4];
		the_file_type.in[1] = catalogInfo.finderInfo[5];
		the_file_type.in[2] = catalogInfo.finderInfo[6];
		the_file_type.in[3] = catalogInfo.finderInfo[7];
		*creator = the_file_type.out;
	} else
		return;
}

void settyp(const FNType *fn, long type, long creator, char isForce) {
	FSRef  ref;
	creator_type the_file_type;
	FSCatalogInfo catalogInfo;

	my_FSPathMakeRef(fn, &ref); 
	if (FSGetCatalogInfo(&ref, kFSCatInfoFinderInfo, &catalogInfo, NULL, NULL, NULL) == noErr) {
		if (type || isForce) {
			the_file_type.out = type;
			catalogInfo.finderInfo[0] = the_file_type.in[0];
			catalogInfo.finderInfo[1] = the_file_type.in[1];
			catalogInfo.finderInfo[2] = the_file_type.in[2];
			catalogInfo.finderInfo[3] = the_file_type.in[3];
		}
		if (creator || isForce) {
			the_file_type.out = creator;
			catalogInfo.finderInfo[4] = the_file_type.in[0];
			catalogInfo.finderInfo[5] = the_file_type.in[1];
			catalogInfo.finderInfo[6] = the_file_type.in[2];
			catalogInfo.finderInfo[7] = the_file_type.in[3];
		}
		FSSetCatalogInfo(&ref, kFSCatInfoFinderInfo, &catalogInfo);
	} else
		return;
}

void DrawMouseCursor(char which) {
	if (which == 3) {
		SetCursor(*GetCursor(128));
	} else if (which == 2) {
		SetCursor(*GetCursor(watchCursor));
	} else if (which == 1) {
		Cursor arrow;
		SetCursor(GetQDGlobalsArrow(&arrow));
	} else {
		SetCursor(*GetCursor(iBeamCursor)); /* iBeamCursor */
	}
}

extern void RestoreWindA4(PrepareStruct *rec);
extern void PrepareWindA4(WindowPtr wind, PrepareStruct *rec);

/* worning event begin */
/*
static void CleanupWarningText(WindowPtr win) {
#pragma unused(win)
	NonModal = 0;
}

static short MainWarningEvent(WindowPtr win, EventRecord *event) {
	short			ModalHit;
	ControlRef		theControl;
	PrepareStruct	saveRec;

	if (event->what == keyDown || event->what == autoKey) {
		ModalHit = event->message & charCodeMask;
		if (ModalHit == RETURN_CHAR || ModalHit == ENTER_CHAR)
			ModalHit = 1;
		else
			ModalHit = 0;
	} else
	if (event->what == mouseDown) {
		ControlPartCode	code;
		ControlID outID;

		GlobalToLocal(&event->where);
		theControl = FindControlUnderMouse(event->where,win,&code);
		if (theControl != NULL) {
			code = HandleControlClick(theControl,event->where,event->modifiers,NULL); 
			if (GetControlID(theControl, &outID) == noErr)
				ModalHit = outID.id;
			if (code == 0)
				ModalHit = 0;
		}
		LocalToGlobal(&event->where);
	}
	if (ModalHit == 1) {
		PrepareWindA4(win, &saveRec);
		mCloseWindow(win);
		RestoreWindA4(&saveRec);
	} else if (ModalHit == 5) {
		PrepareWindA4(win, &saveRec);
		mCloseWindow(win);
		RestoreWindA4(&saveRec);
		isKillProgram = 1;
	}
	return(-1);
}

static void UpdateWarningText(WindowPtr win) {
    Rect	theRect;
	GrafPtr	oldPort;

	if (NonModal != 139)
		return;	

	GetPort(&oldPort);
	SetPortWindowPort(win);
	GetWindowPortBounds(win, &theRect);
	EraseRect(&theRect);
	theRect.bottom = theRect.bottom;
    theRect.top = theRect.top;
	theRect.left = theRect.left;
	theRect.right = theRect.right;

	SetDialogItemUTF8(text, win, 2, FALSE);
	SetDialogItemUTF8(tText, win, 4, FALSE);
	DrawControls(win);
	SetPort(oldPort);
}

void do_warning(const char *str, int delay) {
	int				t;
	short			dialNum;
	long			MaxTick, secs, oldSecs;
	PrepareStruct	saveRec;
	WindowPtr		win;
	extern char		skip_prog;

	saveGlobal_df = global_df;
	saveIsPlayS = isPlayS;
	saveF_key   = F_key;
	if (str == NULL)
		return;

	if (skip_prog != 0 && delay >= 0)
		dialNum = 155;
	else
		dialNum = 139;
	if (OpenWindow(dialNum, warning_str, 0L, false, 0, MainWarningEvent, UpdateWarningText, CleanupWarningText)) 
		return;
	*tText = EOS;
	strcpy(text, str);
	NonModal = 139;
	MaxTick = TickCount() + delay;
	oldSecs = 0L;
	win = FindAWindowNamed(warning_str);
	while (NonModal && RealMainEvent(&t)) {
		if (delay > 0) {
			if (MaxTick < TickCount()) {
				if (win != NULL) {
					PrepareWindA4(win, &saveRec);
					mCloseWindow(win);
					RestoreWindA4(&saveRec);
					NonModal = 0;
				}
			} else if (win != NULL) {
				secs = (MaxTick - TickCount()) / 60;
				if (secs != oldSecs) {
					sprintf(tText, "This message will disappear in %ld sec(s).", secs+1);
					PrepareWindA4(win, &saveRec);
					UpdateWarningText(win);
					RestoreWindA4(&saveRec);
					oldSecs = secs;
				}
			}
		}
	}
	if ((win=FindAWindowNamed(warning_str)) != NULL) {
		PrepareWindA4(win, &saveRec);
		mCloseWindow(win);
		RestoreWindA4(&saveRec);
	}
	global_df = saveGlobal_df;
	isPlayS = saveIsPlayS;
	F_key	= saveF_key;
}
*/
void do_warning(const char *str, int delay) {
	CFStringRef cString;
	OSStatus err;
	DialogRef outAlert;
	AlertStdCFStringAlertParamRec param;
	DialogItemIndex outItemHit;
	extern char		skip_prog;

	if (str == NULL)
		return;

	cString = CFStringCreateWithBytes(NULL, (UInt8 *)str, strlen(str), kCFStringEncodingUTF8, false);
	if (cString != NULL) {
		param.version = kStdCFStringAlertVersionOne;
		param.movable = true;
		param.helpButton = false;
		param.defaultText = CFSTR("Click here when ready to continue");
		if (skip_prog != 0 && delay >= 0) {
			param.cancelText = CFSTR("Quit CLAN Command");
		} else {
			param.cancelText = NULL;
		}
		param.otherText = NULL;
		param.defaultButton = kAlertStdAlertOKButton;
		param.cancelButton = kAlertStdAlertCancelButton;
		param.position = kWindowAlertPositionParentWindow; //kWindowStaggerParentWindow; // kWindowDefaultPosition;
		param.flags = kStdAlertDoNotAnimateOnDefault | kStdAlertDoNotAnimateOnCancel;
		err = CreateStandardAlert(kAlertCautionAlert, cString, NULL, &param, &outAlert);
		CFRelease(cString);
		err = RunStandardAlert(outAlert, NULL, &outItemHit);
		if (outItemHit == kAlertStdAlertCancelButton) {
			isKillProgram = 1;
		}
	}						
}
/* worning event end */
/* progress bar even begin */
static void CleanupProgressText(WindowPtr win) {
#pragma unused(win)
	NonModal = 0;
	global_df = saveGlobal_df;
	isPlayS = saveIsPlayS;
	F_key	= saveF_key;
	ProgressFilled = 0;
}

static short MainProgressEvent(WindowPtr win, EventRecord *event) {
	short			ModalHit;
	ControlRef		theControl;
	PrepareStruct	saveRec;

	if (event->what == keyDown || event->what == autoKey) {
		ModalHit = event->message & charCodeMask;
		if (ModalHit == '\033')
			ModalHit = 2;
		else
			ModalHit = 0;
	} else
	if (event->what == mouseDown) {
		ControlPartCode	code;
		ControlID outID;

		GlobalToLocal(&event->where);
		theControl = FindControlUnderMouse(event->where,win,&code);
		if (theControl != NULL) {
			code = HandleControlClick(theControl,event->where,event->modifiers,NULL); 
			if (GetControlID(theControl, &outID) == noErr)
				ModalHit = outID.id;
			if (code == 0)
				ModalHit = 0;
		}
		LocalToGlobal(&event->where);
	}
	if (ModalHit == 2) {
		PrepareWindA4(win, &saveRec);
		mCloseWindow(win);
		RestoreWindA4(&saveRec);
	}
	return(-1);
}

static void UpdateProgressText(WindowPtr win) {
	Rect	theRect;
	GrafPtr	oldPort;

	GetPort(&oldPort);
	SetPortWindowPort(win);
	GetWindowPortBounds(win, &theRect);
	EraseRect(&theRect);
	SetDialogItemUTF8(text, win, 3, FALSE);
	DrawControls(win);
	SetPort(oldPort);
	UpdateProgressDialog(ProgressFilled);
}

char OpenProgressDialog(const char *str) {
	WindowPtr	win;
    Rect		theRect;
	GrafPtr		oldPort;

	if (OpenWindow(145, progress_str, 0L, false, 0, MainProgressEvent, UpdateProgressText, CleanupProgressText)) 
		return(FALSE);

	strcpy(text, str);
	saveGlobal_df = global_df;
	saveIsPlayS = isPlayS;
	saveF_key   = F_key;
	NonModal = 145;
	ProgressFilled = 0;
	win = FindAWindowNamed(progress_str);
	if (win == NULL)
		return(FALSE);

	GetPort(&oldPort);
	SetPortWindowPort(win);
	GetWindowPortBounds(win, &theRect);
	EraseRect(&theRect);
	SetDialogItemUTF8(text, win, 3, FALSE);
	DrawControls(win);
	SetPort(oldPort);
	if (!UpdateProgressDialog(0))
		return(FALSE);
	return(TRUE);
}

void CloseProgressDialog(void) {
	WindowPtr		win;
	PrepareStruct	saveRec;

	win = FindAWindowNamed(progress_str);
	if (win == NULL)
		return;
	PrepareWindA4(win, &saveRec);
	mCloseWindow(win);
	RestoreWindA4(&saveRec);
}

char UpdateProgressDialog(short percentFilled) {
	Rect		box, fBox;
	int			t;
	RGBColor	rgbBlack 	= {0L, 0L, 0L};
	RGBColor 	rgbBarColor = {17476L, 17476L, 17476L};	
	RGBColor	oldForeColor;
	PenState	oldPen;
	GrafPtr		oldPort;
	short		pixelsDone;
	WindowPtr	win;

	win = FindAWindowNamed(progress_str);
	if (win == NULL)
		return(FALSE);

	if (percentFilled > 100)
		percentFilled = 100;
	if (percentFilled < 0)
		percentFilled = 0;
	ProgressFilled = percentFilled;
	GetPort(&oldPort);
	SetPortWindowPort(win);
	GetPenState(&oldPen);
	GetForeColor(&oldForeColor);

	PenNormal(); /* Reset the drawing pen */

	GetControlBounds(GetWindowItemAsControl(win, 1), &box);

	RGBForeColor(&rgbBlack);/* Frame progress bar */
	FrameRect(&box);

	InsetRect(&box, 1, 1); /* Setup rect for amount filled */
	fBox = box;
	pixelsDone = (box.right - box.left) * percentFilled / 100;
	fBox.right = fBox.left + pixelsDone;

	RGBForeColor(&rgbBarColor); /* Draw full part of the bar */
	PaintRect(&fBox);
	
	SetPenState(&oldPen);
	RGBForeColor(&oldForeColor);
	SetPort(oldPort);
	if (NonModal)
		RealMainEvent(&t);
	return(TRUE);
}
/* progress bar even end */

void ProgExit(const char *err) {	
	if (err != NULL)
		do_warning(err, 0);
	CloseAllWindows();
	exit(1999);
}

void Quit_CED(int num) {
	MACWINDOWS *t;
	extern MACWINDOWS *RootWind;

	mem_error = TRUE;
	if (MEMPROT)
		free(MEMPROT);
	if (num != 1999)
		do_warning("Pausing for you to read the message in \"CLAN OUTPUT\" window, CLAN PROGRAM WILL QUIT.", 0);
	while (RootWind) {
		global_df = RootWind->windRec->FileInfo;
		if (global_df) {
			if (global_df->winID == 1962 && global_df->DataChanged == '\001')
				SaveCKPFile(FALSE);
		}
		t = RootWind;
		RootWind = RootWind->nextWind;
		setCursorPos(t->wind);
		global_df = t->windRec->FileInfo;
		SetPBCglobal_df(true, 0L);
		FreeFileInfo(t->windRec->FileInfo);
		if (t->windRec->VScrollHnd != NULL)
			DisposeControl(t->windRec->VScrollHnd);
		if (t->windRec->HScrollHnd != NULL)
			DisposeControl(t->windRec->HScrollHnd);
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
		ChangeWindowsMenuItem(t->windRec->wname, FALSE);
		if (t->windRec->CloseProc)
			t->windRec->CloseProc(t->wind);
		DisposeWindow(t->wind);
		free(t->windRec);
		free(t);
	}
	global_df = NULL;
	KeyScript(mScript);
	if (isMovieAvialable)
		ExitMovies();

	RemoveAEProcs();
	SelectedFiles = freeSelectedFiles(SelectedFiles);
	ExitToShell();
}

/* locate program save its path and launch it.
*/
/* launch app by path; 
static char LaunchAppFromPath(FSSpec *spec, char *mess) {
	OSErr	anErr;

	if (spec->name[0] == 0) {
		return(-1);
	}

	LaunchParamBlockRec LaunchParams;
	LaunchParams.launchBlockID = extendedBlock;
	LaunchParams.launchEPBLength = extendedBlockLen;
	LaunchParams.launchControlFlags = launchContinue | launchNoFileFlags;
	LaunchParams.launchAppParameters = nil;
	LaunchParams.launchAppSpec = spec;
	anErr = LaunchApplication(&LaunchParams);
	if (anErr == opWrErr) {
		ProcessInfoRec info;
		LaunchParams.launchProcessSN.highLongOfPSN = 0L;
		LaunchParams.launchProcessSN.lowLongOfPSN  = 0L;
		info.processInfoLength = sizeof(info);
		info.processName = templine;
		info.processAppSpec = nil;
		do {
			anErr = GetNextProcess(&LaunchParams.launchProcessSN);
			GetProcessInformation(&LaunchParams.launchProcessSN, &info);
			if (strncmp(info.processName+1, spec.name, info.processName[0]) == 0)
				break;
		} while (anErr != procNotFound) ;
		
		if (anErr == noErr)
			anErr = SetFrontProcess(&LaunchParams.launchProcessSN);
		else
			return(FALSE);
	} else if (anErr != noErr) {
		return(FALSE);
	}
	return(TRUE);
}

#define SPELLMESSAGE "\pPlease locate spell checker application."
	res = LaunchAppFromPath(&spellCheck, SPELLMESSAGE);
	if (res == 0) {
		spellCheck.name[0] = EOS;
		return(LaunchAppFromPath(&spellCheck, SPELLMESSAGE));
	} else if (res == -1)
		return(FALSE);
	else
		return(TRUE);


static char LaunchAppFromPath(FSSpec *spec, char *mess) {
	OSErr	anErr;

	if (spec->name[0] == 0)
		return(FALSE);

	LaunchParamBlockRec LaunchParams;
	LaunchParams.launchBlockID = extendedBlock;
	LaunchParams.launchEPBLength = extendedBlockLen;
	LaunchParams.launchControlFlags = launchContinue | launchNoFileFlags;
	LaunchParams.launchAppParameters = nil;
	LaunchParams.launchAppSpec = spec;
	anErr = LaunchApplication(&LaunchParams);
	if (anErr == opWrErr) {
		ProcessInfoRec info;
		LaunchParams.launchProcessSN.highLongOfPSN = 0L;
		LaunchParams.launchProcessSN.lowLongOfPSN  = 0L;
		info.processInfoLength = sizeof(info);
		info.processName = templine;
		info.processAppSpec = nil;
		do {
			anErr = GetNextProcess(&LaunchParams.launchProcessSN);
			GetProcessInformation(&LaunchParams.launchProcessSN, &info);
			if (strncmp(info.processName+1, spec->name, info.processName[0]) == 0)
				break;
		} while (anErr != procNotFound) ;
		
		if (anErr == noErr)
			anErr = SetFrontProcess(&LaunchParams.launchProcessSN);
		else
			return(FALSE);
	} else if (anErr != noErr) {
		return(FALSE);
	}
	return(TRUE);
}
*/
/*
extern char LaunchAppFromSignature(OSType sig, char isTest);
char LaunchAppFromSignature(OSType sig, char isTest) {
	long	t;
	short	vRefNum;
	FSSpec	spec;
	OSErr	anErr;
	DTPBRec	paramBlock;
	ProcessInfoRec info;
	LaunchParamBlockRec LaunchParams;

	if (!isTest) {
		LaunchParams.launchProcessSN.highLongOfPSN = 0L;
		LaunchParams.launchProcessSN.lowLongOfPSN  = 0L;
		info.processInfoLength = sizeof(info);
		info.processName = templine;
		info.processAppSpec = nil;
		do {
			anErr = GetNextProcess(&LaunchParams.launchProcessSN);
			GetProcessInformation(&LaunchParams.launchProcessSN, &info);
			if (info.processSignature == sig)
				break;
		} while (anErr == noErr) ;
		if (anErr == noErr) {
			SetFrontProcess(&LaunchParams.launchProcessSN);
			return(TRUE);
		}
	}
	if (FindFolder(kOnSystemDisk,kDesktopFolderType,kDontCreateFolder,&vRefNum,&t) != noErr)
		return(FALSE);

	templine[0] = EOS;
	paramBlock.ioNamePtr = templine;
	paramBlock.ioVRefNum = vRefNum;
	if (PBDTGetPath(&paramBlock) != noErr)
		return(FALSE);

	paramBlock.ioFileCreator = sig;
	paramBlock.ioIndex = 0;
	paramBlock.ioCompletion = nil;
	if (PBDTGetAPPL(&paramBlock, FALSE) != noErr)
		return(FALSE);

	if (isTest)
		return(TRUE);

	Pstrcpy(spec.name, paramBlock.ioNamePtr, 63);
	spec.vRefNum = vRefNum;
	spec.parID = paramBlock.ioAPPLParID;
	LaunchParams.launchBlockID = extendedBlock;
	LaunchParams.launchEPBLength = extendedBlockLen;
	LaunchParams.launchControlFlags = launchContinue | launchNoFileFlags;
	LaunchParams.launchAppParameters = nil;
	LaunchParams.launchAppSpec = &spec;
	if (LaunchApplication(&LaunchParams) != noErr)
		return(FALSE);
	return(TRUE);
}
*/
