#include "ced.h"
#include "cu.h"
#include "mac_commands.h"
#include "mac_dial.h"

#define TIERNAMELEN 128

enum {
	HEADT,
	MAINT,
	DEPT,
	ROLE,
	IDS
};
enum {
	EXCLUDEALL,
	INCLUDE1,
	EXCLUDE1
};
enum {
	CANCEL,
	FILES_PAT,
	FILES_IN
};

struct SelectedTiers {
	char type;
	char name[TIERNAMELEN];
	char what2do;
	wchar_t role[TIERNAMELEN+1];
} ;

//#define DIAL_SPEAKER_FILED_LEN 30
#define DIAL_NUMBER_TIERS 48

#define CODESTRLEN  6
struct TiersListS {
	char code[CODESTRLEN+1];
	char *ID;
	int  num;
	struct TiersListS *next_tier;
} ;

#define tierUsrData struct tierUserWindowData
struct tierUserWindowData {
	WindowPtr window;
	short res;
};

extern int  F_numfiles;
extern int  cl_argc;
extern char *cl_argv[];

static char FilePattern[256+1];
//static 	time_t GlobalTime;
static struct TiersListS *spTiersRoot;
static struct TiersListS *depTiersRoot;

static struct SelectedTiers STier;

void InitSelectedTiers(void) {
	STier.type = MAINT;
	STier.name[0] = EOS;
	STier.what2do = INCLUDE1;
	STier.role[0] = EOS;
	strcpy(FilePattern, "*.cha");
}

static pascal OSStatus ClassicTiersDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	HIViewRef itemCtrl;
	tierUsrData *userData = (tierUsrData *)inUserData;
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
				case 'ID12':
					// we got a valid click on the OK button so let's quit our local run loop
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myNO;
					break;
				case 'ID04':
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 4, 1);
					SetDialogItemValue(userData->window, 5, 0);
					SetDialogItemValue(userData->window, 9, 0);
					ControlCTRL(userData->window, 3, ShowCtrl, 0);
					ControlCTRL(userData->window, 6, ShowCtrl, 0);
					ControlCTRL(userData->window, 7, ShowCtrl, 0);
					ControlCTRL(userData->window, 8, ShowCtrl, 0);
					ControlCTRL(userData->window, 10, ShowCtrl, 0);
//					ControlCTRL(userData->window, 11, ShowCtrl, 0);
					AdvanceKeyboardFocus(userData->window);
					SelectWindowItemText(userData->window, 3, HUGE_INTEGER, HUGE_INTEGER);
					break;
				case 'ID05':
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 5, 1);
					SetDialogItemValue(userData->window, 4, 0);
					SetDialogItemValue(userData->window, 9, 0);
					ControlCTRL(userData->window, 3, ShowCtrl, 0);
					ControlCTRL(userData->window, 6, ShowCtrl, 0);
					ControlCTRL(userData->window, 7, ShowCtrl, 0);
					ControlCTRL(userData->window, 8, ShowCtrl, 0);
					ControlCTRL(userData->window, 10, ShowCtrl, 0);
//					ControlCTRL(userData->window, 11, ShowCtrl, 0);
					AdvanceKeyboardFocus(userData->window);
					SelectWindowItemText(userData->window, 3, HUGE_INTEGER, HUGE_INTEGER);
					break;
				case 'ID06':
					strcpy(templineW, "*");
					itemCtrl = GetWindowItemAsControl(userData->window, 3);
					SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
					SelectWindowItemText(userData->window, 3, HUGE_INTEGER, HUGE_INTEGER);
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 6, 1);
					SetDialogItemValue(userData->window, 7, 0);
					SetDialogItemValue(userData->window, 8, 0);
					SetDialogItemValue(userData->window, 10, 0);
//					SetDialogItemValue(userData->window, 11, 0);
					break;
				case 'ID07':
					strcpy(templineW, "%");
					itemCtrl = GetWindowItemAsControl(userData->window, 3);
					SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
					SelectWindowItemText(userData->window, 3, HUGE_INTEGER, HUGE_INTEGER);
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 7, 1);
					SetDialogItemValue(userData->window, 6, 0);
					SetDialogItemValue(userData->window, 8, 0);
					SetDialogItemValue(userData->window, 10, 0);
//					SetDialogItemValue(userData->window, 11, 0);
					break;
				case 'ID08':
					strcpy(templineW, "@");
					itemCtrl = GetWindowItemAsControl(userData->window, 3);
					SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
					SelectWindowItemText(userData->window, 3, HUGE_INTEGER, HUGE_INTEGER);
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 8, 1);
					SetDialogItemValue(userData->window, 6, 0);
					SetDialogItemValue(userData->window, 7, 0);
					SetDialogItemValue(userData->window, 10, 0);
