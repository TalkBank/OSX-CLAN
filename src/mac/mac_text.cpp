#include "ced.h"
#include "c_clan.h"

#if defined(_MAC_CODE) || defined(_WIN32)
extern void RecompEdWinSize(void);
#endif

struct MPSR1016 {
    long topLChr, pos1Chr, pos2Chr;
    long skipTopLChr, skipPos1Chr, skipPos2Chr;
} ;

extern struct DefWin defWinSize;
extern char isCursorPosRestore;

char isAjustCursor = TRUE;
char isTextWinChatMode = TRUE;

static char text_isF1Pressed = FALSE, text_isF2Pressed = FALSE;

#include "mac_text_bullet.h"
#ifndef TEXT_BULLETS_PICTS // text_bullet
int DisplayText(FNType *fname) {
	if (*fname != EOS) {
		if (my_access(fname, 0)) {
			sprintf(templineC, "Can't open file \"%s\".", fname);
			do_warning(templineC, 0);
			return(FALSE);
		}
		isAjustCursor = FALSE;
		isTextWinChatMode = FALSE;
		OpenAnyFile(fname, 1966, FALSE);
		isTextWinChatMode = TRUE;
	}
	return(1);
}
#endif //TEXT_BULLETS_PICTS // text_bullet

static void locateCharInText(myFInfo *fi, ROWS *cRow, long pos, long *aPos, long *skip) {
	long i;
	ROWS *tr;

	*aPos = 0L;
	*skip = 0;
	for (tr=fi->head_text->next_row; tr != fi->tail_text; tr = tr->next_row) {
		if (tr == cRow) {
			for (i=0; tr->line[i] != EOS && i < pos; i++) {
				if (tr->line[i]!='\n' && !isSpace(tr->line[i]) && tr->line[i]!=NL_C && tr->line[i]!=SNL_C) {
					*skip = 0;
					(*aPos)++;
				} else
					(*skip)++;
			}
			break;
		}
		for (i=0; tr->line[i] != EOS; i++) {
			if (tr->line[i]!='\n' && !isSpace(tr->line[i]) && tr->line[i]!=NL_C && tr->line[i]!=SNL_C) {
				*skip = 0;
				(*aPos)++;
			} else
				(*skip)++;
		}
	}
}

static char locateTopWinFromChar(long pos, long skip) {
	long i;
	long cPos, cSkip;
	long trow_win, twindow_rows_offset;
	ROWS *tr;

	trow_win = global_df->row_win;
	twindow_rows_offset = global_df->window_rows_offset;
	cPos = 0L;
	cSkip = 0L;
	tr = global_df->head_text->next_row;
	if (cPos == pos && skip == 0L) {
		global_df->top_win = tr;
		return(TRUE);
	}
	for (; tr != global_df->tail_text; tr = tr->next_row) {
		for (i=0; tr->line[i] != EOS; i++) {
			if (tr->line[i]!='\n' && !isSpace(tr->line[i]) && tr->line[i]!=NL_C && tr->line[i]!=SNL_C) {
				if (cPos == pos && skip != 0L) {
					global_df->top_win = tr;
					return(TRUE);
				}
				cSkip = 0L;
				cPos++;
			} else {
				cSkip++;
			}
			if (cPos == pos && skip == 0L) {
				global_df->top_win = tr;
				return(TRUE);
			}
		}
		global_df->row_win--;
		global_df->window_rows_offset++;
		if (global_df->row_win >= 0L && global_df->row_win < global_df->EdWinSize) 
			global_df->window_rows_offset = 0;
	}
	global_df->row_win = trow_win;
	global_df->window_rows_offset = twindow_rows_offset;
	return(FALSE);
}

static char locateRowTxtFromChar(long pos, long skip) {
	long i;
	long cPos, cSkip;
	long trow_win, twindow_rows_offset, tlineno;
	ROWS *tr;

	trow_win = global_df->row_win;
	twindow_rows_offset = global_df->window_rows_offset;
	tlineno = global_df->lineno;
	cPos = 0L;
	cSkip = 0L;
	tr = global_df->row_txt;
	if (cPos == pos && skip == 0L) {
		global_df->row_txt = tr;
		global_df->col_chr = 0L;
		return(TRUE);
	}
	for (; tr != global_df->tail_text; tr = tr->next_row) {
		for (i=0; tr->line[i] != EOS; i++) {
			if (tr->line[i]!='\n' && !isSpace(tr->line[i]) && tr->line[i]!=NL_C && tr->line[i]!=SNL_C) {
				if (cPos == pos && skip != 0L) {
					global_df->row_txt = tr;
					global_df->col_chr = i;
					return(TRUE);
				}
				cSkip = 0L;
				cPos++;
			} else {
				cSkip++;
			}
			if (cPos == pos && skip == 0L) {
				global_df->row_txt = tr;
				global_df->col_chr = i + 1;
				return(TRUE);
			}
		}
		global_df->lineno++;
		global_df->row_win++;
		global_df->window_rows_offset--;
		if (global_df->row_win >= 0L && global_df->row_win < global_df->EdWinSize) 
			global_df->window_rows_offset = 0;
	}
	global_df->row_win = trow_win;
	global_df->window_rows_offset = twindow_rows_offset;
	global_df->lineno = tlineno;
	return(FALSE);
}

static ROWS *FindRowTxt(ROWS *orgTr) {
	ROWS *tr;

	for (tr=orgTr; tr != global_df->tail_text; tr = tr->next_row) {
		if (tr == global_df->row_txt)
			return(orgTr);
		global_df->row_win2--;
	}
	global_df->row_win2 = 0L;
	return(NULL);
}

static ROWS *locate2RowTxtFromChar(long pos, long skip) {
	char foundRowTxt;
	long i;
	long cPos, cSkip;
	ROWS *tr;

	cPos = 0L;
	cSkip = 0L;
	tr = global_df->head_text->next_row;
	if (tr == global_df->row_txt) {
		foundRowTxt = TRUE;
	} else
		foundRowTxt = FALSE;
	if (cPos == pos && skip == 0L) {
		global_df->col_chr2 = 0L;
		if (!foundRowTxt)
			return(FindRowTxt(tr));
		else
			return(tr);
	}
	for (; tr != global_df->tail_text; tr = tr->next_row) {
		if (tr == global_df->row_txt)
			foundRowTxt = TRUE;
		for (i=0; tr->line[i] != EOS; i++) {
			if (tr->line[i]!='\n' && !isSpace(tr->line[i]) && tr->line[i]!=NL_C && tr->line[i]!=SNL_C) {
				if (cPos == pos && skip != 0L) {
					global_df->col_chr2 = i;
					if (!foundRowTxt)
						return(FindRowTxt(tr));
					else
						return(tr);
				}
				cSkip = 0L;
				cPos++;
			} else {
				cSkip++;
			}
			if (cPos == pos && skip == 0L) {
				global_df->col_chr2 = i + 1;
				if (!foundRowTxt)
					return(FindRowTxt(tr));
				else
					return(tr);
			}
		}
		if (foundRowTxt)
			global_df->row_win2++;
	}
	global_df->row_win2 = 0L;
	global_df->col_win2 = -2L;
	global_df->col_chr2 = -2L;
	return(NULL);
}

