#include "ced.h"
#include "MMedia.h"
#include "mac_MUPControl.h"
#include "search.h"
#include "ids.h"
#include "my_ctype.h"
#include "mac_dial.h"

#define usrData struct userWindowData
struct userWindowData {
	WindowPtr window;
	short res;
};

static char  ClipFreqWhich = 3;
static short ClipFreqCnt = 1;
static short ClipFreqPrecent = 100;

char dial_isF1Pressed = FALSE, dial_isF2Pressed = FALSE;

WindowPtr getNibWindow(CFStringRef kNibName) {
	OSErr err;
	WindowPtr theWindow;
	IBNibRef mNibs;
	
	err = CreateNibReference(CFSTR("CLAN"), &mNibs);
	if (err != noErr)
		theWindow = NULL;
	else {
		err = CreateWindowFromNib(mNibs, kNibName, &theWindow);
		if (err != noErr)
			theWindow = NULL;
		DisposeNibReference(mNibs);
	}
	return(theWindow);
}

void SetWindowUnicodeTextValue(ControlRef iCtrl, char isClearAll, unCH *str, wchar_t c) {
	wchar_t		tstr[2];
	CFIndex		len;
	CFStringRef	theStr = NULL;
	ControlEditTextSelectionRec selection;

	if (str == NULL) {
		tstr[0] = c;
		tstr[1] = EOS;
		str = tstr;
	}
	len = strlen(str);
	if (len > 0 || isClearAll) {
		if (isClearAll) {
			theStr = CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar *)str, strlen(str));
			SetControlData(iCtrl, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &theStr);
			CFRelease(theStr);
			theStr = NULL;
		} else {
			GetControlData(iCtrl, 0, kControlEditTextSelectionTag, sizeof(selection), (Ptr)&selection, NULL);
			GetControlData(iCtrl, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &theStr, NULL);
			len = CFStringGetLength(theStr);
			if (len >= UTTLINELEN+UTTLINELEN)
				len = UTTLINELEN+UTTLINELEN;
			CFStringGetCharacters(theStr, CFRangeMake(0, len), (UniChar *)templineW);
			templineW[len] = 0;
			CFRelease(theStr);
			if (selection.selStart >= len)
				selection.selStart = len;
			if (selection.selEnd >= len)
				selection.selEnd = len;
			if (selection.selStart < selection.selEnd) {
				strcpy(templineW+selection.selStart, templineW+selection.selEnd);
				selection.selEnd = selection.selStart;
			}
			uS.shiftright(templineW+selection.selStart, strlen(str));
			for (selection.selEnd=0; str[selection.selEnd] != EOS; selection.selEnd++)
				templineW[selection.selStart++] = str[selection.selEnd];
			theStr = CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar *)templineW, strlen(templineW));
			SetControlData(iCtrl, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &theStr);
			CFRelease(theStr);
			theStr = NULL;				
			selection.selEnd = selection.selStart;
			SetControlData(iCtrl, 0, kControlEditTextSelectionTag, sizeof( selection ), (Ptr)&selection);
		}
	}
}

void GetWindowUnicodeTextValue(ControlRef iCtrl,  unCH *str, int lenMax) {
	CFIndex		len;
	CFStringRef	theStr = NULL;

	str[0] = EOS;
	GetControlData(iCtrl, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &theStr, NULL);
	len = CFStringGetLength(theStr);
	if (len >= lenMax-1)
		len = lenMax-2;
	CFStringGetCharacters(theStr, CFRangeMake(0, len), (UniChar *)str);
	str[len] = 0;
	CFRelease(theStr);
}

static void GetWindowTextValue(WindowPtr myDlg, SInt32 id, char *text, Size maxLen) {
	ControlRef	itemCtrl;
	HIViewID	viewID;	
	Size		outSize;
	
	viewID.signature = 0;
	viewID.id = id;
	HIViewFindByID(HIViewGetRoot(myDlg), viewID, &itemCtrl);
	GetControlData(itemCtrl, kControlEntireControl, kControlEditTextTextTag, maxLen, text, &outSize);
	text[outSize] = 0;
}

char GetWindowBoolValue(WindowPtr myDlg, SInt32 id) {
	ControlRef	itemCtrl;
	HIViewID	viewID;	
	SInt32		enable;
	
	viewID.signature = 0;
	viewID.id = id;
	HIViewFindByID(HIViewGetRoot(myDlg), viewID, &itemCtrl);
	enable = GetControl32BitValue(itemCtrl);
	return((char)(enable != 0L));
}

void SelectWindowItemText(WindowPtr myDlg, SInt32 id, SInt16 beg, SInt16 end) {
	ControlRef	itemCtrl;
	HIViewID	viewID;
	ControlEditTextSelectionRec selSize;
	
	viewID.signature = 0;
	viewID.id = id;
	HIViewFindByID(HIViewGetRoot(myDlg), viewID, &itemCtrl);
	selSize.selStart = beg;
	selSize.selEnd = end;
	SetControlData(itemCtrl, kControlEntireControl, kControlEditTextSelectionTag, sizeof(selSize), &selSize);
}

ControlRef GetWindowItemAsControl(WindowPtr myDlg, SInt32 id) {
	HIViewID viewID;
	ControlRef itemCtrl;

	viewID.signature = 0;
	viewID.id = id;
	HIViewFindByID(HIViewGetRoot(myDlg), viewID, &itemCtrl);
	return(itemCtrl);
}

void ControlCTRL(WindowPtr win, SInt32 id, short toDo, ControlPartCode val) {
	ControlRef	itemCtrl;
	HIViewID	viewID;

	viewID.signature = 0;
	viewID.id = id;
	HIViewFindByID(HIViewGetRoot(win), viewID, &itemCtrl);
	if (toDo == HideCtrl)
		HideControl(itemCtrl);
	else if (toDo == ShowCtrl)
		ShowControl(itemCtrl);
	else if (toDo == HiliteCtrl)
		HiliteControl(itemCtrl, val);
}

static pascal ControlKeyFilterResult myNumFilter(ControlRef theControl, SInt16 *keyCode, SInt16 *charCode, EventModifiers *modifiers)
{
	// the edit text control can filter keys on his own
	if (!(*modifiers & cmdKey)) {
		if (*charCode >= '0' && *charCode <= '9') 
			return kControlKeyFilterPassKey;
		else if (*charCode == 0x7f || *charCode == 0x8 || *charCode == 0x1c || *charCode == 0x1d ||
				 *charCode == 0x1e || *charCode == 0x1f)
			return kControlKeyFilterPassKey;

		SysBeep(1);
		return kControlKeyFilterBlockKey;
	}
	return kControlKeyFilterPassKey;
}

pascal ControlKeyFilterResult myStringFilter(ControlRef iCtrl, SInt16 *keyCode, SInt16 *charCode, EventModifiers *modifiers)
{
	int			key;
	wchar_t		res;
	CFIndex		len;
	CFStringRef	theStr = NULL;
	ControlEditTextSelectionRec selection;

	if (!(*modifiers & cmdKey)) {
		key = *keyCode;
		if ((key == 0x78 || key == 0x7A || key == 0x7B || key == 0x7C || key == 0x7D || key == 0x7E) && *charCode < 32)
			key = key << 8;
		else
			key = *charCode;

		if (key == 0x7A00) {
			dial_isF1Pressed = TRUE;
			dial_isF2Pressed = FALSE;
			return kControlKeyFilterBlockKey;
		} else if (key == 0x7800) {
			dial_isF2Pressed = TRUE;
			dial_isF1Pressed = FALSE;
			return kControlKeyFilterBlockKey;
		} else if (dial_isF1Pressed || dial_isF2Pressed) {
			if (dial_isF1Pressed)
				res = Char2SpChar(key, 1);
			else if (dial_isF2Pressed)
				res = Char2SpChar(key, 2);
			dial_isF1Pressed = FALSE;
			dial_isF2Pressed = FALSE;
			if (res != 0) {
				GetControlData(iCtrl, 0, kControlEditTextSelectionTag, sizeof(selection), (Ptr)&selection, NULL);
				GetControlData(iCtrl, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &theStr, NULL);
				len = CFStringGetLength(theStr);
				if (len >= UTTLINELEN+UTTLINELEN)
					len = UTTLINELEN+UTTLINELEN;
				CFStringGetCharacters(theStr, CFRangeMake(0, len), (UniChar *)templineW);
				templineW[len] = 0;
				CFRelease(theStr);
				if (selection.selStart >= len)
					selection.selStart = len;
				if (selection.selEnd >= len)
					selection.selEnd = len;
				if (selection.selStart < selection.selEnd) {
					strcpy(templineW+selection.selStart, templineW+selection.selEnd);
					selection.selEnd = selection.selStart;
				}
				uS.shiftright(templineW+selection.selStart, 1);
				templineW[selection.selStart++] = res;
				theStr = CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar *)templineW, strlen(templineW));
				SetControlData(iCtrl, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &theStr);
				CFRelease(theStr);
				theStr = NULL;				
				selection.selEnd = selection.selStart;
				SetControlData(iCtrl, 0, kControlEditTextSelectionTag, sizeof( selection ), (Ptr)&selection);
				return kControlKeyFilterBlockKey;
			}
		}
	}
	return kControlKeyFilterPassKey;
}

void showWindow(WindowPtr win) {
	DrawMouseCursor(1);
	ShowWindow(win);
	BringToFront(win);
	SetPortWindowPort(win);
}

static pascal OSStatus AboutCLANEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
		case kHICommandCancel:
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
	}
	
	return status;
}

void AboutCLAN(char isNear) {
	WindowPtr	myDlg;
	usrData		userData;
	extern char VERSION[];
	
	myDlg = getNibWindow(CFSTR("AboutCLAN"));
	if (isNear)
		CenterWindow(myDlg, -3, -3);
//	CenterWindow(myDlg, -2, -2);
	showWindow(myDlg);
	SetDialogItemUTF8(VERSION, myDlg, 3, FALSE);
	userData.window = myDlg;
	userData.res = 0;

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), AboutCLANEvents, 1, &eventTypeCP, &userData, NULL);
	RunAppModalLoopForWindow(myDlg);
	 
	DisposeWindow(myDlg);
}

static pascal OSStatus QuitDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
		case 'MYNO':
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myNO;
			break;
	}
	
	return status;
}

char QuitDialog(FNType *st) {
	WindowPtr	myDlg;
	usrData		userData;
	char		res;
	
	myDlg = getNibWindow(CFSTR("Quit Dialog"));
	CenterWindow(myDlg, -1, -1);
	showWindow(myDlg);
	SetDialogItemUTF8(st, myDlg, 4, FALSE);

	userData.window = myDlg;
	userData.res = 0;

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), QuitDialogEvents, 1, &eventTypeCP, &userData, NULL);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myNO) {
		res = '3';
	} else {
		res = 'n';
	}

	if (userData.res == myOK) {
		SaveUndoState(FALSE);
		global_df->LastCommand = SaveCurrentFile(0);
		FinishMainLoop();
		if (global_df->DataChanged)
			res = 'n';
		else
			res = 'y';
	}

	DisposeWindow(myDlg);
	return(res);
}

static pascal OSStatus UpdateCLANDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
		case 'SKIP':
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myNO;
			break;
		case 'SUPD':
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = 2011;
			break;
	}
	
	return status;
}

char UpdateCLANDialog(FNType *st) {
	WindowPtr	myDlg;
	usrData		userData;
	char		res;
	
	myDlg = getNibWindow(CFSTR("Update CLAN"));
	CenterWindow(myDlg, -1, -1);
	showWindow(myDlg);
	SetDialogItemUTF8(st, myDlg, 4, FALSE);
	
	userData.window = myDlg;
	userData.res = 0;
	
	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), UpdateCLANDialogEvents, 1, &eventTypeCP, &userData, NULL);
	RunAppModalLoopForWindow(myDlg);
	
	if (userData.res == myOK) {
		res = 'y';
	} else if (userData.res == myCANCEL) {
		res = 'l';
	} else if (userData.res == 2011) {
		res = 'd';
	} else {
		res = 'n';
	}

	DisposeWindow(myDlg);
	return(res);
}

static pascal OSStatus SpeedDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	HIViewRef itemCtrl;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandOK:
					// we got a valid click on the OK button so let's quit our local run loop
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myOK;
					break;
				case kHICommandCancel:
					// we got a valid click on the OK button so let's quit our local run loop
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myCANCEL;
					break;
				case 'ID04':
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 4, 1);
					SetDialogItemValue(userData->window, 5, 0);
					SetDialogItemValue(userData->window, 6, 0);
					SetDialogItemValue(userData->window, 7, 0);
					SetDialogItemValue(userData->window, 8, 0);
					break;
				case 'ID05':
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 5, 1);
					SetDialogItemValue(userData->window, 4, 0);
					SetDialogItemValue(userData->window, 6, 0);
					SetDialogItemValue(userData->window, 7, 0);
					SetDialogItemValue(userData->window, 8, 0);
					break;
				case 'ID06':
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 6, 1);
					SetDialogItemValue(userData->window, 4, 0);
					SetDialogItemValue(userData->window, 5, 0);
					SetDialogItemValue(userData->window, 7, 0);
					SetDialogItemValue(userData->window, 8, 0);
					break;
				case 'ID07':
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 7, 1);
					SetDialogItemValue(userData->window, 4, 0);
					SetDialogItemValue(userData->window, 5, 0);
					SetDialogItemValue(userData->window, 6, 0);
					SetDialogItemValue(userData->window, 8, 0);
					break;
				case 'ID08':
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 8, 1);
					SetDialogItemValue(userData->window, 4, 0);
					SetDialogItemValue(userData->window, 5, 0);
					SetDialogItemValue(userData->window, 6, 0);
					SetDialogItemValue(userData->window, 7, 0);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return status;
}