//					SetDialogItemValue(userData->window, 11, 0);
					break;
				case 'ID10':
					strcpy(templineW, STier.role);
					itemCtrl = GetWindowItemAsControl(userData->window, 3);
					SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
					SelectWindowItemText(userData->window, 3, HUGE_INTEGER, HUGE_INTEGER);
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 10, 1);
					SetDialogItemValue(userData->window, 6, 0);
					SetDialogItemValue(userData->window, 7, 0);
					SetDialogItemValue(userData->window, 8, 0);
//					SetDialogItemValue(userData->window, 11, 0);
					break;
/*
				case 'ID11':
					templineW[0] = EOS;
					itemCtrl = GetWindowItemAsControl(userData->window, 3);
					SetWindowUnicodeTextValue(itemCtrl, TRUE, templineW, 0);
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 11, 1);
					SetDialogItemValue(userData->window, 6, 0);
					SetDialogItemValue(userData->window, 7, 0);
					SetDialogItemValue(userData->window, 8, 0);
					SetDialogItemValue(userData->window, 10, 0);
					break;
*/
				case 'ID09':
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, 9, 1);
					SetDialogItemValue(userData->window, 4, 0);
					SetDialogItemValue(userData->window, 5, 0);
					ControlCTRL(userData->window, 3, HideCtrl, 0);
					ControlCTRL(userData->window, 6, HideCtrl, 0);
					ControlCTRL(userData->window, 7, HideCtrl, 0);
					ControlCTRL(userData->window, 8, HideCtrl, 0);
					ControlCTRL(userData->window, 10, HideCtrl, 0);
//					ControlCTRL(userData->window, 11, HideCtrl, 0);
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

