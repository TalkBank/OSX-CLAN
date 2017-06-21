#include "ced.h"
#include "c_clan.h"
#include "mac_commands.h"
#include "my_ctype.h"
#include "search.h"
#include "MMedia.h"
#include "mac_print.h"
#include "mac_MUPControl.h"

extern MenuRef SizeMenu;
extern MenuRef FontMenu;
// 04/01/04 extern char isSpellCheckFound;

Handle pasteStr;

#define NUMFAVFONTS 4
char FavFonts[NUMFAVFONTS][512];

void initFavFont(void) {
	FavFonts[0][0] = EOS;
	FavFonts[1][0] = EOS;
	FavFonts[2][0] = EOS;
	FavFonts[3][0] = EOS;
}

void addToFavFont(char *fontName, char isSavePrefs) {
	register int i;
	CFStringRef	theStr = NULL;

	for (i=0; i < NUMFAVFONTS; i++) {
		if (FavFonts[i][0] == EOS)
			break;
		if (strcmp(FavFonts[i], fontName) == 0)
			return;
	}
	if (i >= NUMFAVFONTS) {
		for (i=0; i < NUMFAVFONTS-1; i++)
			strcpy(FavFonts[i], FavFonts[i+1]);
	}
	strcpy(FavFonts[i], fontName);

	for (i=0; i < NUMFAVFONTS; i++) {
		if (FavFonts[i][0] != EOS) {
			theStr = my_CFStringCreateWithBytes(FavFonts[i]);
			if (theStr != NULL) {
				SetMenuItemTextWithCFString(FontMenu, Font_Fav_Start+i, theStr);
				CFRelease(theStr);
			} else {
				FavFonts[i][0] = EOS;
				theStr = my_CFStringCreateWithBytes("");
				if (theStr != NULL) {
					SetMenuItemTextWithCFString(FontMenu, Font_Fav_Start+i, theStr);
					CFRelease(theStr);
				}
			}
		} else {
			theStr = my_CFStringCreateWithBytes("");
			if (theStr != NULL) {
				SetMenuItemTextWithCFString(FontMenu, Font_Fav_Start+i, theStr);
				CFRelease(theStr);
			}
		}
	}
	if (isSavePrefs)
		WriteClanPreference();
}

void write_FavFonts_2014(FILE *fp) {
	register int i;

	for (i=0; i < NUMFAVFONTS; i++) {
		fprintf(fp, "%d=%s\n", 2014, FavFonts[i]);
	}
}

/* begin menu */
short DoFile(short theItem) {
	PrepareStruct	saveRec;
	WindowPtr		front;

	if (PlayingSound || PlayingContSound || PlayingContMovie)
		return -1;

 	switch (theItem) {	
		case New_Item:
			isAjustCursor = TRUE;
			OpenAnyFile(NEWFILENAME, 1962, FALSE);
			PrepWindow(0);
			break;
		case Open_Item:
			VisitFile(1);
			PrepWindow(0);
			break;
		case Select_Media:
			SelectMediaFile();
			FinishMainLoop();
			break;
		case Close_Item:
			if (StdInWindow != NULL && global_df != NULL && !strcmp(StdInWindow,global_df->fileName))
				do_warning(StdInErrMessage, 0);
			else {
				WindowProcRec *new_windProc;
				front = FrontWindow();
				new_windProc = WindowProcs(front);
				if (StdInWindow != NULL && new_windProc->id == 501 && !strcmp(StdInWindow,CLAN_Output_str))
					do_warning(StdInErrMessage, 0);
				else {
					PrepareWindA4(front, &saveRec);
					mCloseWindow(front);
					RestoreWindA4(&saveRec);
				}
			}
			break;
		case Save_item:
			if (StdInWindow && global_df && !strcmp(StdInWindow,global_df->fileName)) 
				do_warning(StdInErrMessage, 0);
			else if (global_df) {
				SaveUndoState(FALSE);
				global_df->LastCommand = SaveCurrentFile(0);
				FinishMainLoop();
			}
			break;
		case Save_As_item:
			if (StdInWindow && global_df && !strcmp(StdInWindow,global_df->fileName)) 
				do_warning(StdInErrMessage, 0);
			else if (global_df) {
				SaveUndoState(FALSE);
				global_df->LastCommand = clWriteFile(0);
				FinishMainLoop();
			}
			break;
		case Save_Clip_As_item:
			if (StdInWindow && global_df && !strcmp(StdInWindow,global_df->fileName)) 
				do_warning(StdInErrMessage, 0);
			else if (global_df) {
				if (global_df->SnTr.SoundFile[0] != EOS) {
					SaveSoundClip(&global_df->SnTr, TRUE);
					FinishMainLoop();
				} else {
					do_warning("Please play a clip before saving it", 0);
				}
			}
			break;
		case Print_Setup_Item:
			print_setup();
			break;
		case Print_Item:
			if (global_df) {
				printSelection = FALSE;
				print_file(global_df->head_text->next_row, global_df->tail_text);
			} else
				do_warning("Please open a file you want to print.", 0);
			break;
		case Print_Select_Item:
			if (global_df) {
				if (global_df->ScrollBar && (global_df->row_win2 || global_df->col_win2 != -2L)) {
					printSelection = TRUE;
					print_file(global_df->head_text->next_row, global_df->tail_text);
				} else
					do_warning("Please select text first.", 0);
			} else
				do_warning("Please open a file you want to print and select text.", 0);
			break;
		case Quit_Item:
			isPlayS = 6;
			return 0;
			break;
	}
	return -1;
}

static void doTECopy(WindowProcRec *windProc, char cut) {
	char state;
	ControlRef iCtrl = NULL;
	STPTextPaneVars **tpvars, *varsp;
	TXNOffset	oStartOffset, oEndOffset;
	WindowPtr win;

	win = FindAWindowProc(windProc);
	if (win == NULL)
		return;

	if (GetKeyboardFocus(win, &iCtrl) == noErr) {
		tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
		if (tpvars != NULL) {
			state = HGetState((Handle) tpvars);
			HLock((Handle) tpvars);
			varsp = *tpvars;
			if (!TXNIsSelectionEmpty(varsp->fTXNRec)) {
				ScrapRef	scrap;
				ClearCurrentScrap();
				HLock(TEGetScrapHandle());
				if (!GetCurrentScrap (&scrap)) {
					if (cut) {
						if (windProc->id == 500) {
							TXNGetSelection(varsp->fTXNRec, &oStartOffset, &oEndOffset); 
							saveMovieUndo(win, oStartOffset, oEndOffset, 1);
						}
						TXNCut(varsp->fTXNRec);
					} else
						TXNCopy(varsp->fTXNRec);
				}
				HUnlock(TEGetScrapHandle());
				if (windProc->id == 501) {
					if (cut) {
						if (set_fbuffer(win, FALSE)) {
							SetClanWinIcons(win, fbuffer);
						} else
							SetClanWinIcons(win, NULL);
					}
				}
			}
			HSetState((Handle) tpvars, state);
		}
	}
}