void ajustCursorPos(FNType *name) {
    Handle InfoHand;
	short ResRefNum;
	int   oldResRefNum;
	long  topLChr, skipTopLChr;
	long  pos1Chr, skipPos1Chr;
	long  pos2Chr, skipPos2Chr;
	ROWS  *tr;
	FSRef  ref;
	struct MPSR1016 *cursorInfo;

	if (!isCursorPosRestore)
		return;
	InfoHand = NULL;
	oldResRefNum = CurResFile();
	my_FSPathMakeRef(name, &ref); 
	if ((ResRefNum=FSOpenResFile(&ref, fsRdPerm)) != -1) {
		UseResFile(ResRefNum);
		if ((InfoHand=GetResource('MPSR',1016))) {
			HLock(InfoHand);
			cursorInfo = (struct MPSR1016 *)*InfoHand;
			topLChr = CFSwapInt32BigToHost(cursorInfo->topLChr);
			skipTopLChr = CFSwapInt32BigToHost(cursorInfo->skipTopLChr);
			pos1Chr = CFSwapInt32BigToHost(cursorInfo->pos1Chr);
			skipPos1Chr = CFSwapInt32BigToHost(cursorInfo->skipPos1Chr);
			pos2Chr = CFSwapInt32BigToHost(cursorInfo->pos2Chr);
			skipPos2Chr = CFSwapInt32BigToHost(cursorInfo->skipPos2Chr);
			HUnlock(InfoHand);
		}
		CloseResFile(ResRefNum);
	}
	UseResFile(oldResRefNum);
	if (InfoHand != NULL) {
		DrawCursor(0);
		DrawSoundCursor(0);
		global_df->row_win = 0L;
		global_df->col_win = 0L;
		global_df->col_chr = 0L;
		global_df->row_win2 = 0L;
		global_df->col_win2 = -2L;
		global_df->col_chr2 = -2L;
		global_df->LeaveHighliteOn = FALSE;
		global_df->lineno = 1L;
		global_df->row_txt = global_df->head_text->next_row;

		if (!locateTopWinFromChar(topLChr, skipTopLChr)) {
			DisplayTextWindow(NULL, 1);
			FinishMainLoop();
			return;
		}
		if (!locateRowTxtFromChar(pos1Chr, skipPos1Chr)) {
			DisplayTextWindow(NULL, 1);
			FinishMainLoop();
			return;
		}
		if (global_df->row_txt == global_df->cur_line) {
			global_df->col_txt = global_df->head_row->next_char;
			topLChr = 0L;
			for (; topLChr < global_df->col_chr && global_df->col_txt != global_df->tail_row; topLChr++)
				global_df->col_txt = global_df->col_txt->next_char;
			if (global_df->col_txt->prev_char->c == NL_C &&
							global_df->ShowParags != '\001' && global_df->col_chr > 0) {
				global_df->col_chr--;
				global_df->col_txt = global_df->col_txt->prev_char;
			}
			global_df->col_win = ComColWin(FALSE, NULL, global_df->col_chr);
		} else {
			if (global_df->row_txt->line[global_df->col_chr-1] == NL_C &&
							global_df->ShowParags != '\001' && global_df->col_chr > 0)
				global_df->col_chr--;
			global_df->col_win = ComColWin(FALSE, global_df->row_txt->line, global_df->col_chr);
		}
		if (pos2Chr != pos1Chr || skipPos2Chr != skipPos1Chr) {
			if ((tr=locate2RowTxtFromChar(pos2Chr, skipPos2Chr)) != NULL) {
				if (tr->line[global_df->col_chr2-1] == NL_C &&
								global_df->ShowParags != '\001' && global_df->col_chr2 > 0)
					global_df->col_chr2--;
				global_df->col_win2 = ComColWin(FALSE, tr->line, global_df->col_chr2);
			}
		}
		DisplayTextWindow(NULL, 1);
		FinishMainLoop();
	} else
		DisplayTextWindow(NULL, 1);
}

void setCursorPos(WindowPtr wind) {
    Handle InfoHand;
	int   oldResRefNum;
	char  dateFound;
	UInt32  dateValue;
	long  topLChr, skipTopLChr;
	long  pos1Chr, skipPos1Chr;
	long  pos2Chr, skipPos2Chr;
	short ResRefNum;
	struct MPSR1016 *cursorInfo;
	ROWS  *tr;
	FSRef  ref;
	FSSpec fss;
	WindowProcRec	*windProc;

	windProc = WindowProcs(wind);
	if (windProc == NULL || windProc->FileInfo == NULL)
		return;

	if (getFileDate(windProc->wname, &dateValue)) {
		windProc->fileDate = dateValue;
		dateFound = TRUE;
	} else
		dateFound = FALSE;

	if (!isCursorPosRestore)
		return;

	oldResRefNum = CurResFile();
	my_FSPathMakeRef(windProc->wname, &ref); 
	FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, &fss, NULL);
	if ((ResRefNum=HOpenResFile(fss.vRefNum,fss.parID,fss.name,fsRdWrShPerm)) == -1) {
		if (!my_access(windProc->wname,0)) {
			HCreateResFile(fss.vRefNum,fss.parID,fss.name);
			ResRefNum=HOpenResFile(fss.vRefNum,fss.parID,fss.name,fsRdWrShPerm);
		}
	}
