#include "ced.h"
#include "cu.h"
#include "mac_commands.h"
#include "mac_dial.h"

#define DESELECT_DATABASE 30
#define ANOMIC_TYPE		  31
#define GLOBAL_TYPE		  32
#define BROCA_TYPE		  33
#define WERNICKE_TYPE	  34
#define TRANSSEN_TYPE	  35
#define TRANSMOT_TYPE	  36
#define CONDUCTION_TYPE	  37
#define CONTROL_TYPE	  38
#define NOT_APH_BY_WAB	  39
#define FLUENT_TYPE		  40
#define NONFLUENT_TYPE	  41
#define ALLAPHASIA_TYPE	  42
#define AGE_RANGE		  43
#define MALE_ONLY		  44
#define FEMALE_ONLY		  45

#define SPEECH_GEM		  60
#define STROKE_GEM		  61
#define WINDOW_GEM		  62
#define IMPEVENT_GEM	  63
#define UMBRELLA_GEM	  64
#define CAT_GEM			  65
#define FLOOD_GEM		  66
#define CINDERELLA_GEM	  67
#define SANDWICH_GEM	  68
#define DESELECT_GEMS	  69
#define SELECT_ALL_GEMS	  70
#define UPDATE_DB		  71

#define TOTAL_SP_NUMBER 8

#define CODESTRLEN  6
struct SpeakersListS {
	char code[CODESTRLEN+1];
	int  num;
	short val;
	struct SpeakersListS *next_tier;
} ;

#define optionsUsrData struct optionsUserWindowData
struct optionsUserWindowData {
	WindowPtr  window;
	ControlRef ageCtrl;
	short      res;
};

extern int  F_numfiles;
extern int  cl_argc;
extern char *cl_argv[];

static char AnomicTp, GlobalTp, BrocaTp, WernickeTp, TranssenTp, TransmotTp,
			ConductionTp, ControlTp, NotAphByWab, MaleOnly, FemaleOnly, SpeechGm,
			StrokeGm, WindowGm, Impevent, Umbrella, Cat, Flood, Cinderella, Sandwich;
static char AgeRange[256];
static 	time_t GlobalTime;
static struct SpeakersListS *spRoot;

void InitEvalOptions(void) {
	AnomicTp = 0;
	GlobalTp = 0;
	BrocaTp = 0;
	WernickeTp = 0;
	TranssenTp = 0;
	TransmotTp = 0;
	ConductionTp = 0;
	ControlTp = 0;
	NotAphByWab = 0;
	MaleOnly = 0;
	FemaleOnly = 0;
	SpeechGm = 0;
	StrokeGm = 0;
	WindowGm = 0;
	Impevent = 0;
	Umbrella = 0;
	Cat = 0;
	Flood = 0;
	Cinderella = 0;
	Sandwich = 0;
	AgeRange[0] = EOS;
}

static struct SpeakersListS *cleanSpeakersList(struct SpeakersListS *p) {
	struct SpeakersListS *t;

	while (p != NULL) {
		t = p;
		p = p->next_tier;
		free(t);
	}
	return(NULL);
}

static struct SpeakersListS *addToSpArr(struct SpeakersListS *root, char *code) {
	struct SpeakersListS *nt;

	if (root == NULL) {
		root = NEW(struct SpeakersListS);
		if (root == NULL)
			return(root);
		nt = root;
	} else {
		for (nt=root; nt->next_tier != NULL; nt=nt->next_tier) {
			if (uS.mStricmp(nt->code, code) == 0) {
				nt->num++;
				return(root);
			}
		}
		if (uS.mStricmp(nt->code, code) == 0) {
			nt->num++;
			return(root);
		}
		nt->next_tier = NEW(struct SpeakersListS);
		nt = nt->next_tier;
		if (nt == NULL)
			return(root);
	}
	strncpy(nt->code, code, CODESTRLEN);
	nt->code[CODESTRLEN] = EOS;
	nt->num = 1;
	nt->next_tier = NULL;
	return(root);
}