static char ClassicTiersDialog(void) {
	int			i;
	char		isFound, *isSpCharFound;
	ControlRef	tCtrl;
	WindowPtr	myDlg;
	tierUsrData	userData;
	ControlKeyFilterUPP keyStrFilter = myStringFilter;

	myDlg = getNibWindow(CFSTR("tier_select"));
	CenterWindow(myDlg, -3, -3);
	userData.window = myDlg;
	userData.res = 0;
	dial_isF1Pressed = FALSE;
	STier.name[0] = EOS;
	tCtrl = GetWindowItemAsControl(myDlg, 3);
	SetControlData(tCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	if (*templineW != EOS)
		SelectWindowItemText(myDlg, 3, 0, HUGE_INTEGER);
	AdvanceKeyboardFocus(myDlg);
	if (STier.what2do == EXCLUDEALL) {
    	SetDialogItemValue(myDlg, 4, 0);
    	SetDialogItemValue(myDlg, 5, 0);
    	SetDialogItemValue(myDlg, 9, 1);
		ControlCTRL(myDlg, 3, HideCtrl, 0);
		ControlCTRL(myDlg, 6, HideCtrl, 0);
		ControlCTRL(myDlg, 7, HideCtrl, 0);
		ControlCTRL(myDlg, 8, HideCtrl, 0);
		ControlCTRL(myDlg, 10, HideCtrl, 0);
//		ControlCTRL(myDlg, 11, HideCtrl, 0);
	} else if (STier.what2do == INCLUDE1) {
    	SetDialogItemValue(myDlg, 4, 1);
    	SetDialogItemValue(myDlg, 5, 0);
    	SetDialogItemValue(myDlg, 9, 0);
		ControlCTRL(myDlg, 3, ShowCtrl, 0);
		ControlCTRL(myDlg, 6, ShowCtrl, 0);
		ControlCTRL(myDlg, 7, ShowCtrl, 0);
		ControlCTRL(myDlg, 8, ShowCtrl, 0);
		ControlCTRL(myDlg, 10, ShowCtrl, 0);
//		ControlCTRL(myDlg, 11, ShowCtrl, 0);
	} if (STier.what2do == EXCLUDE1) {
    	SetDialogItemValue(myDlg, 4, 0);
    	SetDialogItemValue(myDlg, 5, 1);
    	SetDialogItemValue(myDlg, 9, 0);
		ControlCTRL(myDlg, 3, ShowCtrl, 0);
		ControlCTRL(myDlg, 6, ShowCtrl, 0);
		ControlCTRL(myDlg, 7, ShowCtrl, 0);
		ControlCTRL(myDlg, 8, ShowCtrl, 0);
		ControlCTRL(myDlg, 10, ShowCtrl, 0);
//		ControlCTRL(myDlg, 11, ShowCtrl, 0);
	}
	if (STier.type == MAINT) {
		strcpy(templineW, "*");
		tCtrl = GetWindowItemAsControl(myDlg, 3);
		SetWindowUnicodeTextValue(tCtrl, TRUE, templineW, 0);
		SelectWindowItemText(myDlg, 3, HUGE_INTEGER, HUGE_INTEGER);
		SetDialogItemValue(myDlg, 6, 1);
		SetDialogItemValue(myDlg, 7, 0);
		SetDialogItemValue(myDlg, 8, 0);
		SetDialogItemValue(myDlg, 10, 0);
//		SetDialogItemValue(myDlg, 11, 0);
	} else if (STier.type == DEPT) {
		strcpy(templineW, "%");
		tCtrl = GetWindowItemAsControl(myDlg, 3);
		SetWindowUnicodeTextValue(tCtrl, TRUE, templineW, 0);
		SelectWindowItemText(myDlg, 3, HUGE_INTEGER, HUGE_INTEGER);
		SetDialogItemValue(myDlg, 6, 0);
		SetDialogItemValue(myDlg, 7, 1);
		SetDialogItemValue(myDlg, 8, 0);
		SetDialogItemValue(myDlg, 10, 0);
//		SetDialogItemValue(myDlg, 11, 0);
	} else if (STier.type == HEADT) {
		strcpy(templineW, "@");
		tCtrl = GetWindowItemAsControl(myDlg, 3);
		SetWindowUnicodeTextValue(tCtrl, TRUE, templineW, 0);
		SelectWindowItemText(myDlg, 3, HUGE_INTEGER, HUGE_INTEGER);
		SetDialogItemValue(myDlg, 6, 0);
		SetDialogItemValue(myDlg, 7, 0);
		SetDialogItemValue(myDlg, 8, 1);
		SetDialogItemValue(myDlg, 10, 0);
//		SetDialogItemValue(myDlg, 11, 0);
	} else if (STier.type == ROLE) {
		strcpy(templineW, STier.role);
		tCtrl = GetWindowItemAsControl(myDlg, 3);
		SetWindowUnicodeTextValue(tCtrl, TRUE, templineW, 0);
		SelectWindowItemText(myDlg, 3, HUGE_INTEGER, HUGE_INTEGER);
		SetDialogItemValue(myDlg, 6, 0);
		SetDialogItemValue(myDlg, 7, 0);
		SetDialogItemValue(myDlg, 8, 0);
		SetDialogItemValue(myDlg, 10, 1);
//		SetDialogItemValue(myDlg, 11, 0);
	}
/*
	else if (STier.type == IDS) {
		SetDialogItemValue(myDlg, 6, 0);
		SetDialogItemValue(myDlg, 7, 0);
		SetDialogItemValue(myDlg, 8, 0);
		SetDialogItemValue(myDlg, 10, 0);
		SetDialogItemValue(myDlg, 11, 1);
	}
*/
	EventTypeSpec eventTypeCP[] = {
		{kEventClassCommand, kEventCommandProcess}
	} ;
	InstallEventHandler(GetWindowEventTarget(myDlg), ClassicTiersDialogEvents, 1, eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);

	isFound = FALSE;
	strcpy(templineC, fbuffer);
	if (userData.res == myOK) {
		GetWindowUnicodeTextValue(tCtrl, templineW, TIERNAMELEN);
		u_strcpy(STier.name, templineW, TIERNAMELEN);
		uS.remFrontAndBackBlanks(STier.name);
		if (GetWindowBoolValue(myDlg, 9) > 0) {
			STier.what2do = EXCLUDEALL;
		} else {
			if (GetWindowBoolValue(myDlg, 4) > 0) {
				STier.what2do = INCLUDE1;
			} else if (GetWindowBoolValue(myDlg, 5) > 0) {
				STier.what2do = EXCLUDE1;
			}
			if (GetWindowBoolValue(myDlg, 6) > 0) {
				STier.type = MAINT;
			} else if (GetWindowBoolValue(myDlg, 7) > 0) {
				STier.type = DEPT;
			} else if (GetWindowBoolValue(myDlg, 8) > 0) {
				STier.type = HEADT;
			} else if (GetWindowBoolValue(myDlg, 10) > 0) {
				STier.type = ROLE;
			}
/*
			else if (GetWindowBoolValue(myDlg, 11) > 0) {
				STier.type = IDS;
			}
*/
		}
		if (STier.what2do == EXCLUDEALL) {
			strcat(templineC, " -t*");
			isFound = TRUE;
		} else {
			strcat(templineC, " ");
			if (STier.what2do == INCLUDE1) {
				strcat(templineC, "+t");
				isFound = TRUE;
			} else if (STier.what2do == EXCLUDE1) {
				strcat(templineC, "-t");
				isFound = TRUE;
			}
			isSpCharFound = strchr(STier.name, '|');
			if (isSpCharFound != NULL || STier.type == IDS)
				strcat(templineC, "\"");
			if (!uS.mStrnicmp(STier.name, "@ID=", 4) || !uS.mStrnicmp(STier.name, "ID=", 3))
				STier.type = HEADT;
			if (STier.name[0] == '#') {
			} else if (STier.type == MAINT) {
				strcat(templineC, "*");
				uS.uppercasestr(STier.name, NULL, FALSE);
			} else if (STier.type == DEPT) {
				strcat(templineC, "%");
				uS.lowercasestr(STier.name, NULL, FALSE);
			} else if (STier.type == HEADT) {
				strcat(templineC, "@");
			} else if (STier.type == ROLE) {
				strcat(templineC, "#");
				for (i=0; templineW[i] != EOS; i++) {
					if (templineW[i] == HIDEN_C)
						templineW[i] = 0x2022;
				}				
				strcpy(STier.role, templineW);
			} else if (STier.type == IDS) {
				strcat(templineC, "@ID=");
				if (STier.name[0] != '*')
					strcat(templineC, "*|");
			} else
				isFound = FALSE;
			if (STier.type != IDS && STier.type != ROLE) {
				for (i=0; STier.name[i] != EOS; i++) {
					if (STier.name[i] != '*' && STier.name[i] != '%' && STier.name[i] != '@' && !isSpace(STier.name[i]))
						break;
				}
			} else
				i = 0;
			strcat(templineC, STier.name+i);
			if (STier.type == IDS && STier.name[0] != '*')
				strcat(templineC, "|*");
			if (isSpCharFound != NULL || STier.type == IDS)
				strcat(templineC, "\"");
		}
	}
	DisposeWindow(myDlg);
	if (isFound)
		AddComStringToComWin(templineC, 0);
	if (userData.res == myNO)
		return(FALSE);
	else
		return(TRUE);
}

static pascal OSStatus SelectFilesDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	tierUsrData *userData = (tierUsrData *)inUserData;
	OSStatus status = eventNotHandledErr;
	
	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandOK:
					// we got a valid click on the OK button so let's quit our local run loop
					QuitAppModalLoopForWindow(userData->window);
					userData->res = FILES_IN;
					break;
				case kHICommandCancel:
					// we got a valid click on the OK button so let's quit our local run loop
					QuitAppModalLoopForWindow(userData->window);
					userData->res = CANCEL;
					break;
				case 'ID03':
					QuitAppModalLoopForWindow(userData->window);
					userData->res = FILES_PAT;
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

static short SelectFilesDialog(char *additions) {
	int			i;
	ControlRef	tCtrl;
	WindowPtr	myDlg;
	tierUsrData	userData;
	ControlKeyFilterUPP keyStrFilter = myStringFilter;
	
	myDlg = getNibWindow(CFSTR("tier_pre_select"));
	CenterWindow(myDlg, -3, -3);
	userData.window = myDlg;
	userData.res = 0;
	dial_isF1Pressed = FALSE;
	tCtrl = GetWindowItemAsControl(myDlg, 4);
	SetControlData(tCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(keyStrFilter), &keyStrFilter);
	strcpy(additions, FilePattern);
	u_strcpy(templineW, additions, UTTLINELEN);
	for (i=0; templineW[i] != EOS; i++) {
		if (templineW[i] == HIDEN_C)
			templineW[i] = 0x2022;
	}
	SetWindowUnicodeTextValue(tCtrl, TRUE, templineW, 0);
	if (*templineW != EOS)
		SelectWindowItemText(myDlg, 4, 0, HUGE_INTEGER);
	AdvanceKeyboardFocus(myDlg);
	EventTypeSpec eventTypeCP[] = {
		{kEventClassCommand, kEventCommandProcess}
	} ;
	InstallEventHandler(GetWindowEventTarget(myDlg), SelectFilesDialogEvents, 1, eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);
	GetWindowUnicodeTextValue(tCtrl, templineW, TIERNAMELEN);
	DisposeWindow(myDlg);
	if (userData.res == FILES_PAT) {
		u_strcpy(additions, templineW, TIERNAMELEN);
		uS.remFrontAndBackBlanks(additions);
		strncpy(FilePattern, additions, 256);
		FilePattern[256] = EOS;
	} else if (userData.res == FILES_IN)
		strcpy(additions, " @");
	else
		additions[0] = EOS;
	return(userData.res);
}

static struct TiersListS *cleanTiersList(struct TiersListS *p) {
	struct TiersListS *t;

	while (p != NULL) {
		t = p;
		p = p->next_tier;
		if (t->ID != NULL) {
			free(t->ID);
		}
		free(t);
	}
	return(NULL);
}

static struct TiersListS *addToTiersArr(struct TiersListS *root, short pixelLen, char *code, char *ID) {
	int IDlen, len;
	char *s;
	struct TiersListS *nt, *tnt;

	len = strlen(code);
	if (ID != NULL) {
		IDlen = strlen(ID);
//		if (ID[0] != '|')
//			IDlen++;
//		while (len+IDlen >= DIAL_SPEAKER_FILED_LEN) {
		pixelLen = pixelLen - TextWidth(code, 0, len);
		while (TextWidth(ID, 0, IDlen) > pixelLen) {
			if (ID[0] == '|')
				ID++;
			s = strchr(ID, '|');
			if (s == NULL) {
				ID = NULL;
				IDlen = 0;
				break;
			}
			ID = s;
			IDlen = strlen(ID);
		}
		len = len + IDlen;
	}
	if (root == NULL) {
		root = NEW(struct TiersListS);
		if (root == NULL)
			return(root);
		nt = root;
		nt->next_tier = NULL;
	} else {
		for (nt=root; nt != NULL; nt=nt->next_tier) {
			if (uS.mStricmp(nt->code, code) == 0)
				return(root);
		}
		tnt = root;
		nt = root;
		while (1) {
			if (nt == NULL)
				break;
			else if (nt->num < len)
				break;
			tnt = nt;
			nt = nt->next_tier;
		}
		if (nt == NULL) {
			nt = NEW(struct TiersListS);
			if (nt == NULL)
				return(root);
			tnt->next_tier = nt;
			nt->next_tier = NULL;
		} else if (nt == root) {
			tnt = NEW(struct TiersListS);
			if (tnt == NULL)
				return(root);
			root = tnt;
			root->next_tier = nt;
			nt = root;
		} else {
			nt = NEW(struct TiersListS);
			if (nt == NULL)
				return(root);
			nt->next_tier = tnt->next_tier;
			tnt->next_tier = nt;
		}
	}
	strncpy(nt->code, code, CODESTRLEN);
	nt->code[CODESTRLEN] = EOS;
	if (ID != NULL) {
		nt->ID = (char *)malloc(strlen(ID)+1);
		if (nt->ID != NULL)
			strcpy(nt->ID, ID);
	} else
		nt->ID = NULL;
	nt->num = len;
	return(root);
}

static char getTierNamesFromFile(ControlRef itemCtrl, int *cnt, short pixelLen, char *fname, char isFoundFile) {
	char	*s, *e, *code;
//	time_t	timer;
	FILE	*fp;

	fp = fopen(fname, "r");
	if (fp == NULL)
		return(isFoundFile);
	*cnt = *cnt + 1;
//	time(&timer);
//	if (timer > GlobalTime) {
		sprintf(templineC3, "Reading file #%d", *cnt);
		SetControlData(itemCtrl,kControlEntireControl,kControlEditTextTextTag,strlen(templineC3),templineC3);
		Draw1Control(itemCtrl);
//		GlobalTime = timer + 1;
//	}
	isFoundFile = TRUE;
	while (fgets_cr(templineC3, 3072, fp)) {
		if (templineC3[0] == '*') {
			s = strchr(templineC3, ':');
			if (s != NULL) {
				*s = EOS;
				spTiersRoot = addToTiersArr(spTiersRoot, pixelLen, templineC3, NULL);
			}
		} else if (templineC3[0] == '%') {
			s = strchr(templineC3, ':');
			if (s != NULL) {
				*s = EOS;
				depTiersRoot = addToTiersArr(depTiersRoot, pixelLen, templineC3, NULL);
			}
		} else if (strncmp(templineC3, "@ID:", 4) == 0) {
			s = strchr(templineC3, '|');
			if (s != NULL) {
				s++;
				code = strchr(s, '|');
				if (code != NULL) {
					*code = '*';
					s = strchr(code, '|');
					if (s != NULL) {
						uS.remblanks(code);
						*s = EOS;
						s++;
						for (e=s; *e != EOS; ) {
							if (*e == '|' && *(e+1) == '|')
								strcpy(e, e+1);
							else
								e++;
						}
						spTiersRoot = addToTiersArr(spTiersRoot, pixelLen, code, s);
					}
				}
			}
		}
	}
	fclose(fp);
	return(isFoundFile);
}

static char makeTiersList(ControlRef itemCtrl, int ProgNum, short pixelLen) {
	int		i, j, len, cnt;
	char	isFoundFile;
	FILE	*fp;

	SetNewVol(wd_dir);
	isFoundFile = FALSE;
	cnt = 0;
	for (i=1; i < cl_argc; i++) {
		if (cl_argv[i][0] == '-' || cl_argv[i][0] == '+') {
			if ((cl_argv[i][1] == 's' || cl_argv[i][1] == 'S') && ProgNum == CHSTRING)
				i++;
		} else {
			if (!strcmp(cl_argv[i], "@")) {
				for (j=1; j <= F_numfiles; j++) {
					get_selected_file(j, FileName1);
					isFoundFile = getTierNamesFromFile(itemCtrl, &cnt, pixelLen, FileName1, isFoundFile);
				}
			} else if (cl_argv[i][0] == '@' && cl_argv[i][1] == ':') {
				uS.str2FNType(FileName1, 0L, cl_argv[i]+2);
				fp = fopen(FileName1, "r");
				if (fp == NULL) {
					return(isFoundFile);
				}
				while (fgets_cr(FileName1, 3072, fp)) {
					uS.remFrontAndBackBlanks(FileName1);
					isFoundFile = getTierNamesFromFile(itemCtrl, &cnt, pixelLen, FileName1, isFoundFile);
				}
				fclose(fp);
			} else if (strchr(cl_argv[i], '*') == NULL) {
				isFoundFile = getTierNamesFromFile(itemCtrl, &cnt, pixelLen, cl_argv[i], isFoundFile);
			} else {
				strcpy(DirPathName, wd_dir);
				len = strlen(DirPathName);
				j = 1;
				while ((j=Get_File(FileName1, j)) != 0) {
					if (uS.fIpatmat(FileName1, cl_argv[i])) {
						addFilename2Path(DirPathName, FileName1);
						isFoundFile = getTierNamesFromFile(itemCtrl, &cnt, pixelLen, DirPathName, isFoundFile);
						DirPathName[len] = EOS;
					}
				}
			}
		}
	}
	return(isFoundFile);
}

static pascal OSStatus selectTiersDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	short	  val;
	HICommand aCommand;
	HIViewRef itemCtrl;
	struct TiersListS *p;
	tierUsrData *userData = (tierUsrData *)inUserData;
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
				case 51:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						val = 1;
					else
						val = 0;
					SetDialogItemValue(userData->window, 51, val);
					for (p=spTiersRoot; p != NULL; p=p->next_tier) {
						SetDialogItemValue(userData->window, p->num, val);
					}
					break;
				case 52:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						val = 1;
					else
						val = 0;
					SetDialogItemValue(userData->window, 52, val);
					for (p=depTiersRoot; p != NULL; p=p->next_tier) {
						SetDialogItemValue(userData->window, p->num, val);
					}
					break;
				default:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, aCommand.commandID, 1);
					else
						SetDialogItemValue(userData->window, aCommand.commandID, 0);
					break;
			}
			break;
		default:
			break;
	}
	return status;
}

