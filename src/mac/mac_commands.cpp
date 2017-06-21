#include "ced.h"
#include "cu.h"
#include "mac_commands.h"
#include "my_ctype.h"
#include "mac_MUPControl.h"
#include "mac_dial.h"

extern int F_numfiles;
extern short mScript;
extern long option_flags[];

#define I_Run		  1
#define I_WD_Button	  2
#define I_WD_Text	  3
#define I_OD_Button	  4
#define I_OD_Text	  5
#define I_Recall	  6
#define I_CM_Text	  7
#define I_Prog_icon	  8
#define I_Help		  9
#define I_Tiers		  10
#define I_Search	  11
#define I_InputFile	  12
#define I_OutputFile  13
#define I_Lib_Button  14
#define I_Lib_Text	  15
#define I_MorLib_But  16
#define I_MorLib_Txt  17
#define I_Version_Txt 18

static int      curProgNum;
static char		*bArgv[10];
static char		FilePattern[256+1];
static WindowPtr ClanWin = NULL;

static MenuRef progsMref;
static ControlRef progsCtrlH;
static ControlRef progsItemCtrl;

long  ClanWinRowLim = 500L;
char  ClanAutoWrap = TRUE;
FNType  wd_dir[FNSize], // working directory
		od_dir[FNSize]; // output directory

char set_fbuffer(WindowPtr win, char isDel) {
	char state;
	unsigned long i;
	ControlRef iCtrl;
    TXNOffset oStartOffset, oEndOffset;
    TXNOffset cStartOffset, cEndOffset;
    Handle DataHandle;
	STPTextPaneVars **tpvars, *varsp;

	if (fbuffer == NULL) {
		do_warning("Can't run CLAN; Out of memory", 0);
		return(FALSE);
	}
	iCtrl = GetWindowItemAsControl(win, I_CM_Text);
	tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
	if (tpvars != NULL) {
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		varsp = *tpvars;
		TXNGetSelection(varsp->fTXNRec, &oStartOffset, &oEndOffset);
		TXNSelectAll(varsp->fTXNRec);
		TXNGetSelection(varsp->fTXNRec, &cStartOffset, &cEndOffset);
		TXNGetDataEncoded(varsp->fTXNRec,cStartOffset,cEndOffset,&DataHandle,kTXNUnicodeTextData);
		fbuffer[0] = EOS;
		if (DataHandle == NULL)
			return(TRUE);

		HLock(DataHandle);

		if (isUTFData)
			UnicodeToUTF8((wchar_t *)*DataHandle, cEndOffset, (unsigned char *)fbuffer, &i, EXPANSION_SIZE);
		else
			UnicodeToANSI((wchar_t *)*DataHandle, cEndOffset, (unsigned char *)fbuffer, &i, EXPANSION_SIZE, /*GetScriptManagerVariable(smKeyScript)*/my_FontToScript(dFnt.fontId, 0));
		fbuffer[i] = EOS;
		for (i=0L; isSpace(fbuffer[i]); i++) ;
		if (i > 0L)
			strcpy(fbuffer, fbuffer+i);
		for (i=0L; fbuffer[i] != EOS && fbuffer[i] != '\r' && fbuffer[i] != '\n'; i++) ;
		if (fbuffer[i] == '\r' || fbuffer[i] == '\n') {
			if (fbuffer[i+1] == EOS) {
				if (i < oStartOffset)
					oStartOffset = i;
				if (i < oEndOffset)
					oEndOffset = i;
			}
			for (; fbuffer[i] != EOS; i++) {
				if (fbuffer[i] == '\r' || fbuffer[i] == '\n')
					fbuffer[i] = ' ';
			}
		}

		HUnlock(DataHandle);

		DisposeHandle(DataHandle);
		if (isDel) {
			Str255 pFontName;
			char FontName[256];
			OSStatus err;
			TXNTypeAttributes iAttributes[2];

			TXNSelectAll(varsp->fTXNRec);
			TXNClear(varsp->fTXNRec);

			GetFontName(dFnt.fontId, pFontName);
			p2cstrcpy(FontName, pFontName);
			if (strcmp(FontName, "Ascender Uni Duo") == 0) {
				iAttributes[0].tag  = kTXNQDFontSizeAttribute;
				iAttributes[0].size = kTXNQDFontSizeAttributeSize;
				iAttributes[0].data.dataValue = dFnt.fontSize;

				err = TXNSetTypeAttributes ((**tpvars).fTXNRec, 1, iAttributes, kTXNUseCurrentSelection, kTXNUseCurrentSelection); 
			} else {
				iAttributes[1].tag  = kTXNQDFontFamilyIDAttribute;
				iAttributes[1].size = kTXNQDFontFamilyIDAttributeSize;
				iAttributes[1].data.dataValue = dFnt.fontId;

				iAttributes[0].tag  = kTXNQDFontSizeAttribute;
				iAttributes[0].size = kTXNQDFontSizeAttributeSize;
				iAttributes[0].data.dataValue = dFnt.fontSize;

				err = TXNSetTypeAttributes ((**tpvars).fTXNRec, 2, iAttributes, kTXNUseCurrentSelection, kTXNUseCurrentSelection); 
			}
			SetClanWinIcons(win, NULL);
		} else {
			TXNSetSelection(varsp->fTXNRec, oStartOffset, oEndOffset);
		}
		HUnlock((Handle) tpvars);
		HSetState((Handle) tpvars, state);
	}
	return(TRUE);
}