static char getSpNamesFromFile(ControlRef itemCtrl, int *cnt, char *fname, char isFoundFile) {
	char	*s;
	time_t	timer;
	FILE	*fp;

	fp = fopen(fname, "r");
	if (fp == NULL)
		return(isFoundFile);
//	*cnt = *cnt + 1;
	time(&timer);
	if (timer > GlobalTime) {
//		sprintf(templineC3, "Reading file #%d", *cnt);
		*cnt = *cnt + 1;
		sprintf(templineC3, "Reading file(s) %d", *cnt);
		SetControlData(itemCtrl,kControlEntireControl,kControlEditTextTextTag,strlen(templineC3),templineC3);
		Draw1Control(itemCtrl);
		GlobalTime = timer + 1;
	}
	isFoundFile = TRUE;
	while (fgets_cr(templineC3, 3072, fp)) {
		if (templineC3[0] == '*') {
			s = strchr(templineC3, ':');
			if (s != NULL) {
				*s = EOS;
				spRoot = addToSpArr(spRoot, templineC3);
			}
		}
	}
	fclose(fp);
	return(isFoundFile);
}

static char makeSpeakersList(ControlRef itemCtrl) {
	int		i, j, len, cnt;
	char	isFoundFile;
	FILE	*fp;

	SetNewVol(wd_dir);
	isFoundFile = FALSE;
	cnt = 0;
	for (i=1; i < cl_argc; i++) {
		if (cl_argv[i][0] == '-' || cl_argv[i][0] == '+') {
		} else {
			if (!strcmp(cl_argv[i], "@")) {
				for (j=1; j <= F_numfiles; j++) {
					get_selected_file(j, FileName1);
					isFoundFile = getSpNamesFromFile(itemCtrl, &cnt, FileName1, isFoundFile);
				}
			} else if (cl_argv[i][0] == '@' && cl_argv[i][1] == ':') {
				uS.str2FNType(FileName1, 0L, cl_argv[i]+2);
				fp = fopen(FileName1, "r");
				if (fp == NULL) {
					return(isFoundFile);
				}
				while (fgets_cr(FileName1, 3072, fp)) {
					uS.remFrontAndBackBlanks(FileName1);
					isFoundFile = getSpNamesFromFile(itemCtrl, &cnt, FileName1, isFoundFile);
				}
				fclose(fp);
			} else if (strchr(cl_argv[i], '*') == NULL) {
				isFoundFile = getSpNamesFromFile(itemCtrl, &cnt, cl_argv[i], isFoundFile);
			} else {
				strcpy(DirPathName, wd_dir);
				len = strlen(DirPathName);
				j = 1;
				while ((j=Get_File(FileName1, j)) != 0) {
					if (uS.fIpatmat(FileName1, cl_argv[i])) {
						addFilename2Path(DirPathName, FileName1);
						isFoundFile = getSpNamesFromFile(itemCtrl, &cnt, DirPathName, isFoundFile);
						DirPathName[len] = EOS;
					}
				}
			}
		}
	}
	return(isFoundFile);
}

static pascal ControlKeyFilterResult myAgeFilter(ControlRef theControl, SInt16 *keyCode, SInt16 *charCode, EventModifiers *modifiers)
{
	if (!(*modifiers & cmdKey)) {
		if (*charCode >= '0' && *charCode <= '9') 
			return kControlKeyFilterPassKey;
		else if (*charCode == '-' || *charCode == ';') 
			return kControlKeyFilterPassKey;
		else if (*charCode == 0x7f || *charCode == 0x8 || *charCode == 0x1c || *charCode == 0x1d ||
				 *charCode == 0x1e || *charCode == 0x1f)
			return kControlKeyFilterPassKey;
		
		SysBeep(1);
		return kControlKeyFilterBlockKey;
	}
	return kControlKeyFilterPassKey;
}