static char isExcludeHeadFromProg(int ProgNum) {
	if (ProgNum == FREQMERGE || ProgNum == STATFREQ  || ProgNum == TEXTIN   || ProgNum == ANVIL2CHAT||
		ProgNum == COMBTIER  || ProgNum == ELAN2CHAT || ProgNum == LAB2CHAT || ProgNum == LIPP2CHAT ||
		ProgNum == OLAC_P    || ProgNum == PRAAT2CHAT|| ProgNum == RTFIN     || ProgNum == SALTIN    ||
		ProgNum == SUBTITLES || ProgNum == UNIQ      || ProgNum == COMPOUND || ProgNum == DOS2UNIX) 
		return(FALSE);
	return(TRUE);
}

static char isExcludeDepFromProg(int ProgNum) {
	if (ProgNum == CHAINS || ProgNum == FLO    || ProgNum == KEYMAP || ProgNum == MODREP  ||
		ProgNum == CHECK  || ProgNum == CP2UTF || ProgNum == IMDI_P || ProgNum == LOWCASE || 
		ProgNum == RETRACE) 
		return(FALSE);
	return(TRUE);
}

static char selectTiersDialog(int ProgNum) {
	int			i, len, oldHead;
	Str255		title;
	Rect		box;
	WindowPtr	myDlg;
	ControlRef	itemCtrl;
	tierUsrData	userData;
	struct TiersListS *p, *pp;

	myDlg = getNibWindow(CFSTR("tier_code_select"));
	CenterWindow(myDlg, -3, -3);
	userData.window = myDlg;
	userData.res = 0;
	dial_isF1Pressed = FALSE;
	itemCtrl = GetWindowItemAsControl(myDlg, 3);
	GetControlBounds(itemCtrl, &box);
	for (i=1; i < DIAL_NUMBER_TIERS; i++)
		ControlCTRL(myDlg, i, HideCtrl, 0);
	ControlCTRL(myDlg, 51, HideCtrl, 0);
	ControlCTRL(myDlg, 52, HideCtrl, 0);
	EventTypeSpec eventTypeCP[] = {
		{kEventClassCommand, kEventCommandProcess}
	} ;
	InstallEventHandler(GetWindowEventTarget(myDlg), selectTiersDialogEvents, 1, eventTypeCP, &userData, NULL);
	showWindow(myDlg);
//	GlobalTime = 0L;
	spTiersRoot = NULL;
	depTiersRoot = NULL;
	itemCtrl = GetWindowItemAsControl(myDlg, 50);
	if (makeTiersList(itemCtrl, ProgNum, box.right-box.left-25) == FALSE) {
		spTiersRoot = cleanTiersList(spTiersRoot);
		depTiersRoot = cleanTiersList(depTiersRoot);
		DisposeWindow(myDlg);
		return(FALSE);
	}
	ControlCTRL(myDlg, 50, HideCtrl, 0);
	itemCtrl = GetWindowItemAsControl(myDlg, 1);
	ShowControl(itemCtrl);
	SetControl32BitValue(itemCtrl, 0);
	itemCtrl = GetWindowItemAsControl(myDlg, 2);
	ShowControl(itemCtrl);
	if (isExcludeHeadFromProg(ProgNum)) {
		oldHead = 0;
		SetControl32BitValue(itemCtrl, 0);
	} else {
		oldHead = 1;
		SetControl32BitValue(itemCtrl, 1);
	}
	i = 3;
	pp = spTiersRoot;
	for (p=spTiersRoot; p != NULL && i < DIAL_NUMBER_TIERS; p=p->next_tier) {
		pp = p;
		itemCtrl = GetWindowItemAsControl(myDlg, i);
		strcpy((char *)title+1, p->code);
		if (p->ID != NULL && i < 18) {
			len = strlen(p->code);
			if (p->ID[0] != '|') {
				title[len+1] = '|';
				len++;
			}
			strcpy((char *)title+len+1, p->ID);
		}
		p->num = i;
		title[0] = strlen((char *)title+1);
		ShowControl(itemCtrl);
		SetControlTitle(itemCtrl, title);
		SetControl32BitValue(itemCtrl, 1);
		i++;
	}
	if (i >= DIAL_NUMBER_TIERS) {
		pp->next_tier = cleanTiersList(pp->next_tier);
		depTiersRoot = cleanTiersList(depTiersRoot);
	}
	if (i < 18)
		i = 18;
	pp = depTiersRoot;
	if (isExcludeDepFromProg(ProgNum)) {
		for (p=depTiersRoot; p != NULL && i < DIAL_NUMBER_TIERS; p=p->next_tier) {
			pp = p;
			p->num = i;
			strcpy((char *)title+1, p->code);
			title[0] = strlen((char *)title+1);
			itemCtrl = GetWindowItemAsControl(myDlg, i);
			ShowControl(itemCtrl);
			SetControlTitle(itemCtrl, title);
			if (strcmp(p->code, "%mor") == 0) {
				if (ProgNum == MLU || ProgNum == WDLEN  || ProgNum == MORTABLE || ProgNum == POSTMORTEM)
					SetControl32BitValue(itemCtrl, 1);
				else
					SetControl32BitValue(itemCtrl, 0);
			} else
				SetControl32BitValue(itemCtrl, 0);
			i++;
		}
		itemCtrl = GetWindowItemAsControl(myDlg, 52);
		ShowControl(itemCtrl);
		SetControl32BitValue(itemCtrl, 0);
	} else {
		for (p=depTiersRoot; p != NULL && i < DIAL_NUMBER_TIERS; p=p->next_tier) {
			pp = p;
			p->num = i;
			strcpy((char *)title+1, p->code);
			title[0] = strlen((char *)title+1);
			itemCtrl = GetWindowItemAsControl(myDlg, i);
			ShowControl(itemCtrl);
			SetControlTitle(itemCtrl, title);
			SetControl32BitValue(itemCtrl, 1);
			i++;
		}
		itemCtrl = GetWindowItemAsControl(myDlg, 52);
		ShowControl(itemCtrl);
		SetControl32BitValue(itemCtrl, 1);
	}
	itemCtrl = GetWindowItemAsControl(myDlg, 51);
	ShowControl(itemCtrl);
	SetControl32BitValue(itemCtrl, 1);
	if (i >= DIAL_NUMBER_TIERS && depTiersRoot != NULL) {
		pp->next_tier = cleanTiersList(pp->next_tier);
	}
	DrawControls(myDlg);

	RunAppModalLoopForWindow(myDlg);

	templineC3[0] = EOS;
	if (GetWindowBoolValue(myDlg, 1) > 0) {
		strcat(templineC3, " -t*");
	}
	if (GetWindowBoolValue(myDlg, 2) > 0) {
		if (oldHead != 1) {
			strcat(templineC3, " +t@");
		}
	} else {
		if (oldHead != 0) {
			strcat(templineC3, " -t@");
		}
	}
	for (p=spTiersRoot; p != NULL; p=p->next_tier) {
		if (GetWindowBoolValue(myDlg, p->num) > 0)
			p->num = 1;
		else
			p->num = 0;
	}
	for (p=depTiersRoot; p != NULL; p=p->next_tier) {
		if (GetWindowBoolValue(myDlg, p->num) > 0)
			p->num = 1;
		else
			p->num = 0;
	}
	DisposeWindow(myDlg);
	i = 0;
	len = 0;
	for (p=spTiersRoot; p != NULL; p=p->next_tier) {
		len++;
		if (p->num == 1)
			i++;
	}
	if (i != len) {
		len = len / 2;
		if (i > len) {
			for (p=spTiersRoot; p != NULL; p=p->next_tier) {
				if (p->num == 0) {
					strcat(templineC3, " -t");
					strcat(templineC3, p->code);
					strcat(templineC3, ":");
				}
			}
		} else if (i <= len) {
			for (p=spTiersRoot; p != NULL; p=p->next_tier) {
				if (p->num == 1) {
					strcat(templineC3, " +t");
					strcat(templineC3, p->code);
					strcat(templineC3, ":");
				}
			}
		}
	}
	i = 0;
	len = 0;
	for (p=depTiersRoot; p != NULL; p=p->next_tier) {
		len++;
		if (p->num == 1)
			i++;
	}	
	if (i != len) {
		len = len / 2;
		if (i > len) {
			for (p=depTiersRoot; p != NULL; p=p->next_tier) {
				if (p->num == 0) {
					strcat(templineC3, " -t");
					strcat(templineC3, p->code);
				}
			}
		} else if (i <= len) {
			for (p=depTiersRoot; p != NULL; p=p->next_tier) {
				if (p->num == 1) {
					if (strcmp(p->code, "%mor") == 0) {
						if (ProgNum == MLU || ProgNum == WDLEN  || ProgNum == MORTABLE || ProgNum == POSTMORTEM) {
						} else {
							strcat(templineC3, " +t");
							strcat(templineC3, p->code);
							strcat(templineC3, ":");
						}
					} else {
						strcat(templineC3, " +t");
						strcat(templineC3, p->code);
						strcat(templineC3, ":");
					}
				} else if (strcmp(p->code, "%mor") == 0) {
					if (ProgNum == MLU || ProgNum == WDLEN  || ProgNum == MORTABLE || ProgNum == POSTMORTEM) {
						strcat(templineC3, " -t");
						strcat(templineC3, p->code);
						strcat(templineC3, ":");
					}
				}
			}
		}
	}
	spTiersRoot = cleanTiersList(spTiersRoot);
	depTiersRoot = cleanTiersList(depTiersRoot);
	return(TRUE);
}