void SpeedDialog(void) {
	int isSpeedChanged;
	WindowPtr	myDlg;
	usrData		userData;
	PrepareStruct	saveRec;

	myDlg = getNibWindow(CFSTR("speed_select"));
	CenterWindow(myDlg, -3, -3);
	userData.window = myDlg;
	userData.res = 0;
	isSpeedChanged = streamSpeedNumber;
	if (streamSpeedNumber == 0) {
    	SetDialogItemValue(myDlg, 4, 1);
    	SetDialogItemValue(myDlg, 5, 0);
    	SetDialogItemValue(myDlg, 6, 0);
    	SetDialogItemValue(myDlg, 7, 0);
    	SetDialogItemValue(myDlg, 8, 0);
	} else if (streamSpeedNumber == 1) {
    	SetDialogItemValue(myDlg, 4, 0);
    	SetDialogItemValue(myDlg, 5, 1);
    	SetDialogItemValue(myDlg, 6, 0);
    	SetDialogItemValue(myDlg, 7, 0);
    	SetDialogItemValue(myDlg, 8, 0);
	} else if (streamSpeedNumber == 2) {
    	SetDialogItemValue(myDlg, 4, 0);
    	SetDialogItemValue(myDlg, 5, 0);
    	SetDialogItemValue(myDlg, 6, 1);
    	SetDialogItemValue(myDlg, 7, 0);
    	SetDialogItemValue(myDlg, 8, 0);
	} else if (streamSpeedNumber == 3) {
    	SetDialogItemValue(myDlg, 4, 0);
    	SetDialogItemValue(myDlg, 5, 0);
    	SetDialogItemValue(myDlg, 6, 0);
    	SetDialogItemValue(myDlg, 7, 1);
    	SetDialogItemValue(myDlg, 8, 0);
	} else if (streamSpeedNumber == 4) {
    	SetDialogItemValue(myDlg, 4, 0);
    	SetDialogItemValue(myDlg, 5, 0);
    	SetDialogItemValue(myDlg, 6, 0);
    	SetDialogItemValue(myDlg, 7, 0);
    	SetDialogItemValue(myDlg, 8, 1);
	}
	EventTypeSpec eventTypeCP[] = {
		{kEventClassCommand, kEventCommandProcess}
	} ;
	InstallEventHandler(GetWindowEventTarget(myDlg), SpeedDialogEvents, 1, eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);
	if (userData.res == myOK) {
		if (GetWindowBoolValue(myDlg, 4) > 0) {
			streamSpeedNumber = 0;
		} else if (GetWindowBoolValue(myDlg, 5) > 0) {
			streamSpeedNumber = 1;
		} else if (GetWindowBoolValue(myDlg, 6) > 0) {
			streamSpeedNumber = 2;
		} else if (GetWindowBoolValue(myDlg, 7) > 0) {
			streamSpeedNumber = 3;
		} else if (GetWindowBoolValue(myDlg, 8) > 0) {
			streamSpeedNumber = 4;
		}
	}
	DisposeWindow(myDlg);
	if (isSpeedChanged != streamSpeedNumber) {
	    WriteCedPreference();
		if (theMovie != NULL) {
			PrepareWindA4(theMovie->win, &saveRec);
			mCloseWindow(theMovie->win);
			RestoreWindA4(&saveRec);
		}
	}
}

static pascal OSStatus QueryDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
		case 'MYNO':
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myNO;
			break;
	}
	
	return status;
}

int QueryDialog(const char *st, short id) {
	int			res;
	WindowPtr	myDlg;
	usrData		userData;
	
	if (id == 140)
		myDlg = getNibWindow(CFSTR("yes/cancel/no"));
	else if (id == 147)
		myDlg = getNibWindow(CFSTR("yes/no"));
	else
		return(0);
		
	CenterWindow(myDlg, -1, -1);
	showWindow(myDlg);

	userData.window = myDlg;
	userData.res = 0;

	SetDialogItemUTF8(st, myDlg, 4, FALSE);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), QueryDialogEvents, 1, &eventTypeCP, &userData, NULL);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myNO) {
		res = -1;
	} else if (userData.res == myCANCEL) {
		res = 0;
	} else
		res = 1;

	DisposeWindow(myDlg);
	return(res);
}

static pascal OSStatus CEDOptionsEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
//	FNType		*sfFile[FNSize];
//	OSType		typeList[1];
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;

	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
/*
		case 'CDFI':
			typeList[0] = 'TEXT';
			sfFile[0] = EOS;
			if (myNavGetFile(nil, -1, typeList, nil, sfFile)) {
				strcpy(CodesFName, sfFile);
				SetDialogItemUTF8(CodesFName, userData->window, 7, TRUE);
			} else
				SetDialogItemUTF8("No file selected", userData->window, 7, FALSE);
			break;
		case 'KEFI':
			typeList[0] = 'TEXT';
			sfFile[0] = EOS;
			if (myNavGetFile(nil, -1, typeList, nil, sfFile)) {
				strcpy(KeysFName, sfFile);
				SetDialogItemUTF8(KeysFName, userData->window, 9, TRUE);
			} else
				SetDialogItemUTF8("No file selected", userData->window, 9, FALSE);
			break;
*/
	}
	
	return status;
}


void DoCEDOptions(void) {
	char		text[256];
	char		doReWrapChanged;
	char		oldDoMixedSTWave;
	usrData		userData;
	WindowPtr	myDlg;
	ControlRef	iCtrl;
	extern char DefClan;
	extern char DefWindowDims;
	extern char ClanAutoWrap;
	extern char isUpdateCLAN;
	extern char isCursorPosRestore;
	extern long ClanWinRowLim;

	myDlg = getNibWindow(CFSTR("CED options"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

	uS.sprintf(templineW, cl_T("%d"), FreqCountLimit);
	iCtrl = GetWindowItemAsControl(myDlg, 5);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SelectWindowItemText(myDlg, 5, 0, HUGE_INTEGER);
/*
	if (strcmp(CODEFNAME, CODES_FILE)) {
		strcpy(CodesFName, CODEFNAME);
		SetDialogItemUTF8(CodesFName, myDlg, 7, TRUE);					// -c
	} else {
		*CodesFName = EOS;
		SetDialogItemUTF8("No file selected", myDlg, 7, TRUE);			// -c
	}
	if (strcmp(STATEFNAME, KEYS_BIND_FILE)) {
		strcpy(KeysFName, STATEFNAME);
		SetDialogItemUTF8(KeysFName, myDlg, 9, TRUE);					// -k
	} else {
		*KeysFName = EOS;
		SetDialogItemUTF8("No file selected", myDlg, 9, TRUE);			// -k
	}
*/
	SetDialogItemValue(myDlg, 10, !MakeBackupFile);		/* -d */
	SetDialogItemValue(myDlg, 11, !StartInEditorMode);	/* -e */
//	SetDialogItemValue(myDlg, 12, ShowPercentOfFile);		/* -p */
	SetDialogItemValue(myDlg, 17, DefAutoWrap);			/* -a */
	SetDialogItemValue(myDlg, 19, DefClan);
	SetDialogItemValue(myDlg, 20, DefWindowDims);
	SetDialogItemValue(myDlg, 23, ClanAutoWrap);
	iCtrl = GetWindowItemAsControl(myDlg, 25);
	u_strcpy(templineW, DisTier, UTTLINELEN);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
//	SetDialogItemValue(myDlg, 26, IsGSet(1));				/* -g1 */
//	SetDialogItemValue(myDlg, 27, IsGSet(3));				/* -g3 */
//	SetDialogItemValue(myDlg, 28, IsGSet(2));				/* -g3 */
	SetDialogItemValue(myDlg, 29, isCursorPosRestore);
//	SetDialogItemValue(myDlg, 30, doReWrap);
	SetDialogItemValue(myDlg, 31, doMixedSTWave);
//	SetDialogItemValue(myDlg, 32, rawTextInput);
	SetDialogItemValue(myDlg, 33, isUnixCRs);
//	SetDialogItemValue(myDlg, 34, isUTFData);
//	SetDialogItemValue(myDlg, 35, isUseSPCKeyShortcuts);
	SetDialogItemValue(myDlg, 36, isUpdateCLAN);
	
	oldDoMixedSTWave = doMixedSTWave;
	doReWrapChanged = doReWrap;

	uS.sprintf(templineW, cl_T("%d"), ClanWinRowLim);
	iCtrl = GetWindowItemAsControl(myDlg, 21);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);

//    if (DefChatMode == 2) SetDialogItemValue(myDlg, 16, 1);	/* no +/- w */
//    else if (DefChatMode) SetDialogItemValue(myDlg, 14, 1);	/* +w */
//    else SetDialogItemValue(myDlg, 15, 1);					/* -w */
	AdvanceKeyboardFocus(myDlg);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), CEDOptionsEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);


	if (userData.res == myOK) {
		GetWindowTextValue(myDlg, 5, text, 255);
		if (*text)
			FreqCountLimit = atoi(text);

//		GetDialogItem(myDlg, 7, &type, &hdl, &box);

//		GetDialogItem(myDlg, 9, &type, &hdl, &box);

		MakeBackupFile = !GetWindowBoolValue(myDlg, 10);

		StartInEditorMode = !GetWindowBoolValue(myDlg, 11);

		DefAutoWrap = GetWindowBoolValue(myDlg, 17);
		if (global_df != NULL)
			global_df->AutoWrap = DefAutoWrap;

		DefClan = GetWindowBoolValue(myDlg, 19);

		DefWindowDims = GetWindowBoolValue(myDlg, 20);

		GetWindowTextValue(myDlg, 21, text, 255);
		if (*text) {
			WindowPtr	wind;	
			WindowProcRec *rec;	

			ClanWinRowLim = atol(text);
			if (ClanWinRowLim != 0L && ClanWinRowLim < 10L)
				ClanWinRowLim = 10L;

			if ((wind=FindAWindowNamed(CLAN_Output_str)) != NULL) {
				if ((rec=WindowProcs(wind)) != NULL) {
					if (rec->FileInfo != NULL && ClanWinRowLim > rec->FileInfo->RowLimit)
						rec->FileInfo->RowLimit = ClanWinRowLim;	
				}
			}
		}
		ClanAutoWrap = GetWindowBoolValue(myDlg, 23);

		isCursorPosRestore = GetWindowBoolValue(myDlg, 29);

		isUnixCRs = GetWindowBoolValue(myDlg, 33);

		doReWrapChanged = ((doReWrapChanged != doReWrap) && doReWrap);
		SetTextWinMenus(TRUE);

		doMixedSTWave = GetWindowBoolValue(myDlg, 31);

		isUpdateCLAN = GetWindowBoolValue(myDlg, 36);

		GetWindowTextValue(myDlg, 25, text, 255);
		strcpy(DisTier, text);
		uS.uppercasestr(DisTier, &dFnt, C_MBF);

//		SetGOption(1, GetWindowBoolValue(myDlg, 26));
//		SetGOption(3, GetWindowBoolValue(myDlg, 27));
//		SetGOption(2, GetWindowBoolValue(myDlg, 28));
	}
	DisposeWindow(myDlg);

	if (userData.res == myOK) {
	    WriteCedPreference();

	    if (doReWrapChanged && global_df != NULL) {
			Re_WrapLines(AddLineToRow, 0L, TRUE, NULL);
			DisplayTextWindow(NULL, 1);
		}
		FinishMainLoop();
	}

	if (oldDoMixedSTWave != doMixedSTWave && global_df->SnTr.IsSoundOn && global_df != NULL) {
		cChan = -1;
		soundwindow(0);
		global_df->SnTr.IsSoundOn = (char)(soundwindow(1) == 0);
		if (global_df->SnTr.IsSoundOn)
			cChan = global_df->SnTr.SNDchan;
	}

}

static pascal OSStatus DoDefContStringsEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
	}
	
	return status;
}

void DoDefContStrings(void) {
	int			num;
	char		text[512];
	WindowPtr	myDlg;
	usrData		userData;
	ControlRef	iCtrl;
	ControlKeyFilterUPP keyStrFilter = myStringFilter;
	ControlKeyFilterUPP keyNumFilter = myNumFilter;

	myDlg = getNibWindow(CFSTR("Define Macros"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

	for (num=1; num < 10; num++) {
		if (ConstString[num] == NULL)
			break;
	}
	if (num < 10)
		sprintf(text, "%d", num);
	else
		sprintf(text, "%d", 0);

	iCtrl = GetWindowItemAsControl(myDlg, 5);
	u_strcpy(templineW, text, UTTLINELEN);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SelectWindowItemText(myDlg, 5, 0, HUGE_INTEGER);
	SetControlData(GetWindowItemAsControl(myDlg, 5), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyNumFilter), &keyNumFilter);

	iCtrl = GetWindowItemAsControl(myDlg, 7);
	if (num >= 10 && ConstString[0] != NULL) {
		u_strcpy(templineW, ConstString[0], UTTLINELEN);
		SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
		SelectWindowItemText(myDlg, 7, 0, HUGE_INTEGER);
	} else {
		u_strcpy(templineW, "", UTTLINELEN);
		SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	}
	SetControlData(iCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	AdvanceKeyboardFocus(myDlg);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoDefContStringsEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowTextValue(myDlg, 5, text, 511);
		if (*text) {
			num = atoi(text);
			if (*text == '0')
				num = 10;
		} else
			num = 0;
		if (num > 0 && num <= 10) {
			GetWindowUnicodeTextValue(iCtrl, templineW, 512);
			u_strcpy(text, templineW, 511);
			if (num == 10)
				AllocConstString(text, 0);
			else
				AllocConstString(text, num);
		}
	}
	DisposeWindow(myDlg);
}

static pascal OSStatus DoConstantStringEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	UInt32 resEventKind;


	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandCancel:
					// we got a valid click on the OK button so let's quit our local run loop
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myCANCEL;
					break;
				case 'ID13':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 1;
					break;
				case 'ID14':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 2;
					break;
				case 'ID15':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 3;
					break;
				case 'ID16':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 4;
					break;
				case 'ID17':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 5;
					break;
				case 'ID18':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 6;
					break;
				case 'ID19':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 7;
					break;
				case 'ID20':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 8;
					break;
				case 'ID21':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 9;
					break;
				case 'ID22':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = 0;
					break;
				case 'swin':
					break;
				default:
					break;
			}
			break;
		case kEventClassKeyboard:
			switch ((resEventKind=GetEventKind(inEvent))) {
				case kEventRawKeyDown:
				{
					char c;
					GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(c), NULL, &c);
					if (c == '1') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 1;
					} else if (c == '2') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 2;
					} else if (c == '3') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 3;
					} else if (c == '4') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 4;
					} else if (c == '5') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 5;
					} else if (c == '6') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 6;
					} else if (c == '7') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 7;
					} else if (c == '8') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 8;
					} else if (c == '9') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 9;
					} else if (c == '0') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = 0;
					}
					break;
				}
				default:
					break;
			}
			break;
		default:
			break;
	}
	return status;
}