/*
	 if ((ResRefNum=FSOpenResFile(&ref, fsRdWrShPerm)) == -1) {
		if (!my_access(windProc->wname,0)) {
			FSCreateResourceFork(&ref, 0, NULL, 0);
			ResRefNum = FSOpenResFile(&ref, fsRdWrShPerm);
		} else
			ResRefNum = -1;
	}
*/
	InfoHand = NULL;
	if (ResRefNum != -1) {
		tr = windProc->FileInfo->row_txt;
		locateCharInText(windProc->FileInfo, windProc->FileInfo->top_win, 0, &topLChr, &skipTopLChr);
		locateCharInText(windProc->FileInfo, tr, windProc->FileInfo->col_chr, &pos1Chr, &skipPos1Chr);
		if (windProc->FileInfo->row_win2 || windProc->FileInfo->col_win2 != -2L) {
			pos2Chr = windProc->FileInfo->row_win2;
			if (pos2Chr < 0L) {
				for (; pos2Chr && tr->prev_row != global_df->head_text; pos2Chr++, tr=tr->prev_row) ;
			} else if (pos2Chr > 0L) {
				for (; pos2Chr && tr->next_row != global_df->tail_text; pos2Chr--, tr=tr->next_row) ;
			}
			locateCharInText(windProc->FileInfo, tr, windProc->FileInfo->col_chr2, &pos2Chr, &skipPos2Chr);
		} else {
			pos2Chr = pos1Chr;
			skipPos2Chr = skipPos1Chr;
		}

		UseResFile(ResRefNum);
		InfoHand = Get1Resource('MPSR',1016);
		if (InfoHand) {
			HLock(InfoHand);
			cursorInfo = (struct MPSR1016 *)*InfoHand;
			cursorInfo->topLChr = CFSwapInt32HostToBig(topLChr);
			cursorInfo->skipTopLChr = CFSwapInt32HostToBig(skipTopLChr);
			cursorInfo->pos1Chr = CFSwapInt32HostToBig(pos1Chr);
			cursorInfo->skipPos1Chr = CFSwapInt32HostToBig(skipPos1Chr);
			cursorInfo->pos2Chr = CFSwapInt32HostToBig(pos2Chr);
			cursorInfo->skipPos2Chr = CFSwapInt32HostToBig(skipPos2Chr);
			HUnlock(InfoHand);
			ChangedResource(InfoHand);
		} else {
			InfoHand = NewHandle(sizeof(struct MPSR1016));
			if (InfoHand) {
				HLock(InfoHand);
				cursorInfo = (struct MPSR1016 *)*InfoHand;
				cursorInfo->topLChr = CFSwapInt32HostToBig(topLChr);
				cursorInfo->skipTopLChr = CFSwapInt32HostToBig(skipTopLChr);
				cursorInfo->pos1Chr = CFSwapInt32HostToBig(pos1Chr);
				cursorInfo->skipPos1Chr = CFSwapInt32HostToBig(skipPos1Chr);
				cursorInfo->pos2Chr = CFSwapInt32HostToBig(pos2Chr);
				cursorInfo->skipPos2Chr = CFSwapInt32HostToBig(skipPos2Chr);
				HUnlock(InfoHand);
				AddResource(InfoHand, 'MPSR', 1016, "\p");
			}
		}
		CloseResFile(ResRefNum);
	}
	if (dateFound && InfoHand != NULL)
		setFileDate(windProc->wname,dateValue);
	UseResFile(oldResRefNum);
}

int OpenAnyFile(const FNType *fn, int id, char winShift) {
	FNType	name[FNSize];
	long	width, height, top, left;	

	if (fn == NULL) {
		OSType typeList[1];
		typeList[0] = 'TEXT';
		name[0] = EOS;
		if (!myNavGetFile(nil, -1, typeList, nil, name))
			return(59);
	} else {
		strcpy(name, fn);
	}
	if (OpenWindow(id, name, 0L, TRUE, 2, MainTextEvent, UpdateMainText, NULL))
		ProgExit("Can't open any window");
	else {
		if (winShift) {
			WindowPtr win = FindAWindowNamed(name);
			if (win != NULL && GetPrefSize(id, name, &height, &width, &top, &left)) {
				if (labs(defWinSize.top-top) < 10 || labs(defWinSize.left-left) < 10) {
					MoveWindow(win, left+20, top+20, FALSE);
				}
			}
		}
		if (!isTextWinChatMode) {
			WindowPtr win = FindAWindowNamed(name);
			WindowProcRec *windProc = WindowProcs(win);
			if (windProc != NULL && windProc->FileInfo->ChatMode) {
				myFInfo *saveGlobal_df;

				saveGlobal_df = global_df;
				global_df = windProc->FileInfo;
				ChatModeSet(-1);
				global_df = saveGlobal_df;
			}
		}
	}
	return(59);
}

/* CenterWindow: puts topleft corner at (forLR, forTB), but centers if forLR/TB==-1
   Centers CenterWindow horizontally if forLR==-1, else left=forlR;
   Centers CenterWindow vertically if forTB==-1, else top=forTB; */
void CenterWindow(WindowPtr GetSelection, short forLR, short forTB) {
	char  textWinFound;
	short width, height, left, top, sHeight, sWidth;
	Rect		tempRect, dialRect;
	WindowPtr	front;
	WindowProcRec *windProc;
	extern Rect	mainScreenRect;

	GetPortBounds(GetWindowPort(GetSelection), &dialRect);
	textWinFound = FALSE;
	if ((front=FrontWindow()) != NULL) {
		if ((windProc=WindowProcs(front)) != NULL) {
			if (windProc->FileInfo != NULL || (forTB == -3 && windProc->id == 501)) {
				GetWindTopLeft(front, &left, &top);
				width = windWidth(front);
				height = windHeight(front);
				textWinFound = TRUE;
			}
		} else if (forTB == -4) {
			GetWindTopLeft(front, &left, &top);
			width = windWidth(front);
			height = windHeight(front);
			textWinFound = TRUE;
		}
	}
	if (forTB == -3 || forTB == -4) {
		forTB = -1;
		forLR = -1;
	}
	tempRect.top = forTB;
	tempRect.left = forLR;
	if (forTB == -2){
		sHeight = dialRect.bottom - dialRect.top + 10;
		tempRect.top = ((mainScreenRect.bottom+mainScreenRect.top+20)-sHeight)/2;
	} else if (forTB == -1){
		sHeight = dialRect.bottom - dialRect.top + 10;
		if (textWinFound) {
			if (mainScreenRect.bottom - top < sHeight)
				tempRect.top = mainScreenRect.bottom - sHeight;
			else if (top+height > mainScreenRect.bottom)
				tempRect.top = top + ((mainScreenRect.bottom - top - sHeight) / 2);
			else if (height > sHeight)
				tempRect.top = top + ((height - sHeight) / 2);
			else
				tempRect.top = top;

			if (tempRect.top < mainScreenRect.top+30)
				tempRect.top = mainScreenRect.top+30;
		} else {
			tempRect.top = ((mainScreenRect.bottom+mainScreenRect.top+20)-sHeight)/2;
		}
	}
	if (forLR == -2){
		sWidth = dialRect.right - dialRect.left + 10;
		tempRect.left = ((mainScreenRect.right+mainScreenRect.left)-sWidth)/2;
	} else if (forLR == -1){
		sWidth = dialRect.right - dialRect.left + 10;
		if (textWinFound) {
			if (left - mainScreenRect.left >= sWidth)
				tempRect.left = left - sWidth;
			else if (mainScreenRect.right - (left + width) >= sWidth)
				tempRect.left = left + width;
			else
				tempRect.left = mainScreenRect.right - sWidth;

			if (tempRect.left < mainScreenRect.left)
				tempRect.left = mainScreenRect.left;
		} else {
			tempRect.left = ((mainScreenRect.right+mainScreenRect.left)-sWidth)/2;
		}
	}
	MoveWindow(GetSelection, tempRect.left, tempRect.top, FALSE);
}