void re_readAliases(void) {
	if (ClanWin != NULL) {
//		free_aliases();
//		readAliases(1);
	}
}

char isAtFound(char *s) {
	char qf;

	if (F_numfiles <= 0)
		return(FALSE);

	qf = FALSE;
	while (*s != EOS) {
		if (*s == '@') {
			if (!qf)
				return(TRUE);
		} else if (*s == '\'' || *s == '"') {
			if (!qf) {
				qf = *s;
			} else if (qf == *s) {
				qf = FALSE;
			}
		}
		s++;
 	}
	return(FALSE);
}

static void HideClanWinIcons(WindowPtr win) {
	if (win == NULL) {
		win = ClanWin;
		if (win == NULL)
			return;
	}
	ControlCTRL(win, I_Tiers, HideCtrl, 0);
	ControlCTRL(win, I_Search, HideCtrl, 0);
	ControlCTRL(win, I_InputFile, HideCtrl, 0);
	ControlCTRL(win, I_OutputFile, HideCtrl, 0);
}

void SetClanWinIcons(WindowPtr win, char *com) {
	int ProgNum;
	char *s, progName[512+1];
	ControlRef theControl;

	if (com == NULL || *com == EOS)
		HideClanWinIcons(win);
	else {
		s = strchr(com, ' ');
		if (s != NULL)
			*s = EOS;
		if (getAliasProgName(com, progName, 512)) {
			if (s != NULL)
				*s = ' ';
			s = NULL;
			com = progName;
		}
		if ((ProgNum=get_clan_prog_num(com, FALSE)) < 0) {
			if (strcmp(com, "bat") == 0 || strcmp(com, "batch") == 0 || strcmp(com, "dir") == 0 || strcmp(com, "list") == 0) {
				theControl = GetWindowItemAsControl(win, I_InputFile);
				SetControlTitle(theControl, "\pFile In"); 
				ControlCTRL(win, I_InputFile, ShowCtrl, 0);
				ControlCTRL(win, I_Tiers, HideCtrl, 0);
				ControlCTRL(win, I_Search, HideCtrl, 0);
				ControlCTRL(win, I_OutputFile, HideCtrl, 0);
			} else
				HideClanWinIcons(win);
		} else {
			curProgNum = ProgNum;
			theControl = GetWindowItemAsControl(win, I_InputFile);
			if (curProgNum == EVAL)
				SetControlTitle(theControl, "\pOption"); 
			else
				SetControlTitle(theControl, "\pFile In"); 
			ControlCTRL(win, I_InputFile, ShowCtrl, 0);
			if (option_flags[ProgNum] & T_OPTION && curProgNum != EVAL)
				ControlCTRL(win, I_Tiers, ShowCtrl, 0);
			else
				ControlCTRL(win, I_Tiers, HideCtrl, 0);
			if (option_flags[ProgNum] & SP_OPTION || option_flags[ProgNum] & SM_OPTION)
				ControlCTRL(win, I_Search, ShowCtrl, 0);
			else
				ControlCTRL(win, I_Search, HideCtrl, 0);
#ifdef COMMANDS_TEST
			ControlCTRL(win, I_OutputFile, ShowCtrl, 0);
#else
			ControlCTRL(win, I_OutputFile, HideCtrl, 0);
#endif // ! COMMANDS_TEST
		}
		if (s != NULL)
			*s = ' ';
	}
}