int DoConstantString(void) {
	WindowPtr	myDlg;
	HIViewRef	itemCtrl;
	usrData		userData;
	CFStringRef	theStr = NULL;

	myDlg = getNibWindow(CFSTR("Get Macros"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

	if (ConstString[1] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 13);
		theStr = my_CFStringCreateWithBytes(ConstString[1]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
	}
	if (ConstString[2] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 14);
		theStr = my_CFStringCreateWithBytes(ConstString[2]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
//		SetControlData(itemCtrl, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &theStr);
	}
	if (ConstString[3] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 15);
		theStr = my_CFStringCreateWithBytes(ConstString[3]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
	}
	if (ConstString[4] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 16);
		theStr = my_CFStringCreateWithBytes(ConstString[4]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
	}
	if (ConstString[5] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 17);
		theStr = my_CFStringCreateWithBytes(ConstString[5]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
	}
	if (ConstString[6] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 18);
		theStr = my_CFStringCreateWithBytes(ConstString[6]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
	}
	if (ConstString[7] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 19);
		theStr = my_CFStringCreateWithBytes(ConstString[7]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
	}
	if (ConstString[8] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 20);
		theStr = my_CFStringCreateWithBytes(ConstString[8]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
	}
	if (ConstString[9] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 21);
		theStr = my_CFStringCreateWithBytes(ConstString[9]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
	}
	if (ConstString[0] != NULL) {
		itemCtrl = GetWindowItemAsControl(myDlg, 22);
		theStr = my_CFStringCreateWithBytes(ConstString[0]);
		if (theStr != NULL) {
			HIViewSetText(itemCtrl, theStr);
			CFRelease(theStr);
		}
	}


	EventTypeSpec eventTypeCP[] = {
									{kEventClassKeyboard, kEventRawKeyDown},
									{kEventClassCommand, kEventCommandProcess}
								  } ;
	InstallEventHandler(GetWindowEventTarget(myDlg), DoConstantStringEvents, 2, eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myCANCEL)
		userData.res = -1;

	DisposeWindow(myDlg);
	return(userData.res);
}

static pascal OSStatus DoGetFreqCountEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	HIViewRef itemCtrl;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
		case 'ID05':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl) == 0L)
				SetDialogItemValue(userData->window, 5, 1);
			SetDialogItemValue(userData->window, 6, 0);
			SetDialogItemValue(userData->window, 9, 0);
			SetDialogItemValue(userData->window, 12, 0);
			ControlCTRL(userData->window, 7, HideCtrl, 0);
			ControlCTRL(userData->window, 10, HideCtrl, 0);
			break;
		case 'ID06':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl) == 0L)
				SetDialogItemValue(userData->window, 6, 1);
			SetDialogItemValue(userData->window, 5, 0);
			SetDialogItemValue(userData->window, 9, 0);
			SetDialogItemValue(userData->window, 12, 0);
			ControlCTRL(userData->window, 7, ShowCtrl, 0);
			ControlCTRL(userData->window, 10, HideCtrl, 0);
			break;
		case 'ID09':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl) == 0L)
				SetDialogItemValue(userData->window, 9, 1);
			SetDialogItemValue(userData->window, 5, 0);
			SetDialogItemValue(userData->window, 6, 0);
			SetDialogItemValue(userData->window, 12, 0);
			ControlCTRL(userData->window, 7, HideCtrl, 0);
			ControlCTRL(userData->window, 10, ShowCtrl, 0);
			break;
		case 'ID12':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl) == 0L)
				SetDialogItemValue(userData->window, 12, 1);
			SetDialogItemValue(userData->window, 5, 0);
			SetDialogItemValue(userData->window, 6, 0);
			SetDialogItemValue(userData->window, 9, 0);
			ControlCTRL(userData->window, 7, HideCtrl, 0);
			ControlCTRL(userData->window, 10, HideCtrl, 0);
			break;
	}
	
	return status;
}

long DoGetFreqCount(long num) {
	long		freqCnt;
	WindowPtr	myDlg;
	usrData		userData;
	ControlRef	iCtrl;
	ControlKeyFilterUPP keyNumFilter = myNumFilter;
	
	myDlg = getNibWindow(CFSTR("Get Movie Clips Freq"));
	CenterWindow(myDlg, -1, -1);
	showWindow(myDlg);

	userData.window = myDlg;
	userData.res = 0;

	sprintf(templineC, "%ld", num);
	SetDialogItemUTF8(templineC, myDlg, 3, FALSE);

	uS.sprintf(templineW, cl_T("%d"), ClipFreqCnt);
	iCtrl = GetWindowItemAsControl(myDlg, 7);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SetControlData(iCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyNumFilter), &keyNumFilter);

	uS.sprintf(templineW, cl_T("%d"), ClipFreqCnt);
	iCtrl = GetWindowItemAsControl(myDlg, 10);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SetControlData(iCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyNumFilter), &keyNumFilter);

    if (ClipFreqWhich == 3) {
    	SetDialogItemValue(myDlg, 12, 1);
    	ControlCTRL(myDlg, 7, HideCtrl, 0);
    	ControlCTRL(myDlg, 10, HideCtrl, 0);
    } else if (ClipFreqWhich == 2) {
    	SetDialogItemValue(myDlg, 9, 1);
		SelectWindowItemText(myDlg, 10, 0, HUGE_INTEGER);
    	ControlCTRL(myDlg, 7, HideCtrl, 0);
    	ControlCTRL(myDlg, 10, ShowCtrl, 0);
    } else if (ClipFreqWhich == 1) {
    	SetDialogItemValue(myDlg, 6, 1);
		SelectWindowItemText(myDlg, 7, 0, HUGE_INTEGER);
    	ControlCTRL(myDlg, 7, ShowCtrl, 0);
    	ControlCTRL(myDlg, 10, HideCtrl, 0);
    } else {
    	SetDialogItemValue(myDlg, 5, 1);
    	ControlCTRL(myDlg, 7, HideCtrl, 0);
    	ControlCTRL(myDlg, 10, HideCtrl, 0);
    }

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoGetFreqCountEvents, 1, &eventTypeCP, &userData, NULL);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowTextValue(myDlg, 7, templineC, UTTLINELEN);
		ClipFreqCnt = atoi(templineC);

		GetWindowTextValue(myDlg, 10, templineC, UTTLINELEN);
		ClipFreqPrecent = atoi(templineC);

		if (GetWindowBoolValue(myDlg, 5) > 0) {
			ClipFreqWhich = 0;
			freqCnt = 1L;
		}
		if (GetWindowBoolValue(myDlg, 6) > 0) {
			ClipFreqWhich = 1;
			freqCnt = (long)ClipFreqCnt;
			if (freqCnt < 1L)
				freqCnt = 1L;
		}
		if (GetWindowBoolValue(myDlg, 9) > 0) {
			ClipFreqWhich = 2;
			freqCnt = num / ((num * ClipFreqPrecent) / 100L);
			if (freqCnt < 1L)
				freqCnt = 1L;
		}
		if (GetWindowBoolValue(myDlg, 12) > 0) {
			ClipFreqWhich = 3;
			freqCnt = -1L;
		}
	} else
		freqCnt = 0L;

	DisposeWindow(myDlg);
	return(freqCnt);
}

static pascal OSStatus DoLineNumberEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
	}
	
	return status;
}

long DoLineNumber(long lineno) {
	char		text[256];
	WindowPtr	myDlg;
	ControlRef	iCtrl;
	usrData		userData;

	myDlg = getNibWindow(CFSTR("Line Number"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;
	uS.sprintf(templineW, cl_T("%ld"), lineno);
	iCtrl = GetWindowItemAsControl(myDlg, 4);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SelectWindowItemText(myDlg, 4, 0, HUGE_INTEGER);
	ControlKeyFilterUPP keyNumFilter = myNumFilter;
	SetControlData(GetWindowItemAsControl(myDlg, 4), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyNumFilter), &keyNumFilter);
	AdvanceKeyboardFocus(myDlg);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoLineNumberEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowTextValue(myDlg, 4, text, 255);
		if (*text)
			lineno = atol(text);
	}

	DisposeWindow(myDlg);
	return(lineno);
}

static pascal OSStatus DoURLFileNameEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
	}
	
	return status;
}

char DoURLFileName(char *tName) {
	WindowPtr	myDlg;
	ControlRef	iCtrl;
	usrData		userData;

	myDlg = getNibWindow(CFSTR("get File Name over net"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

	u_strcpy(templineW, tName, UTTLINELEN);
	iCtrl = GetWindowItemAsControl(myDlg, 4);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SelectWindowItemText(myDlg, 4, 0, HUGE_INTEGER);
	AdvanceKeyboardFocus(myDlg);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoURLFileNameEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowTextValue(myDlg, 4, templineC4, 1024);
		if (*templineC4) {
			strcpy(tName, templineC4);
			uS.remFrontAndBackBlanks(tName);
		}
	}

	DisposeWindow(myDlg);
	return(userData.res == myOK);
}

static pascal OSStatus setTabSizeEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
	}
	
	return status;
}

void setTabSize(void) {
	char		text[256];
	WindowPtr	myDlg;
	ControlRef	iCtrl;
	usrData		userData;

	myDlg = getNibWindow(CFSTR("Tab Size"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

	uS.sprintf(templineW, cl_T("%d"), TabSize);
	iCtrl = GetWindowItemAsControl(myDlg, 4);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SelectWindowItemText(myDlg, 4, 0, HUGE_INTEGER);
	ControlKeyFilterUPP keyNumFilter = myNumFilter;
	SetControlData(GetWindowItemAsControl(myDlg, 4), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyNumFilter), &keyNumFilter);
	AdvanceKeyboardFocus(myDlg);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), setTabSizeEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowTextValue(myDlg, 4, text, 255);
		if (*text) {
			TabSize = atoi(text);
			if (TabSize <= 0 || TabSize > 40)
				TabSize = 8;
			WriteCedPreference();
			RefreshAllTextWindows(FALSE);
		}
	}

	DisposeWindow(myDlg);
}

static pascal OSStatus DoStringEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
	}
	return status;
}

void DoString(unCH *code) {
	WindowPtr	myDlg;
	ControlRef	CodeCtrl;
	usrData		userData;
	ControlKeyFilterUPP keyStrFilter = myStringFilter;

	myDlg = getNibWindow(CFSTR("Generic String"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;
	dial_isF1Pressed = FALSE;
	dial_isF2Pressed = FALSE;

	CodeCtrl = GetWindowItemAsControl(myDlg, 3);
	SetControlData(CodeCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	SetWindowUnicodeTextValue(CodeCtrl, TRUE, code, 0);
	if (*code)
		SelectWindowItemText(myDlg, 3, 0, HUGE_INTEGER);

	AdvanceKeyboardFocus(myDlg);
	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoStringEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowUnicodeTextValue(CodeCtrl, templine2, SPEAKERLEN);
		if (*templine2)
			strcpy(code, templine2);
	}

	DisposeWindow(myDlg);
}

static pascal OSStatus DoSndAnaOptionsEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	HIViewRef itemCtrl;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
		case 'PRAA':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl))
				SetDialogItemValue(userData->window, 4, 0);
			else {
				SetDialogItemValue(userData->window, 3, 1);
				SetDialogItemValue(userData->window, 4, 0);
			}
			break;
		case 'PITC':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl))
				SetDialogItemValue(userData->window, 3, 0);
			else {
				SetDialogItemValue(userData->window, 4, 1);
				SetDialogItemValue(userData->window, 3, 0);
			}
			break;
	}
	
	return status;
}

void DoSndAnaOptions(void) {
	WindowPtr	myDlg;
	usrData		userData;

	myDlg = getNibWindow(CFSTR("Select sound analyzer"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

    if (sendMessageTargetApp == PRAAT)
    	SetDialogItemValue(myDlg, 3, 1);
    else if (sendMessageTargetApp == PITCHWORKS)
    	SetDialogItemValue(myDlg, 4, 1);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoSndAnaOptionsEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		if (GetWindowBoolValue(myDlg, 3) > 0)
			sendMessageTargetApp = PRAAT;
		if (GetWindowBoolValue(myDlg, 4) > 0)
			sendMessageTargetApp = PITCHWORKS;
		WriteCedPreference();
	}

	DisposeWindow(myDlg);
}

static pascal OSStatus DoSndF5OptionsEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	HIViewRef itemCtrl;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
		case 'LINE':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl))
				SetDialogItemValue(userData->window, 4, 0);
			else {
				SetDialogItemValue(userData->window, 3, 1);
				SetDialogItemValue(userData->window, 4, 0);
			}
			break;
		case 'TIER':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl))
				SetDialogItemValue(userData->window, 3, 0);
			else {
				SetDialogItemValue(userData->window, 4, 1);
				SetDialogItemValue(userData->window, 3, 0);
			}
			break;
	}
	
	return status;
}

void DoSndF5Options(void) {
	char		text[256];
	WindowPtr	myDlg;
	ControlRef	iCtrl;
	usrData		userData;

	myDlg = getNibWindow(CFSTR("Select F5 Options"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

    if (F5Option == EVERY_LINE)
    	SetDialogItemValue(myDlg, 3, 1);
    else if (F5Option == EVERY_TIER)
    	SetDialogItemValue(myDlg, 4, 1);

	uS.sprintf(templineW, cl_T("%ld"), F5_Offset);
	iCtrl = GetWindowItemAsControl(myDlg, 6);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SelectWindowItemText(myDlg, 6, 0, HUGE_INTEGER);
	ControlKeyFilterUPP keyNumFilter = myNumFilter;
	SetControlData(GetWindowItemAsControl(myDlg, 6), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyNumFilter), &keyNumFilter);
	AdvanceKeyboardFocus(myDlg);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoSndF5OptionsEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		if (GetWindowBoolValue(myDlg, 3) > 0)
			F5Option = EVERY_LINE;
		if (GetWindowBoolValue(myDlg, 4) > 0)
			F5Option = EVERY_TIER;
		GetWindowTextValue(myDlg, 6, text, 255);
		if (*text) {
			F5_Offset = atol(text);
			if (F5_Offset < 0L)
				F5_Offset = 0L;
		}
		WriteCedPreference();
	}

	DisposeWindow(myDlg);
}

static pascal OSStatus DoLineNumSizeEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	HIViewRef itemCtrl;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
		case 'ID05':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl) == 0L)
				SetDialogItemValue(userData->window, 5, 1);
			SetDialogItemValue(userData->window, 6, 0);
			break;
		case 'ID06':
			itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
			if (GetControl32BitValue(itemCtrl) == 0L)
				SetDialogItemValue(userData->window, 6, 1);
			SetDialogItemValue(userData->window, 5, 0);
			break;
	}
	
	return status;
}