int VisitFile(int i) {
	isAjustCursor = TRUE;
	if (i > -1)
		OpenAnyFile(NULL, 1962, FALSE);
	else
		OpenAnyFile(NEWFILENAME, 1962, FALSE);
	return(59);
}

static void lExtractFileName(unCH *line, FNType *fname, char term) {
	long  i;
	FNType mFileName[FNSize];

	for (i=0L; line[i] != term && line[i] != EOS; i++) ;
	if (line[i] == term) {
		line[i] = EOS;
		if (!strcmp(line, "Stdin"))
			return;

		if (!isRefEQZero(line)) {
			u_strcpy(fname, line, FNSize);
		} else {
			u_strcpy(mFileName, line, FNSize);
			extractPath(fname, global_df->fileName);
			addFilename2Path(fname, mFileName);
			if (my_access(fname, 0)) {
				strcpy(fname, wd_dir);
				addFilename2Path(fname, mFileName);
			}
		}
		strcpy(line, line+i+1);
	}
}

char FindFileLine(char isTest, char *text) {
	FNType fname[FNSize], *ext;
	char isZeroFound = FALSE;
	unCH *line;
	long len;
	long ln = 0L;
	LINE *tcol;
	extern char isDontAskDontTell;

	if (text != NULL) {
		strncpy(ced_line, text, UTTLINELEN);
		ced_line[UTTLINELEN] = EOS;
	} else if (global_df->row_txt == global_df->cur_line) {
		tcol = global_df->head_row->next_char;
		for (len=0; tcol->c != NL_C && tcol != global_df->tail_row; tcol=tcol->next_char) {
			ced_line[len++] = tcol->c;
		}
		ced_line[len] = EOS;
	} else {
		strncpy(ced_line, global_df->row_txt->line, UTTLINELEN);
		ced_line[UTTLINELEN] = EOS;
	}
	if (ced_line[0] == '0') {
		isZeroFound = TRUE;
		strcpy(ced_line, ced_line+1);
	}
	len = (long)strlen("@Comment:	");
	if (strncmp(ced_line, "@Comment:	", (size_t)len) == 0)
		strcpy(ced_line, ced_line+len);
	line = ced_line;
	*fname = EOS;

	if (strncmp(line, "From file <", (size_t)11) == 0) {
		strcpy(line, line+11);
		lExtractFileName(line, fname, '>');
	} else if (strncmp(line, "Output file <", (size_t)13) == 0) {
		strcpy(line, line+13);
		lExtractFileName(line, fname, '>');
	} else if (*line == '*' || *line == ' ') {
		for (len=0L; line[len] == '*'; len++) ;
		for (; line[len] == ' '; len++) ;
		strcpy(line, line+len);
		len = strlen("File \"");
		if (strncmp(line, "File \"", len) == 0) {
			strcpy(line, line+len);
			lExtractFileName(line, fname, '"');
		}
		if (*fname == EOS)
			return(FALSE);

		for (len=0L; line[len] == ':' || line[len] == ' '; len++) ;
		if (len > 0L)
			strcpy(line, line+len);
		len = strlen("line ");
		if (strncmp(line, "line ", len) == 0) {
			strcpy(line, line+len);
			ln = atol(line);
		}
	} else
		return(FALSE);

	if (*fname != EOS && !isTest) {
		if (my_access(fname, 0)) {
			sprintf(templineC, "Can't open file \"%s\".", fname);
			do_warning(templineC, 0);
			return(FALSE);
		}
		isAjustCursor = FALSE;
		ext = strrchr(fname, '.');
		if (ext != NULL && !strcmp(ext, ".xls")) {
			strcpy(FileName2, "open \"");
			strcat(FileName2, fname);
			strcat(FileName2, "\"");
			if (!system(FileName2))
				return(TRUE);
		}
		if (isZeroFound)
			isDontAskDontTell = TRUE;
		OpenAnyFile(fname, 1962, TRUE);
		isDontAskDontTell = FALSE;
		if (ln > 0L && global_df != NULL) {
			SaveUndoState(FALSE);
			DrawCursor(0);
			DrawSoundCursor(0);
			global_df->row_win2 = 0L;
			global_df->col_win2 = 0L;
			global_df->col_chr2 = 0L;
//			global_df->LeaveHighliteOn = FALSE;
			MoveToLine(ln, 1);
			global_df->row_win2 = 1L;
			global_df->LeaveHighliteOn = TRUE;
			FlushEvents(mDownMask, 0);
			FlushEvents(mUpMask, 0);
			draw_mid_wm();
		}
	}
	return(TRUE);
}