void AddComStringToComWin(char *com, short show) {
	register char *s;
	char state;
	unsigned long i, len;
	ControlRef iCtrl;
	STPTextPaneVars **tpvars, *varsp;

	if (ClanWin == NULL) {
		OpenCommandsWindow(FALSE);
		if (ClanWin == NULL)
			return;
	} else {
		OpenCommandsWindow(TRUE);
	}	

	iCtrl = GetWindowItemAsControl(ClanWin, I_CM_Text);
	tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
	if (tpvars != NULL) {
		OSStatus err;
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		varsp = *tpvars;
		TXNSelectAll(varsp->fTXNRec);
		TXNClear(varsp->fTXNRec);
		TXNTypeAttributes iAttributes[2];
		Str255 pFontName;
		char FontName[256];
		GetFontName(dFnt.fontId, pFontName);
		p2cstrcpy(FontName, pFontName);
		if (strcmp(FontName, "Ascender Uni Duo") == 0) {
			iAttributes[0].tag  = kTXNQDFontSizeAttribute;
			iAttributes[0].size = kTXNQDFontSizeAttributeSize;
			iAttributes[0].data.dataValue = dFnt.fontSize;

			err = TXNSetTypeAttributes ((**tpvars).fTXNRec, 1, iAttributes, kTXNUseCurrentSelection, kTXNUseCurrentSelection); 
		} else {
			iAttributes[1].tag  = kTXNQDFontFamilyIDAttribute;
			iAttributes[1].size = kTXNQDFontFamilyIDAttributeSize;
			iAttributes[1].data.dataValue = dFnt.fontId;

			iAttributes[0].tag  = kTXNQDFontSizeAttribute;
			iAttributes[0].size = kTXNQDFontSizeAttributeSize;
			iAttributes[0].data.dataValue = dFnt.fontSize;

			err = TXNSetTypeAttributes ((**tpvars).fTXNRec, 2, iAttributes, kTXNUseCurrentSelection, kTXNUseCurrentSelection); 
		}

		for (s=com, i=0L; *s && *s != NL_C; s++, i++) {
			if (*s == SNL_C)
				fbuffer[i] = ' ';
			else
				fbuffer[i] = *s;
		}
		fbuffer[i] =  EOS;

		UTF8ToUnicode((unsigned char *)fbuffer, i, (wchar_t *)templine4, &len, UTTLINELEN);
		if (len > 0) 
			TXNSetData(varsp->fTXNRec, kTXNUnicodeTextData, templine4, len*2, 0, 0);
		if (com != fbuffer)
			fbuffer[0] = EOS;
		HSetState((Handle) tpvars, state);
	}
	if (show)
		ControlCTRL(ClanWin, show, ShowCtrl, 0);
	else
		SetClanWinIcons(ClanWin, com);
}

static char *getBatchArgs(char *com) {
	register int i;
	register char *endCom;

	for (i=0; i < 10; i++)
		bArgv[i] = NULL;

	i = 0;
	while (*com != EOS) {
		for (; *com == ' ' || *com == '\t'; com++) ;
		if (*com == EOS)
			break;
		endCom = NextArg(com);
		if (endCom == NULL)
			return(NULL);

		if (i >= 10) {
			do_warning("out of memory; Too many arguments.", 0);
			return(NULL);
		}
		bArgv[i++] = com;
		com = endCom;
	}
	return(com);
}

static char fixArgs(char *fname, char *com) {
	register int  i;
	register int  num;
	register char qt = 0;

	for (i=0; com[i] != EOS; i++) {
		if (com[i] == '\'' /*'*/ || com[i] == '"') {
			if (qt == 0)
				qt = com[i];
			else if (qt == com[i])
				qt = 0;
		} else if (qt == 0 && com[i] == '%' && isdigit(com[i+1]) && !isdigit(com[i+2])) {
			num = atoi(com+i+1) - 1;
			if (num < 0 || num > 9 || bArgv[num] == NULL) {
				sprintf(com, "Argument %%%d was not specified with batch file \"%s\".", num+1, fname);
				do_warning(com, 0);
				return(FALSE);
			}
			strcpy(com+i, com+i+2);
			uS.shiftright(com+i,(int)strlen(bArgv[num]));
			strncpy(com+i,bArgv[num],strlen(bArgv[num]));
			i = i + strlen(bArgv[num]) - 1;
		}
	}
	return(TRUE);
}