void DoLineNumSize(void) {
	char		text[256];
	WindowPtr	myDlg;
	ControlRef	iCtrl;
	usrData		userData;

	myDlg = getNibWindow(CFSTR("Line Number Size"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

	uS.sprintf(templineW, cl_T("%ld"), LineNumberDigitSize);
	iCtrl = GetWindowItemAsControl(myDlg, 4);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SelectWindowItemText(myDlg, 4, 0, HUGE_INTEGER);
	AdvanceKeyboardFocus(myDlg);

    if (LineNumberingType == 0)
    	SetDialogItemValue(myDlg, 5, 1);
    else if (LineNumberingType == 1)
    	SetDialogItemValue(myDlg, 6, 1);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoLineNumSizeEvents, 1, &eventTypeCP, &userData, NULL);
	ShowWindow(myDlg);
	BringToFront(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowTextValue(myDlg, 4, text, 255);
		LineNumberDigitSize = atol(text);
		if (LineNumberDigitSize > 20)
			LineNumberDigitSize = 20;

		if (GetWindowBoolValue(myDlg, 5) > 0)
			LineNumberingType = 0;
		if (GetWindowBoolValue(myDlg, 6) > 0)
			LineNumberingType = 1;

		WriteCedPreference();
	}
	DisposeWindow(myDlg);
	if (userData.res == myOK && global_df != NULL) {
		RefreshAllTextWindows(TRUE);
	}
}

static pascal OSStatus DoThumbnailsEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
	}
	
	return status;
}

void DoThumbnails(void) {
	char		text[512];
	WindowPtr	myDlg;
	ControlRef	iCtrl;
	usrData		userData;
	ControlKeyFilterUPP keyNumFilter = myNumFilter;

	myDlg = getNibWindow(CFSTR("Thumbnails"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

	iCtrl = GetWindowItemAsControl(myDlg, 4);
	sprintf(text, "%ld", (long)ThumbnailsHight);
	u_strcpy(templineW, text, UTTLINELEN);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SelectWindowItemText(myDlg, 4, 0, HUGE_INTEGER);
	SetControlData(GetWindowItemAsControl(myDlg, 5), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyNumFilter), &keyNumFilter);
	AdvanceKeyboardFocus(myDlg);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoThumbnailsEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowTextValue(myDlg, 4, templineC4, 1024);
		ThumbnailsHight = atof(templineC4);
		if (ThumbnailsHight < 30.0000) {
			ThumbnailsHight = 30.0000;
		}
		WriteCedPreference();
	}
	DisposeWindow(myDlg);
}

static pascal OSStatus DoURLAddressEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;

	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
	}
	return status;
}

void DoURLAddress(void) {
	int			i;
	WindowPtr	myDlg;
	ControlRef	iCtrl;
	usrData		userData;

	myDlg = getNibWindow(CFSTR("URL Address"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;

	u_strcpy(templineW, URL_Address, UTTLINELEN);
	iCtrl = GetWindowItemAsControl(myDlg, 4);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	SelectWindowItemText(myDlg, 4, 0, HUGE_INTEGER);
	AdvanceKeyboardFocus(myDlg);

	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), DoURLAddressEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowTextValue(myDlg, 4, templineC4, 1024);
		if (*templineC4) {
			if (uS.mStrnicmp(templineC4, "http://", 7)) {
				strcpy(URL_Address, "http://");
				strcat(URL_Address, templineC4);
			} else
				strcpy(URL_Address, templineC4);
			i = strlen(URL_Address);
			if (URL_Address[i-1] != '/')
				strcat(URL_Address, "/");
			for (i=0; isSpace(URL_Address[i]); i++) ;
			if (i > 0)
				strcpy(URL_Address, URL_Address+i);
			uS.remblanks(URL_Address);
		} else
			URL_Address[0] = EOS;
		WriteCedPreference();
	}
	DisposeWindow(myDlg);
}

// Color Keywords START
struct colorUserWindowData {
	WindowPtr window;
	ControlRef theList;
	RGBColor oldColor;
	RgnHandle visRgn;
	char ColorChanged;
	DataBrowserItemID ItemSelected;
	COLORTEXTLIST *ColorText;
} colorUD;

static pascal Boolean myEventProc(EventRecord *event) {
	return(false);
}

static pascal void myColorChangeFunction(long data, PMColorPtr color) {
}

static COLORTEXTLIST *doKeywordColor(int num, RGBColor *theColor, char isSetColor) {	
	COLORTEXTLIST *t;

	if (num < 1)
		return(colorUD.ColorText);
	for (t=colorUD.ColorText; t != NULL; t=t->nextCT, num--) {
		if (num == 1) {
			if (theColor != NULL) {
				if (isSetColor) {
					t->red = theColor->red;
					t->green = theColor->green;
					t->blue = theColor->blue;
				} else {
					theColor->red   = t->red;
					theColor->green = t->green;
					theColor->blue  = t->blue;
				}
			}
			return(t);
		}
	}
	return(colorUD.ColorText);
}

static void FreeOneColorKeyword(int num) {
	COLORTEXTLIST *t, *tt;

	if (num < 1)
		return;
	if (num == 1) {
		t = colorUD.ColorText;
		colorUD.ColorText = colorUD.ColorText->nextCT;
		free(t->keyWord);
		free(t->fWord);
		free(t);
	} else {
		tt = colorUD.ColorText;
		t  = colorUD.ColorText->nextCT;
		for (num--; t != NULL; num--) {
			if (num == 1) {
				tt->nextCT = t->nextCT;
				free(t->keyWord);
				free(t->fWord);
				free(t);
				break;
			}
			tt = t;
			t = t->nextCT;
		}	
	}
}

static COLORTEXTLIST *copyList(COLORTEXTLIST *RootColorText) {
	COLORTEXTLIST *root, *p;

	root = NULL;
	while (RootColorText != NULL) {
		if (root == NULL) {
			root = NEW(COLORTEXTLIST);
			if (root == NULL)
				mem_err(TRUE, global_df);
			p = root;
		} else {
			p->nextCT = NEW(COLORTEXTLIST);
			if (p->nextCT == NULL)
				mem_err(TRUE, global_df);
			p = p->nextCT;
		}
		p->nextCT = NULL;
		p->cWordFlag = RootColorText->cWordFlag;
		p->keyWord = (unCH *)malloc((strlen(RootColorText->keyWord)+1)*sizeof(unCH));
		if (p->keyWord == NULL) mem_err(TRUE, global_df);
		strcpy(p->keyWord, RootColorText->keyWord);
		p->fWord = (unCH *)malloc((strlen(RootColorText->fWord)+1)*sizeof(unCH));
		if (p->fWord == NULL) mem_err(TRUE, global_df);
		strcpy(p->fWord, RootColorText->fWord);
		p->len = RootColorText->len;
		p->red = RootColorText->red;
		p->green = RootColorText->green;
		p->blue = RootColorText->blue;
		RootColorText = RootColorText->nextCT;
	}
	return(root);
}

static void callColorPicker(void) {
	Rect listBox;
	RGBColor theColor;
	ControlRef itemCtrl;
	ColorPickerInfo theColorInfo;

	if (colorUD.ItemSelected != 0) {
		doKeywordColor(colorUD.ItemSelected, &theColor, FALSE);
		theColorInfo.theColor.profile 			= nil;
		theColorInfo.theColor.color.rgb.red 	= theColor.red;
		theColorInfo.theColor.color.rgb.green	= theColor.green;
		theColorInfo.theColor.color.rgb.blue	= theColor.blue;
		theColorInfo.dstProfile					= nil;
		theColorInfo.flags						= kColorPickerDialogIsMoveable;
		theColorInfo.placeWhere					= kCenterOnMainScreen;
		theColorInfo.pickerType					= 0;
		theColorInfo.eventProc					= NewUserEventUPP(myEventProc);
		theColorInfo.colorProc					= NewColorChangedUPP(myColorChangeFunction);
		theColorInfo.colorProcData				= 1962;
//		strcpy(theColorInfo.prompt+1,"Choose color for keyword: DVD");
//		theColorInfo.prompt[0] = strlen("Choose color for keyword: DVD");
		theColorInfo.prompt[0] = 0;
		theColorInfo.mInfo.editMenuID			= Edit_Menu_ID;
		theColorInfo.mInfo.cutItem				= cut_id;
		theColorInfo.mInfo.copyItem				= copy_id;
		theColorInfo.mInfo.pasteItem			= paste_id;
		theColorInfo.mInfo.undoItem				= undo_id;
		PickColor(&theColorInfo);
		showWindow(colorUD.window);
		if (theColorInfo.newColorChosen) {
			theColor.red = theColorInfo.theColor.color.rgb.red;
			theColor.green = theColorInfo.theColor.color.rgb.green;
			theColor.blue = theColorInfo.theColor.color.rgb.blue;
			doKeywordColor(colorUD.ItemSelected, &theColor, TRUE);
			colorUD.ColorChanged = TRUE;
			itemCtrl = GetWindowItemAsControl(colorUD.window, 7);
			GetControlBounds(itemCtrl, &listBox);
			RGBForeColor(&theColor);
			PaintRect(&listBox);
			RGBForeColor(&colorUD.oldColor);
		}
	}
}

static pascal OSStatus listCallback(ControlRef browser,DataBrowserItemID itemID,DataBrowserPropertyID property,
									 DataBrowserItemDataRef itemData,Boolean changeValue) {
	OSStatus err = noErr;
	CFStringRef text;
	COLORTEXTLIST *nt;
	RGBColor theColor;

	switch ( property ) {
		case 'COLL':
			if ( changeValue ) { /* this would involve renaming the file. we don't want to do that here. */
				err = errDataBrowserPropertyNotSupported;
			} else {
				nt = doKeywordColor(itemID, &theColor, FALSE);
				u_strcpy(templineC, nt->keyWord, UTTLINELEN);
				text = my_CFStringCreateWithBytes(templineC);
				err = SetDataBrowserItemDataText(itemData, text);
				if (text != NULL) {
					CFRelease(text);
				}
			}
			break;
		case kDataBrowserItemIsActiveProperty:
			if ( ! changeValue ) /* is it active? yes */
				err = SetDataBrowserItemDataBooleanValue( itemData, true);
			break;
		case kDataBrowserItemIsSelectableProperty:
			if ( ! changeValue ) /* can we select it? yes */
				err = SetDataBrowserItemDataBooleanValue( itemData, true);
			break;
		case kDataBrowserContainerIsSortableProperty:
		case kDataBrowserItemIsEditableProperty:
		case kDataBrowserItemIsContainerProperty:
			if ( ! changeValue ) /* can we edit it, sort it, or put things in it? no */
				err = SetDataBrowserItemDataBooleanValue( itemData, false);
			break;
		default: /* unrecognized property */
			err = errDataBrowserPropertyNotSupported;
			break;
	}
	return err;
}

static pascal void listNotificationCallback(ControlRef browser,DataBrowserItemID item,DataBrowserItemNotification message) {
	char cWordFlag;
	COLORTEXTLIST *nt;
	Rect listBox;
	RGBColor theColor;
	ControlRef	itemCtrl;
	
	if ( message == kDataBrowserItemDoubleClicked ) {
		colorUD.ItemSelected = item;
		nt = doKeywordColor(item, &theColor, FALSE);
		cWordFlag = nt->cWordFlag;
		SetDialogItemValue(colorUD.window, 9, is_all_line(cWordFlag));
		SetDialogItemValue(colorUD.window, 10, is_match_word(cWordFlag));
		SetDialogItemValue(colorUD.window, 12, is_case_word(cWordFlag));
		SetDialogItemValue(colorUD.window, 14, is_wild_card(cWordFlag));
		itemCtrl = GetWindowItemAsControl(colorUD.window, 7);
		GetControlBounds(itemCtrl, &listBox);
		RGBForeColor(&theColor);
		PaintRect(&listBox);
		RGBForeColor(&colorUD.oldColor);
		callColorPicker();
	} else if ( message == kDataBrowserItemSelected ) {
		colorUD.ItemSelected = item;
		nt = doKeywordColor(item, &theColor, FALSE);
		cWordFlag = nt->cWordFlag;
		SetDialogItemValue(colorUD.window, 9, is_all_line(cWordFlag));
		SetDialogItemValue(colorUD.window, 10, is_match_word(cWordFlag));
		SetDialogItemValue(colorUD.window, 12, is_case_word(cWordFlag));
		SetDialogItemValue(colorUD.window, 14, is_wild_card(cWordFlag));
		itemCtrl = GetWindowItemAsControl(colorUD.window, 7);
		GetControlBounds(itemCtrl, &listBox);
		RGBForeColor(&theColor);
		PaintRect(&listBox);
		RGBForeColor(&colorUD.oldColor);
	}
}

static void displayColorinList(char isSelect) {
	UInt32 num;
	ItemCount	numItems;
	COLORTEXTLIST 	*nt;

	if (colorUD.theList)
		RemoveDataBrowserItems(colorUD.theList, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty);
	if (colorUD.ColorText != NULL && colorUD.theList != NULL) {
		num = 0;
		for (nt=colorUD.ColorText; nt != NULL; nt=nt->nextCT) {
			num++;
		}
		AddDataBrowserItems(colorUD.theList, kDataBrowserNoItem, num, NULL, kDataBrowserItemNoProperty);
		if (isSelect) {
			colorUD.ItemSelected = 1;
			GetDataBrowserItemCount(colorUD.theList,kDataBrowserNoItem,false,kDataBrowserItemAnyState,&numItems);
			if (colorUD.ItemSelected > numItems && numItems > 0)
				colorUD.ItemSelected--;
			if (colorUD.ItemSelected > 0) {
				SetDataBrowserSelectedItems(colorUD.theList,1,&colorUD.ItemSelected,kDataBrowserItemsAdd);
				RevealDataBrowserItem(colorUD.theList, colorUD.ItemSelected, kDataBrowserNoItem, kDataBrowserRevealOnly);
			}
		} else
			colorUD.ItemSelected = 0;
	} else
		colorUD.ItemSelected = 0;
}