short MainTextEvent(WindowPtr win, EventRecord *event) {
	int				extend = 0;
	int				code;
	int				saveIsPlayS;
	int				saveF_key;
	ControlRef		theControl;
	WindowProcRec	*windProc;
	myFInfo 		*saveGlobal_df;

	windProc = WindowProcs(win);
	if (windProc == NULL) return(-1);
	if (windProc->FileInfo == NULL) return(-1);
	saveGlobal_df = global_df;
	saveIsPlayS = isPlayS;
	saveF_key   = F_key;
	global_df = windProc->FileInfo;

	if (event->what == keyDown || event->what == autoKey) {
		int		key;
		wchar_t res;

		global_df->isExtend = 0;
		if (event->modifiers & shiftKey) {
			global_df->isExtend = 1;
			global_df->LeaveHighliteOn = TRUE;
		}
		key = event->message & keyCodeMask;
		if (key == 0x7200)
			isPlayS = 15;
		else if (key == 0x7300) {
			if (event->modifiers & controlKey)
				isPlayS = 28;
			else
				isPlayS = 44;
		} else if (key == 0x7700) {
			if (event->modifiers & controlKey)
				isPlayS = 29;
			else
				isPlayS = 45;
		} else if (key == 0x7400)
			isPlayS = 31;
		else if (key == 0x7900)
			isPlayS = 30;
		else if (key == 0x7C00 && event->modifiers & controlKey)
			isPlayS = 94;
		else if (key == 0x7B00 && event->modifiers & controlKey)
			isPlayS = 95;
		else if (key == 0x7C00)
			isPlayS = 18;
		else if (key == 0x7B00)
			isPlayS = 19;
		else if (key == 0x7D00)
			isPlayS = 16;
		else if (key == 0x7E00)
			isPlayS = 17;
		else if (key == 0x7A00)
			F_key = 1;
		else if (key == 0x7800)
			F_key = 2;
		else if (key == 0x6300)
			F_key = 3;
		else if (key == 0x7600)
			F_key = 4;
		else if (key == 0x6000)
			F_key = 5;
		else if (key == 0x6100) {
			if ((event->modifiers & shiftKey) || (event->modifiers & rightShiftKey))
				F_key = 12+6;
			else
				F_key = 6;
		} else if (key == 0x6200) {
			if ((event->modifiers & shiftKey) || (event->modifiers & rightShiftKey))
				F_key = 12+7;
			else
				F_key = 7;
		} else if (key == 0x6400) {
			if ((event->modifiers & shiftKey) || (event->modifiers & rightShiftKey))
				F_key = 12+8;
			else
				F_key = 8;
		} else if (key == 0x6500)
			F_key = 9;
		else if (key == 0x6D00)
			F_key = 10;
		else if (key == 0x6700)
			F_key = 11;
		else if (key == 0x6F00)
			F_key = 12;
		else
			key = event->message & charCodeMask;
			
			
#if (TARGET_API_MAC_CARBON == 1)
		if (global_df->isUTF && isUseSPCKeyShortcuts && text_isF1Pressed) {
			if ((res=Char2SpChar(key, 1)) != 0) {
				if (UniInputBuf.len > 0)
					UniInputBuf.unicodeInputString[UniInputBuf.len-1] = res;
				else {
					UniInputBuf.unicodeInputString[UniInputBuf.len] = res;
					UniInputBuf.len++;
				}
				isPlayS = 0; global_df->isExtend = 0;
			}
		} else if (global_df->isUTF && isUseSPCKeyShortcuts && text_isF2Pressed) {
			if ((res=Char2SpChar(key, 2)) != 0) {
				if (UniInputBuf.len > 0)
					UniInputBuf.unicodeInputString[UniInputBuf.len-1] = res;
				else {
					UniInputBuf.unicodeInputString[UniInputBuf.len] = res;
					UniInputBuf.len++;
				}
				isPlayS = 0; global_df->isExtend = 0;
			}
		}
		
		if (F_key == 1 && global_df->EditorMode) {
			text_isF1Pressed = TRUE;
			text_isF2Pressed = FALSE;
			F_key = 0;
			UniInputBuf.len = 0;
			key = -1;
		} else if (F_key == 2 && global_df->EditorMode) {
			text_isF2Pressed = TRUE;
			text_isF1Pressed = FALSE;
			F_key = 0;
			UniInputBuf.len = 0;
			key = -1;
		} else {
			text_isF1Pressed = FALSE;
			text_isF2Pressed = FALSE;
		}
#endif
		return key;
	}
	strcpy(global_df->err_message, DASHES);
	
	if (event->modifiers & optionKey)
		extend = 3;
	if (event->modifiers & controlKey)
		extend = 4;
	if (event->modifiers & shiftKey)
		extend = 1;
	if (event->modifiers & cmdKey)
		extend = 2;

	GlobalToLocal(&event->where);
	code = FindControl(event->where, win, &theControl);/* Get type of control */
	if (code != 0) {
		if (code == kControlUpButtonPart || code == kControlDownButtonPart || 
			code == kControlIndicatorPart || code == kControlPageDownPart || 
			code == kControlPageUpPart) {
			DrawCursor(0);
			DrawSoundCursor(0);
			Do_A_ScrollBar(code,theControl,event->where);/* Do scrollbars */
		}
	} else {
		SelectCursorPosition(event, extend);
	}
	if (event->what != keyDown && event->what != autoKey && event->what != mouseDown) {
		if (global_df != saveGlobal_df) {
			isPlayS = saveIsPlayS;
			F_key   = saveF_key;
		}
		global_df = saveGlobal_df;
	}
	return(-1);
}

void UpdateMainText(WindowPtr win) {
	WindowProcRec	*windProc;
	myFInfo 		*saveGlobal_df;
    WINDOW 			*t;
	Rect			rect;
	GrafPtr			oldPort;

	windProc = WindowProcs(win);
	if (windProc == NULL) return;
	if (windProc->FileInfo == NULL) return;
	GetPort(&oldPort);
    SetPortWindowPort(win);
	GetWindowPortBounds(win, &rect);
	EraseRect(&rect);
	saveGlobal_df = global_df;
	global_df = windProc->FileInfo;
	DrawCursor(0);
	DrawSoundCursor(0);
	global_df->WinChange = FALSE;
	touchwin(global_df->w1);
	wrefresh(global_df->w1);
	if (global_df->wm != NULL) {
		touchwin(global_df->wm);
		touchwin(global_df->w2);
		wrefresh(global_df->wm);
		wrefresh(global_df->w2);
	    for (t=global_df->RootWindow; t != NULL; t=t->NextWindow) {
			if (t != global_df->w1 && t != global_df->w2 && t != global_df->wm) {
				touchwin(t);
				wrefresh(t);
			}
		}
	}
	global_df->WinChange = TRUE;
	if (global_df->wind == FrontWindow())
		SetTextKeyScript(FALSE);
	DrawGrowIcon(win);
	DrawControls(win);
	global_df = saveGlobal_df;
	SetPort(oldPort);
}

void RefreshOtherCursesWindows(char all) {
	WINDOW *t;

	if (global_df == NULL) return;
	if (global_df->RootWindow == NULL) return;
	if (all) RefreshScreen(-1);
	global_df->WinChange = FALSE;
	for (t=global_df->RootWindow; t != NULL; t=t->NextWindow) {
		if (t != global_df->w1 && t != global_df->w2 && t != global_df->wm) {
			touchwin(t);
			wrefresh(t);
		}
	}
    global_df->WinChange = TRUE;
}