static void doTEPaste(WindowProcRec *windProc) {
	char	state;
	long    len;
	unCH	*pt;
	ScrapRef scrap;
	ControlRef iCtrl = NULL;
    TXNOffset oStartOffset, oEndOffset;
	STPTextPaneVars **tpvars, *varsp;
	WindowPtr win;

	win = FindAWindowProc(windProc);
	if (win == NULL)
		return;

	if (GetKeyboardFocus(win, &iCtrl) == noErr) {

		if (GetCurrentScrap(&scrap))
			goto fin;
		if (GetScrapFlavorSize(scrap,kScrapFlavorTypeUnicode,&len) == noErr) {
			if (len > 0) {
				ReserveMem(len);
				if (MemError( ))
					goto fin;

				pasteStr  = NewHandle(len);
				if (MemError ( ))
					goto fin;

				if (GetScrapFlavorData(scrap,kScrapFlavorTypeUnicode,&len,*pasteStr) != noErr) {
					DisposeHandle(pasteStr);
					pasteStr = 0;
					goto fin;
				}

				HLock(pasteStr);
				if ((pt=(unCH *)malloc(len+1)) == NULL)
					pt = templineW;
				strncpy(pt, (unCH *)*pasteStr, len/2);
				pt[len/2] = EOS;
				HUnlock(pasteStr);
				DisposeHandle(pasteStr);
				pasteStr = 0;

				ClearCurrentScrap();
				if (GetCurrentScrap(&scrap))
					goto fin;

				PutScrapFlavor(scrap, kScrapFlavorTypeUnicode, 0, len, (wchar_t *)pt);
				if (pt != templineW)
					free(pt);
			}
		}
fin:
		tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
		if (tpvars != NULL) {
			state = HGetState((Handle) tpvars);
			HLock((Handle) tpvars);
			varsp = *tpvars;
			if (windProc->id == 500) {
				TXNGetSelection(varsp->fTXNRec, &oStartOffset, &oEndOffset);
				saveMovieUndo(win, oStartOffset, oEndOffset, 1);
			}
			TXNPaste(varsp->fTXNRec);
			HSetState((Handle) tpvars, state);
			if (windProc->id == 500) {
				if (whichActive(win) == 5) {
					setMovieCursor(win);
				}
			} else if (windProc->id == 501) {
				if (set_fbuffer(win, FALSE)) {
					SetClanWinIcons(win, fbuffer);
				} else
					SetClanWinIcons(win, NULL);
			}
		}
	}
}

static void doTESelectAll(WindowProcRec *windProc) {
	char state;
	ControlRef iCtrl = NULL;
	STPTextPaneVars **tpvars, *varsp;
	WindowPtr win;

	win = FindAWindowProc(windProc);
	if (win == NULL)
		return;


	if (GetKeyboardFocus(win, &iCtrl) == noErr) {
		tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
		if (tpvars != NULL) {
			state = HGetState((Handle) tpvars);
			HLock((Handle) tpvars);
			varsp = *tpvars;
			TXNSelectAll(varsp->fTXNRec);
			HSetState((Handle) tpvars, state);
		}
	}
}

short DoEdit(short theItem, WindowProcRec *windProc) {
 	switch (theItem) {
		case undo_id:
			if (global_df) {
				SaveUndoState(TRUE);
				global_df->LastCommand = Undo(1);
				PosAndDispl();
			} else if (windProc->id == 500)
				doMovieUndo(windProc);
			break;
		case redo_id:
			if (global_df) {
				SaveUndoState(TRUE);
				global_df->LastCommand = Redo(1);
				PosAndDispl();
			} else if (windProc->id == 500)
				doMovieRedo(windProc);
			break;
		case cut_id:
			if (global_df) {
				doCut();
			} else
				doTECopy(windProc, TRUE);
			break;
		case copy_id:
			if (global_df) 
				doCopy();
			else
				doTECopy(windProc, FALSE);
			break;
		case paste_id:
			if (global_df) {
				doPaste();
			} else
				doTEPaste(windProc);
			break;
		case selectall_id:
			if (global_df) {
				isPlayS = 75;
			} else
				doTESelectAll(windProc);
 			break;
		case find_id:
			if (global_df == NULL) ;
			else if (FindDialog(SearchString, 0)) {
			    global_df->row_win2 = 0L;
			    global_df->col_win2 = -2L;
				global_df->col_chr2 = -2L;
				SaveUndoState(FALSE);
				if (SearchFFlag)
					global_df->LastCommand = SearchForward(-1);
				else
					global_df->LastCommand = SearchReverse(-1);
				FinishMainLoop();
			}
			break;
		case enterselct_id:
			EnterSelection();
			break;
		case findsame_id:
			if (global_df)
				isPlayS = 76;
			break;
		case replace_id:
			if (global_df) {
				isPlayS = 7;
			}
			break;
		case replacefind_id:
			if (global_df) {
				isPlayS = 86;
			}
			break;
		case goto_id:
			if (global_df)
				isPlayS = 12;
			break;

		case toupper_id:
			if (global_df != NULL)
				isPlayS = 10;
			break;
		case tolower_id:
			if (global_df != NULL)
				isPlayS = 11;
			break;
		case clanoptions_id: 
			DoCEDOptions();
			break;
		case setdefsndana_id: 
			DoSndAnaOptions();
			break;
		case setF5option_id: 
			DoSndF5Options();
			break;
		case setlinenumsize_id: 
			DoLineNumSize();
			break;
		case setURLaddress_id: 
			DoURLAddress();
			break;
		case setThumbnails_id: 
			DoThumbnails();
			break;
		case setStreamingSpeed_id: 
			SpeedDialog();
			break;
		case resetAllOptions_id:
			resetOptions();
			break;
		default:
			break;
	}
	return -1;
}

