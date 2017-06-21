#include "ced.h"
#include "c_clan.h"
#include "mac_commands.h"
#include "mac_dial.h"
#include "cl_search.h"

enum {
	INCLUDE,
	EXCLUDE
};

enum {
	FILE_B,
	WORD_B,
	CODE_B,
	PCODE_B,
	LCODE_B,
	MOR_B
};

#define MORSTLEN 100

static char col1 = INCLUDE, col2 = WORD_B, sto;
static wchar_t st21[MORSTLEN+1], st22[MORSTLEN+1], st23[MORSTLEN+1], 
				st24[MORSTLEN+1], st25[MORSTLEN+1], st26[MORSTLEN+1], st09[MORSTLEN+1];
static FNType searchFileName[FNSize];
static MenuItemIndex menuItem, numberPresets;

#define searchUsrData struct searchUserWindowData
struct searchUserWindowData {
	WindowPtr window;
	MenuRef mref40;
	ControlRef ctrl40;
	short res;
};

#define helpUsrData struct helpUserWindowData
struct helpUserWindowData {
	WindowPtr window;
	short res;
};

void InitSelectedSearch(void) {
	col1 = INCLUDE;
	col2 = WORD_B;
	searchFileName[0] = EOS;
	st09[0] = EOS;
	st21[0] = EOS;
	st22[0] = EOS;
	st23[0] = EOS;
	st24[0] = EOS;
	st25[0] = EOS;
	st26[0] = EOS;
	sto = FALSE;
	menuItem = 1;
	numberPresets = 1;
}

static pascal OSStatus HelpEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	helpUsrData *userData = (helpUsrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
	switch (aCommand.commandID) {
		case kHICommandOK: case kHICommandCancel:
			QuitAppModalLoopForWindow(userData->window);
			userData->res = myOK;
			break;
	}
	
	return status;
}

static void morHelpDialog(void) {
	WindowPtr		myDlg;
	helpUsrData		userData;
	
	myDlg = getNibWindow(CFSTR("morHelp"));
	CenterWindow(myDlg, -4, -4);
	showWindow(myDlg);
	userData.window = myDlg;
	userData.res = 0;
	
	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), HelpEvents, 1, &eventTypeCP, &userData, NULL);
	RunAppModalLoopForWindow(myDlg);
	
	DisposeWindow(myDlg);
}

static void removeBrackets(char chr, wchar_t *str) {
	int i;

	i = 0;
	if (str[i] == '[' || str[i] == '<') {
		i++;
		if (str[i] == chr)
			i++;
	}
	while (isSpace(str[i]))
		i++;
	if (i > 0)
		strcpy(str, str+i);
	i = strlen(str) - 1;
	if (str[i] == ']' || str[i] == '>')
		str[i] = EOS;
	uS.remblanks(str);
}

static char createWord(WindowPtr myDlg, char obr, char cbr, char code, char *toStr, wchar_t *fromStr) {
	int		i;
	wchar_t *wqt;

	if (GetWindowBoolValue(myDlg, 4) > 0) {
		col1 = INCLUDE;
		strcat(toStr, " +s");
	} else if (GetWindowBoolValue(myDlg, 5) > 0) {
		col1 = EXCLUDE;
		strcat(toStr, " -s");
	} else
		return(FALSE);
	wqt = strrchr(fromStr, '"');
	if (wqt == NULL)
		strcat(toStr, "\"");
	else
		strcat(toStr, "'");
	i = strlen(toStr);
	if (obr != '\0') {
		toStr[i++] = obr;
		if (code != '\0') {
			toStr[i++] = code;
			toStr[i++] = ' ';
		}
	}
	u_strcpy(toStr+i, fromStr, UTTLINELEN-i);
	i = strlen(toStr);
	if (cbr != '\0') {
		toStr[i++] = cbr;
		toStr[i] = EOS;
	}
	if (wqt == NULL)
		strcat(toStr, "\"");
	else
		strcat(toStr, "'");
	return(TRUE);
}