static void EmptyCommandLine(WindowPtr win) {
	char state;
	ControlRef iCtrl;
	STPTextPaneVars **tpvars;
	GrafPtr		oldPort;

	GetPort(&oldPort);
	iCtrl = GetWindowItemAsControl(win, I_CM_Text);
	tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
	if (tpvars != NULL) {
		OSStatus err;

		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		SetPort((**tpvars).fDrawingEnvironment);

		TXNSelectAll((**tpvars).fTXNRec);
		TXNClear((**tpvars).fTXNRec);

		TXNTypeAttributes iAttributes[2];
		Str255 pFontName;
		char FontName[256];
		GetFontName(dFnt.fontId, pFontName);
		p2cstrcpy(FontName, pFontName);
		if (strcmp(FontName, "Ascender Uni Duo") == 0) {
			iAttributes[0].tag  = kTXNQDFontSizeAttribute;
			iAttributes[0].size = kTXNQDFontSizeAttributeSize;
			iAttributes[0].data.dataValue = dFnt.fontSize;

			err = TXNSetTypeAttributes ((**tpvars).fTXNRec, 1, iAttributes, kTXNUseCurrentSelection, kTXNUseCurrentSelection); 
		} else {
			iAttributes[1].tag  = kTXNQDFontFamilyIDAttribute;
			iAttributes[1].size = kTXNQDFontFamilyIDAttributeSize;
			iAttributes[1].data.dataValue = dFnt.fontId;

			iAttributes[0].tag  = kTXNQDFontSizeAttribute;
			iAttributes[0].size = kTXNQDFontSizeAttributeSize;
			iAttributes[0].data.dataValue = dFnt.fontSize;

			err = TXNSetTypeAttributes ((**tpvars).fTXNRec, 2, iAttributes, kTXNUseCurrentSelection, kTXNUseCurrentSelection); 
		}

		HSetState((Handle) tpvars, state);
	}
	SetPort(oldPort);
	HideClanWinIcons(win);
}

static void set_CommandsInfo(WindowPtr win, char isSetPort) {
	GrafPtr oldPort;

	if (isSetPort) {
		GetPort(&oldPort);
		SetPortWindowPort(win);
	}

	SetDialogItemUTF8(wd_dir, win, I_WD_Text, TRUE);
	if (!WD_Not_Eq_OD)
		SetDialogItemUTF8("", win, I_OD_Text, TRUE);
	else
		SetDialogItemUTF8(od_dir, win, I_OD_Text, TRUE);
	SetDialogItemUTF8(lib_dir, win, I_Lib_Text, TRUE);
	SetDialogItemUTF8(mor_lib_dir, win, I_MorLib_Txt, TRUE);

	if (isSetPort)
		SetPort(oldPort);
}