static void SetCurrentFontSize(short font, long fsize) {
	long  size;
	short j;
	char  ItemStr[256];
	CFStringRef outString;

	for (j=1; j <= Font_Size_End; j++) {
		CopyMenuItemTextAsCFString(SizeMenu, j, &outString);
		my_CFStringGetBytes(outString, ItemStr, 256);
		CFRelease(outString);
		size = atol(ItemStr);
		if (fsize == size) {
			SetItemMark(SizeMenu,j,checkMark);
		}
	}
	if (global_df != NULL)
		sprintf(spC+strlen(spC), "%ld", fsize);
}

void SetCurrentFontParams(short font, long size) {
	short j, fc;
	int   k, len;
	Str255 ItemStr;
	Str255 FontName;

	GetFontName(font,FontName);
	if (*FontName == 0)
		return;
	fc = CountMenuItems(FontMenu);
	for (j=1; j <= Font_Size_End; j++) {
		SetItemMark(SizeMenu,j,noMark);
		SetItemStyle(SizeMenu,j,0);
	}
	for (j=Font_Fav_Start; j <= Font_Fav_End; j++) {
		GetMenuItemText(FontMenu, j, ItemStr);
		if (EqualString(FontName,ItemStr,FALSE,FALSE)) {
			SetItemMark(FontMenu,j,checkMark);
		} else
			SetItemMark(FontMenu,j,noMark);
	}
	for (j=Font_Name_Start; j <= fc; j++) {
		GetMenuItemText(FontMenu, j, ItemStr);
		if (EqualString(FontName,ItemStr,FALSE,FALSE)) {
			SetItemMark(FontMenu,j,checkMark);
			if (global_df != NULL) {
				strcpy(spC, "Font: ");
				k = strlen(spC);
				len = ItemStr[0];
				strncpy(spC+k, (char *)ItemStr+1, len);
				spC[k+len] = EOS;
				strcat(spC, ":");
			}
			SetCurrentFontSize(font, size);
		} else
			SetItemMark(FontMenu,j,noMark);
	}
}

static void finishDefFontSize(short font, long size) {
	Str255			pFontName;
	WindowPtr		win;
	GrafPtr			savePort;
	WindowProcRec	*windProc;
	extern void reopenCLANText(void);

	if ((win=FrontWindow()) != NULL) {
		if ((windProc=WindowProcs(win))) {
			GetPort(&savePort);
			SetPortWindowPort(win);
//			TextFont(font);
//			TextSize(size);
			if (AutoScriptSelect) {
				cedDFnt.Encod = my_FontToScript(font, 0);
				if (windProc->FileInfo == NULL || !windProc->FileInfo->isUTF)
					KeyScript(cedDFnt.Encod);
			}
			SetPort(savePort);
			SetCurrentFontParams(font, size);
			dFnt.fontId = font;
			dFnt.fontSize = size;
			dFnt.Encod = my_FontToScript(dFnt.fontId, 0);
			dFnt.orgEncod = dFnt.Encod;
			dFnt.orgFType = NOCHANGE;
			dFnt.CharSet = dFnt.Encod;
			GetFontName(dFnt.fontId, pFontName);
			p2cstrcpy(defUniFontName, pFontName);
			defUniFontSize = dFnt.fontSize;

			if (!strcmp(defUniFontName, "Arial Unicode MS") || !strcmp(defUniFontName, "CAfont"/*UNICODEFONT*/))
				dFnt.isUTF = 1;
			copyNewFontInfo(&oFnt, &dFnt);
			if (defUniFontName[0] != 0) {
				C_MBF = (dFnt.Encod == 1 || dFnt.Encod == 2 || dFnt.Encod == 3);
			}
			WriteCedPreference();
//			if (windProc->id == 501) {
				reopenCLANText();
//				UpdateWindowProc(windProc);
//			}
		}
	}
}

void DoDefFont(short theItem) {
    short	font;
	Str255	ItemStr;
	char	favFont[512];
	OSStatus err;
	CFStringRef outString;

	if (theItem >= Font_Fav_Start && theItem <= Font_Fav_End) {
		GetMenuItemText(FontMenu, theItem, ItemStr);
		font = FMGetFontFamilyFromName(ItemStr);
//		GetFNum(ItemStr,&font);
		if (font != 0)
			finishDefFontSize(font, dFnt.fontSize);
	} else if (theItem >= Font_Name_Start && theItem <= CountMenuItems(FontMenu)) {
		GetMenuItemText(FontMenu, theItem, ItemStr);
		font = FMGetFontFamilyFromName(ItemStr);
//		GetFNum(ItemStr,&font);
		err = CopyMenuItemTextAsCFString(FontMenu, theItem, &outString);
		if (err == noErr) {
			my_CFStringGetBytes(outString, favFont, 512);
			addToFavFont(favFont, TRUE);
			CFRelease(outString);
		}
		finishDefFontSize(font, dFnt.fontSize);
	}
}

void DoDefSize_Style(short theItem) {
    long		fsize;
	char		ItemStr[256];
	CFStringRef outString;

	fsize = dFnt.fontSize;
	if (theItem > 0 && theItem <= Font_Size_End) {
		CopyMenuItemTextAsCFString(SizeMenu, theItem, &outString);
		my_CFStringGetBytes(outString, ItemStr, 256);
		CFRelease(outString);
		fsize = atol(ItemStr);
		finishDefFontSize(dFnt.fontId, fsize);
	} else if (theItem == Font_Smaller_Size && fsize > 8) {
		fsize--;
		finishDefFontSize(dFnt.fontId, fsize);
	} else if (theItem == Font_Larger_Size && fsize < 48) {
		fsize++;
		finishDefFontSize(dFnt.fontId, fsize);
	} else if (theItem == Font_Tab_Size) {
		setTabSize();
	}
}