static void ctrlAllMor(WindowPtr myDlg, short toDo) {
	ControlCTRL(myDlg, 21, toDo, 0);
	ControlCTRL(myDlg, 22, toDo, 0);
	ControlCTRL(myDlg, 23, toDo, 0);
	ControlCTRL(myDlg, 24, toDo, 0);
	ControlCTRL(myDlg, 25, toDo, 0);
	ControlCTRL(myDlg, 26, toDo, 0);
	ControlCTRL(myDlg, 29, toDo, 0);
	ControlCTRL(myDlg, 30, toDo, 0);
	ControlCTRL(myDlg, 31, toDo, 0);
	ControlCTRL(myDlg, 32, toDo, 0);
	ControlCTRL(myDlg, 33, toDo, 0);
	ControlCTRL(myDlg, 34, toDo, 0);
	ControlCTRL(myDlg, 35, toDo, 0);
//	ControlCTRL(myDlg, 36, toDo, 0);
}

static void setAllMorText(WindowPtr myDlg, ControlKeyFilterUPP keyStrFilter, SInt32 id, wchar_t *inStr) {
	ControlRef	tCtrl;
	
	tCtrl = GetWindowItemAsControl(myDlg, id);
	SetControlData(tCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	SetWindowUnicodeTextValue(tCtrl, TRUE, inStr, 0);
}

static void setIDTextValue(WindowPtr myDlg, SInt32 id, wchar_t *inStr) {
	ControlRef	tCtrl;

	tCtrl = GetWindowItemAsControl(myDlg, id);
	SetWindowUnicodeTextValue(tCtrl, TRUE, inStr, 0);
}

static char createMor(WindowPtr win, char isFnd, char code, SInt32 id, char *toSt, wchar_t *stn, char n) {
	int		i;
	wchar_t	*bg, *eg, div;
	ControlRef	tCtrl;

	tCtrl = GetWindowItemAsControl(win, id);
	GetWindowUnicodeTextValue(tCtrl, templineW, UTTLINELEN);
	uS.remFrontAndBackBlanks(templineW);
	if (templineW[0] == EOS) {
		if (n == 0)
			strcpy(stn, templineW);
		return(isFnd);
	}
	if (n == 0) {
		strncpy(stn, templineW, MORSTLEN);
		stn[MORSTLEN] = EOS;
		eg = strchr(templineW, '|');
		if (eg != NULL)
			*eg = EOS;
		bg = templineW;
	} else {
		bg = templineW;
		for (i=0; i < n; i++) {
			bg = strchr(bg, '|');
			if (bg == NULL)
				return(isFnd);
			bg++;
		}
		eg = strchr(bg, '|');
		if (eg != NULL)
			*eg = EOS;
	}
	do {
		eg = strchr(bg, ',');
		if (eg != NULL)
			*eg = EOS;
		uS.remFrontAndBackBlanks(bg);
		if (*bg == '+') {
			div = '+';
			bg++;
		} else
			div = '-';
		if (*bg != EOS) {
			if (isFnd)
				strcat(toSt, ",");
			i = strlen(toSt);
			if (*bg == '@' && (*(bg+1) == '+' || *(bg+1) == '-')) {
			} else {
				toSt[i++] = code;
				toSt[i++] = div;
			}
			u_strcpy(toSt+i, bg, UTTLINELEN);
			isFnd = TRUE;
		}
		if (eg != NULL)
			bg = eg + 1;
		else {
			bg = NULL;
			break;
		}
	} while (bg != NULL) ;
	return(isFnd);
}	

static void addToMenu(MenuRef mref, int i, const char *str) {
	CFStringRef cString;

	cString = my_CFStringCreateWithBytes(str);
	if (cString != NULL) {
		InsertMenuItemTextWithCFString(mref, cString, i, kMenuItemAttrCustomDraw, i+1);
		CFRelease(cString);
	}
}

static pascal OSStatus SearchDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	FNType		sfFile[FNSize];
	HICommand aCommand;
	HIViewRef itemCtrl;
	searchUsrData *userData = (searchUsrData *)inUserData;
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
				case 'ID04': // include
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 4, 1);
					SetDialogItemValue(userData->window, 5, 0);
					break;
				case 'ID05': //  exclude
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 5, 1);
					SetDialogItemValue(userData->window, 4, 0);
					break;
				case 'ID39': // "find" file button
					unCH *t;
					OSType typeList[1];
					SetDialogItemValue(userData->window, 6, 1);
					ControlCTRL(userData->window, 7, ShowCtrl, 0);
					SetDialogItemValue(userData->window, 8, 0);
					ControlCTRL(userData->window, 9, HideCtrl, 0);
					SetDialogItemValue(userData->window, 10, 0);
					SetDialogItemValue(userData->window, 11, 0);
					SetDialogItemValue(userData->window, 12, 0);
					ControlCTRL(userData->window, 13, HideCtrl, 0);
					ControlCTRL(userData->window, 14, HideCtrl, 0);
					SetDialogItemValue(userData->window, 20, 0);
					ctrlAllMor(userData->window, HideCtrl);
					typeList[0] = 'TEXT';
					sfFile[0] = EOS;
					if (myNavGetFile(nil, -1, typeList, nil, sfFile)) {
						strcpy(searchFileName, sfFile);
						u_strcpy(templineW, searchFileName, UTTLINELEN);
						t = strrchr(templineW, '/');
						if (t != NULL)
							strcpy(templineW, t+1);
						itemCtrl = GetWindowItemAsControl(userData->window, 7);
						SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
						Draw1Control(itemCtrl);
					}
					break;
				case 'ID06': // file
					SetDialogItemValue(userData->window, 6, 1);
					ControlCTRL(userData->window, 7, ShowCtrl, 0);
					SetDialogItemValue(userData->window, 8, 0);
					ControlCTRL(userData->window, 9, HideCtrl, 0);
					SetDialogItemValue(userData->window, 10, 0);
					SetDialogItemValue(userData->window, 11, 0);
					SetDialogItemValue(userData->window, 12, 0);
					ControlCTRL(userData->window, 13, HideCtrl, 0);
					ControlCTRL(userData->window, 14, HideCtrl, 0);
					SetDialogItemValue(userData->window, 20, 0);
					ctrlAllMor(userData->window, HideCtrl);
					if (searchFileName[0] == EOS) {
						typeList[0] = 'TEXT';
						sfFile[0] = EOS;
						if (myNavGetFile(nil, -1, typeList, nil, sfFile)) {
							strcpy(searchFileName, sfFile);
							u_strcpy(templineW, searchFileName, UTTLINELEN);
							t = strrchr(templineW, '/');
							if (t != NULL)
								strcpy(templineW, t+1);
							itemCtrl = GetWindowItemAsControl(userData->window, 7);
							SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
							Draw1Control(itemCtrl);
						}
					}
					break;
				case 'ID08': // word
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					SetDialogItemValue(userData->window, 6, 0);
					ControlCTRL(userData->window, 7, HideCtrl, 0);
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 8, 1);
					ControlCTRL(userData->window, 9, ShowCtrl, 0);
					SetDialogItemValue(userData->window, 10, 0);
					SetDialogItemValue(userData->window, 11, 0);
					SetDialogItemValue(userData->window, 12, 0);
					ControlCTRL(userData->window, 13, HideCtrl, 0);
					ControlCTRL(userData->window, 14, HideCtrl, 0);
					SetDialogItemValue(userData->window, 20, 0);
					ctrlAllMor(userData->window, HideCtrl);
					AdvanceKeyboardFocus(userData->window);
					break;
				case 'ID10': // code
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					SetDialogItemValue(userData->window, 6, 0);
					ControlCTRL(userData->window, 7, HideCtrl, 0);
					SetDialogItemValue(userData->window, 8, 0);
					ControlCTRL(userData->window, 9, ShowCtrl, 0);
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 10, 1);
					SetDialogItemValue(userData->window, 11, 0);
					SetDialogItemValue(userData->window, 12, 0);
					SetDialogItemValue(userData->window, 13, 1);
					SetDialogItemValue(userData->window, 14, 0);
					ControlCTRL(userData->window, 13, ShowCtrl, 0);
					ControlCTRL(userData->window, 14, ShowCtrl, 0);
					SetDialogItemValue(userData->window, 20, 0);
					ctrlAllMor(userData->window, HideCtrl);
					AdvanceKeyboardFocus(userData->window);
					break;
				case 'ID11': //  poastcode
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					SetDialogItemValue(userData->window, 6, 0);
					ControlCTRL(userData->window, 7, HideCtrl, 0);
					SetDialogItemValue(userData->window, 8, 0);
					ControlCTRL(userData->window, 9, ShowCtrl, 0);
					SetDialogItemValue(userData->window, 10, 0);
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 11, 1);
					SetDialogItemValue(userData->window, 12, 0);
					SetDialogItemValue(userData->window, 13, 0);
					SetDialogItemValue(userData->window, 14, 1);
					ControlCTRL(userData->window, 13, ShowCtrl, 0);
					ControlCTRL(userData->window, 14, ShowCtrl, 0);
					SetDialogItemValue(userData->window, 20, 0);
					ctrlAllMor(userData->window, HideCtrl);
					AdvanceKeyboardFocus(userData->window);
					break;
				case 'ID12': // language code
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					SetDialogItemValue(userData->window, 6, 0);
					ControlCTRL(userData->window, 7, HideCtrl, 0);
					SetDialogItemValue(userData->window, 8, 0);
					ControlCTRL(userData->window, 9, ShowCtrl, 0);
					SetDialogItemValue(userData->window, 10, 0);
					SetDialogItemValue(userData->window, 11, 0);
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 12, 1);
					SetDialogItemValue(userData->window, 13, 0);
					SetDialogItemValue(userData->window, 14, 1);
					ControlCTRL(userData->window, 13, ShowCtrl, 0);
					ControlCTRL(userData->window, 14, ShowCtrl, 0);
					SetDialogItemValue(userData->window, 20, 0);
					ctrlAllMor(userData->window, HideCtrl);
					AdvanceKeyboardFocus(userData->window);
					break;
				case 'ID13': // code itself
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 13, 1);
					SetDialogItemValue(userData->window, 14, 0);
					break;
				case 'ID14': // data associated with code
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					SetDialogItemValue(userData->window, 13, 0);
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 14, 1);
					break;
				case 'ID20': // %mor:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					SetDialogItemValue(userData->window, 6, 0);
					ControlCTRL(userData->window, 7, HideCtrl, 0);
					SetDialogItemValue(userData->window, 8, 0);
					SetDialogItemValue(userData->window, 10, 0);
					SetDialogItemValue(userData->window, 11, 0);
					SetDialogItemValue(userData->window, 12, 0);
					ControlCTRL(userData->window, 9, HideCtrl, 0);
					ControlCTRL(userData->window, 13, HideCtrl, 0);
					ControlCTRL(userData->window, 14, HideCtrl, 0);
					ctrlAllMor(userData->window, ShowCtrl);
					if (GetControl32BitValue(itemCtrl) == 0L) {
						SetDialogItemValue(userData->window, 20, 1);
					}
					AdvanceKeyboardFocus(userData->window);
					break;
				case 'ID29': // remove all empty elements
					if (GetWindowBoolValue(userData->window, 29) > 0)
						SetDialogItemValue(userData->window, 29, 0);
					else
						SetDialogItemValue(userData->window, 29, 1);
					break;
				case 'ID45': // %mor: Help dialog
					morHelpDialog();
					break;
				default:
					break;
			}
			break;
		case kEventClassControl:
			Rect  tempRect;
			Point theMouse;
			ControlID outID;
			Str255 itemString;
			MenuItemIndex tMenuItem;

			if (userData->mref40 != NULL) {
				GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &itemCtrl);
				if (GetControlID(itemCtrl, &outID) == noErr) {
					if (outID.id == 40) {
						itemCtrl = GetWindowItemAsControl(userData->window, 20);
						if (GetControl32BitValue(itemCtrl) == 0L) {
							SetDialogItemValue(userData->window, 6, 0);
							ControlCTRL(userData->window, 7, HideCtrl, 0);
							SetDialogItemValue(userData->window, 8, 0);
							SetDialogItemValue(userData->window, 10, 0);
							SetDialogItemValue(userData->window, 11, 0);
							SetDialogItemValue(userData->window, 12, 0);
							ControlCTRL(userData->window, 9, HideCtrl, 0);
							ControlCTRL(userData->window, 13, HideCtrl, 0);
							ControlCTRL(userData->window, 14, HideCtrl, 0);
							ctrlAllMor(userData->window, ShowCtrl);
							SetDialogItemValue(userData->window, 20, 1);
							AdvanceKeyboardFocus(userData->window);
							DrawControls(userData->window);
						}
						itemCtrl = GetWindowItemAsControl(userData->window, 40);
						GetControlBounds(itemCtrl, &tempRect);
						theMouse.h = tempRect.left;
						theMouse.v = tempRect.top;
						LocalToGlobal(&theMouse);
						SetMenuItemText(userData->mref40, 1, "\pChoose preset examples");
						tMenuItem = LoWord(PopUpMenuSelect(userData->mref40, theMouse.v, theMouse.h, menuItem));
						if (tMenuItem > 0)
							menuItem = tMenuItem;
						if (menuItem > 1 && menuItem <= numberPresets) {
							GetMenuItemText(userData->mref40, menuItem, itemString);
							SetMenuItemText(userData->mref40, 1, itemString);
							SetDialogItemValue(userData->window, 4, presets[menuItem-2].include);
							SetDialogItemValue(userData->window, 5, presets[menuItem-2].exclude);
							setIDTextValue(userData->window, 21, cl_T(presets[menuItem-2].POS));
							setIDTextValue(userData->window, 22, cl_T(presets[menuItem-2].stem));
							setIDTextValue(userData->window, 23, cl_T(presets[menuItem-2].prefix));
							setIDTextValue(userData->window, 24, cl_T(presets[menuItem-2].suffix));
							setIDTextValue(userData->window, 25, cl_T(presets[menuItem-2].fusion));
							setIDTextValue(userData->window, 26, cl_T(presets[menuItem-2].trans));
							SetDialogItemValue(userData->window, 29, presets[menuItem-2].oOption);
						} else
							menuItem = 1;
						Draw1Control(userData->ctrl40);
					}
				}
			}
			break;
		default:
			break;
	}
	return status;
}