static pascal OSStatus DoColorKeywordsEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	OSStatus status = eventNotHandledErr;
	int	wordLen, tergetSelect;
	char cWordFlag;
	Rect listBox;
	RGBColor theColor;
	ControlRef itemCtrl;
	COLORTEXTLIST *nt, *tnt;
	ItemCount	numItems;
	DataBrowserItemID item;
	extern char showColorKeywords;

	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandOK:
					itemCtrl = GetWindowItemAsControl(colorUD.window, 3);
					GetWindowUnicodeTextValue(itemCtrl, templineW, UTTLINELEN);
					wordLen = strlen(templineW);
					if (wordLen > 0) {
						itemCtrl = GetWindowItemAsControl(colorUD.window, 3);
						SetWindowUnicodeTextValue(itemCtrl, TRUE, cl_T(""), 0);
						if (colorUD.ColorText == NULL) {
							colorUD.ColorText = NEW(COLORTEXTLIST);
							if (colorUD.ColorText == NULL)
								mem_err(TRUE, global_df);
							nt = colorUD.ColorText;
							nt->nextCT = NULL;
							tergetSelect = 1;
						} else {
							tergetSelect = 1;
							tnt= colorUD.ColorText;
							nt = colorUD.ColorText;
							while (1) {
								if (nt == NULL) {
									tnt->nextCT = NEW(COLORTEXTLIST);
									if (tnt->nextCT == NULL)
										mem_err(TRUE, global_df);
									nt = tnt->nextCT;
									nt->nextCT = NULL;
									break;
								} else {
									if (uS.mStricmp(nt->keyWord, templineW) == 0) {
										nt = NULL;
										break;
									} else if (uS.mStricmp(nt->keyWord, templineW) > 0) {
										if (nt == colorUD.ColorText) {
											colorUD.ColorText = NEW(COLORTEXTLIST);
											if (colorUD.ColorText == NULL)
												mem_err(TRUE, global_df);
											colorUD.ColorText->nextCT = nt;
											nt = colorUD.ColorText;
										} else {
											nt = NEW(COLORTEXTLIST);
											if (nt == NULL)
												mem_err(TRUE, global_df);
											nt->nextCT = tnt->nextCT;
											tnt->nextCT = nt;
										}
										break;
									}
								}
								tnt = nt;
								nt = nt->nextCT;
								tergetSelect++;
							}
						}
						if (nt != NULL) {
							nt->keyWord = (unCH *)malloc((wordLen+1)*sizeof(unCH));
							if (nt->keyWord == NULL)
								mem_err(TRUE, global_df);
							strcpy(nt->keyWord, templineW);
							nt->fWord = (unCH *)malloc((wordLen+1)*sizeof(unCH));
							if (nt->fWord == NULL)
								mem_err(TRUE, global_df);
							strcpy(nt->fWord, templineW);
							uS.uppercasestr(nt->fWord, &dFnt, C_MBF);
							nt->len = wordLen;
							nt->cWordFlag = 0;
							nt->red = 0;
							nt->green = 0;
							nt->blue = 0;
							colorUD.ColorChanged = TRUE;
							theColor.red   = nt->red;
							theColor.green = nt->green;
							theColor.blue  = nt->blue;
							cWordFlag = nt->cWordFlag;
							SetDialogItemValue(colorUD.window, 9, is_all_line(cWordFlag));
							SetDialogItemValue(colorUD.window, 10, is_match_word(cWordFlag));
							SetDialogItemValue(colorUD.window, 12, is_case_word(cWordFlag));
							SetDialogItemValue(colorUD.window, 14, is_wild_card(cWordFlag));
							itemCtrl = GetWindowItemAsControl(colorUD.window, 7);
							GetControlBounds(itemCtrl, &listBox);
							RGBForeColor(&theColor);
							PaintRect(&listBox);
							RGBForeColor(&colorUD.oldColor);
							displayColorinList(FALSE);
							colorUD.ItemSelected = tergetSelect;
							GetDataBrowserItemCount(colorUD.theList,kDataBrowserNoItem,false,kDataBrowserItemAnyState,&numItems);
							if (colorUD.ItemSelected > numItems && numItems > 0)
								colorUD.ItemSelected--;
							if (colorUD.ItemSelected > 0) {
								SetDataBrowserSelectedItems(colorUD.theList,1,&colorUD.ItemSelected,kDataBrowserItemsAdd);
								RevealDataBrowserItem(colorUD.theList, colorUD.ItemSelected, kDataBrowserNoItem, kDataBrowserRevealOnly);
							}
						}
					}
					break;
				case 'ID05':
					if (colorUD.ItemSelected != 0) {
						FreeOneColorKeyword(colorUD.ItemSelected);
						colorUD.ColorChanged = TRUE;
						theColor.red = 0;
						theColor.green = 0;
						theColor.blue = 0;
						cWordFlag = 0;
						SetDialogItemValue(colorUD.window, 9, is_all_line(cWordFlag));
						SetDialogItemValue(colorUD.window, 10, is_match_word(cWordFlag));
						SetDialogItemValue(colorUD.window, 12, is_case_word(cWordFlag));
						SetDialogItemValue(colorUD.window, 14, is_wild_card(cWordFlag));
						itemCtrl = GetWindowItemAsControl(colorUD.window, 7);
						GetControlBounds(itemCtrl, &listBox);
						RGBForeColor(&theColor);
						PaintRect(&listBox);
						RGBForeColor(&colorUD.oldColor);
						item = colorUD.ItemSelected;
						displayColorinList(FALSE);
						if (colorUD.theList) {
							GetDataBrowserItemCount(colorUD.theList,kDataBrowserNoItem,false,kDataBrowserItemAnyState,&numItems);
							if (item > numItems && numItems > 0)
								item--;
							SetDataBrowserSelectedItems(colorUD.theList,1,&item,kDataBrowserItemsAdd);
							colorUD.ItemSelected = item;
							RevealDataBrowserItem(colorUD.theList, colorUD.ItemSelected, kDataBrowserNoItem, kDataBrowserRevealOnly);
						}
					}
					break;
				case 'ID06':
					FreeColorText(colorUD.ColorText);
					colorUD.ColorText = NULL;
					colorUD.ColorChanged = TRUE;
					theColor.red = 0;
					theColor.green = 0;
					theColor.blue = 0;
					cWordFlag = 0;
					SetDialogItemValue(colorUD.window, 9, is_all_line(cWordFlag));
					SetDialogItemValue(colorUD.window, 10, is_match_word(cWordFlag));
					SetDialogItemValue(colorUD.window, 12, is_case_word(cWordFlag));
					SetDialogItemValue(colorUD.window, 14, is_wild_card(cWordFlag));
					itemCtrl = GetWindowItemAsControl(colorUD.window, 7);
					GetControlBounds(itemCtrl, &listBox);
					RGBForeColor(&theColor);
					PaintRect(&listBox);
					RGBForeColor(&colorUD.oldColor);
					displayColorinList(FALSE);
					break;
//				case 'ID07':
				case 'ID08':
					callColorPicker();
					break;
				case 'ID09':
					if (colorUD.ItemSelected != 0) {
						nt = doKeywordColor(colorUD.ItemSelected, NULL, FALSE);
						if (is_all_line(nt->cWordFlag))
							nt->cWordFlag = set_all_line_to_0(nt->cWordFlag);
						else
							nt->cWordFlag = set_all_line_to_1(nt->cWordFlag);
						SetDialogItemValue(colorUD.window, 9, is_all_line(nt->cWordFlag));
						colorUD.ColorChanged = TRUE;
					}
					break;
				case 'ID10':
					if (colorUD.ItemSelected != 0) {
						nt = doKeywordColor(colorUD.ItemSelected, NULL, FALSE);
						if (is_match_word(nt->cWordFlag))
							nt->cWordFlag = set_match_word_to_0(nt->cWordFlag);
						else
							nt->cWordFlag = set_match_word_to_1(nt->cWordFlag);
						SetDialogItemValue(colorUD.window, 10, is_match_word(nt->cWordFlag));
						colorUD.ColorChanged = TRUE;
					}
					break;
				case 'ID11':
					showColorKeywords = !showColorKeywords;
					SetDialogItemValue(colorUD.window, 11, showColorKeywords);
					colorUD.ColorChanged = TRUE;
					break;
				case 'ID12':
					if (colorUD.ItemSelected != 0) {
						nt = doKeywordColor(colorUD.ItemSelected, NULL, FALSE);
						if (is_case_word(nt->cWordFlag)) {
							nt->cWordFlag = set_case_word_to_0(nt->cWordFlag);
							uS.uppercasestr(nt->fWord, &dFnt, C_MBF);
						} else {
							nt->cWordFlag = set_case_word_to_1(nt->cWordFlag);
							strcpy(nt->fWord, nt->keyWord);
						}
						SetDialogItemValue(colorUD.window, 12, is_case_word(nt->cWordFlag));
						colorUD.ColorChanged = TRUE;
					}
					break;
				case 'ID14':
					if (colorUD.ItemSelected != 0) {
						nt = doKeywordColor(colorUD.ItemSelected, NULL, FALSE);
						if (is_wild_card(nt->cWordFlag))
							nt->cWordFlag = set_wild_card_to_0(nt->cWordFlag);
						else
							nt->cWordFlag = set_wild_card_to_1(nt->cWordFlag);
						SetDialogItemValue(colorUD.window, 14, is_wild_card(nt->cWordFlag));
						colorUD.ColorChanged = TRUE;
					}
					break;
				case 'ID15':
					QuitAppModalLoopForWindow(colorUD.window);
					break;
				case kHICommandCancel:
					colorUD.ColorChanged = FALSE;
					QuitAppModalLoopForWindow(colorUD.window);
					break;
				default:
					break;
			}
			break;
		case kEventClassControl:
			switch (GetEventKind(inEvent)) {
				case kEventControlClick:
					HIPoint hPoint;
					ControlID outID;

					GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &itemCtrl);
					GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &hPoint);
					if (GetControlID(itemCtrl, &outID) != noErr)
						return status;
					if (outID.id == 7)
						callColorPicker();
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return status;
}

void DoColorKeywords(void) {
	int				wordLen;
	char			cWordFlag;
	unCH			text[256];
	long			sc, ec;
	Rect			listBox;
	COLORTEXTLIST 	*nt;
	RGBColor		theColor;
	WindowPtr		myDlg;
	ControlRef 		itemCtrl;
	ControlID		DcontrolID = { 'CLST', 4 };
	DataBrowserCallbacks dataBrowserHooks;		
	extern char		showColorKeywords;

	if (global_df == NULL)
		return;
	myDlg = getNibWindow(CFSTR("Color Keywords"));
	CenterWindow(myDlg, -1, -1);
	showWindow(myDlg);

	if (global_df->row_win2 || global_df->col_win2 != -2L) {
		ChangeCurLineAlways(0);
		if (global_df->row_win2 == 0) {
			if (global_df->col_win > global_df->col_win2) {
				sc = global_df->col_chr2;
				ec = global_df->col_chr;
			} else {
				sc = global_df->col_chr;
				ec = global_df->col_chr2;
			}
			for (wordLen=0; sc < ec && wordLen < 255; sc++, wordLen++)
				text[wordLen] = global_df->row_txt->line[sc];
			text[wordLen] = EOS;
			itemCtrl = GetWindowItemAsControl(myDlg, 3);
			SetWindowUnicodeTextValue(itemCtrl, TRUE, text, 0);
			SelectWindowItemText(myDlg, 3, 0, HUGE_INTEGER);
		} else {
		}
	}
	AdvanceKeyboardFocus(myDlg);
	SetDialogItemValue(myDlg, 11, showColorKeywords);
	EventTypeSpec eventTypeCP[] = {
									{kEventClassCommand, kEventCommandProcess},
									{kEventClassControl, kEventControlClick}
								  } ;
	InstallEventHandler(GetWindowEventTarget(myDlg), DoColorKeywordsEvents, 2, eventTypeCP, NULL, NULL);
	GetForeColor(&colorUD.oldColor);
	colorUD.window = myDlg;
	colorUD.ColorText = copyList(global_df->RootColorText);
	colorUD.ColorChanged = FALSE;

	if ( GetControlByID(myDlg, &DcontrolID, &colorUD.theList) != noErr )
		goto bail;
	colorUD.ItemSelected = 0;
	dataBrowserHooks.version = kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&dataBrowserHooks);
	dataBrowserHooks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(listCallback);
	dataBrowserHooks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(listNotificationCallback);
	SetDataBrowserCallbacks(colorUD.theList, &dataBrowserHooks);
	if (colorUD.ColorText != NULL) {
		displayColorinList(TRUE);
		nt = doKeywordColor(1, &theColor, FALSE);
		cWordFlag = nt->cWordFlag;
	} else {
		theColor.red = 0;
		theColor.green = 0;
		theColor.blue = 0;
		cWordFlag = 0;
	}
	SetDialogItemValue(colorUD.window, 9, is_all_line(cWordFlag));
	SetDialogItemValue(colorUD.window, 10, is_match_word(cWordFlag));
	SetDialogItemValue(colorUD.window, 12, is_case_word(cWordFlag));
	SetDialogItemValue(colorUD.window, 14, is_wild_card(cWordFlag));
	itemCtrl = GetWindowItemAsControl(colorUD.window, 7);
	GetControlBounds(itemCtrl, &listBox);
	RGBForeColor(&theColor);
	PaintRect(&listBox);
	RGBForeColor(&colorUD.oldColor);
	colorUD.visRgn = NewRgn();
	RunAppModalLoopForWindow(myDlg);
	if (colorUD.ColorChanged) {
		FreeColorText(global_df->RootColorText);
		global_df->RootColorText = colorUD.ColorText;
		global_df->DataChanged = TRUE;
		touchwin(global_df->w1);
		wrefresh(global_df->w1);
	} else
		FreeColorText(colorUD.ColorText);
	DisposeRgn(colorUD.visRgn);
bail:
	DisposeWindow(myDlg);
}
// Color Keywords END


// IDs BEGIN
#define idsUsrData struct idsUserWindowData
struct idsUserWindowData {
	WindowPtr window;
	short res;
	IDSTYPE *rootIDs;
	ControlRef lanItemCtrl;
	ControlRef codeItemCtrl;
	MenuItemIndex spItem;
	MenuRef spMref;
	ControlRef spCtrlH;
	ControlRef spItemCtrl;
	MenuRef roleMref;
	ControlRef roleCtrlH;
	ControlRef roleItemCtrl;
};