static void finishFontSize(short font, short fsize) {
	int tScript;
	short fheight;
	GrafPtr savePort;
	FontInfo fi;
    ROWS *rt;

	GetPort(&savePort);
	SetPortWindowPort(FrontWindow());
	TextFont(font);
	TextSize(fsize);
	tScript = my_FontToScript(font, 0);
	if (AutoScriptSelect) {
		cedDFnt.Encod = tScript;
		cedDFnt.isUTF = global_df->isUTF;
		if (!global_df->isUTF)
			KeyScript(cedDFnt.Encod);
	}
	GetFontInfo(&fi);
	fheight = fi.ascent + fi.descent + fi.leading + FLINESPACE;
	if (global_df->MinFontSize > fheight || global_df->MinFontSize == 0)
		global_df->MinFontSize = fheight;
	SetPort(savePort);
	
	ChangeCurLineAlways(0);
	if (global_df->row_win2 == 0L && global_df->col_win2 == -2L) {
		for (rt=global_df->head_text->next_row; rt != global_df->tail_text; rt=ToNextRow(rt, FALSE)) {
			rt->Font.FName  = font;
			rt->Font.FSize = fsize;
			rt->Font.CharSet = tScript;
			rt->Font.FHeight = fheight;
		}
	} else {
		long cnt, tcnt;

		if (global_df->row_win2 == 0) {
			global_df->row_txt->Font.FName  = font;
			global_df->row_txt->Font.FSize = fsize;
			global_df->row_txt->Font.CharSet = tScript;
			global_df->row_txt->Font.FHeight = fheight;
		} else {
			cnt = global_df->row_win2;
			if (global_df->row_win2 < 0L) {
				if (global_df->col_win > 0)
					rt = global_df->row_txt;
				else {
					cnt++;
					rt = ToPrevRow(global_df->row_txt, FALSE);
				}
				for (; cnt<=0 && rt!=global_df->head_text; cnt++, rt=ToPrevRow(rt,FALSE)) {
					rt->Font.FName  = font;
					rt->Font.FSize = fsize;
					rt->Font.CharSet = tScript;
					rt->Font.FHeight = fheight;
				}
			} else if (global_df->row_win2 > 0L) {
				if (global_df->col_win2 <= 0) {
					tcnt = cnt;
					for (rt=global_df->row_txt; tcnt >= 0 && rt != global_df->tail_text;
										tcnt--, rt=ToNextRow(rt, FALSE)) ;
					if (ToNextRow(rt, FALSE) != global_df->tail_text)
						cnt--;
				}
				for (rt=global_df->row_txt; cnt >= 0 && rt != global_df->tail_text;
										cnt--, rt=ToNextRow(rt, FALSE)) {
					rt->Font.FName  = font;
					rt->Font.FSize = fsize;
					rt->Font.CharSet = tScript;
					rt->Font.FHeight = fheight;
				}
			}
		}
	}
	clear();
	global_df->total_num_rows = 1;
	global_df->CodeWinStart = 0;
	if (!init_windows(true, 1, false))
    	mem_err(TRUE, global_df);
	if (global_df->DataChanged == '\0')
		global_df->DataChanged = '\001';
	SetCurrentFontParams(font, fsize);
	global_df->WinChange = FALSE;
	strcpy(global_df->err_message, "-");
	strncat(global_df->err_message, spC, ERRMESSAGELEN-3);
	draw_mid_wm();
	global_df->WinChange = TRUE;
	*spC = EOS;
	strcpy(global_df->err_message, DASHES);
}

void DoFont(short theItem) {
    short font, fsize;
	Str255 ItemStr;
	char	favFont[512];
    ROWS *rt;
	OSStatus err;
	CFStringRef outString;

	if (global_df->row_win < 0 || global_df->row_win >= (long)global_df->EdWinSize)
		PutCursorInWindow(global_df->w1);
	DrawCursor(0);
	DrawSoundCursor(0);
	if (global_df->row_win2 == 0L && global_df->col_win2 == -2L) {
		font = global_df->row_txt->Font.FName;
		fsize = global_df->row_txt->Font.FSize;
	} else {
		if (global_df->col_win <= 0L && global_df->row_win2 < 0L)
			rt = ToPrevRow(global_df->row_txt, FALSE);
		else
			rt = global_df->row_txt;
		font  = rt->Font.FName;
		fsize = rt->Font.FSize;
	}

	if (theItem >= Font_Fav_Start && theItem <= Font_Fav_End) {
		GetMenuItemText(FontMenu, theItem, ItemStr);
		font = FMGetFontFamilyFromName(ItemStr);
//		GetFNum(ItemStr,&font);
		if (font != 0) {
			finishFontSize(font, fsize);
			if (global_df->row_win2 != 0L && global_df->col_win2 != -2L)
				finishDefFontSize(font, fsize);
		}
	} else if (theItem >= Font_Name_Start && theItem <= CountMenuItems(FontMenu)) {
		GetMenuItemText(FontMenu, theItem, ItemStr);
		font = FMGetFontFamilyFromName(ItemStr);
//		GetFNum(ItemStr,&font);
		err = CopyMenuItemTextAsCFString(FontMenu, theItem, &outString);
		if (err == noErr) {
			my_CFStringGetBytes(outString, favFont, 512);
			addToFavFont(favFont, TRUE);
			CFRelease(outString);
		}
		finishFontSize(font, fsize);
		if (global_df->row_win2 != 0L && global_df->col_win2 != -2L)
			finishDefFontSize(font, fsize);
	}
}