static void RunCommand(WindowPtr win) {
	char	tDataChanged;
	FNType	tname[FNSize];
	FILE	*fp;
	myFInfo	*saveGlobal_df;
	WindowProcRec	*rec;	
	extern WindowProcRec *FindAProcRecID(short id);

	if (!set_fbuffer(win, TRUE))
		return;
	isKillProgram = 0;
	uS.remFrontAndBackBlanks(fbuffer);
	if (fbuffer[0] != EOS) {
		AddToClan_commands(fbuffer);
		set_CommandsInfo(win, TRUE);
		if (StdInWindow == NULL) {
			if (OpenWindow(1964, CLAN_Output_str, ClanWinRowLim, false, 2, MainTextEvent, UpdateMainText, NULL))
				ProgExit("Can't open \"CLAN Output\" window");
		} else {
			strcpy(tname, StdInWindow);
			if (OpenWindow(1962, tname, ClanWinRowLim, false, 2, MainTextEvent, UpdateMainText, NULL))
				ProgExit("Can't open window");
		}
		if (StdInWindow != NULL && global_df != NULL && !strcmp(StdInWindow,global_df->fileName)) {
			do_warning(StdInErrMessage, 0);
		} else if (global_df) {
			if (StdInWindow != NULL) {
				if (OpenWindow(1964, CLAN_Output_str, ClanWinRowLim, false, 2, MainTextEvent, UpdateMainText, NULL))
					ProgExit("Can't open \"CLAN Output\" window");
			}
			global_df->AutoWrap = ClanAutoWrap;
			if (isUTFData && !dFnt.isUTF) {
				SetDefaultUnicodeFinfo(&dFnt);
				dFnt.fontTable = NULL;
				dFnt.isUTF = 1;
			}
			dFnt.charHeight = GetFontHeight(NULL, &dFnt, win);
			dFnt.Encod = my_FontToScript(dFnt.fontId, dFnt.CharSet);
			dFnt.orgEncod = dFnt.Encod;
			copyNewFontInfo(&oFnt, &dFnt);
			tDataChanged = global_df->DataChanged;
			EndOfFile(1);
			SetScrollControl();
			ResetUndos();
			copyNewToFontInfo(&global_df->row_txt->Font, &dFnt);
			global_df->attOutputToScreen = 0;
			if (global_df->lineno == 1)
				OutputToScreen(cl_T("> "));
			dFnt.isUTF = isUTFData;
			if (!uS.mStrnicmp(fbuffer, "batch ", 6) || !uS.mStrnicmp(fbuffer, "bat ", 4)) {
				char *com, *eCom;
				FNType mDirPathName[FNSize];

				uS.remFrontAndBackBlanks(fbuffer+1);
#ifdef _UNICODE
				UTF8ToUnicode((unsigned char *)fbuffer, strlen(fbuffer), (wchar_t *)templine4, NULL, UTTLINELEN);
				OutputToScreen(templine4);
#else
				OutputToScreen(fbuffer);
#endif
				for (com=fbuffer; *com != EOS && *com != ' ' && *com != '\t'; com++) ;
				for (; *com == ' ' || *com == '\t'; com++) ;
				eCom = com;
				if (*com != EOS) {
					for (; *eCom != EOS && *eCom != ' ' && *eCom != '\t'; eCom++) ;
					if (*eCom != EOS) {
						*eCom = EOS;
						eCom++;
					}
				}
				strcpy(mDirPathName, wd_dir);
				addFilename2Path(mDirPathName, com);
				if ((fp=fopen(mDirPathName, "r")) == NULL) {
					strcpy(mDirPathName, lib_dir);
					addFilename2Path(mDirPathName, com);
					if ((fp=fopen(mDirPathName, "r")) == NULL) {
						sprintf(ced_lineC, "Can't find batch file \"%s\" in either working or library directories", com);
						do_warning(ced_lineC, 0);
					}
				}
				if (fp != NULL) {
					if ((eCom=getBatchArgs(eCom)) != NULL) {
						eCom++;
						while (fgets_ced(eCom, 1024, fp, NULL)) {
							uS.remFrontAndBackBlanks(eCom);
							if (uS.isUTF8(eCom) || uS.partcmp(eCom,FONTHEADER,FALSE,FALSE) || eCom[0] == '%' || eCom[0] == '#' || eCom[0] == EOS)
								continue;
							if (!fixArgs(com, eCom))
								break;
							if (*eCom == EOS ||
								((char)toupper((unsigned char)eCom[0]) == 'T' && (char)toupper((unsigned char)eCom[1]) == 'Y') ||
								((char)toupper((unsigned char)eCom[0]) == 'P' && (char)toupper((unsigned char)eCom[1]) == 'A')) ;
							else {
								OutputToScreen(cl_T("\nBATCH> "));
								execute(eCom, tDataChanged);
								if (isKillProgram) {
									if (isKillProgram != 2)
										OutputToScreen(cl_T("\n    BATCH FILE ABORTED\n"));
									break;
								}
							}
						}
					}
					fclose(fp);
				}
			} else
				execute(fbuffer, tDataChanged);

			copyNewFontInfo(&dFnt, &oFnt);
			if (isKillProgram)
				isKillProgram = 0;
			saveGlobal_df = global_df;
			if (global_df == NULL || global_df->winID != 1964) {
				if ((rec=FindAProcRecID(1964)) != NULL) {
					global_df = rec->FileInfo;
				} else
					global_df = NULL;
			}
			if (global_df != NULL) {
				OutputToScreen(cl_T("\n> "));
				global_df->DataChanged = tDataChanged;
				PosAndDispl();
			}
			global_df = saveGlobal_df;
			if (global_df != NULL && global_df->winID == 1964)
				OpenCommandsWindow(TRUE);
		}
	}
}