static pascal ControlKeyFilterResult myAgeFilter(ControlRef theControl, SInt16 *keyCode, SInt16 *charCode, EventModifiers *modifiers)
{
	unCH num[CODESIZE+1];
	ControlEditTextSelectionRec selection;
	
	// the edit text control can filter keys on his own
	if (!(*modifiers & cmdKey)) {
		if (iswdigit(*charCode)) {
			GetWindowUnicodeTextValue(theControl, num, CODESIZE);
			GetControlData(theControl, 0, kControlEditTextSelectionTag, sizeof(selection), (Ptr)&selection, NULL);
			if (selection.selStart != selection.selEnd || strlen(num) < 2)
				return kControlKeyFilterPassKey;
		} else if (*charCode == 0x7f || *charCode == 0x8 || *charCode == 0x1c || *charCode == 0x1d ||
				   *charCode == 0x1e || *charCode == 0x1f)
			return kControlKeyFilterPassKey;
		
		SysBeep(1);
		return kControlKeyFilterBlockKey;
	}
	return kControlKeyFilterPassKey;
}

static char saveIDFields(idsUsrData *userData, int item) {
	unCH	numS[CODESIZE];
	char	err_mess[512];
	IDSTYPE *p;
	CFStringRef cString;

	if (item <= 0 || item > CountMenuItems(userData->spMref))
		return(TRUE);
	for (p=userData->rootIDs; p != NULL; p=p->next_id) {
		item--;
		if (item <= 0)
			break;
	}
	if (p != NULL) {
		GetWindowUnicodeTextValue(userData->lanItemCtrl,  p->language, IDFIELSSIZE);
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 8),  p->corpus, IDFIELSSIZE);
		GetWindowUnicodeTextValue(userData->codeItemCtrl,  p->code, CODESIZE);
		uS.uppercasestr(p->code, NULL, 0);
		p->ageAbout = GetWindowBoolValue(userData->window, 12) > 0;
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 13),  numS, CODESIZE);
		if (numS[0] == EOS) p->age1y = -1;
		else p->age1y = atoi(numS);
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 15),  numS, CODESIZE);
		if (numS[0] == EOS) p->age1m = -1;
		else p->age1m = atoi(numS);
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 17),  numS, CODESIZE);
		if (numS[0] == EOS) p->age1d = -1;
		else p->age1d = atoi(numS);
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 19),  numS, CODESIZE);
		if (numS[0] == EOS) p->age2y = -1;
		else p->age2y = atoi(numS);
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 21),  numS, CODESIZE);
		if (numS[0] == EOS) p->age2m = -1;
		else p->age2m = atoi(numS);
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 23),  numS, CODESIZE);
		if (numS[0] == EOS) p->age2d = -1;
		else p->age2d = atoi(numS);
		if (GetWindowBoolValue(userData->window, 26) > 0)
			p->sex = 'm';
		else if (GetWindowBoolValue(userData->window, 27) > 0)
			p->sex = 'f';
		else
			p->sex = 0;
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 29),  p->group, IDFIELSSIZE);
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 31),  p->SES, IDFIELSSIZE);
		item = GetControl32BitValue(userData->roleCtrlH);
		if (item == 1) {
			p->role[0] = EOS;
		} else {
			CopyMenuItemTextAsCFString(userData->roleMref, item, &cString);
			if (cString != NULL) {
				item = CFStringGetLength(cString);
				CFStringGetCharacters(cString, CFRangeMake(0, item), (UniChar *)p->role);
				p->role[item] = EOS;
				CFRelease(cString);
			}
		}
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 35),  p->education, IDFIELSSIZE);
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 37),  p->custom_field, IDFIELSSIZE);
		GetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 39),  p->spname, IDFIELSSIZE);
		if (p->language[0] == EOS) {
			sprintf(err_mess, "Please fill-in \"Language\" field");
			do_warning(err_mess, -1);
			return(FALSE);
		}
		if (p->code[0] == EOS) {
			sprintf(err_mess, "Please fill-in \"Name code\" field");
			do_warning(err_mess, -1);
			return(FALSE);
		}
		if (p->role[0] == EOS) {
			sprintf(err_mess, "Please select a \"Role\"");
			do_warning(err_mess, -1);
			return(FALSE);
		}
		if (p->ageAbout && (p->age1m != -1 || p->age1d != -1 || p->age2m != -1 || p->age2d != -1)) {
			do_warning("if '~' is used, then only year number is allowed", -1);
			return(FALSE);
		}
		if (p->age1m != -1) {
			if (p->age1m < 0 || p->age1m >= 12) {
				sprintf(err_mess, "Illegal month number: %d, please choose 0 - 11", p->age1m);
				do_warning(err_mess, -1);
				return(FALSE);
			}
			if (p->age1y == -1) {
				sprintf(err_mess, "Please specify a year for age");
				do_warning(err_mess, -1);
				return(FALSE);
			}
		}
		if (p->age1d != -1) {
			if (p->age1d < 0 || p->age1d > 31) {
				sprintf(err_mess, "Illegal date number: %d, please choose 1 - 31", p->age1d);
				do_warning(err_mess, -1);
				return(FALSE);
			}
			if (p->age1y == -1) {
				sprintf(err_mess, "Please specify a year for age");
				do_warning(err_mess, -1);
				return(FALSE);
			}
			if (p->age1m == -1) {
				sprintf(err_mess, "Please specify a month for age");
				do_warning(err_mess, -1);
				return(FALSE);
			}
		}
		if (p->age2m != -1) {
			if (p->age2m < 0 || p->age2m >= 12) {
				sprintf(err_mess, "Illegal second month number: %d, please choose 0 - 11", p->age2m);
				do_warning(err_mess, -1);
				return(FALSE);
			}
			if (p->age2y == -1) {
				sprintf(err_mess, "Please specify a second year for age");
				do_warning(err_mess, -1);
				return(FALSE);
			}
		}
		if (p->age2d != -1) {
			if (p->age2d < 0 || p->age2d > 31) {
				sprintf(err_mess, "Illegal second date number: %d, please choose 1 - 31", p->age2d);
				do_warning(err_mess, -1);
				return(FALSE);
			}
			if (p->age2y == -1) {
				sprintf(err_mess, "Please specify a second year for age");
				do_warning(err_mess, -1);
				return(FALSE);
			}
			if (p->age2m == -1) {
				sprintf(err_mess, "Please specify a second month for age");
				do_warning(err_mess, -1);
				return(FALSE);
			}
		}
	}
	return(TRUE);
}

static void fillInIDFields(idsUsrData *userData) {
	int		item;
	unCH	numS[CODESIZE];
	IDSTYPE *p;
	CFStringRef cStringRole, cStringMenu;

	if (userData->spItem <= 0 || userData->spItem > CountMenuItems(userData->spMref))
		return;
	item = userData->spItem;
	for (p=userData->rootIDs; p != NULL; p=p->next_id) {
		item--;
		if (item <= 0)
			 break;
	}
	if (p != NULL) {
		SetWindowUnicodeTextValue(userData->lanItemCtrl, TRUE, p->language, 0);
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 8), TRUE, p->corpus, 0);
		SetWindowUnicodeTextValue(userData->codeItemCtrl, TRUE, p->code, 0);
		SetDialogItemValue(userData->window, 12, p->ageAbout);
		if (p->age1y != -1) uS.sprintf(numS, cl_T("%d"), p->age1y);
		else numS[0] = EOS;
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 13), TRUE, numS, 0);
		if (p->age1m != -1) uS.sprintf(numS, cl_T("%d"), p->age1m);
		else numS[0] = EOS;
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 15), TRUE, numS, 0);
		if (p->age1d != -1) uS.sprintf(numS, cl_T("%d"), p->age1d);
		else numS[0] = EOS;
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 17), TRUE, numS, 0);
		if (p->age2y != -1) uS.sprintf(numS, cl_T("%d"), p->age2y);
		else numS[0] = EOS;
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 19), TRUE, numS, 0);
		if (p->age2m != -1) uS.sprintf(numS, cl_T("%d"), p->age2m);
		else numS[0] = EOS;
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 21), TRUE, numS, 0);
		if (p->age2d != -1) uS.sprintf(numS, cl_T("%d"), p->age2d);
		else numS[0] = EOS;
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 23), TRUE, numS, 0);
		if (p->sex == 'm' || p->sex == 'M') {
			SetDialogItemValue(userData->window, 26, 1); // male
			SetDialogItemValue(userData->window, 27, 0); // female
			SetDialogItemValue(userData->window, 25, 0); // unknown
		} else if (p->sex == 'f' || p->sex == 'F') {
			SetDialogItemValue(userData->window, 26, 0); // male
			SetDialogItemValue(userData->window, 27, 1); // female
			SetDialogItemValue(userData->window, 25, 0); // unknown
		} else {
			SetDialogItemValue(userData->window, 26, 0); // male
			SetDialogItemValue(userData->window, 27, 0); // female
			SetDialogItemValue(userData->window, 25, 1); // unknown
		}
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 29), TRUE, p->group, 0);
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 31), TRUE, p->SES, 0);
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 35), TRUE, p->education, 0);
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 37), TRUE, p->custom_field, 0);
		SetWindowUnicodeTextValue(GetWindowItemAsControl(userData->window, 39), TRUE, p->spname, 0);
		if (p->role[0] == EOS)
			SetControl32BitValue(userData->roleCtrlH, 1);
		else {
			cStringRole = CFStringCreateWithBytes(NULL, (UInt8 *)p->role, strlen(p->role)*sizeof(unCH), kCFStringEncodingUTF16, false);
			if (cStringRole != NULL) {
				for (item=1; item < CountMenuItems(userData->roleMref); item++) {
					CopyMenuItemTextAsCFString(userData->roleMref, item, &cStringMenu);
					if (cStringMenu != NULL) {
						if (CFStringCompare(cStringRole,cStringMenu,kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
							SetControl32BitValue(userData->roleCtrlH, item);
							CFRelease(cStringMenu);
							break;
						}
						CFRelease(cStringMenu);
					}
				}
				CFRelease(cStringRole);
			}
		}
		Draw1Control(userData->roleCtrlH);
	}
}

static void createPopupRoleMenu(idsUsrData *userData, ROLESTYPE *roles) {
	int			i;
	CFStringRef cString;
	
	if ((i=CountMenuItems(userData->roleMref)) > 0) {
		for (; i > 0; i--)
			DeleteMenuItem(userData->roleMref, i);
	}
	i = 0;
	cString = CFStringCreateWithBytes(NULL, (UInt8 *)"Choose one role", strlen("Choose one role"), kCFStringEncodingUTF8, false);
	if (cString != NULL) {
		InsertMenuItemTextWithCFString(userData->roleMref, cString, i, kMenuItemAttrCustomDraw, i);
		CFRelease(cString);
		i++;
	}
	for (; roles != NULL; roles=roles->nextrole) {
		cString = CFStringCreateWithBytes(NULL, (UInt8 *)roles->role, strlen(roles->role)*sizeof(unCH), kCFStringEncodingUTF16, false);
		if (cString != NULL) {
			InsertMenuItemTextWithCFString(userData->roleMref, cString, i, kMenuItemAttrCustomDraw, i);
			CFRelease(cString);
			i++;
		}
	}
	InsertMenu(userData->roleMref, -1);
	Draw1Control(userData->roleCtrlH);
}

static void createPopupSpeakerMenu(idsUsrData *userData, char isInit) {
	int			i;
	IDSTYPE		*p;
	CFStringRef cString;

	if ((i=CountMenuItems(userData->spMref)) > 0) {
		for (; i > 0; i--)
			DeleteMenuItem(userData->spMref, i);
	}
	i = 0;
	for (p=userData->rootIDs; p != NULL; p=p->next_id) {
		cString = CFStringCreateWithBytes(NULL, (UInt8 *)p->code, strlen(p->code)*sizeof(unCH), kCFStringEncodingUTF16, false);
		if (cString != NULL) {
			InsertMenuItemTextWithCFString(userData->spMref, cString, i, kMenuItemAttrCustomDraw, i);
			CFRelease(cString);
			i++;
		}
	}
	if (isInit)
		InsertMenu(userData->spMref, -1);
	if (userData->spItem <= 0 || userData->spItem > CountMenuItems(userData->spMref))
		userData->spItem = 1;
	SetControl32BitValue(userData->spCtrlH, userData->spItem);
	Draw1Control(userData->spCtrlH);
}

static void updateSpeakerMenu(idsUsrData *userData) {
	wchar_t code[CODESIZE];
	CFStringRef cString;
	ControlEditTextSelectionRec selection;

	if (userData->spItem <= 0 || userData->spItem > CountMenuItems(userData->spMref))
		return;
	GetWindowUnicodeTextValue(userData->codeItemCtrl, code, CODESIZE);
	uS.uppercasestr(code, NULL, 0);
	GetControlData(userData->codeItemCtrl, 0, kControlEditTextSelectionTag, sizeof(selection), (Ptr)&selection, NULL);
	SetWindowUnicodeTextValue(userData->codeItemCtrl, TRUE, code, 0);
	SetControlData(userData->codeItemCtrl, 0, kControlEditTextSelectionTag, sizeof( selection ), (Ptr)&selection);
	cString = CFStringCreateWithBytes(NULL, (UInt8 *)code, strlen(code)*sizeof(unCH), kCFStringEncodingUTF16, false);
	if (cString != NULL) {
		SetMenuItemTextWithCFString(userData->spMref, userData->spItem, cString);
		CFRelease(cString);
	}						
	Draw1Control(userData->spCtrlH);
}

static UInt32 filterLanguage(wchar_t *str, UInt32 outActualSize) {
	int i;
	
	outActualSize = outActualSize / 2;
	for (i=0; i < outActualSize; i++) {
		if (iswalpha(str[i])) {
			str[i] = towlower(str[i]);
		} else if (str[i] == ',' || str[i] == 0xfffe || str[i] == 0xfeff || str[i] == 0x7f || str[i] == 0x8 || str[i] == 0x9 || 
				   str[i] == 0x1b || str[i] == 0x1c || str[i] == 0x1d || str[i] == 0x1e || str[i] == 0x1f) {
		} else {
			strcpy(str+i, str+i+1);
			outActualSize--;
			SysBeep(1);
		}
	}
	return(outActualSize*2);
}

static UInt32 filterCode(wchar_t *str, UInt32 outActualSize) {
	int i;

	outActualSize = outActualSize / 2;
	for (i=0; i < outActualSize; i++) {
		if (str[i] == '*' || str[i] == ':') {
			strcpy(str+i, str+i+1);
			outActualSize--;
			do_warning("Please do not include either '*' or ':' character", -1);
		} else if (iswalpha(str[i])) {
			str[i] = towupper(str[i]);
		} else if (iswdigit(str[i]) || str[i] == '-' || str[i] == 0xfffe || str[i] == 0xfeff || str[i] == 0x7f || str[i] == 0x8 ||
				   str[i] == 0x9 || str[i] == 0x1b || str[i] == 0x1c || str[i] == 0x1d || str[i] == 0x1e || str[i] == 0x1f) {
		} else {
			strcpy(str+i, str+i+1);
			outActualSize--;
			SysBeep(1);
		}
	}
	return(outActualSize*2);
}