void DoSize_Style(short theItem) {
    short font, fsize;
    ROWS *rt;
	char		ItemStr[256];
	CFStringRef outString;

	if (theItem == Font_Color_Keywords) {
		if (global_df) {
			isPlayS = 85;
		}
		return;
	}
	if (global_df->row_win < 0 || global_df->row_win >= (long)global_df->EdWinSize)
		PutCursorInWindow(global_df->w1);
	DrawCursor(0);
	DrawSoundCursor(0);
	if (global_df->row_win2 == 0L && global_df->col_win2 == -2L) {
		font = global_df->row_txt->Font.FName;
		fsize = global_df->row_txt->Font.FSize;
	} else {
		if (global_df->col_win <= 0L && global_df->row_win2 < 0L)
			rt = ToPrevRow(global_df->row_txt, FALSE);
		else
			rt = global_df->row_txt;
		font  = rt->Font.FName;
		fsize = rt->Font.FSize;
	}

	if (theItem == Font_Plain) {
		StyleItems(0, 0);
	} else if (theItem == Font_Underline) {
		StyleItems(1, 0);
	} else if (theItem == Font_Italic) {
		if (!global_df->ChatMode)
			StyleItems(2, 0);
	} else if (theItem == Font_Bold) {
		if (!global_df->ChatMode)
			StyleItems(3, 0);
/* 18-07-2008	
	} else if (theItem == Font_Color_Words) {
		SelectWordColor();
*/
	} else if (theItem == Font_Tab_Size) {
		setTabSize();
	} else if (theItem <= Font_Size_End) {
		CopyMenuItemTextAsCFString(SizeMenu, theItem, &outString);
		my_CFStringGetBytes(outString, ItemStr, 256);
		CFRelease(outString);
		fsize = (short)atoi(ItemStr);
		finishFontSize(font, fsize);
		if (global_df->row_win2 != 0L && global_df->col_win2 != -2L)
			finishDefFontSize(font, fsize);
	} else if (theItem == Font_Smaller_Size && fsize > 8) {
		fsize--;
		finishFontSize(font, fsize);
		if (global_df->row_win2 != 0L && global_df->col_win2 != -2L)
			finishDefFontSize(font, fsize);
	} else if (theItem == Font_Larger_Size && fsize < global_df->TextWinSize / 2) {
		fsize++;
		finishFontSize(font, fsize);
		if (global_df->row_win2 != 0L && global_df->col_win2 != -2L)
			finishDefFontSize(font, fsize);
	}
}

/*
int SelectWordColor(void) { // F7
	char color;
	char SB;
	COLORWORDLIST *t;

	if (cColor == NULL)
		cColor = global_df->RootColorWord;
	else {
		for (t=global_df->RootColorWord; cColor != t && t != NULL; t=t->nextCW) ;
		if (cColor == t && cColor != NULL) {
			cColor = cColor->nextCW;
		} else
			cColor = global_df->RootColorWord;
	}
	if (cColor != NULL)
		color = cColor->color;
	else
		color = 0;

	SB = global_df->ScrollBar;
	global_df->ScrollBar = 1;
	StyleItems(4, color);
    global_df->ScrollBar = SB;

	global_df->LeaveHighliteOn = TRUE;
	return(83);
}
*/

void doCut(void) {
	if (global_df->ScrollBar && (global_df->row_win2 || global_df->col_win2 != -2L)) {
		if (global_df->row_win2 < -25 || global_df->row_win2 > 25)
			ResetUndos();
		SaveUndoState(FALSE);
		doCopy();
		DeleteChank(1);
		PosAndDispl();
		global_df->row_win2 = 0L;
		global_df->col_win2 = -2L;
		global_df->col_chr2 = -2L;
		SetTextWinMenus(TRUE);
	}
}

static AttTYPE *CHECKATT(AttTYPE *att, long *cnt, AttTYPE *oldAtt) {
	if (att != NULL) {
		*cnt = DealWithAtts(NULL, *cnt, *att, *oldAtt, TRUE);
		*oldAtt = *att;
		att++;
	} else {
		*cnt = DealWithAtts(NULL, *cnt, 0, *oldAtt, TRUE);
		*oldAtt = 0;
	}
	return(att);
}

static AttTYPE *HANDLEATT(unCH *pt, AttTYPE *att, long *cnt, AttTYPE *oldAtt) {
	if (att != NULL) {
		*cnt = DealWithAtts(pt, *cnt, *att, *oldAtt, FALSE);
		*oldAtt = *att;
		att++;
	} else {
		*cnt = DealWithAtts(pt, *cnt, 0, *oldAtt, FALSE);
		oldAtt = 0;
	}
	return(att);
}

static void CHECKDATA(unCH *src, long *cnt, char *NL_F) {
	if (*src == NL_C) {
		(*cnt) += 1;
		*NL_F = TRUE;
	} else {
		(*cnt) += 1;
	}
}

static void COPYDATA(unCH *pt, unCH *src, long *cnt, char *NL_F) {
	if (*src == NL_C) {
		pt[*cnt] = '\r';
		(*cnt) += 1;
		*NL_F = TRUE;
	} else {
		pt[*cnt] = *src;
		(*cnt) += 1;
	}
}

static void CHECKLINETERM(long *cnt, char NL_F) {
	if (global_df->ChatMode && !NL_F) {
		(*cnt) += 1;
	} else if (!NL_F) {
		(*cnt) += 1;
	}
}

static void LINETERMINATOR(unCH *pt, long *cnt, char NL_F) {
	if (global_df->ChatMode && !NL_F) {	\
		pt[*cnt] = '\r';
		(*cnt) += 1;
	} else if (!NL_F) {
		pt[*cnt] = ' ';
		(*cnt) += 1;
	}
}

static unsigned long createLineNumberStr(unCH *pt, long cnt, ROWS *st, long lineno) {
	long nDigs, len;

	len = 0;
	if (LineNumberingType == 0 || isMainSpeaker(st->line[0])) {
		uS.sprintf(templine4, cl_T("%ld"), lineno);
		if (LineNumberDigitSize < 0)
			nDigs = 6L;
		else if (LineNumberDigitSize > 0)
			nDigs = (unsigned long)LineNumberDigitSize;
		else
			nDigs = 6;
		if (nDigs > 20)
			nDigs = 20;
		len = strlen(templine4);
		while (len < nDigs) {
			strcat(templine4, " ");
			len++;
		}
	}
	if (pt != NULL)
		strcpy(pt+cnt, templine4);
	return(cnt+len);
}