static pascal OSStatus evalOptionsDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	HIViewRef itemCtrl;
	struct SpeakersListS *p;
	optionsUsrData *userData = (optionsUsrData *)inUserData;
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
				case DESELECT_DATABASE:
					SetDialogItemValue(userData->window, ANOMIC_TYPE, 0);
					SetDialogItemValue(userData->window, GLOBAL_TYPE, 0);
					SetDialogItemValue(userData->window, BROCA_TYPE, 0);
					SetDialogItemValue(userData->window, WERNICKE_TYPE, 0);
					SetDialogItemValue(userData->window, TRANSSEN_TYPE, 0);
					SetDialogItemValue(userData->window, TRANSMOT_TYPE, 0);
					SetDialogItemValue(userData->window, CONDUCTION_TYPE, 0);
					SetDialogItemValue(userData->window, CONTROL_TYPE, 0);
					SetDialogItemValue(userData->window, NOT_APH_BY_WAB, 0);
					SetDialogItemValue(userData->window, MALE_ONLY, 0);
					SetDialogItemValue(userData->window, FEMALE_ONLY, 0);
					templineW[0] = 0;
					SetWindowUnicodeTextValue(userData->ageCtrl, TRUE, templineW, 0);
					break;
				case ANOMIC_TYPE:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L) {
						SetDialogItemValue(userData->window, ANOMIC_TYPE, 1);
					} else
						SetDialogItemValue(userData->window, ANOMIC_TYPE, 0);
					break;
				case GLOBAL_TYPE:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, GLOBAL_TYPE, 1);
					else
						SetDialogItemValue(userData->window, GLOBAL_TYPE, 0);
					break;
				case BROCA_TYPE:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, BROCA_TYPE, 1);
					else
						SetDialogItemValue(userData->window, BROCA_TYPE, 0);
					break;
				case WERNICKE_TYPE:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, WERNICKE_TYPE, 1);
					else
						SetDialogItemValue(userData->window, WERNICKE_TYPE, 0);
					break;
				case TRANSSEN_TYPE:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, TRANSSEN_TYPE, 1);
					else
						SetDialogItemValue(userData->window, TRANSSEN_TYPE, 0);
					break;
				case TRANSMOT_TYPE:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, TRANSMOT_TYPE, 1);
					else
						SetDialogItemValue(userData->window, TRANSMOT_TYPE, 0);
					break;
				case CONDUCTION_TYPE:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, CONDUCTION_TYPE, 1);
					else
						SetDialogItemValue(userData->window, CONDUCTION_TYPE, 0);
					break;
				case CONTROL_TYPE:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, CONTROL_TYPE, 1);
					else
						SetDialogItemValue(userData->window, CONTROL_TYPE, 0);
					break;
				case NOT_APH_BY_WAB:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, NOT_APH_BY_WAB, 1);
					else
						SetDialogItemValue(userData->window, NOT_APH_BY_WAB, 0);
					break;
				case FLUENT_TYPE:
					SetDialogItemValue(userData->window, ANOMIC_TYPE, 1);
					SetDialogItemValue(userData->window, WERNICKE_TYPE, 1);
					SetDialogItemValue(userData->window, TRANSSEN_TYPE, 1);
					SetDialogItemValue(userData->window, CONDUCTION_TYPE, 1);
					SetDialogItemValue(userData->window, NOT_APH_BY_WAB, 1);
					SetDialogItemValue(userData->window, GLOBAL_TYPE, 0);
					SetDialogItemValue(userData->window, BROCA_TYPE, 0);
					SetDialogItemValue(userData->window, TRANSMOT_TYPE, 0);
					SetDialogItemValue(userData->window, CONTROL_TYPE, 0);
					break;
				case NONFLUENT_TYPE:
					SetDialogItemValue(userData->window, GLOBAL_TYPE, 1);
					SetDialogItemValue(userData->window, BROCA_TYPE, 1);
					SetDialogItemValue(userData->window, TRANSMOT_TYPE, 1);
					SetDialogItemValue(userData->window, ANOMIC_TYPE, 0);
					SetDialogItemValue(userData->window, WERNICKE_TYPE, 0);
					SetDialogItemValue(userData->window, TRANSSEN_TYPE, 0);
					SetDialogItemValue(userData->window, CONDUCTION_TYPE, 0);
					SetDialogItemValue(userData->window, CONTROL_TYPE, 0);
					SetDialogItemValue(userData->window, NOT_APH_BY_WAB, 0);
					break;
				case ALLAPHASIA_TYPE:
					SetDialogItemValue(userData->window, ANOMIC_TYPE, 1);
					SetDialogItemValue(userData->window, GLOBAL_TYPE, 1);
					SetDialogItemValue(userData->window, BROCA_TYPE, 1);
					SetDialogItemValue(userData->window, WERNICKE_TYPE, 1);
					SetDialogItemValue(userData->window, TRANSSEN_TYPE, 1);
					SetDialogItemValue(userData->window, TRANSMOT_TYPE, 1);
					SetDialogItemValue(userData->window, CONDUCTION_TYPE, 1);
					SetDialogItemValue(userData->window, NOT_APH_BY_WAB, 1);
					SetDialogItemValue(userData->window, CONTROL_TYPE, 0);
					break;
				case MALE_ONLY:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L) {
						SetDialogItemValue(userData->window, MALE_ONLY, 1);
						SetDialogItemValue(userData->window, FEMALE_ONLY, 0);
					} else
						SetDialogItemValue(userData->window, MALE_ONLY, 0);
					break;
				case FEMALE_ONLY:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L) {
						SetDialogItemValue(userData->window, FEMALE_ONLY, 1);
						SetDialogItemValue(userData->window, MALE_ONLY, 0);
					} else
						SetDialogItemValue(userData->window, FEMALE_ONLY, 0);
					break;
				case DESELECT_GEMS:
					SetDialogItemValue(userData->window, SPEECH_GEM, 0);
					SetDialogItemValue(userData->window, STROKE_GEM, 0);
					SetDialogItemValue(userData->window, WINDOW_GEM, 0);
					SetDialogItemValue(userData->window, IMPEVENT_GEM, 0);
					SetDialogItemValue(userData->window, UMBRELLA_GEM, 0);
					SetDialogItemValue(userData->window, CAT_GEM, 0);
					SetDialogItemValue(userData->window, FLOOD_GEM, 0);
					SetDialogItemValue(userData->window, CINDERELLA_GEM, 0);
					SetDialogItemValue(userData->window, SANDWICH_GEM, 0);
					break;
				case SELECT_ALL_GEMS:
					SetDialogItemValue(userData->window, SPEECH_GEM, 1);
					SetDialogItemValue(userData->window, STROKE_GEM, 1);
					SetDialogItemValue(userData->window, WINDOW_GEM, 1);
					SetDialogItemValue(userData->window, IMPEVENT_GEM, 1);
					SetDialogItemValue(userData->window, UMBRELLA_GEM, 1);
					SetDialogItemValue(userData->window, CAT_GEM, 1);
					SetDialogItemValue(userData->window, FLOOD_GEM, 1);
					SetDialogItemValue(userData->window, CINDERELLA_GEM, 1);
					SetDialogItemValue(userData->window, SANDWICH_GEM, 1);
					break;
				case SPEECH_GEM:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, SPEECH_GEM, 1);
					else
						SetDialogItemValue(userData->window, SPEECH_GEM, 0);
					break;
				case STROKE_GEM:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, STROKE_GEM, 1);
					else
						SetDialogItemValue(userData->window, STROKE_GEM, 0);
					break;
				case WINDOW_GEM:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, WINDOW_GEM, 1);
					else
						SetDialogItemValue(userData->window, WINDOW_GEM, 0);
					break;
				case IMPEVENT_GEM:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, IMPEVENT_GEM, 1);
					else
						SetDialogItemValue(userData->window, IMPEVENT_GEM, 0);
					break;
				case UMBRELLA_GEM:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, UMBRELLA_GEM, 1);
					else
						SetDialogItemValue(userData->window, UMBRELLA_GEM, 0);
					break;
				case CAT_GEM:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, CAT_GEM, 1);
					else
						SetDialogItemValue(userData->window, CAT_GEM, 0);
					break;
				case FLOOD_GEM:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, FLOOD_GEM, 1);
					else
						SetDialogItemValue(userData->window, FLOOD_GEM, 0);
					break;
				case CINDERELLA_GEM:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, CINDERELLA_GEM, 1);
					else
						SetDialogItemValue(userData->window, CINDERELLA_GEM, 0);
					break;
				case SANDWICH_GEM:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L)
						SetDialogItemValue(userData->window, SANDWICH_GEM, 1);
					else
						SetDialogItemValue(userData->window, SANDWICH_GEM, 0);
					break;
				case UPDATE_DB:
					strcpy(FileName1, wd_dir);
					addFilename2Path(FileName1, "eval_db1.txt");
					strcpy(templineC3, "http://talkbank.org/data/eval_db.txt");
					if (DownloadURL(templineC3, 6500000L, NULL, 0, FileName1, FALSE, FALSE)) {
						strcpy(FileName2, wd_dir);
						addFilename2Path(FileName2, "eval_db.txt");
						unlink(FileName2);
						if (rename(FileName1,FileName2)) {
						}
					} else
						unlink(FileName1);
					break;
				default:
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					for (p=spRoot; p != NULL; p=p->next_tier) {
						if (aCommand.commandID == p->num) {
							if (GetControl32BitValue(itemCtrl) == 0L) {
								SetDialogItemValue(userData->window, aCommand.commandID, 1);
								p->val = 1;
							} else {
								SetDialogItemValue(userData->window, aCommand.commandID, 0);
								p->val = 0;
							}
							break;
						}
					}
					break;
			}
			break;
		default:
			break;
	}
	return status;
}