/* Scroll begin */
void SetScrollControl(void) {
	short tVal, i;
	long len, shiftLen, t;
	long colWin, newcol;
	double max, shiftMax, width;
	ROWS  *win;
	GrafPtr savePort;
	LINE *tl;

	if (global_df == NULL)
		return;
	if (global_df->ScrollBar != '\001' && global_df->ScrollBar != '\255')
		return;
	GetPort(&savePort);
 	SetPortWindowPort(global_df->wind);
 	if (global_df->VScrollHnd != NULL) {
		if (AtBotEnd(global_df->head_text, global_df->top_win, FALSE) && 
									global_df->numberOfRows <= global_df->EdWinSize) {
			HiliteControl(global_df->VScrollHnd,255);
		} else {
			max = 100.0000 / (double)global_df->numberOfRows;
			if (global_df->lineno == 1)
				max = global_df->window_rows_offset * max;
			else
				max = (double)(global_df->lineno + global_df->window_rows_offset) * max;
			tVal = (short)roundUp(max);
			HiliteControl(global_df->VScrollHnd,0);
			SetControlMaximum(global_df->VScrollHnd, 100);
			SetControlValue(global_df->VScrollHnd, tVal);
		}
	}
	if (global_df->ScrollBar == '\255') {
		SetPort(savePort);
		return;
	}
	if (global_df->HScrollHnd != NULL) {
		if (global_df->SoundWin) {
			max = (double)global_df->SnTr.SoundFileSize;
			if (max < (double)(global_df->SnTr.EndF-global_df->SnTr.BegF)) {
				HiliteControl(global_df->HScrollHnd,255);
			} else {
				max = 100.0000 / max;
				max = (double)global_df->SnTr.WBegF * max;
				tVal = (short)roundUp(max);
				HiliteControl(global_df->HScrollHnd,0);
				SetControlMaximum(global_df->HScrollHnd, 100);
				SetControlValue(global_df->HScrollHnd, tVal);
			}
		} else {
			Rect rect;
#ifndef _UNICODE
			short res;
			NewFontInfo finfo;
#endif
#if (TARGET_API_MAC_CARBON == 1)
			GetWindowPortBounds(global_df->wind, &rect);
#else
			rect = global_df->wind->portRect;
#endif
			max = 0.0000;
			shiftMax = 0.0000;
			win = global_df->top_win;
			width = rect.right-rect.left-LEFTMARGIN-global_df->w1->textOffset-SCROLL_BAR_SIZE;
			for (i=0; i < global_df->EdWinSize && win!=global_df->tail_text; i++, win=ToNextRow(win,FALSE)) {
				TextFont(win->Font.FName);
				TextSize(win->Font.FSize);
				shiftLen = 0L;
				colWin = 0L;
				len = 0L;
#ifndef _UNICODE
				finfo.isUTF = global_df->isUTF;
				finfo.Encod = my_FontToScript(win->Font.FName, win->Font.CharSet);
#endif
				if (win == global_df->cur_line) {
					for (tl=global_df->head_row->next_char; 
							tl != global_df->tail_row && len < UTTLINELEN-1; tl=tl->next_char) {
						if (tl->c == '\t') {
							newcol = (((colWin / TabSize) + 1) * TabSize);
							for (; colWin < newcol; colWin++) 
								templine4[len++] = ' ';
						} else {
							templine4[len] = tl->c;
#ifndef _UNICODE
							if (tl->next_char != global_df->tail_row)
								templine4[len+1] = tl->next_char->c;
							else
								templine4[len+1] = '\0';
							res = my_CharacterByteType(templine4, len, &finfo);
							if (res == 0 || res == 1) {
								colWin++;
							}
#else
							colWin++;
#endif
							len++;
						}
					}
				} else {
					for (t=0L; win->line[t] != EOS && len < UTTLINELEN-1; t++) {
						if (win->line[t] == '\t') {
							newcol = (((colWin / TabSize) + 1) * TabSize);
							for (; colWin < newcol; colWin++) 
								templine4[len++] = ' ';
						} else {
							templine4[len++] = win->line[t];
#ifndef _UNICODE
							res = my_CharacterByteType(win->line, t, &finfo);
							if (res == 0 || res == 1) {
								colWin++;
							}
#else
							colWin++;
#endif
						}
					}
				}
				templine4[len] = EOS;
				if (global_df->LeftCol > 0L) {
					shiftLen = TextWidthInPix(templine4, 0, ((global_df->LeftCol >= len) ? len : global_df->LeftCol), &win->Font, 0);
				}
				len = TextWidthInPix(templine4, 0, len, &win->Font, 0);
				if (max < (double)len)
					max = (double)len;
				if (shiftMax < (double)shiftLen)
					shiftMax = (double)shiftLen;
			}
			if (global_df->LeftCol == 0 && max < width) {
				HiliteControl(global_df->HScrollHnd,255);
			} else {
				max = 100.0000 / max;
				max = (double)shiftMax * max;
				tVal = (short)roundUp(max);
				HiliteControl(global_df->HScrollHnd,0);
				SetControlMaximum(global_df->HScrollHnd, 100);
				SetControlValue(global_df->HScrollHnd, tVal);
			}
		}
	}
	SetPort(savePort);
}

static long Con_MoveLineUp(long col) {
    if (!AtTopEnd(global_df->top_win, global_df->head_text, FALSE)) {
		global_df->row_win++;
		global_df->window_rows_offset--;
		global_df->top_win = ToPrevRow(global_df->top_win, FALSE);
		DisplayTextWindow(NULL, 1);
		if (global_df->row_win >= 0L && global_df->row_win < global_df->EdWinSize) 
			global_df->window_rows_offset = 0;
	} else if (global_df->row_txt != global_df->top_win && 
								global_df->row_win < (long)global_df->EdWinSize) {
		global_df->row_win2 = 0L;
		global_df->col_win2 = -2L;
		global_df->col_chr2 = -2L;
		MoveUp(-1);
		col = global_df->col_win - global_df->LeftCol;
	}
	return(col);
}

static void Con_MoveLineDown(void) {
    if (!AtBotEnd(global_df->top_win, global_df->tail_text, FALSE)) {
		global_df->row_win--;
		global_df->window_rows_offset++;
		global_df->top_win = ToNextRow(global_df->top_win, FALSE);
		DisplayTextWindow(NULL, 1);
		if (global_df->row_win >= 0L && global_df->row_win < global_df->EdWinSize) 
			global_df->window_rows_offset = 0;
	}
}