static short ClanEvent(WindowPtr win, EventRecord *event) {
	short		ModalHit;
	FNType		mDirPathName[FNSize];
	WindowPtr	tWind;

	ModalHit = 0;
	if (event->what == mouseDown) {
		ControlRef		theControl;
		ControlPartCode	code;
		ControlID outID;

		GlobalToLocal(&event->where);
		theControl = FindControlUnderMouse(event->where,win,&code); //code = FindControl(event->where, win, &theControl);
		if (theControl != NULL) {
			code = HandleControlClick(theControl,event->where,event->modifiers,NULL); 
			if (GetControlID(theControl, &outID) == noErr)
				ModalHit = outID.id;
			if (code == 0 && ModalHit != I_CM_Text && ModalHit != I_WD_Text && ModalHit != I_Lib_Text &&
				ModalHit != I_MorLib_Txt && ModalHit != I_Prog_icon && ModalHit != I_Version_Txt)
				ModalHit = 0;
/* 2007-01-27
			if (ModalHit == I_CM_Text) {
				char state;
				STPTextPaneVars **tpvars, *varsp;

				tpvars = (STPTextPaneVars **) GetControlReference(theControl);
				if (tpvars != NULL) {
					state = HGetState((Handle) tpvars);
					HLock((Handle) tpvars);
					varsp = *tpvars;
					SetPort(varsp->fDrawingEnvironment);
					TXNClick(varsp->fTXNRec, event);
					HUnlock((Handle) tpvars);
					HSetState((Handle) tpvars, state);
				}
			}
*/
		}
		LocalToGlobal(&event->where);
	}
/* 2007-01-30
	 else if (event->what == keyDown || event->what == autoKey) {
		short type;

		type = (event->message & keyCodeMask)/256;
		if (type == '\33')
			EmptyCommandLine(win);
		else if (type == up_key || type == down_key)
			RecallCommand(win, type);
		else {
			type = event->message & charCodeMask;
			if (type == RETURN_CHAR || type == ENTER_CHAR)
				ModalHit = I_Run;
			else if (set_fbuffer(win, FALSE))
				SetClanWinIcons(win, fbuffer);
		}
	}
*/
	
	if (ModalHit == I_Help) {
		strcpy(mDirPathName, lib_dir);
		addFilename2Path(mDirPathName, "commands.cut");
		if (!access(mDirPathName, 0)) {
			char help_s[256];
			strcpy(help_s, "help");
			AddComStringToComWin(help_s, 0);
			RunCommand(win);
			isAjustCursor = FALSE;
			OpenAnyFile(mDirPathName, 1962, TRUE);
		} else
			do_warning("Can't open help file: commands.cut. Check to see if lib directory is set correctly.", 0);
	}

	if (ModalHit == I_WD_Text) {
		OpenFoldersWindow(1);
	}
	if (ModalHit == I_Lib_Text) {
		OpenFoldersWindow(2);
	}
	if (ModalHit == I_MorLib_Txt) {
		OpenFoldersWindow(3);
	}

	if (ModalHit == I_WD_Button) {
	    GrafPtr oldPort;

		GetPort(&oldPort);
		SetPortWindowPort(win);
		TextFont(0);
		TextSize(0);
		TextFace(0);
		strcpy(mDirPathName, wd_dir);
		LocateDir("Please locate working directory",wd_dir,true);
		SetDialogItemUTF8(wd_dir, win, I_WD_Text, TRUE);

		if (pathcmp(od_dir, mDirPathName) == 0)
			strcpy(od_dir, wd_dir);

		if (!WD_Not_Eq_OD)
			SetDialogItemUTF8("", win, I_OD_Text, FALSE);
		else
			SetDialogItemUTF8(od_dir, win, I_OD_Text, TRUE);
		SetPort(oldPort);
		WriteCedPreference();
		addToFolders(1, wd_dir);
	}

	if (ModalHit == I_OD_Button) {
	    GrafPtr oldPort;

		GetPort(&oldPort);
		SetPortWindowPort(win);
		TextFont(0);
		TextSize(0);
		TextFace(0);
		LocateDir("Please locate output directory",od_dir,false);
		if (!WD_Not_Eq_OD)
			SetDialogItemUTF8("", win, I_OD_Text, FALSE);
		else
			SetDialogItemUTF8(od_dir, win, I_OD_Text, TRUE);
		SetPort(oldPort);
		WriteCedPreference();
	}

	if (ModalHit == I_Run) {
		RunCommand(win);
	}

	if (ModalHit == I_Recall) {
		OpenRecallWindow();
	}

	if (ModalHit == I_Prog_icon) {
//		OpenProgsWindow(); for button in CLAN.nib
		int item;
		Rect  tempRect;
		Point theMouse;
		GetControlBounds(progsCtrlH, &tempRect);
		theMouse.h = tempRect.left;
		theMouse.v = tempRect.top;
		HandleControlClick(progsCtrlH, theMouse, 0, NULL);
		item = GetControl32BitValue(progsCtrlH) - 2;
		if (item >= 0) {
			strcpy(fbuffer, clan_name[item]);
			strcat(fbuffer, " ");
			AddComStringToComWin(fbuffer, 0);
		}
		SetControl32BitValue(progsCtrlH, 1);
	}

	if (ModalHit == I_InputFile) {
		if (curProgNum == EVAL) {
			EvalDialog(win);
		} else {
			if ((tWind=FindAWindowNamed(CLAN_Programs_str)) != NULL) {
				PrepareStruct	saveRec;
				
				PrepareWindA4(tWind, &saveRec);
				mCloseWindow(tWind);
				RestoreWindA4(&saveRec);
			}
			myget();
			if (set_fbuffer(win, FALSE)) {
				if (F_numfiles > 0) {
					if (!isAtFound(fbuffer)) {
						strcat(fbuffer, " @");
						AddComStringToComWin(fbuffer, 0);
					} else
						SetClanWinIcons(win, fbuffer);
				} else
					SetClanWinIcons(win, fbuffer);
			}
		}
	}
	
	if (ModalHit == I_Tiers) {
		TiersDialog(win);
	}
	
	if (ModalHit == I_Search) {
		SearchDialog(win);
	}

	if (ModalHit == I_Lib_Button) {
	    GrafPtr oldPort;

		GetPort(&oldPort);
		SetPortWindowPort(win);
		TextFont(0);
		TextSize(0);
		TextFace(0);
		LocateDir("Please locate library directory",lib_dir,false);
		SetDialogItemUTF8(lib_dir, win, I_Lib_Text, TRUE);
		SetPort(oldPort);
		WriteCedPreference();
		addToFolders(2, lib_dir);
		free_aliases();
		readAliases(1);
	}
	if (ModalHit == I_MorLib_But) {
	    GrafPtr oldPort;
		char *s;
		int  len;

		GetPort(&oldPort);
		SetPortWindowPort(win);
		TextFont(0);
		TextSize(0);
		TextFace(0);
		strcpy(mDirPathName, mor_lib_dir);
		len = strlen(mDirPathName) - 1;
		if (len >= 0 && mDirPathName[len] == PATHDELIMCHR)
			mDirPathName[len] = EOS;
		s = strrchr(mDirPathName, PATHDELIMCHR);
		if (s != NULL)
			*s = EOS;
		if (LocateDir("Please locate mor library directory",mDirPathName /*mor_lib_dir*/,false))
			strcpy(mor_lib_dir, mDirPathName);
		SetDialogItemUTF8(mor_lib_dir, win, I_MorLib_Txt, TRUE);
		SetPort(oldPort);
		WriteCedPreference();
		addToFolders(3, mor_lib_dir);
	}
	if (ModalHit == I_Version_Txt) {
		AboutCLAN(TRUE);
	}
	return -1;
}