char doCopy(void) {
    unCH *src;
    AttTYPE *att, oldAtt;
    char NL_F;
	long cnt;
	long sc, ec, tc;
	unsigned long len;
	long lineno;
	unCH *pt;
	ROWS *tst, *st, *et;

	if (global_df->ScrollBar && (global_df->row_win2 || global_df->col_win2 != -2L)) {
		ScrapRef scrap;
		ClearCurrentScrap();
		if (GetCurrentScrap (&scrap))
			return(FALSE);
		ChangeCurLineAlways(0);
		if (global_df->row_win2 == 0) {
			// for CLAN
			if (global_df->col_win > global_df->col_win2) {
				sc = global_df->col_chr2;
				ec = global_df->col_chr;
			} else {
				sc = global_df->col_chr;
				ec = global_df->col_chr2;
			}
			if (global_df->row_txt->att != NULL)
				att = global_df->row_txt->att + sc;
			else
				att = NULL;
			oldAtt = 0;
			cnt = 0L;
			for (tc=sc; tc < ec; tc++) {
				att = CHECKATT(att, &cnt, &oldAtt);
				cnt++;
			}
			src = global_df->row_txt->line;
			if ((pt=(unCH *)malloc((cnt+1)*sizeof(unCH))) == NULL) {
				strcpy(global_df->err_message, "+Cut/Copy; out of memory!");
				PosAndDispl();
				return(FALSE);
    		}
			if (global_df->row_txt->att != NULL)
				att = global_df->row_txt->att + sc;
			else
				att = NULL;
			oldAtt = 0;
			cnt = 0L;
			while (sc < ec) {
				att = HANDLEATT(pt, att, &cnt, &oldAtt);
				pt[cnt++] = src[sc++];
			}
			pt[cnt] = EOS;
			PutScrapFlavor(scrap, 'UCED', 0, cnt*2, pt);
			free(pt);

			// for other APPs
			if (FALSE/*isShowLineNums*/) {
				lineno = 1L;
				for (tst=global_df->head_text->next_row; tst != global_df->tail_text; tst=tst->next_row) {
					if (tst == global_df->row_txt)
						break;
					if (LineNumberingType == 0 || isMainSpeaker(tst->line[0]))
						lineno++;
				}
			}
			if (global_df->col_win > global_df->col_win2) {
				sc = global_df->col_chr2;
				ec = global_df->col_chr;
			} else {
				sc = global_df->col_chr;
				ec = global_df->col_chr2;
			}
			cnt = 0L;
			if (FALSE/*isShowLineNums*/)
				cnt = createLineNumberStr(NULL, cnt, global_df->row_txt, lineno);
			for (tc=sc; tc < ec; tc++) {
				cnt++;
			}
			src = global_df->row_txt->line;
			if ((pt=(unCH *)malloc((cnt+1)*sizeof(unCH))) == NULL) {
				strcpy(global_df->err_message, "+Cut/Copy; out of memory!");
				PosAndDispl();
				return(FALSE);
    		}
			cnt = 0L;
			if (FALSE/*isShowLineNums*/)
				cnt = createLineNumberStr(pt, cnt, global_df->row_txt, lineno);
			while (sc < ec) {
				if (src[sc] == HIDEN_C && global_df->ShowParags != '\002') {
					sc++;
					if (sc >= ec)
						break;
					while (sc < ec && src[sc] != HIDEN_C)
						sc++;
				} else
					pt[cnt++] = src[sc];
				sc++;
			}
			pt[cnt] = EOS;
			for (cnt=0L; pt[cnt]; ) {
				if (pt[cnt] == HIDEN_C) {
					pt[cnt] = 0x2022;
					cnt++;
				} else if (pt[cnt] == ATTMARKER)
					strcpy(pt+cnt, pt+cnt+2);
				else
					cnt++;
			}
			if (global_df->isUTF) {
				PutScrapFlavor(scrap, kScrapFlavorTypeUnicode, 0, cnt*2, pt);
				UnicodeToANSI(pt, cnt, (unsigned char *)templine4, &len, UTTLINELEN, FontToScript(global_df->row_txt->Font.FName));
				PutScrapFlavor(scrap, kScrapFlavorTypeText, 0, len, templine4);
			} else
				PutScrapFlavor(scrap, kScrapFlavorTypeText, 0, cnt, pt);
			free(pt);
		} else {
			if (global_df->row_win2 < 0L) {
				et = global_df->row_txt;
				for (cnt=global_df->row_win2, st=global_df->row_txt; cnt && !AtTopEnd(st,global_df->head_text,FALSE); 
								cnt++, st=ToPrevRow(st, FALSE)) ;
				sc = global_df->col_chr2; ec = global_df->col_chr;
			} else if (global_df->row_win2 > 0L) {
				st = global_df->row_txt;
				for (cnt=global_df->row_win2, et=global_df->row_txt; cnt && !AtBotEnd(et,global_df->tail_text,FALSE);
								cnt--, et=ToNextRow(et, FALSE)) ;
				sc = global_df->col_chr; ec = global_df->col_chr2;
			}
			tst = st;
    		cnt = 0L;
    		NL_F = FALSE;
			if (st->att != NULL)
				att = st->att + sc;
			else
				att = NULL;
			oldAtt = 0;
    		for (src=st->line+sc; *src; src++) {
				att = CHECKATT(att, &cnt, &oldAtt);
				CHECKDATA(src, &cnt, &NL_F);
   			}
   			CHECKLINETERM(&cnt, NL_F);
			if (!AtBotEnd(st,et,FALSE)) {
				do {
					st = ToNextRow(st, FALSE);
  			  		NL_F = FALSE;
					att = st->att;
  			  		for (src=st->line; *src; src++) {
  			  			att = CHECKATT(att, &cnt, &oldAtt);
  			  			CHECKDATA(src, &cnt, &NL_F);
   					}
   					CHECKLINETERM(&cnt, NL_F);
				} while (!AtBotEnd(st,et,FALSE)) ;
			}
			st = ToNextRow(st, FALSE);
			att = st->att;
			if (ec == strlen(st->line))
 	 			NL_F = FALSE;
			for (src=st->line, tc = ec; tc && *src; tc--, src++) {
				att = CHECKATT(att, &cnt, &oldAtt);
				CHECKDATA(src, &cnt, &NL_F);
			}
			if (ec == strlen(st->line)) {
   				CHECKLINETERM(&cnt, NL_F);
   			}
			if ((pt=(unCH *)malloc((cnt+1)*sizeof(unCH))) == NULL) {
				strcpy(global_df->err_message, "+Cut/Copy; out of memory!");
				PosAndDispl();
				return(FALSE);
    		}
			st = tst;
    		cnt = 0L;
    		NL_F = FALSE;
			if (st->att != NULL)
				att = st->att + sc;
			else
				att = NULL;
			oldAtt = 0;
    		for (src=st->line+sc; *src; src++) {
				att = HANDLEATT(pt, att, &cnt, &oldAtt);
				COPYDATA(pt, src, &cnt, &NL_F);
   			}
   			LINETERMINATOR(pt, &cnt, NL_F);
			if (!AtBotEnd(st,et,FALSE)) {
				do {
					st = ToNextRow(st, FALSE);
  			  		NL_F = FALSE;
					att = st->att;
  			  		for (src=st->line; *src; src++) {
  			  			att = HANDLEATT(pt, att, &cnt, &oldAtt);
  			  			COPYDATA(pt, src, &cnt, &NL_F);
   					}
   					LINETERMINATOR(pt, &cnt, NL_F);
				} while (!AtBotEnd(st,et,FALSE)) ;
			}
			st = ToNextRow(st, FALSE);
			att = st->att;
			if (ec == strlen(st->line))
 	 			NL_F = FALSE;
			for (src=st->line, tc = ec; tc && *src; tc--, src++) {
				att = HANDLEATT(pt, att, &cnt, &oldAtt);
				COPYDATA(pt, src, &cnt, &NL_F);
			}
			if (ec == strlen(st->line)) {
   				LINETERMINATOR(pt, &cnt, NL_F);
   			}
			pt[cnt] = EOS;
			PutScrapFlavor(scrap, 'UCED', 0, cnt*2, pt);

			// for other APPs
			if (global_df->row_win2 < 0L) {
				et = global_df->row_txt;
				for (cnt=global_df->row_win2, st=global_df->row_txt; cnt && !AtTopEnd(st,global_df->head_text,FALSE); 
					 cnt++, st=ToPrevRow(st, FALSE)) ;
				sc = global_df->col_chr2; ec = global_df->col_chr;
			} else if (global_df->row_win2 > 0L) {
				st = global_df->row_txt;
				for (cnt=global_df->row_win2, et=global_df->row_txt; cnt && !AtBotEnd(et,global_df->tail_text,FALSE);
					 cnt--, et=ToNextRow(et, FALSE)) ;
				sc = global_df->col_chr; ec = global_df->col_chr2;
			}
			if (isShowLineNums) {
				lineno = 1L;
				for (tst=global_df->head_text->next_row; tst != global_df->tail_text; tst=tst->next_row) {
					if (tst == st)
						break;
					if (LineNumberingType == 0 || isMainSpeaker(tst->line[0]))
						lineno++;
				}
			}
			tst = st;
    		cnt = 0L;
    		NL_F = FALSE;
			if (isShowLineNums)
				cnt = createLineNumberStr(NULL, cnt, st, lineno);
    		for (src=st->line+sc; *src; src++) {
				CHECKDATA(src, &cnt, &NL_F);
   			}
   			CHECKLINETERM(&cnt, NL_F);
			if (!AtBotEnd(st,et,FALSE)) {
				do {
					st = ToNextRow(st, FALSE);
  			  		NL_F = FALSE;
					if (isShowLineNums)
						cnt = createLineNumberStr(NULL, cnt, st, lineno);
  			  		for (src=st->line; *src; src++) {
  			  			CHECKDATA(src, &cnt, &NL_F);
   					}
   					CHECKLINETERM(&cnt, NL_F);
				} while (!AtBotEnd(st,et,FALSE)) ;
			}
			st = ToNextRow(st, FALSE);
			if (ec == strlen(st->line))
 	 			NL_F = FALSE;
			if (isShowLineNums)
				cnt = createLineNumberStr(NULL, cnt, st, lineno);
			for (src=st->line, tc = ec; tc && *src; tc--, src++) {
				CHECKDATA(src, &cnt, &NL_F);
			}
			if (ec == strlen(st->line)) {
   				CHECKLINETERM(&cnt, NL_F);
   			}
			if ((pt=(unCH *)malloc((cnt+1)*sizeof(unCH))) == NULL) {
				strcpy(global_df->err_message, "+Cut/Copy; out of memory!");
				PosAndDispl();
				return(FALSE);
    		}
			st = tst;
    		cnt = 0L;
    		NL_F = FALSE;
			if (isShowLineNums) {
				cnt = createLineNumberStr(pt, cnt, st, lineno);
				lineno++;
			}
    		for (src=st->line+sc; *src; src++) {
				if (*src == HIDEN_C && global_df->ShowParags != '\002') {
					src++;
					if (*src == EOS)
						break;
					while (*src != EOS && *src != HIDEN_C)
						src++;
					if (*src == EOS)
						break;
				} else
					COPYDATA(pt, src, &cnt, &NL_F);
   			}
   			LINETERMINATOR(pt, &cnt, NL_F);
			if (!AtBotEnd(st,et,FALSE)) {
				do {
					st = ToNextRow(st, FALSE);
  			  		NL_F = FALSE;
					if (isShowLineNums) {
						cnt = createLineNumberStr(pt, cnt, st, lineno);
						lineno++;
					}
  			  		for (src=st->line; *src; src++) {
						if (*src == HIDEN_C && global_df->ShowParags != '\002') {
							src++;
							if (*src == EOS)
								break;
							while (*src != EOS && *src != HIDEN_C)
								src++;
							if (*src == EOS)
								break;
						} else
							COPYDATA(pt, src, &cnt, &NL_F);
   					}
   					LINETERMINATOR(pt, &cnt, NL_F);
				} while (!AtBotEnd(st,et,FALSE)) ;
			}
			st = ToNextRow(st, FALSE);
			if (ec == strlen(st->line))
 	 			NL_F = FALSE;
			if (isShowLineNums) {
				cnt = createLineNumberStr(pt, cnt, st, lineno);
				lineno++;
			}
			for (src=st->line, tc = ec; tc && *src; tc--, src++) {
				if (*src == HIDEN_C && global_df->ShowParags != '\002') {
					src++;
					tc--;
					if (!tc || *src == EOS)
						break;
					while (tc && *src != EOS && *src != HIDEN_C) {
						src++;
						tc--;
					}
					if (!tc || *src == EOS)
						break;
				} else
					COPYDATA(pt, src, &cnt, &NL_F);
			}
			if (ec == strlen(st->line)) {
   				LINETERMINATOR(pt, &cnt, NL_F);
   			}
			pt[cnt] = EOS;
			for (cnt=0L; pt[cnt]; ) {
				if (pt[cnt] == HIDEN_C) {
					pt[cnt] = 0x2022;
					cnt++;
				} else if (pt[cnt] == ATTMARKER)
					strcpy(pt+cnt, pt+cnt+2);
				else
					cnt++;
			}
			if (global_df->isUTF) {
				PutScrapFlavor(scrap, kScrapFlavorTypeUnicode, 0, cnt*2, pt);
				UnicodeToANSI(pt, cnt, (unsigned char *)templine4, &len, UTTLINELEN, FontToScript(global_df->row_txt->Font.FName));
				PutScrapFlavor(scrap, kScrapFlavorTypeText, 0, len, templine4);
			} else
				PutScrapFlavor(scrap, kScrapFlavorTypeText, 0, cnt, pt);
			free(pt);
		}
		return(TRUE);
	} else
		return(FALSE);
}