void SearchDialog(WindowPtr win) {
	int			i;
	unCH		*t;
	char		isFound, isFound2, *qt;
	ControlRef	tCtrl, tCtrl9;
	WindowPtr	myDlg;
	IBNibRef	mNibs;
	Rect		tempRect;
	Str255		itemString;
	searchUsrData userData;
	ControlKeyFilterUPP keyStrFilter = myStringFilter;
	
	if (!set_fbuffer(win, FALSE)) {
		do_warning("Internal ERROR; no memory allocated to variable \"fbuffer\"", 0);
		return;
	}
	myDlg = getNibWindow(CFSTR("search_select"));
	CenterWindow(myDlg, -3, -3);
	userData.window = myDlg;
	userData.res = 0;
	dial_isF1Pressed = FALSE;
	dial_isF2Pressed = FALSE;
	userData.mref40 = NULL;
	if (CreateNibReference(CFSTR("CLAN"), &mNibs) == noErr) {
		if (CreateMenuFromNib(mNibs, CFSTR("pop_up"), &userData.mref40) != noErr)
			userData.mref40 = NULL;
		DisposeNibReference(mNibs);
	}
	userData.ctrl40 = NULL;
	if (userData.mref40 != NULL) {
		InsertMenuItemText(userData.mref40, "\pChoose preset examples", 0);
		InsertMenu(userData.mref40, -1);
		tCtrl = GetWindowItemAsControl(myDlg, 40);
		GetControlBounds(tCtrl, &tempRect);
		userData.ctrl40 = NewControl(myDlg,&tempRect,"\p",true,0,128,0,1008,0);
		SetControlPopupMenuHandle(userData.ctrl40, userData.mref40);
		for (i=0; presets[i].label != NULL; i++)
			addToMenu(userData.mref40, i+1, presets[i].label);
		numberPresets = i + 1;
	}
	if (col1 == EXCLUDE) {
		SetDialogItemValue(myDlg, 4, 0);
		SetDialogItemValue(myDlg, 5, 1);
	} else {
		SetDialogItemValue(myDlg, 4, 1);
		SetDialogItemValue(myDlg, 5, 0);
	}
	tCtrl9 = GetWindowItemAsControl(myDlg, 9);
	SetControlData(tCtrl9, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	SetWindowUnicodeTextValue(tCtrl9, TRUE, st09, 0);
	u_strcpy(templineW, searchFileName, UTTLINELEN);
	t = strrchr(templineW, '/');
	if (t != NULL)
		strcpy(templineW, t+1);
	tCtrl = GetWindowItemAsControl(myDlg, 7);
	SetWindowUnicodeTextValue(tCtrl, TRUE, templineW, 0);
	setAllMorText(myDlg, keyStrFilter, 21, st21);
	setAllMorText(myDlg, keyStrFilter, 22, st22);
	setAllMorText(myDlg, keyStrFilter, 23, st23);
	setAllMorText(myDlg, keyStrFilter, 24, st24);
	setAllMorText(myDlg, keyStrFilter, 25, st25);
	setAllMorText(myDlg, keyStrFilter, 26, st26);
	SetDialogItemValue(myDlg, 29, sto);
	menuItem = 1;
	if (menuItem > 1 && menuItem <= numberPresets) {
		GetMenuItemText(userData.mref40, menuItem, itemString);
		SetMenuItemText(userData.mref40, 1, itemString);
	} else
		menuItem = 1;
	if (col2 == FILE_B) {
		SetDialogItemValue(myDlg, 6, 1);
		SetDialogItemValue(myDlg, 8, 0);
		SetDialogItemValue(myDlg, 10, 0);
		SetDialogItemValue(myDlg, 11, 0);
		SetDialogItemValue(myDlg, 12, 0);
		SetDialogItemValue(myDlg, 13, 0);
		SetDialogItemValue(myDlg, 14, 0);
		SetDialogItemValue(myDlg, 20, 0);
		ControlCTRL(myDlg, 7, ShowCtrl, 0);
		ControlCTRL(myDlg, 9, HideCtrl, 0);
		ControlCTRL(myDlg, 13, HideCtrl, 0);
		ControlCTRL(myDlg, 14, HideCtrl, 0);
		ctrlAllMor(myDlg, HideCtrl);
	} else if (col2 == MOR_B) {
		SetDialogItemValue(myDlg, 6, 0);
		SetDialogItemValue(myDlg, 8, 0);
		SetDialogItemValue(myDlg, 10, 0);
		SetDialogItemValue(myDlg, 11, 0);
		SetDialogItemValue(myDlg, 12, 0);
		SetDialogItemValue(myDlg, 13, 0);
		SetDialogItemValue(myDlg, 14, 0);
		SetDialogItemValue(myDlg, 20, 1);
		ControlCTRL(myDlg, 7, HideCtrl, 0);
		ControlCTRL(myDlg, 9, HideCtrl, 0);
		ControlCTRL(myDlg, 13, HideCtrl, 0);
		ControlCTRL(myDlg, 14, HideCtrl, 0);
		ctrlAllMor(myDlg, ShowCtrl);
	} else if (col2 == CODE_B) {
		SetDialogItemValue(myDlg, 6, 0);
		SetDialogItemValue(myDlg, 8, 0);
		SetDialogItemValue(myDlg, 10, 1);
		SetDialogItemValue(myDlg, 11, 0);
		SetDialogItemValue(myDlg, 12, 0);
		SetDialogItemValue(myDlg, 13, 1);
		SetDialogItemValue(myDlg, 14, 0);
		SetDialogItemValue(myDlg, 20, 0);
		ControlCTRL(myDlg, 7, HideCtrl, 0);
		ControlCTRL(myDlg, 9, ShowCtrl, 0);
		ControlCTRL(myDlg, 13, ShowCtrl, 0);
		ControlCTRL(myDlg, 14, ShowCtrl, 0);
		ctrlAllMor(myDlg, HideCtrl);
	} else if (col2 == PCODE_B) {
		SetDialogItemValue(myDlg, 6, 0);
		SetDialogItemValue(myDlg, 8, 0);
		SetDialogItemValue(myDlg, 10, 0);
		SetDialogItemValue(myDlg, 11, 1);
		SetDialogItemValue(myDlg, 12, 0);
		SetDialogItemValue(myDlg, 13, 0);
		SetDialogItemValue(myDlg, 14, 1);
		SetDialogItemValue(myDlg, 20, 0);
		ControlCTRL(myDlg, 7, HideCtrl, 0);
		ControlCTRL(myDlg, 9, ShowCtrl, 0);
		ControlCTRL(myDlg, 13, ShowCtrl, 0);
		ControlCTRL(myDlg, 14, ShowCtrl, 0);
		ctrlAllMor(myDlg, HideCtrl);
	} else if (col2 == LCODE_B) {
		SetDialogItemValue(myDlg, 6, 0);
		SetDialogItemValue(myDlg, 8, 0);
		SetDialogItemValue(myDlg, 10, 0);
		SetDialogItemValue(myDlg, 11, 0);
		SetDialogItemValue(myDlg, 12, 1);
		SetDialogItemValue(myDlg, 13, 0);
		SetDialogItemValue(myDlg, 14, 1);
		SetDialogItemValue(myDlg, 20, 0);
		ControlCTRL(myDlg, 7, HideCtrl, 0);
		ControlCTRL(myDlg, 9, ShowCtrl, 0);
		ControlCTRL(myDlg, 13, ShowCtrl, 0);
		ControlCTRL(myDlg, 14, ShowCtrl, 0);
		ctrlAllMor(myDlg, HideCtrl);
	} else {
		SetDialogItemValue(myDlg, 6, 0);
		SetDialogItemValue(myDlg, 8, 1);
		SetDialogItemValue(myDlg, 10, 0);
		SetDialogItemValue(myDlg, 11, 0);
		SetDialogItemValue(myDlg, 12, 0);
		SetDialogItemValue(myDlg, 13, 0);
		SetDialogItemValue(myDlg, 14, 0);
		SetDialogItemValue(myDlg, 20, 0);
		ControlCTRL(myDlg, 7, HideCtrl, 0);
		ControlCTRL(myDlg, 9, ShowCtrl, 0);
		ControlCTRL(myDlg, 13, HideCtrl, 0);
		ControlCTRL(myDlg, 14, HideCtrl, 0);
		ctrlAllMor(myDlg, HideCtrl);
	}
	AdvanceKeyboardFocus(myDlg);
	EventTypeSpec eventTypeCP[] = {
		{kEventClassControl, kEventControlClick},
		{kEventClassCommand, kEventCommandProcess}
	} ;
	InstallEventHandler(GetWindowEventTarget(myDlg), SearchDialogEvents, 2, eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	isFound = FALSE;
	strcpy(templineC, fbuffer);
	if (userData.res == myOK) {
		if (GetWindowBoolValue(myDlg, 8) > 0) {
			col2 = WORD_B;
			GetWindowUnicodeTextValue(tCtrl9, templineW, UTTLINELEN);
			uS.remFrontAndBackBlanks(templineW);
			if (templineW[0] != EOS) {
				strncpy(st09, templineW, MORSTLEN);
				st09[MORSTLEN] = EOS;
				if (templineW[0] == '@' && isalpha(templineW[1])) {
					uS.shiftright(templineW, 1);
					templineW[0] = '*';
				}
				if (createWord(myDlg, '\0', '\0', '\0', templineC, templineW))
					isFound = TRUE;
			}
		} else if (GetWindowBoolValue(myDlg, 10) > 0) {
			col2 = CODE_B;
			GetWindowUnicodeTextValue(tCtrl9, templineW, UTTLINELEN);
			uS.remFrontAndBackBlanks(templineW);
			if (templineW[0] == '@' && isalpha(templineW[1])) {
				strncpy(st09, templineW, MORSTLEN);
				st09[MORSTLEN] = EOS;
				uS.shiftright(templineW, 1);
				templineW[0] = '*';
				if (createWord(myDlg, '\0', '\0', '\0', templineC, templineW))
					isFound = TRUE;
			} else {
				removeBrackets('\0', templineW);
				if (templineW[0] != EOS) {
					strncpy(st09, templineW, MORSTLEN);
					st09[MORSTLEN] = EOS;
					if (GetWindowBoolValue(myDlg, 13) > 0) {
						if (createWord(myDlg, '[', ']', '\0', templineC, templineW))
							isFound = TRUE;
					}
					if (GetWindowBoolValue(myDlg, 14) > 0) {
						if (createWord(myDlg, '<', '>', '\0', templineC, templineW))
							isFound = TRUE;
					}
				}
			}
		} else if (GetWindowBoolValue(myDlg, 11) > 0) {
			col2 = PCODE_B;
			GetWindowUnicodeTextValue(tCtrl9, templineW, UTTLINELEN);
			uS.remFrontAndBackBlanks(templineW);
			removeBrackets('+', templineW);
			if (templineW[0] != EOS) {
				strncpy(st09, templineW, MORSTLEN);
				st09[MORSTLEN] = EOS;
				if (GetWindowBoolValue(myDlg, 13) > 0) {
					if (createWord(myDlg, '<', '>', '+', templineC, templineW))
						isFound = TRUE;
				}
				if (GetWindowBoolValue(myDlg, 14) > 0) {
					if (createWord(myDlg, '[', ']', '+', templineC, templineW))
						isFound = TRUE;
				}
			}
		} else if (GetWindowBoolValue(myDlg, 12) > 0) {
			col2 = LCODE_B;
			GetWindowUnicodeTextValue(tCtrl9, templineW, UTTLINELEN);
			uS.remFrontAndBackBlanks(templineW);
			removeBrackets('-', templineW);
			if (templineW[0] != EOS) {
				strncpy(st09, templineW, MORSTLEN);
				st09[MORSTLEN] = EOS;
				if (GetWindowBoolValue(myDlg, 13) > 0) {
					if (createWord(myDlg, '<', '>', '-', templineC, templineW))
						isFound = TRUE;
				}
				if (GetWindowBoolValue(myDlg, 14) > 0) {
					if (createWord(myDlg, '[', ']', '-', templineC, templineW))
						isFound = TRUE;
				}
			}
		} else if (GetWindowBoolValue(myDlg, 20) > 0) {
			col2 = MOR_B;
			for (i=0; i < 4; i++) {
				isFound2 = FALSE;
				if (GetWindowBoolValue(myDlg, 4) > 0) {
					col1 = INCLUDE;
					strcpy(templineC2, " +s\"@");
				} else if (GetWindowBoolValue(myDlg, 5) > 0) {
					col1 = EXCLUDE;
					strcpy(templineC2, " -s\"@");
				} else
					break;
				isFound2 = createMor(myDlg, isFound2, '|', 21, templineC2, st21, i);
				isFound2 = createMor(myDlg, isFound2, 'r', 22, templineC2, st22, i);
				isFound2 = createMor(myDlg, isFound2, '#', 23, templineC2, st23, i);
				isFound2 = createMor(myDlg, isFound2, '-', 24, templineC2, st24, i);
				isFound2 = createMor(myDlg, isFound2, '&', 25, templineC2, st25, i);
				isFound2 = createMor(myDlg, isFound2, '=', 26, templineC2, st26, i);
				if (GetWindowBoolValue(myDlg, 29) > 0)
					strcat(templineC2, ",o%");
				strcat(templineC2, "\"");
				if (isFound2) {
					strcat(templineC, templineC2);
					isFound = TRUE;
				}
			}
			if (isFound) {
				if (GetWindowBoolValue(myDlg, 29) > 0)
					sto = TRUE;
				else
					sto = FALSE;
			}
		} else if (GetWindowBoolValue(myDlg, 6) > 0) {
			col2 = FILE_B;
			if (GetWindowBoolValue(myDlg, 4) > 0) {
				col1 = INCLUDE;
				strcat(templineC, " +s");
			} else if (GetWindowBoolValue(myDlg, 5) > 0) {
				col1 = EXCLUDE;
				strcat(templineC, " -s");
			}
			uS.remFrontAndBackBlanks(searchFileName);
			qt = strrchr(searchFileName, '"');
			if (qt == NULL)
				strcat(templineC, "\"@");
			else
				strcat(templineC, "'@");
			strcat(templineC, searchFileName);
			if (qt == NULL)
				strcat(templineC, "\"");
			else
				strcat(templineC, "'");
			if (searchFileName[0] != EOS)
				isFound = TRUE;
		}
	}
	if (userData.ctrl40 != NULL) {
		DisposeControl(userData.ctrl40);
	}
	if (userData.mref40 != NULL) {
		if ((i=CountMenuItems(userData.mref40)) > 0) {
			for (; i > 0; i--)
				DeleteMenuItem(userData.mref40, i-1);
		}
		DeleteMenu(GetMenuID(userData.mref40));
		DisposeMenu(userData.mref40);
	}
	DisposeWindow(myDlg);
	if (isFound)
		AddComStringToComWin(templineC, 0);	
}