static int comKeys(WindowPtr win, EventRecord *event, short var) {
	if (var == 30) {
		RecallCommand(win, up_key);
		return(1);
	} else if (var == 31) {
		RecallCommand(win, down_key);
		return(1);
	} else if (var == RETURN_CHAR || var == ENTER_CHAR) {
		RunCommand(win);
		return(1);
	} else if (var == '\33') {
		EmptyCommandLine(win);
		return(1);
	}
	if (set_fbuffer(win, FALSE)) {
		if (AgmentCurrentMLTEText(GetWindowItemAsControl(win, I_CM_Text), event, fbuffer, var))
			SetClanWinIcons(win, fbuffer);
	}
	return(0);
}

ACT_FUNC_INFO keyCommandsActions = {
	comKeys, nil, 501
} ;

extern void reopenCLANText(void);

void reopenCLANText(void) {
	ControlRef iCtrl;

	if (ClanWin != NULL) {
		iCtrl = GetWindowItemAsControl(ClanWin, I_CM_Text);
		mUPCloseControl(iCtrl);
		keyCommandsActions.win = ClanWin;
		mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyCommandsActions);
		AdvanceKeyboardFocus(ClanWin);		
	}
}

static void CleanupClan(WindowPtr win) {
	int i;

	mUPCloseControl(GetWindowItemAsControl(win, I_CM_Text));
	ClanWin = NULL;
	free_aliases();
	if (progsCtrlH!= NULL)
		DisposeControl(progsCtrlH);
	if (progsMref != NULL) {
		if ((i=CountMenuItems(progsMref)) > 0) {
			for (; i > 0; i--)
				DeleteMenuItem(progsMref, i-1);
		}
		DeleteMenu(GetMenuID(progsMref));
		DisposeMenu(progsMref);
	}
}

static void UpdateClan(WindowPtr win) {
	GrafPtr		oldPort;
	Rect		rect;
	extern char ced_version[];

	GetPort(&oldPort);
    SetPortWindowPort(win);

	GetWindowPortBounds(win, &rect);
	EraseRect(&rect);

	set_CommandsInfo(win, FALSE);

	SetDialogItemUTF8(ced_version, win, I_Version_Txt, FALSE);
	DrawControls(win);
	SetPort(oldPort);
}

void changeCommandFolder(int which, FNType *text) {
	int	   len;
	FNType mDirPathName[FNSize];

	if (text == NULL || text[0] == EOS)
		return;
	if (!access(text, 0)) {
		if (which == 1) {
			strcpy(mDirPathName, wd_dir);
			strcpy(wd_dir, text);
			if (pathcmp(od_dir, mDirPathName) == 0)
				strcpy(od_dir, wd_dir);
			if (ClanWin != NULL)
				UpdateClan(ClanWin);
			strcpy(defaultPath, wd_dir);
			len = strlen(defaultPath);
			if (defaultPath[len-1] != PATHDELIMCHR)
				uS.str2FNType(defaultPath, len, PATHDELIMSTR);
			WriteCedPreference();
		} else if (which == 2) {
			strcpy(lib_dir, text);
			if (ClanWin != NULL)
				UpdateClan(ClanWin);
			WriteCedPreference();
		} else if (which == 3) {
			strcpy(mor_lib_dir, text);
			if (ClanWin != NULL)
				UpdateClan(ClanWin);
			WriteCedPreference();
		}
	} else {
		do_warning("Can't select specified Folders. Perhaps this folder no longer exists.", 0);
	}
}