void doPaste(void) {
	wchar_t *ws;
	char    *s;
	long    len;

	if (global_df->isTempFile == 1 || (global_df->isTempFile && global_df->EditorMode)) {
		do_warning("This is read only file. It can not be modified.", 0);
		return;
	}

	SaveUndoState(FALSE);
	ScrapRef scrap;
	if (GetCurrentScrap(&scrap))
		return;

	if (DeleteChank(1)) {
		global_df->row_win2 = 0L;
		global_df->col_win2 = -2L;
		global_df->col_chr2 = -2L;

		if (GetScrapFlavorSize(scrap,'UCED',&len) == noErr) {
			ReserveMem(len);
			if (MemError( ))
				return;
		} else
			len = 0L;
		pasteStr = NewHandle(len);
		if (MemError( ))
			return;
		if (len > 0) {
			if (global_df->row_win < 0 || global_df->row_win >= (long)global_df->EdWinSize)
				PutCursorInWindow(global_df->w1);
			if (GetScrapFlavorData(scrap,'UCED',&len,*pasteStr) != noErr) {
				DisposeHandle(pasteStr);
				pasteStr = 0;
				return;
			}
			HLock(pasteStr);
			len /= 2;
			ws = (wchar_t *)*pasteStr;
			ws[len] = 0;
			for (; *ws; ws++) {
				if (*ws == '\r') {
					if (global_df->ChatMode)
						*ws = SNL_C;
					else
						*ws = NL_C;
				}
			}
			if (global_df->RdW == global_df->w1) {
				global_df->WinChange = FALSE;
				if (len > 1625)
					ResetUndos();
				SaveUndoState(FALSE);
				AddText((unCH *)*pasteStr, EOS, 0, len);
				HUnlock(pasteStr);
				DisposeHandle(pasteStr);
				pasteStr = 0;
				DisplayTextWindow(NULL, 1);
				global_df->WinChange = TRUE;
				PosAndDispl();
			} else {
				HUnlock(pasteStr);
			}
			return;
		} else {
			DisposeHandle(pasteStr);
			pasteStr = 0;
		}

		if (GetScrapFlavorSize(scrap,kScrapFlavorTypeUnicode,&len) == noErr) {
			ReserveMem(len);
			if (MemError( ))
				return;
		} else
			len = 0L;
		pasteStr  = NewHandle(len);
		if (MemError ( ))
			return;
		if (len > 0) {
			if (global_df->row_win < 0 || global_df->row_win >= (long)global_df->EdWinSize)
				PutCursorInWindow(global_df->w1);
			if (GetScrapFlavorData(scrap,kScrapFlavorTypeUnicode,&len,*pasteStr) != noErr) {
				DisposeHandle(pasteStr);
				pasteStr = 0;
				return;
			}

			HLock(pasteStr);
			len /= 2;
			ws = (wchar_t *)*pasteStr;
			ws[len] = 0;
			for (; *ws; ws++) {
				if (*ws == 0x2022)
					*ws = HIDEN_C;
				else if (*ws == '\r') {
					if (global_df->ChatMode)
						*ws = SNL_C;
					else
						*ws = NL_C;
				}
			}
			if (global_df->RdW == global_df->w1) {
				global_df->WinChange = FALSE;
				if (len > 1625)
					ResetUndos();
				SaveUndoState(FALSE);
				AddText((unCH *)*pasteStr, EOS, 0, len);
				HUnlock(pasteStr);
				DisposeHandle(pasteStr);
				pasteStr = 0;
				DisplayTextWindow(NULL, 1);
				global_df->WinChange = TRUE;
				PosAndDispl();
			} else {
				HUnlock(pasteStr);
			}
			return;
		} else {
			DisposeHandle(pasteStr);
			pasteStr = 0;
		}
		if (GetScrapFlavorSize(scrap,kScrapFlavorTypeText,&len) == noErr) {
			ReserveMem(len);
			if (MemError( ))
				return;
		} else
			len = 0L;
		pasteStr = NewHandle(len);
		if (MemError ( ))
			return;
		if (len > 0) {
			if (global_df->row_win < 0 || global_df->row_win >= (long)global_df->EdWinSize)
				PutCursorInWindow(global_df->w1);
			if (GetScrapFlavorData(scrap,kScrapFlavorTypeText,&len,*pasteStr) != noErr) {
				DisposeHandle(pasteStr);
				pasteStr = 0;
				return;
			}
			HLock(pasteStr);
			(*pasteStr)[len] = 0;
			for (s=*pasteStr; *s; s++) {
				if (*s == '\r') {
					if (global_df->ChatMode)
						*s = SNL_C;
					else
						*s = NL_C;
				}
			}
			if (global_df->RdW == global_df->w1) {
				global_df->WinChange = FALSE;
				if (len > 1625)
					ResetUndos();
				SaveUndoState(FALSE);

				Handle tPasteStr;
				long tLen = (len+2)*sizeof(wchar_t);
				
				tPasteStr = NewHandle(tLen);
				HLock(tPasteStr);
				ANSIToUnicode((unsigned char *)*pasteStr, len, (wchar_t *)*tPasteStr, NULL, tLen, GetScriptManagerVariable(smKeyScript));
				AddText((unCH *)*tPasteStr, EOS, 0, len);
				HUnlock(tPasteStr);
				DisposeHandle(tPasteStr);
				HUnlock(pasteStr);
				DisposeHandle(pasteStr);
				pasteStr = 0;
				DisplayTextWindow(NULL, 1);
				global_df->WinChange = TRUE;
				PosAndDispl();
			} else {
				HUnlock(pasteStr);
			}
		} else {
			DisposeHandle(pasteStr);
			pasteStr = 0;
		}
	}
}
/* end menu */