static pascal OSStatus IDDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	int i, item;
	Rect  tempRect;
	Point theMouse;
	HICommand aCommand;
	ControlRef	selectedItemCtrl;
	idsUsrData *userData = (idsUsrData *)inUserData;
	OSStatus status = eventNotHandledErr;

	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandOK:
					if (saveIDFields(userData, userData->spItem)) {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myOK;
					}
					break;
				case kHICommandCancel:
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myCANCEL;
					break;
				case 'UNKN':
					SetDialogItemValue(userData->window, 26, 0); // male
					SetDialogItemValue(userData->window, 27, 0); // female
					SetDialogItemValue(userData->window, 25, 1); // unknown
					break;
				case 'MALE':
					SetDialogItemValue(userData->window, 26, 1); // male
					SetDialogItemValue(userData->window, 27, 0); // female
					SetDialogItemValue(userData->window, 25, 0); // unknown
					break;
				case 'FEMA':
					SetDialogItemValue(userData->window, 26, 0); // male
					SetDialogItemValue(userData->window, 27, 1); // female
					SetDialogItemValue(userData->window, 25, 0); // unknown
					break;
				case 'DELE':
					i = CountMenuItems(userData->spMref);
					if (i > 1) {
						userData->rootIDs = deleteID(userData->rootIDs, userData->spItem);
						userData->spItem = 1;
						createPopupSpeakerMenu(userData, FALSE);
						fillInIDFields(userData);
					} else {
						saveIDFields(userData, userData->spItem);
					}
					break;
				case 'CREA':
					if (saveIDFields(userData, userData->spItem)) {
						userData->rootIDs = createNewId(userData->rootIDs, 0);
						userData->spItem = 1;
						createPopupSpeakerMenu(userData, FALSE);
						fillInIDFields(userData);
					}
					break;
				case 'COPY':
					if (saveIDFields(userData, userData->spItem)) {
						userData->rootIDs = createNewId(userData->rootIDs, userData->spItem);
						userData->spItem = 1;
						createPopupSpeakerMenu(userData, FALSE);
						fillInIDFields(userData);
					}
					break;
				case 'SPKM':
					break;
				case 'ROLM':
					break;
			}
			break;
		case kEventClassControl:
			if (GetEventKind(inEvent) == kEventControlClick) {
				GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(selectedItemCtrl), NULL, &selectedItemCtrl);
				if (selectedItemCtrl == userData->spItemCtrl) {
					GetControlBounds(userData->spCtrlH, &tempRect);
					theMouse.h = tempRect.left;
					theMouse.v = tempRect.top;
//					TrackControl(userData->spCtrlH, theMouse, NULL);
					HandleControlClick(userData->spCtrlH, theMouse, 0, NULL);
					item = GetControl32BitValue(userData->spCtrlH);
					if (item > 0 && item != userData->spItem) {
						if (saveIDFields(userData, userData->spItem)) {
							userData->spItem = item;
							fillInIDFields(userData);
						}
						SetControl32BitValue(userData->spCtrlH, userData->spItem);
					}
				} else if (selectedItemCtrl == userData->roleItemCtrl) {
					GetControlBounds(userData->roleCtrlH, &tempRect);
					theMouse.h = tempRect.left;
					theMouse.v = tempRect.top;
//					TrackControl(userData->roleCtrlH, theMouse, NULL);
					HandleControlClick(userData->roleCtrlH, theMouse, 0, NULL);
				}
			}
			break;
		case kEventClassKeyboard:
			if (GetEventKind(inEvent) == kEventRawKeyDown) {
				UInt32 outActualSize;
				wchar_t str[256];
				GetKeyboardFocus(userData->window, &selectedItemCtrl);
				if (selectedItemCtrl == userData->lanItemCtrl) {
					GetEventParameter(inEvent, kEventParamKeyUnicodes, typeUTF16ExternalRepresentation, NULL, sizeof(wchar_t)*256, &outActualSize, str);
					outActualSize = filterLanguage(str, outActualSize);
					SetEventParameter(inEvent, kEventParamKeyUnicodes, typeUTF16ExternalRepresentation, outActualSize, str);
				}
				if (selectedItemCtrl == userData->codeItemCtrl) {
					GetEventParameter(inEvent, kEventParamKeyUnicodes, typeUTF16ExternalRepresentation, NULL, sizeof(wchar_t)*256, &outActualSize, str);
					outActualSize = filterCode(str, outActualSize);
					SetEventParameter(inEvent, kEventParamKeyUnicodes, typeUTF16ExternalRepresentation, outActualSize, str);
				}
			} else if (GetEventKind(inEvent) == kEventRawKeyUp) {
				GetKeyboardFocus(userData->window, &selectedItemCtrl);
				if (selectedItemCtrl == userData->codeItemCtrl) {
					updateSpeakerMenu(userData);
				}
			}
			break;
		default:
			break;
	}
	
	return status;
}

char IDDialog(IDSTYPE **rootIDs, ROLESTYPE *rootRoles) {
	int			i;
	WindowPtr	myDlg;
	Rect		tempRect;
	IBNibRef	mNibs;
	OSErr		err;
	idsUsrData	userData;
	ControlKeyFilterUPP keyStrFilter = myStringFilter;
	ControlKeyFilterUPP keyAgeFilter = myAgeFilter;
	EventTypeSpec eventTypeCP[] = {
									{kEventClassKeyboard, kEventRawKeyDown},
									{kEventClassKeyboard, kEventRawKeyUp},
									{kEventClassControl, kEventControlClick},
									{kEventClassCommand, kEventCommandProcess}
								  } ;
	
	myDlg = getNibWindow(CFSTR("IDs"));
	CenterWindow(myDlg, -1, -1);
	showWindow(myDlg);

	userData.window = myDlg;
	userData.res = 0;
	userData.rootIDs = *rootIDs;
	userData.spMref = NULL;
	userData.spItem = 1;
	userData.spCtrlH = NULL;
	userData.roleMref = NULL;
	userData.roleCtrlH = NULL;
	userData.lanItemCtrl = GetWindowItemAsControl(myDlg,  6);
	userData.codeItemCtrl = GetWindowItemAsControl(myDlg, 10);

	err = CreateNibReference(CFSTR("CLAN"), &mNibs);
	if (err != noErr)
		goto finIdd;
	err = CreateMenuFromNib(mNibs, CFSTR("pop_up"), &userData.spMref);
	if (err != noErr) {
		userData.spMref = NULL;
		goto finIdd;
	}

	err = CreateMenuFromNib(mNibs, CFSTR("pop_up"), &userData.roleMref);
	if (err != noErr) {
		userData.roleMref = NULL;
		goto finIdd;
	}
	createPopupRoleMenu(&userData, rootRoles);
	DisposeNibReference(mNibs);

	userData.roleItemCtrl = GetWindowItemAsControl(userData.window, 33);
	GetControlBounds(userData.roleItemCtrl, &tempRect);
	userData.roleCtrlH = NewControl(userData.window,&tempRect,"\p",true,0,128,0,1008,0);
	if (userData.roleCtrlH == nil)
		goto finIdd;
	SetControlPopupMenuHandle(userData.roleCtrlH, userData.roleMref);
	
	userData.spItemCtrl = GetWindowItemAsControl(userData.window, 4);
	GetControlBounds(userData.spItemCtrl, &tempRect);
	userData.spCtrlH = NewControl(userData.window,&tempRect,"\p",true,0,128,0,1008,0);
	if (userData.spCtrlH == nil)
		goto finIdd;
	SetControlPopupMenuHandle(userData.spCtrlH, userData.spMref);
	createPopupSpeakerMenu(&userData, TRUE);
	fillInIDFields(&userData);

	SetControlData(GetWindowItemAsControl(myDlg,  8), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 29), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 31), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 35), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 37), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 39), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);

	SetControlData(GetWindowItemAsControl(myDlg, 13), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyAgeFilter), &keyAgeFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 15), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyAgeFilter), &keyAgeFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 17), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyAgeFilter), &keyAgeFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 19), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyAgeFilter), &keyAgeFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 21), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyAgeFilter), &keyAgeFilter);
	SetControlData(GetWindowItemAsControl(myDlg, 23), kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyAgeFilter), &keyAgeFilter);
	
	dial_isF1Pressed = FALSE;
	dial_isF2Pressed = FALSE;
	
	AdvanceKeyboardFocus(myDlg);
	InstallEventHandler(GetWindowEventTarget(myDlg), IDDialogEvents, 4, eventTypeCP, &userData, NULL);
	RunAppModalLoopForWindow(myDlg);

finIdd:
	*rootIDs = userData.rootIDs;
	if (userData.spCtrlH != NULL)
		DisposeControl(userData.spCtrlH);
	if (userData.spMref != NULL) {
		if ((i=CountMenuItems(userData.spMref)) > 0) {
			for (; i > 0; i--)
				DeleteMenuItem(userData.spMref, i-1);
		}
		DeleteMenu(GetMenuID(userData.spMref));
		DisposeMenu(userData.spMref);
	}

	if (userData.roleCtrlH != NULL)
		DisposeControl(userData.roleCtrlH);
	if (userData.roleMref != NULL) {
		if ((i=CountMenuItems(userData.roleMref)) > 0) {
			for (; i > 0; i--)
				DeleteMenuItem(userData.roleMref, i-1);
		}
		DeleteMenu(GetMenuID(userData.roleMref));
		DisposeMenu(userData.roleMref);
	}

	DisposeWindow(myDlg);

	if (userData.res == myOK) {
		return(TRUE);
	} else
		return(FALSE);
}
// IDs END

static pascal OSStatus FindDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	FNType		sfFile[FNSize];
	ControlRef	itemCtrl;
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;

	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
		case kHICommandCancel:
			// we got a valid click on the OK button so let's quit our local run loop
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myCANCEL;
			break;
		case 'ID04':
			itemCtrl = GetWindowItemAsControl(userData->window, 4);
			SetControl32BitValue(itemCtrl,!GetControl32BitValue(itemCtrl));
			break;
		case 'ID06':
			itemCtrl = GetWindowItemAsControl(userData->window, 6);
			SetControl32BitValue(itemCtrl,!GetControl32BitValue(itemCtrl));
			break;
		case 'ID07':
			itemCtrl = GetWindowItemAsControl(userData->window, 7);
			SetControl32BitValue(itemCtrl,!GetControl32BitValue(itemCtrl));
			break;
		case 'ID12':
			itemCtrl = GetWindowItemAsControl(userData->window, 12);
			SetControl32BitValue(itemCtrl,!GetControl32BitValue(itemCtrl));
			break;
		case 'INCR':
			itemCtrl = GetWindowItemAsControl(userData->window, 3);
			SetWindowUnicodeTextValue(itemCtrl, FALSE, NULL, '\r');
			break;
		case 'INTB':
			itemCtrl = GetWindowItemAsControl(userData->window, 3);
			SetWindowUnicodeTextValue(itemCtrl, FALSE, NULL, '\t');
			break;
		case 'INBU':
			itemCtrl = GetWindowItemAsControl(userData->window, 3);
			SetWindowUnicodeTextValue(itemCtrl, FALSE, NULL, 0x2022);
			break;
		case 'FNDF':
			OSType typeList[1];
			typeList[0] = 'TEXT';
			sfFile[0] = EOS;
			if (myNavGetFile(nil, -1, typeList, nil, sfFile)) {
				strcpy(searchListFileName, sfFile);
				if (readSearchList(searchListFileName)) {
					if (isThereSearchList()) {
						SetDialogItemValue(userData->window, 12, 1);
						ControlCTRL(userData->window, 12, HiliteCtrl, 0);
					} else {
						searchListFileName[0] = EOS;
						ControlCTRL(userData->window, 12, HiliteCtrl, 255);
					}
					u_strcpy(templineW, searchListFileName, UTTLINELEN);
					itemCtrl = GetWindowItemAsControl(userData->window, 13);
					SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
					if (searchListFileName[0] != EOS)
						ControlCTRL(userData->window, 13, ShowCtrl, 0);
					else
						ControlCTRL(userData->window, 13, HideCtrl, 0);
				}
//				HIViewSetNeedsDisplay(GetWindowItemAsControl(userData->window, 13), true);
			}
			break;
	}
	
	return status;
}