static char evalOptionsDialog(void) {
	int			i;
	char		isGender;
	Str255		title;
	WindowPtr	myDlg;
	ControlRef	itemCtrl;
	optionsUsrData	userData;
	ControlKeyFilterUPP ageStrFilter = myAgeFilter;
	struct SpeakersListS *p, *root, *nt, *tnt;

	myDlg = getNibWindow(CFSTR("eval_options"));
	CenterWindow(myDlg, -3, -3);
	userData.window = myDlg;
	userData.res = 0;
	dial_isF1Pressed = FALSE;
	dial_isF2Pressed = FALSE;
	for (i=1; i <= TOTAL_SP_NUMBER; i++)
		ControlCTRL(myDlg, i, HideCtrl, 0);
	ControlCTRL(myDlg, 54, HideCtrl, 0);
	EventTypeSpec eventTypeCP[] = {
		{kEventClassCommand, kEventCommandProcess}
	} ;
	InstallEventHandler(GetWindowEventTarget(myDlg), evalOptionsDialogEvents, 1, eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	GlobalTime = 0L;
	spRoot = NULL;
	itemCtrl = GetWindowItemAsControl(myDlg, 50);
	if (makeSpeakersList(itemCtrl) == FALSE) {
		spRoot = cleanSpeakersList(spRoot);
		DisposeWindow(myDlg);
		return(FALSE);
	}
	ControlCTRL(myDlg, 50, HideCtrl, 0);
	root = NULL;
	p = spRoot;
	while (p != NULL) {
		spRoot = p;
		p = p->next_tier;
		if (root == NULL) {
			root = spRoot;
			nt = root;
			nt->next_tier = NULL;
		} else {
			tnt = root;
			nt = root;
			while (1) {
				if (nt == NULL)
					break;
				else if (nt->num < spRoot->num)
					break;
				tnt = nt;
				nt = nt->next_tier;
			}
			if (nt == NULL) {
				nt = spRoot;
				tnt->next_tier = nt;
				nt->next_tier = NULL;
			} else if (nt == root) {
				tnt = spRoot;
				root = tnt;
				root->next_tier = nt;
				nt = root;
			} else {
				nt = spRoot;
				nt->next_tier = tnt->next_tier;
				tnt->next_tier = nt;
			}
		}
	}
	spRoot = root;
	i = 1;
	for (p=spRoot; p != NULL; p=p->next_tier) {
		if (i <= TOTAL_SP_NUMBER) {
			if (uS.mStricmp(p->code, "*PAR") == 0)
				p->val = 1;
			else
				p->val = 0;
			strcpy((char *)title+1, p->code);
			title[0] = strlen((char *)title+1);
			itemCtrl = GetWindowItemAsControl(myDlg, i);
			ShowControl(itemCtrl);
			SetControlTitle(itemCtrl, title);
			SetControl32BitValue(itemCtrl, p->val);
			p->num = i;
		} else
			p->num = 0;
		i++;
	}
	if (spRoot != NULL)
		ControlCTRL(myDlg, 54, ShowCtrl, 0);
	userData.ageCtrl = GetWindowItemAsControl(myDlg, AGE_RANGE);
	u_strcpy(templineW, AgeRange, UTTLINELEN);
	SetWindowUnicodeTextValue(userData.ageCtrl, TRUE, templineW, 0);
	if (templineW[0] != EOS)
		SelectWindowItemText(myDlg, AGE_RANGE, 0, HUGE_INTEGER);
	if (AnomicTp != 0)
		SetDialogItemValue(myDlg, ANOMIC_TYPE, 1);
	if (GlobalTp != 0)
		SetDialogItemValue(myDlg, GLOBAL_TYPE, 1);
	if (BrocaTp != 0)
		SetDialogItemValue(myDlg, BROCA_TYPE, 1);
	if (WernickeTp != 0)
		SetDialogItemValue(myDlg, WERNICKE_TYPE, 1);
	if (TranssenTp != 0)
		SetDialogItemValue(myDlg, TRANSSEN_TYPE, 1);
	if (TransmotTp != 0)
		SetDialogItemValue(myDlg, TRANSMOT_TYPE, 1);
	if (ConductionTp != 0)
		SetDialogItemValue(myDlg, CONDUCTION_TYPE, 1);
	if (ControlTp != 0)
		SetDialogItemValue(myDlg, CONTROL_TYPE, 1);
	if (NotAphByWab != 0)
		SetDialogItemValue(myDlg, NOT_APH_BY_WAB, 1);
	if (MaleOnly != 0)
		SetDialogItemValue(myDlg, MALE_ONLY, 1);
	if (FemaleOnly != 0)
		SetDialogItemValue(myDlg, FEMALE_ONLY, 1);

	if (SpeechGm != 0)
		SetDialogItemValue(myDlg, SPEECH_GEM, 1);
	if (StrokeGm != 0)
		SetDialogItemValue(myDlg, STROKE_GEM, 1);
	if (WindowGm != 0)
		SetDialogItemValue(myDlg, WINDOW_GEM, 1);
	if (Impevent != 0)
		SetDialogItemValue(myDlg, IMPEVENT_GEM, 1);
	if (Umbrella != 0)
		SetDialogItemValue(myDlg, UMBRELLA_GEM, 1);
	if (Cat != 0)
		SetDialogItemValue(myDlg, CAT_GEM, 1);
	if (Flood != 0)
		SetDialogItemValue(myDlg, FLOOD_GEM, 1);
	if (Cinderella != 0)
		SetDialogItemValue(myDlg, CINDERELLA_GEM, 1);
	if (Sandwich != 0)
		SetDialogItemValue(myDlg, SANDWICH_GEM, 1);
	AdvanceKeyboardFocus(myDlg);
	SetControlData(userData.ageCtrl, kControlEntireControl, kControlEditTextKeyFilterTag, sizeof(ageStrFilter), &ageStrFilter);
	DrawControls(myDlg);

	RunAppModalLoopForWindow(myDlg);

	templineC3[0] = EOS;
	if (userData.res == myOK) {
		AnomicTp = (GetWindowBoolValue(myDlg, ANOMIC_TYPE) > 0);
		GlobalTp = (GetWindowBoolValue(myDlg, GLOBAL_TYPE) > 0);
		BrocaTp = (GetWindowBoolValue(myDlg, BROCA_TYPE) > 0);
		WernickeTp = (GetWindowBoolValue(myDlg, WERNICKE_TYPE) > 0);
		TranssenTp = (GetWindowBoolValue(myDlg, TRANSSEN_TYPE) > 0);
		TransmotTp = (GetWindowBoolValue(myDlg, TRANSMOT_TYPE) > 0);
		ConductionTp = (GetWindowBoolValue(myDlg, CONDUCTION_TYPE) > 0);
		ControlTp = (GetWindowBoolValue(myDlg, CONTROL_TYPE) > 0);
		NotAphByWab = (GetWindowBoolValue(myDlg, NOT_APH_BY_WAB) > 0);
		MaleOnly = (GetWindowBoolValue(myDlg, MALE_ONLY) > 0);
		FemaleOnly = (GetWindowBoolValue(myDlg, FEMALE_ONLY) > 0);
		SpeechGm = (GetWindowBoolValue(myDlg, SPEECH_GEM) > 0);
		StrokeGm = (GetWindowBoolValue(myDlg, STROKE_GEM) > 0);
		WindowGm = (GetWindowBoolValue(myDlg, WINDOW_GEM) > 0);
		Impevent = (GetWindowBoolValue(myDlg, IMPEVENT_GEM) > 0);
		Umbrella = (GetWindowBoolValue(myDlg, UMBRELLA_GEM) > 0);
		Cat = (GetWindowBoolValue(myDlg, CAT_GEM) > 0);
		Flood = (GetWindowBoolValue(myDlg, FLOOD_GEM) > 0);
		Cinderella = (GetWindowBoolValue(myDlg, CINDERELLA_GEM) > 0);
		Sandwich = (GetWindowBoolValue(myDlg, SANDWICH_GEM) > 0);
		GetWindowUnicodeTextValue(userData.ageCtrl, templineW, UTTLINELEN);
		u_strcpy(AgeRange, templineW, 256);
		if (SpeechGm && StrokeGm && WindowGm && Impevent && Umbrella && Cat && Flood && Cinderella && Sandwich) {
			SpeechGm = StrokeGm = WindowGm = Impevent = Umbrella = Cat = Flood = Cinderella = Sandwich = 0;
		}
		for (p=spRoot; p != NULL; p=p->next_tier) {
			if (p->val == 1) {
				strcat(templineC3, " +t");
				strcat(templineC3, p->code);
				strcat(templineC3, ":");
			}
		}
		if (MaleOnly)
			isGender = 1;
		else if (FemaleOnly)
			isGender = 2;
		else 
			isGender = 0;
		if (AnomicTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "Anomic");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (GlobalTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "Global");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (BrocaTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "Broca");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (WernickeTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "Wernicke");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (TranssenTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "TransSensory");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (TransmotTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "TransMotor");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (ConductionTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "Conduction");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (ControlTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "control");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (NotAphByWab) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "NotAphasicByWAB");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (SpeechGm) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Speech");
			strcat(templineC3, "\"");
		}
		if (StrokeGm) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Stroke");
			strcat(templineC3, "\"");
		}
		if (WindowGm) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Window");
			strcat(templineC3, "\"");
		}
		if (Impevent) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Important_Event");
			strcat(templineC3, "\"");
		}
		if (Umbrella) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Umbrella");
			strcat(templineC3, "\"");
		}
		if (Cat) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Cat");
			strcat(templineC3, "\"");
		}
		if (Flood) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Flood");
			strcat(templineC3, "\"");
		}
		if (Cinderella) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Cinderella");
			strcat(templineC3, "\"");
		}
		if (Sandwich) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Sandwich");
			strcat(templineC3, "\"");
		}
		strcat(templineC3, " +u");
	}
	DisposeWindow(myDlg);
	spRoot = cleanSpeakersList(spRoot);
	return(TRUE);
}

void EvalDialog(WindowPtr win) {
	WindowPtr	tWind;
	PrepareStruct saveRec;

	if (!set_fbuffer(win, FALSE)) {
		do_warning("Internal ERROR; no memory allocated to variable \"fbuffer\"", 0);
		return;
	}	
	strcpy(fbuffer, clan_name[EVAL]);
	spRoot = NULL;
	if ((tWind=FindAWindowNamed(CLAN_Programs_str)) != NULL) {
		PrepareWindA4(tWind, &saveRec);
		mCloseWindow(tWind);
		RestoreWindA4(&saveRec);
	}
	myget();
	if (F_numfiles <= 0) {
		do_warning("ERROR eval. Please specify input data files first", 0);
		return;
	}
	cl_argc = 1;
	strcpy(templineC2, " @");
	if (!MakeArgs(templineC2))
		return;
	if (evalOptionsDialog() == FALSE) {
		do_warning("Can't open any of specified files. Please check for specified files in working directory", 0);
	} else if (templineC3[0] != EOS) {
		if (!isAtFound(fbuffer))
			strcat(fbuffer, " @");
		strcat(fbuffer, templineC3);
		AddComStringToComWin(fbuffer, 0);
	}
}