static void createPopupProgMenu(void) {
	int			i, pi;
	CFStringRef cString;
	
	if ((i=CountMenuItems(progsMref)) > 0) {
		for (; i > 0; i--)
			DeleteMenuItem(progsMref, i);
	}
	i = 0;
	cString = CFStringCreateWithBytes(NULL, (UInt8 *)"Progs", strlen("Progs"), kCFStringEncodingUTF8, false);
	if (cString != NULL) {
		InsertMenuItemTextWithCFString(progsMref, cString, i, kMenuItemAttrCustomDraw, i);
		CFRelease(cString);
		i++;
	}
	for (pi=0; pi < MEGRASP; pi++) {
		cString = CFStringCreateWithBytes(NULL, (UInt8 *)clan_name[pi], strlen(clan_name[pi]), kCFStringEncodingUTF8, false);
		if (cString != NULL) {
			InsertMenuItemTextWithCFString(progsMref, cString, i, kMenuItemAttrCustomDraw, i);
			CFRelease(cString);
			i++;
		}
	}
	InsertMenu(progsMref, -1);
	Draw1Control(progsCtrlH);
}

void OpenCommandsWindow(char reOpen) {
	ControlRef	iCtrl;
	Rect		tempRect;
	OSErr		err;
	IBNibRef	mNibs;

	if (!init_clan())
		return;
	if (!reOpen) {
		if (isRefEQZero(wd_dir))
			strcpy(wd_dir, lib_dir);
		if (isRefEQZero(mor_lib_dir))
			strcpy(mor_lib_dir, lib_dir);
		if (isRefEQZero(od_dir)) {
			strcpy(od_dir, wd_dir);
		}
	}
	if (OpenWindow(501, Commands_str, 0L, false, 0, ClanEvent, UpdateClan, CleanupClan))
		ProgExit("Can't open select Clan window");
	if (!reOpen && ClanWin == NULL) {
		curProgNum = 0;
		strcpy(FilePattern, "*.cha");
		readAliases(1);
		func_init();
		InitOptions();
		ClanWin = FindAWindowNamed(Commands_str);
		if (ClanWin) {

			if ((err=CreateNibReference(CFSTR("CLAN"), &mNibs)) != noErr)
				progsMref = NULL;
			else  if ((err=CreateMenuFromNib(mNibs, CFSTR("pop_up"), &progsMref)) != noErr)
				progsMref = NULL;
			else {
				createPopupProgMenu();
				DisposeNibReference(mNibs);
				progsItemCtrl = GetWindowItemAsControl(ClanWin, I_Prog_icon);
				GetControlBounds(progsItemCtrl, &tempRect);
				progsCtrlH = NewControl(ClanWin,&tempRect,"\p",true,0,128,0,1008,0);
				if (progsCtrlH != nil)
					SetControlPopupMenuHandle(progsCtrlH, progsMref);
			}

			iCtrl = GetWindowItemAsControl(ClanWin, I_CM_Text);
			keyCommandsActions.win = ClanWin;
			mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyCommandsActions);
			AdvanceKeyboardFocus(ClanWin);
			HideClanWinIcons(ClanWin);
			KeyScript(mScript);
		}
	}
}

extern int  cl_argc;
extern char *cl_argv[];

#define usrData struct userWindowData
struct userWindowData {
	WindowPtr window;
	short res;
};

char isFilesSpecified(int *ProgNum) {
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

static pascal OSStatus SelectFilesDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
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

short SelectFilesDialog(char *additions) {
	int			i;
	ControlRef	tCtrl;
	WindowPtr	myDlg;
	usrData	userData;
	ControlKeyFilterUPP keyStrFilter = myStringFilter;
	
	myDlg = getNibWindow(CFSTR("tier_pre_select"));
	CenterWindow(myDlg, -3, -3);
	userData.window = myDlg;
	userData.res = 0;
	dial_isF1Pressed = FALSE;
	dial_isF2Pressed = FALSE;
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
	GetWindowUnicodeTextValue(tCtrl, templineW, UTTLINELEN);
	DisposeWindow(myDlg);
	if (userData.res == FILES_PAT) {
		u_strcpy(additions, templineW, UTTLINELEN);
		uS.remFrontAndBackBlanks(additions);
		strncpy(FilePattern, additions, 256);
		FilePattern[256] = EOS;
	} else if (userData.res == FILES_IN)
		strcpy(additions, " @");
	else
		additions[0] = EOS;
	return(userData.res);
}