char FindDialog(unCH *SearchString, char ActiveSDirection) {
	int			i;
	char		oldSearchWrap;
	ControlRef	SearchCtrl, iCtrl;
	WindowPtr	myDlg;
	usrData		userData;
	ControlKeyFilterUPP keyStrFilter = myStringFilter;

	myDlg = getNibWindow(CFSTR("Find Dialog"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;
	dial_isF1Pressed = FALSE;
	dial_isF2Pressed = FALSE;

	SearchCtrl = GetWindowItemAsControl(myDlg, 3);
	SetControlData(SearchCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	strcpy(templineW, SearchString);
	for (i=0; templineW[i] != EOS; i++) {
		if (templineW[i] == HIDEN_C)
			templineW[i] = 0x2022;
	}
	SetWindowUnicodeTextValue(SearchCtrl, TRUE, templineW, 0);
	if (*SearchString)
		SelectWindowItemText(myDlg, 3, 0, HUGE_INTEGER);

	SetDialogItemValue(myDlg, 4, !SearchFFlag);
	ControlCTRL(myDlg, 4, HiliteCtrl, ActiveSDirection);
	SetDialogItemValue(myDlg, 6, CaseSensSearch);
	SetDialogItemValue(myDlg, 7, SearchWrap);
	oldSearchWrap = SearchWrap;
	SetDialogItemValue(myDlg, 12, SearchFromList);
	ControlCTRL(myDlg, 12, HiliteCtrl, (isThereSearchList() ? 0 : 255));

	u_strcpy(templineW, searchListFileName, UTTLINELEN);
	iCtrl = GetWindowItemAsControl(myDlg, 13);
	SetWindowUnicodeTextValue(iCtrl, TRUE, templineW, 0);
	if (searchListFileName[0] != EOS)
		ControlCTRL(myDlg, 13, ShowCtrl, 0);
	else
		ControlCTRL(myDlg, 13, HideCtrl, 0);

	AdvanceKeyboardFocus(myDlg);
	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), FindDialogEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowUnicodeTextValue(SearchCtrl, SearchString, SPEAKERLEN);
		for (i=0; SearchString[i] != EOS; i++) {
			if (SearchString[i] == 0x2022)
				SearchString[i] = HIDEN_C;
		}

		SearchFFlag = !GetWindowBoolValue(myDlg, 4) > 0;
		CaseSensSearch = GetWindowBoolValue(myDlg, 6) > 0;
		SearchWrap = GetWindowBoolValue(myDlg, 7) > 0;
		SearchFromList = GetWindowBoolValue(myDlg, 12) > 0;
	}

	DisposeWindow(myDlg);

	if (userData.res == myOK) {
		if (!CaseSensSearch)
			uS.uppercasestr(SearchString, &dFnt, C_MBF);
		if (oldSearchWrap != SearchWrap)
			WriteCedPreference();
		return(TRUE);
	} else
		return(FALSE);
}

#define repUsrData struct replaceUserWindowData
struct replaceUserWindowData {
	WindowPtr window;
	short res;
	FNType *fname;
	ControlRef SearchCtrl;
	ControlRef ReplCtrl;
};

static pascal OSStatus ContReplaceDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	int		  res;
	ControlRef	itemCtrl;
	HICommand aCommand;
	UInt32 resEventKind;
	repUsrData *userData = (repUsrData *)inUserData;
	OSStatus status = eventNotHandledErr;

	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandCancel:
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myCANCEL;
					break;
				case 'ID07':
					itemCtrl = GetWindowItemAsControl(userData->window, 7);
					SetControl32BitValue(itemCtrl,!GetControl32BitValue(itemCtrl));
					break;
				case 'MYNO':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myNO;
					break;
				case kHICommandOK:
					//			QuitAppModalLoopForWindow(userData->window);
					//			userData->res = myOK;
				case 'RPLF':
					GetWindowUnicodeTextValue(userData->SearchCtrl, SearchString, SPEAKERLEN);
					GetWindowUnicodeTextValue(userData->ReplCtrl, ReplaceString, SPEAKERLEN);
					CaseSensSearch = GetWindowBoolValue(userData->window, 7) > 0;
					res = replaceAndFindNext(TRUE);
					if (res == 0) {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myCANCEL;
					} else if (res == 2) {
						replaceExecPos = 0;
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myOK;
					}
					break;
				case 'FNDN':
					GetWindowUnicodeTextValue(userData->SearchCtrl, SearchString, SPEAKERLEN);
					CaseSensSearch = GetWindowBoolValue(userData->window, 7) > 0;
					res = replaceAndFindNext(FALSE);
					if (res == 0) {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myCANCEL;
					} else if (res == 2) {
						replaceExecPos = 0;
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myOK;
					}
					break;
				case 'RPLA':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myOK;
					break;
				default:
					break;
			}
			break;
		case kEventClassKeyboard:
			switch ((resEventKind=GetEventKind(inEvent))) {
				case kEventRawKeyDown:
					char c;
					UInt32 mod;
					GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(mod), NULL, &mod);
					GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(c), NULL, &c);
					if (mod & cmdKey && (c == 'f' || c == 'F')) {
						GetWindowUnicodeTextValue(userData->SearchCtrl, SearchString, SPEAKERLEN);
						CaseSensSearch = GetWindowBoolValue(userData->window, 7) > 0;
						res = replaceAndFindNext(FALSE);
						if (res == 0) {
							QuitAppModalLoopForWindow(userData->window);
							userData->res = myCANCEL;
						} else if (res == 2) {
							replaceExecPos = 0;
							QuitAppModalLoopForWindow(userData->window);
							userData->res = myOK;
						}
					} else if (mod & cmdKey && (c == 'r' || c == 'R' || c == 'a' || c == 'A')) {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myOK;
					} else if (c == 's' || c == 'S' || c == ' ') {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myNO;
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return status;
}

char ContReplaceDialog(unCH *SearchString, unCH *ReplaceString) {
	ControlRef	SearchCtrl,
				ReplCtrl;
	WindowPtr	myDlg;
	repUsrData	userData;

	myDlg = getNibWindow(CFSTR("Continue Replace Dialog"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;
	dial_isF1Pressed = FALSE;
	dial_isF2Pressed = FALSE;

	SearchCtrl = GetWindowItemAsControl(myDlg, 3);
	userData.SearchCtrl = SearchCtrl;
	SetWindowUnicodeTextValue(SearchCtrl, TRUE, SearchString, 0);

	ReplCtrl = GetWindowItemAsControl(myDlg, 5);
	userData.ReplCtrl = ReplCtrl;
	SetWindowUnicodeTextValue(ReplCtrl, TRUE, ReplaceString, 0);

	AdvanceKeyboardFocus(myDlg);

	SetDialogItemValue(myDlg, 7, CaseSensSearch);

	EventTypeSpec eventTypeCP[] = {
									{kEventClassKeyboard, kEventRawKeyDown},
									{kEventClassCommand, kEventCommandProcess}
								  } ;
	InstallEventHandler(GetWindowEventTarget(myDlg), ContReplaceDialogEvents, 2, eventTypeCP, &userData, NULL);
//	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
//	InstallEventHandler(GetWindowEventTarget(myDlg), ContReplaceDialogEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowUnicodeTextValue(SearchCtrl, SearchString, SPEAKERLEN);
		GetWindowUnicodeTextValue(ReplCtrl, ReplaceString, SPEAKERLEN);
		CaseSensSearch = GetWindowBoolValue(myDlg, 7) > 0;
	} else if (userData.res == myNO)
		CaseSensSearch = GetWindowBoolValue(myDlg, 7) > 0;

	DisposeWindow(myDlg);

	if (userData.res == myOK) {
		return(2);
    } else if (userData.res == myNO) {
    	return(1);
	} else
		return(0);
}

static pascal OSStatus ReplaceDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	int			res;
	FNType		sfFile[FNSize];
	ControlRef	itemCtrl;
	OSType		typeList[1];
	HICommand	aCommand;
	UInt32		resEventKind;
	repUsrData	*userData = (repUsrData *)inUserData;
	OSStatus	status = eventNotHandledErr;

	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandCancel:
					// we got a valid click on the OK button so let's quit our local run loop
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myCANCEL;
					break;
				case 'ID07':
					itemCtrl = GetWindowItemAsControl(userData->window, 7);
					SetControl32BitValue(itemCtrl,!GetControl32BitValue(itemCtrl));
					break;
				case 'INCR':
					GetKeyboardFocus(userData->window, &itemCtrl);
					SetWindowUnicodeTextValue(itemCtrl, FALSE, NULL, '\r');
					break;
				case 'INTB':
					GetKeyboardFocus(userData->window, &itemCtrl);
					SetWindowUnicodeTextValue(itemCtrl, FALSE, NULL, '\t');
					break;
				case 'INBU':
					GetKeyboardFocus(userData->window, &itemCtrl);
					SetWindowUnicodeTextValue(itemCtrl, FALSE, NULL, 0x2022);
					break;
				case kHICommandOK:
//					QuitAppModalLoopForWindow(userData->window);
//					userData->res = myOK;
				case 'RPLF':
					GetWindowUnicodeTextValue(userData->SearchCtrl, SearchString, SPEAKERLEN);
					GetWindowUnicodeTextValue(userData->ReplCtrl, ReplaceString, SPEAKERLEN);
					CaseSensSearch = GetWindowBoolValue(userData->window, 7) > 0;
					res = replaceAndFindNext(TRUE);
					if (res == 0) {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myCANCEL;
					} else if (res == 2) {
						replaceExecPos = 0;
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myCANCEL;
					}
					break;
				case 'FNDN':
					GetWindowUnicodeTextValue(userData->SearchCtrl, SearchString, SPEAKERLEN);
					GetWindowUnicodeTextValue(userData->ReplCtrl, ReplaceString, SPEAKERLEN);
					CaseSensSearch = GetWindowBoolValue(userData->window, 7) > 0;
					res = replaceAndFindNext(FALSE);
					if (res == 0) {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myCANCEL;
					} else if (res == 2) {
						replaceExecPos = 0;
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myCANCEL;
					}
					break;
				case 'RPLA':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myOK;
					break;
				case 'USFL':
					if (userData->fname[0] == EOS) {
						typeList[0] = 'TEXT';
						strcpy(sfFile, userData->fname);
						if (myNavGetFile(nil, -1, typeList, nil, sfFile)) {
							strcpy(userData->fname, sfFile);
							QuitAppModalLoopForWindow(userData->window);
							ReplaceFromList = TRUE;
							userData->res = myNO;
						}
						u_strcpy(templineW, userData->fname, UTTLINELEN);
						itemCtrl = GetWindowItemAsControl(userData->window, 15);
						SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
						if (userData->fname[0] != EOS) {
							ControlCTRL(userData->window, 15, ShowCtrl, 0);
							ControlCTRL(userData->window, 16, HiliteCtrl, 0);
						} else {
							ControlCTRL(userData->window, 15, HideCtrl, 0);
							ControlCTRL(userData->window, 16, HiliteCtrl, 255);
						}
					} else {
						QuitAppModalLoopForWindow(userData->window);
						ReplaceFromList = TRUE;
						userData->res = myNO;
					}
					break;
				case 'FNDF':
					typeList[0] = 'TEXT';
					strcpy(sfFile, userData->fname);
					if (myNavGetFile(nil, -1, typeList, nil, sfFile)) {
						strcpy(userData->fname, sfFile);
						QuitAppModalLoopForWindow(userData->window);
						ReplaceFromList = TRUE;
						userData->res = myNO;
					}
					u_strcpy(templineW, userData->fname, UTTLINELEN);
					itemCtrl = GetWindowItemAsControl(userData->window, 15);
					SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
					if (userData->fname[0] != EOS) {
						ControlCTRL(userData->window, 15, ShowCtrl, 0);
						ControlCTRL(userData->window, 16, HiliteCtrl, 0);
					} else {
						ControlCTRL(userData->window, 15, HideCtrl, 0);
						ControlCTRL(userData->window, 16, HiliteCtrl, 255);
					}
					break;
				default:
					break;
			}
			break;
		case kEventClassKeyboard:
			switch ((resEventKind=GetEventKind(inEvent))) {
				case kEventRawKeyDown:
					char c;
					UInt32 mod;
					GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(mod), NULL, &mod);
					GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(c), NULL, &c);
					if (mod & cmdKey && (c == 'f' || c == 'F')) {
						GetWindowUnicodeTextValue(userData->SearchCtrl, SearchString, SPEAKERLEN);
						GetWindowUnicodeTextValue(userData->ReplCtrl, ReplaceString, SPEAKERLEN);
						CaseSensSearch = GetWindowBoolValue(userData->window, 7) > 0;
						res = replaceAndFindNext(FALSE);
						if (res == 0) {
							QuitAppModalLoopForWindow(userData->window);
							userData->res = myCANCEL;
						} else if (res == 2) {
							replaceExecPos = 0;
							QuitAppModalLoopForWindow(userData->window);
							userData->res = myCANCEL;
						}
					} else if (mod & cmdKey && (c == 'r' || c == 'R')) {
						QuitAppModalLoopForWindow(userData->window);
						userData->res = myOK;
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return status;
}

char ReplaceDialog(unCH *SearchString, unCH *ReplaceString, FNType *fname) {
	int			i;
	ControlRef	SearchCtrl,
				ReplCtrl,
				itemCtrl;
	WindowPtr	myDlg;
	repUsrData	userData;
	ControlKeyFilterUPP keyStrFilter = myStringFilter;
	extern char CaseSensSearch;

	myDlg = getNibWindow(CFSTR("Replace Dialog"));
	CenterWindow(myDlg, -1, -1);

	userData.window = myDlg;
	userData.res = 0;
	userData.fname = fname;
	userData.SearchCtrl = SearchCtrl;
	userData.ReplCtrl = ReplCtrl;
	dial_isF1Pressed = FALSE;
	dial_isF2Pressed = FALSE;
	ReplaceFromList = FALSE;

	SearchCtrl = GetWindowItemAsControl(myDlg, 3);
	userData.SearchCtrl = SearchCtrl;
	SetControlData(SearchCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	strcpy(templineW, SearchString);
	for (i=0; templineW[i] != EOS; i++) {
		if (templineW[i] == HIDEN_C)
			templineW[i] = 0x2022;
	}
	SetWindowUnicodeTextValue(SearchCtrl, TRUE, templineW, 0);
	if (*SearchString)
		SelectWindowItemText(myDlg, 3, 0, HUGE_INTEGER);
	ReplCtrl = GetWindowItemAsControl(myDlg, 5);
	userData.ReplCtrl = ReplCtrl;
	SetControlData(ReplCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	strcpy(templineW, ReplaceString);
	for (i=0; templineW[i] != EOS; i++) {
		if (templineW[i] == HIDEN_C)
			templineW[i] = 0x2022;
	}
	SetWindowUnicodeTextValue(ReplCtrl, TRUE, templineW, 0);
	AdvanceKeyboardFocus(myDlg);
	SetDialogItemValue(myDlg, 7, CaseSensSearch);
	u_strcpy(templineW, userData.fname, UTTLINELEN);
	itemCtrl = GetWindowItemAsControl(userData.window, 15);
	SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
	if (userData.fname[0] != EOS) {
		ControlCTRL(userData.window, 15, ShowCtrl, 0);
		ControlCTRL(userData.window, 16, HiliteCtrl, 0);
	} else {
		ControlCTRL(userData.window, 15, HideCtrl, 0);
		ControlCTRL(userData.window, 16, HiliteCtrl, 255);
	}
	EventTypeSpec eventTypeCP[] = {
									{kEventClassKeyboard, kEventRawKeyDown},
									{kEventClassCommand, kEventCommandProcess}
								  } ;
	InstallEventHandler(GetWindowEventTarget(myDlg), ReplaceDialogEvents, 2, eventTypeCP, &userData, NULL);
//	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
//	InstallEventHandler(GetWindowEventTarget(myDlg), ReplaceDialogEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	if (userData.res == myOK) {
		GetWindowUnicodeTextValue(SearchCtrl, SearchString, SPEAKERLEN);
		GetWindowUnicodeTextValue(ReplCtrl, ReplaceString, SPEAKERLEN);
		CaseSensSearch = GetWindowBoolValue(myDlg, 7) > 0;
    } else	if (userData.res == myNO) {
		CaseSensSearch = GetWindowBoolValue(myDlg, 7) > 0;
	}

	DisposeWindow(myDlg);

	if (userData.res == myOK)
		return(TRUE);
	else if (userData.res == myCANCEL)
		return(FALSE);
	else
		return(TRUE);
}