static char isFilesSpecified(int *ProgNum) {
	int  i;
	char *com, progName[512+1];

	com = cl_argv[0];
	if (getAliasProgName(com, progName, 512)) {
		com = progName;
	}
	*ProgNum = get_clan_prog_num(com, FALSE);
	if (cl_argc < 2)
		return(FALSE);
	for (i=1; i < cl_argc; i++) {
		if (cl_argv[i][0] == '-' || cl_argv[i][0] == '+') {
			if ((cl_argv[i][1] == 's' || cl_argv[i][1] == 'S') && *ProgNum == CHSTRING)
				i++;
		} else
			return(TRUE);
	}
	return(FALSE);
}

void TiersDialog(WindowPtr win) {
	int			ProgNum;
	char		*com;
	short		res;
	WindowPtr	tWind;

	if (!set_fbuffer(win, FALSE)) {
		do_warning("Internal ERROR; no memory allocated to variable \"fbuffer\"", 0);
		return;
	}	
	strcpy(templineC1, fbuffer);
	com = templineC1;
	cl_argc = 0;
	cl_argv[cl_argc++] = com;
	for (; *com != EOS && *com != ' ' && *com != '\t'; com++) {
		*com = (char)tolower((unsigned char)*com);
	}
	if (*com != EOS) {
		*com = EOS;
		com++;
		if (!MakeArgs(com))
			return;
	}
	spTiersRoot = NULL;
	depTiersRoot = NULL;
	ProgNum = -1;
	if (!ClassicTiersDialog()) {
		if (!isFilesSpecified(&ProgNum)) {
			res = SelectFilesDialog(templineC2);
			if (res == CANCEL)
				return;
			if (res == FILES_PAT) {
				strcat(fbuffer, " ");
				strcat(fbuffer, templineC2);
				AddComStringToComWin(fbuffer, 0);
			} else if (res == FILES_IN) {
				PrepareStruct saveRec;
				if ((tWind=FindAWindowNamed(CLAN_Programs_str)) != NULL) {
					PrepareWindA4(tWind, &saveRec);
					mCloseWindow(tWind);
					RestoreWindA4(&saveRec);
				}
				myget();
				if (F_numfiles > 0) {
					if (!isAtFound(fbuffer)) {
						strcat(fbuffer, " @");
						AddComStringToComWin(fbuffer, 0);
					}
				}
			}
			cl_argc = 1;
			if (!MakeArgs(templineC2))
				return;
			if (selectTiersDialog(ProgNum) == FALSE) {
				do_warning("Can't open any of specified files. Please check for specified files in working directory", 0);
			} else if (templineC3[0] != EOS) {
				strcat(fbuffer, templineC3);
				AddComStringToComWin(fbuffer, 0);
			}
		} else {
			if (selectTiersDialog(ProgNum) == FALSE) {
				do_warning("Can't open any of specified files. Please check for specified files in working directory", 0);
			} else if (templineC3[0] != EOS) {
				strcat(fbuffer, templineC3);
				AddComStringToComWin(fbuffer, 0);
			}
		}
	}
}