long Con_PrevPage(long col) {
    register int num;

    if (!AtTopEnd(global_df->top_win, global_df->head_text, FALSE)) {
		num = global_df->TextWinSize - global_df->top_win->Font.FHeight;
		while (num > 0) {
			if (!AtTopEnd(global_df->top_win,global_df-> head_text, FALSE)) 
				global_df->top_win = ToPrevRow(global_df->top_win, FALSE);
			else {
				global_df->window_rows_offset = 0L - global_df->lineno;
				break;
			}
	    	num -= global_df->top_win->Font.FHeight;
			global_df->row_win++;
			global_df->window_rows_offset--;
		}
		if (num < 0) {
			if (!AtBotEnd(global_df->top_win, global_df->tail_text, FALSE)) {
				global_df->top_win = ToNextRow(global_df->top_win, FALSE);
				global_df->row_win--;
				global_df->window_rows_offset++;
			}
		}
		if (AtTopEnd(global_df->top_win, global_df->head_text, FALSE))
			global_df->window_rows_offset = 0L - global_df->lineno;
		if (global_df->row_win >= 0L && global_df->row_win < global_df->EdWinSize) 
			global_df->window_rows_offset = 0L;
		DisplayTextWindow(NULL, 1);
	} else if (global_df->row_txt != global_df->top_win) {
		BeginningOfFile(-1);
		col = global_df->col_win - global_df->LeftCol;
	}
	return(col);
}

void Con_NextPage(void) {
	register int i;

    if (!AtBotEnd(global_df->top_win, global_df->tail_text, FALSE)) {
		for (i=1; i < global_df->EdWinSize; i++) {
			if (!AtBotEnd(global_df->top_win, global_df->tail_text, FALSE)) 
				global_df->top_win = ToNextRow(global_df->top_win, FALSE);
			else
				break;
			global_df->row_win--;
			global_df->window_rows_offset++;
		}
		if (global_df->row_win >= 0L && global_df->row_win < global_df->EdWinSize) 
			global_df->window_rows_offset = 0L;
		DisplayTextWindow(NULL, 1);
	}
}

static void Con_GotoLine(long num) {
	register long fp = global_df->lineno+global_df->window_rows_offset;

	if (num < fp) {
		for (; num < fp; num++) {
			if (!AtTopEnd(global_df->top_win, global_df->head_text, FALSE)) 
				global_df->top_win = ToPrevRow(global_df->top_win, FALSE);
			else {
				global_df->window_rows_offset = 0L - global_df->lineno;
				break;
			}
			global_df->row_win++;
			global_df->window_rows_offset--;
		}
		if (AtTopEnd(global_df->top_win, global_df->head_text, FALSE))
			global_df->window_rows_offset = 0L - global_df->lineno;
	} else {
		for (; num > fp; num--) {
			if (!AtBotEnd(global_df->top_win, global_df->tail_text, FALSE)) 
				global_df->top_win = ToNextRow(global_df->top_win, FALSE);
			else break;
			global_df->row_win--;
			global_df->window_rows_offset++;
		}
	}
	if (global_df->row_win >= 0L && global_df->row_win < global_df->EdWinSize) 
		global_df->window_rows_offset = 0;
	DisplayTextWindow(NULL, 1);
}

void  HandleVScrollBar (
short		code,   				/*  Selection code for part of scroll bar  */
ControlRef	theControl, 			/*  Handle to the scroll bar control */
Point   	myPt)   				/*  Returned point from track the scrollbar */
{   								/*  Start of this function */
    long MaxTick;
	long orgCol_win;
    double val;

	if (!global_df->ScrollBar) return;
	orgCol_win = global_df->col_win;
	global_df->col_win = global_df->LeftCol + 1;
	do { 								/* Do the scroll as long as the button is down */
		HiliteControl(theControl, code);	/* Darken the arrow */
		switch (code) {
			case kControlUpButtonPart:
				orgCol_win = Con_MoveLineUp(orgCol_win);
				PosAndDispl();
				break;

			case kControlDownButtonPart:
				Con_MoveLineDown();
				PosAndDispl();
				break;
		
			case kControlPageUpPart:
				if (global_df->ScrollBar == '\001')
					orgCol_win = Con_PrevPage(orgCol_win);
				else
					orgCol_win = Con_MoveLineUp(orgCol_win);
				PosAndDispl();
				break;

			case kControlPageDownPart:
				if (global_df->ScrollBar == '\001')
					Con_NextPage();
				else
					Con_MoveLineDown();
				PosAndDispl();
				break;

			case kControlIndicatorPart:
				if (global_df->ScrollBar != '\001') {
					strcpy(global_df->err_message, "+Finish coding current line!");
				} else {
					code = TrackControl(theControl, myPt, NULL);
					val = (double)GetControlValue(theControl);
					val = (double)global_df->numberOfRows * (val / 100.0000);
					Con_GotoLine((long)val);
				}
				PosAndDispl();
				break;	
		}

		if (global_df->ScrollBar != '\001') HiliteControl(theControl,0);
		else SetScrollControl();

		if (code == kControlUpButtonPart || code == kControlDownButtonPart)
			MaxTick = TickCount() + 8;/* Time delay for auto-scroll */
		else
			MaxTick = TickCount() + 20;/* Time delay for auto-scroll */
		do {  
			if (TickCount() > MaxTick) break;		 
		} while ( Button() == TRUE) ;
	} while (StillDown() == TRUE) ;
	global_df->col_win = orgCol_win;
	wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
}

void  HandleHScrollBar (
short		code,   				/*  Selection code for part of scroll bar  */
ControlRef  theControl, 			/*  Handle to the scroll bar control */
Point		myPt)   				/*  Returned point from track the scrollbar */
{   								/*  Start of this function */
    long  MaxTick, orgCol_win;
	unsigned long  max;
	double val;
	short i;
	ROWS  *win;

	if (!global_df->ScrollBar) return;
	orgCol_win = global_df->col_win;
	do { 								/* Do the scroll as long as the button is down */
		HiliteControl(theControl, code);	/* Darken the arrow */
		switch (code) {
			case kControlUpButtonPart:
				if (global_df->SoundWin) {
					int rm, cm;
					Size t;
					DrawSoundCursor(0);
					GetSoundWinDim(&rm, &cm);
					t = ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample) / 50;
					if (global_df->SnTr.WBegF >= t)
						global_df->SnTr.WBegF -= t;
					else
						global_df->SnTr.WBegF = 0L;
					global_df->SnTr.WBegF = AlignMultibyteMediaStream(global_df->SnTr.WBegF, '+');
					global_df->WinChange = FALSE;
					touchwin(global_df->SoundWin); wrefresh(global_df->SoundWin);
					global_df->WinChange = TRUE;
					DrawSoundCursor(1);
					delay_mach(1);
				} else {
					global_df->col_win = global_df->LeftCol + 1;
					if (global_df->LeftCol > 0L) {
						global_df->LeftCol--;
						DisplayTextWindow(NULL, 1);
					}
					PosAndDispl();
				}
				break;

			case kControlDownButtonPart:
				if (global_df->SoundWin) {
					int rm, cm;
					Size t, tf;
					DrawSoundCursor(0);
					GetSoundWinDim(&rm, &cm);
					tf = ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
					t = tf / 50;
					if (global_df->SnTr.WBegF+t+tf < global_df->SnTr.SoundFileSize)
						global_df->SnTr.WBegF += t;
					else
						global_df->SnTr.WBegF = global_df->SnTr.SoundFileSize - tf;
					global_df->SnTr.WBegF = AlignMultibyteMediaStream(global_df->SnTr.WBegF, '-');
					global_df->WinChange = FALSE;
					touchwin(global_df->SoundWin); wrefresh(global_df->SoundWin);
					global_df->WinChange = TRUE;
					DrawSoundCursor(1);
					delay_mach(1);
				} else {
					long max, len;
					ROWS *win;
					max = 0L;
					win = global_df->top_win;
					for (i=0; i < global_df->EdWinSize && win!=global_df->tail_text; i++, win=ToNextRow(win,FALSE)) {
						if (win == global_df->cur_line) {
							len = ComColWin(FALSE, NULL, global_df->head_row_len);
							if (max < len)
								max = len;
						} else {
							len = ComColWin(FALSE, win->line, strlen(win->line));
							if (max < len)
								max = len;
						}
					}
					if (global_df->LeftCol < max+1)
						global_df->LeftCol++;
					global_df->col_win = global_df->LeftCol + 1;
					DisplayTextWindow(NULL, 1);
					PosAndDispl();
				}
				break;
		
			case kControlPageUpPart:
				if (global_df->SoundWin) {
					Size t;
					DrawSoundCursor(0);
					t = global_df->SnTr.WEndF - global_df->SnTr.WBegF;
					if (global_df->SnTr.WBegF >= t)
						global_df->SnTr.WBegF -= t;
					else
						global_df->SnTr.WBegF = 0L;
					global_df->SnTr.WBegF = AlignMultibyteMediaStream(global_df->SnTr.WBegF, '+');
					global_df->WinChange = FALSE;
					touchwin(global_df->SoundWin); wrefresh(global_df->SoundWin);
					global_df->WinChange = TRUE;
					DrawSoundCursor(1);
					delay_mach(1);
				} else {
					if (global_df->LeftCol > 0L) {
						global_df->LeftCol -= 10;
						if (global_df->LeftCol < 0L)
							global_df->LeftCol = 0L;
						global_df->col_win = global_df->LeftCol + 1;
						DisplayTextWindow(NULL, 1);
					}
				}
				PosAndDispl();
				break;

			case kControlPageDownPart:
				if (global_df->SoundWin) {
					Size t;
					DrawSoundCursor(0);
					t = global_df->SnTr.WEndF - global_df->SnTr.WBegF;
					if (global_df->SnTr.WBegF+t < global_df->SnTr.SoundFileSize)
						global_df->SnTr.WBegF += t;
					else
						global_df->SnTr.WBegF = global_df->SnTr.SoundFileSize - t;
					global_df->SnTr.WBegF = AlignMultibyteMediaStream(global_df->SnTr.WBegF, '-');
					global_df->WinChange = FALSE;
					touchwin(global_df->SoundWin); wrefresh(global_df->SoundWin);
					global_df->WinChange = TRUE;
					DrawSoundCursor(1);
					delay_mach(1);
				} else {
					global_df->LeftCol += 10;
					global_df->col_win = global_df->LeftCol + 1;
					DisplayTextWindow(NULL, 1);
					PosAndDispl();
				}
				break;

			case kControlIndicatorPart:
				if (global_df->SoundWin) {
					DrawSoundCursor(0);
					max = (unsigned long)global_df->SnTr.SoundFileSize;
					code = TrackControl(theControl, myPt, NULL);
					val = (double)GetControlValue(theControl);
					val = (double)max * (val / 100.0000);
					global_df->SnTr.WBegF = AlignMultibyteMediaStream((long)val, '-');
					global_df->WinChange = FALSE;
					touchwin(global_df->SoundWin); wrefresh(global_df->SoundWin);
					global_df->WinChange = TRUE;
					DrawSoundCursor(1);
					delay_mach(1);
				} else {
					max = 0L;
					win = global_df->top_win;
					for (i=0; i < global_df->EdWinSize && win!=global_df->tail_text; i++, win=ToNextRow(win,FALSE)) {
						if (win == global_df->cur_line) {
							if (max < global_df->head_row_len)
								max = global_df->head_row_len;
						} else {
							if (max < strlen(win->line))
								max = strlen(win->line);
						}
					}
					if (global_df->ScrollBar != '\001') {
						strcpy(global_df->err_message, "+Finish coding current line!");
					} else {
						max++;
						code = TrackControl(theControl, myPt, NULL);
						val = (double)GetControlValue(theControl);
						val = (double)max * (val / 100.0000);
						global_df->LeftCol = (long)val;
//6-12-99						global_df->LeftCol = max * val / 100L;
					}
					global_df->col_win = global_df->LeftCol + 1;
					DisplayTextWindow(NULL, 1);
					PosAndDispl();
				}
				break;	
		}
		if (global_df->ScrollBar != '\001') HiliteControl(theControl,0);
		else SetScrollControl();

		if (code != kControlUpButtonPart && code != kControlDownButtonPart)
			MaxTick = TickCount() + 6;
		else if (global_df->SoundWin)
			MaxTick = TickCount();
		else
			MaxTick = TickCount() + 3;
		do {  
			if (TickCount() > MaxTick) break;		 
		} while ( Button() == TRUE) ;
	} while (StillDown() == TRUE) ;
	global_df->col_win = orgCol_win;
	wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
}

void Do_A_ScrollBar(				/* Handle a ScrollBar being pressed */
short		code,   				/* Which place in scrollbar was pressed */
ControlRef	theControl, 			/* Which place in scrollbar was pressed */
Point		myPt)   				/* Where the scrollbar was pressed */
{   								/* Start of a ScrollBar being pressed */
    short RefCon;

	RefCon = GetControlReference(theControl);	/* get control refcon */
	
	switch  (RefCon)    				/* Select correct scrollbar */
	{   								/*Start correct scrollbar */
		case I_VScroll_bar:  			/* Scroll bar, scroll bar */
			HandleVScrollBar(code,theControl,myPt);/* code,Min,Max,Inc,PageInc,handle,Pt */
			break;  					/* End for handling a scroll bar */
	
		case I_HScroll_bar:  			/* Scroll bar, scroll bar */
			HandleHScrollBar(code,theControl,myPt);/* code,Min,Max,Inc,PageInc,handle,Pt */
			break;  					/* End for handling a scroll bar */
	
		default:    					/* allow other scrollbars (lists), trap for debug */
			break;  					/* end of default */
	}   								/* end of switch */
}   									/* Handle a ScrollBar being pressed */
/* Scroll End */
