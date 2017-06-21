/*
printf("\x1b[25;1H");
printf("num=%d;", num);
*/
#include "ced.h"
#include "my_ctype.h"
#include "MMedia.h"
#include "mac_MUPControl.h"

static char isword(unCH c) {
	if (is_unCH_alnum(c) ||  (c < 1 && c != EOS) || c > 127 || c == '-' || c == '\'' || c == '_')
		return(TRUE);
	else
		return(FALSE);
}

#include <script.h>

char  DoubleClickCount = 1;
long  last_row_win, last_col_win;
long  lastWhen = 0L;
unCH  *tempSt = NULL;
unCH  errTempSt[1];
Point lastWhere = {0,0};

void PosAndDispl(void) {
	global_df->WinChange = FALSE;
	if (CheckLeftCol(global_df->col_win))
		DisplayTextWindow(NULL, 1);
	wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
	if (!global_df->err_message[0] && global_df->SoundWin != NULL) 
		PutSoundStats(global_df->SnTr.WEndF - global_df->SnTr.WBegF);
	else
		draw_mid_wm();
	global_df->WinChange = TRUE;
}

#ifndef _UNICODE
static void CpHead_rowToTempSt(void) {
	unCH *s;
	LINE *tl;

	if (tempSt == NULL) {
		tempSt = (unCH *)malloc((global_df->head_row_len+1)*sizeof(unCH));
		if (tempSt == NULL) {
			errTempSt[0] = EOS;
			tempSt = errTempSt;
			mem_err(TRUE, global_df);
		}
	} else if (global_df->head_row_len >= (int)strlen(tempSt)) {
		free(tempSt);
		tempSt = (unCH *)malloc((global_df->head_row_len+1)*sizeof(unCH));
		if (tempSt == NULL) {
			errTempSt[0] = EOS;
			tempSt = errTempSt;
			mem_err(TRUE, global_df);
		}
	}
	s = tempSt;
	tl = global_df->head_row->next_char;
	while (tl != global_df->tail_row) {
		*s++ = tl->c;
		tl = tl->next_char;
	}
	*s = EOS;
}
#endif

static int FindRightColumn(int h, unCH *s, int row, WINDOW *t) {
	register int tcol;
//	register int pcol;
	register int ccol;
	register int res;
	register int i;
#ifndef _UNICODE
	register short icharCode;
#endif
	GrafPtr oldPort;

	GetPort(&oldPort);
	SetPortWindowPort(global_df->wind);
	TextFont(t->RowFInfo[row]->FName);
	TextSize(t->RowFInfo[row]->FSize);
	if (AutoScriptSelect) {
		cedDFnt.Encod = my_FontToScript(t->RowFInfo[row]->FName, t->RowFInfo[row]->CharSet);
		if (!global_df->isUTF)
			KeyScript(cedDFnt.Encod);
	}
	cedDFnt.isUTF = t->isUTF;
	ccol = tcol = 0;
	h -= (LEFTMARGIN + t->textOffset);
	for (i=0; s[i] && i < t->num_cols; i++) {
//		pcol = ComColWin(FALSE, s, i+1);
		res = TextWidthInPix(s, 0, i+1, t->RowFInfo[row], 0);
		if (res > ccol) {
			ccol = res;
			tcol = (ccol - tcol) / 2;
		}
		if (h < ccol-tcol) {
			SetPort(oldPort);
			if (i == 0)
				return(global_df->LeftCol-1);
//			i -= 2;
#ifdef _UNICODE
			i--;
#else
			do {
				i--;
				icharCode = my_CharacterByteType(s, i, &cedDFnt);
			} while (icharCode != 0 && icharCode != -1 && i > 0) ;
			if (icharCode == -1)
				i--;
#endif
			return(i+global_df->LeftCol);
		}
		if (res >= ccol)
			tcol = ccol;
	}
	if (h < tcol+(ccol/2)) i--;
	SetPort(oldPort);
	if (i < 0)
		return(global_df->LeftCol-1);
	i -= 1;
#ifdef _UNICODE
	i++;
#else
	do {
		i++;
		icharCode = my_CharacterByteType(s, i, &cedDFnt);
	} while (icharCode != 0 && icharCode != 1 && s[i]) ;
#endif
	return(i+global_df->LeftCol);
}

static void DelOldCur(long row_wint, long col_wint) {
	long rt, ct, ct2, rt2;

	rt  = global_df->row_win;  ct  = global_df->col_win;
	rt2 = global_df->row_win2; ct2 = global_df->col_win2;
	global_df->row_win = row_wint; global_df->col_win = col_wint;
	global_df->row_win2 = 0; global_df->col_win2 = -2;
	DrawCursor(0);
	global_df->row_win  = rt;  global_df->col_win  = ct;
	global_df->row_win2 = rt2; global_df->col_win2 = ct2;
}

static void SortOutPos(long row_wint, long col_wint, long col_chrt, char cnv) {
	long rt, ct;

	if ((global_df->row_win > row_wint) || (global_df->row_win == row_wint && global_df->col_win > col_wint)) {
		if ((global_df->row_win2 == row_wint && global_df->col_win2 == col_wint) ||
			(global_df->row_win2 == row_wint-1 && global_df->col_win2 == 0 && col_wint == 0))
			DelOldCur(row_wint, col_wint);
		else if (last_row_win==global_df->row_win2 && last_col_win==global_df->col_win2 && cnv== 0) {
			rt = global_df->row_win; ct = global_df->col_win;
			global_df->row_win = row_wint; global_df->col_win = col_wint; global_df->row_win2 -= global_df->row_win;
			DrawCursor(2);
			global_df->row_win2 = row_wint; global_df->col_win2 = col_wint; global_df->col_chr2 = col_chrt;
			global_df->row_win = rt; global_df->col_win = ct;
		}
			rt = global_df->row_win2; ct = global_df->col_win2;
			global_df->row_win2 = row_wint - global_df->row_win; global_df->col_win2 = col_wint;
			DrawCursor(2);
			global_df->row_win2 = rt; global_df->col_win2 = ct;
	} else if ((global_df->row_win<global_df->row_win2) || (global_df->row_win==global_df->row_win2 && global_df->col_win<global_df->col_win2)) {
		if (global_df->row_win2 == row_wint && global_df->col_win2 == col_wint)
			DelOldCur(row_wint, col_wint);
		else if (last_row_win==row_wint && last_col_win==col_wint && cnv== 0) {
			rt = global_df->row_win; ct = global_df->col_win;
			global_df->row_win = row_wint; global_df->col_win = col_wint; global_df->row_win2 -= global_df->row_win;
			DrawCursor(2);
			global_df->row_win2 += global_df->row_win;
			row_wint = global_df->row_win2; col_wint = global_df->col_win2; col_chrt = global_df->col_chr2;
			global_df->row_win = rt; global_df->col_win = ct;
		}
		global_df->row_win2 -= global_df->row_win;
		DrawCursor(2);
		global_df->row_win2 = row_wint; global_df->col_win2 = col_wint; global_df->col_chr2 = col_chrt;
	} else if ((global_df->row_win > global_df->row_win2 && global_df->row_win < row_wint) ||
				   (global_df->row_win==global_df->row_win2 && global_df->row_win< row_wint && global_df->col_win> global_df->col_win2) ||
				   (global_df->row_win> global_df->row_win2 && global_df->row_win==row_wint && global_df->col_win< col_wint) ||
				   (global_df->row_win == global_df->row_win2 && global_df->row_win == row_wint && 
					global_df->col_win > global_df->col_win2  && global_df->col_win <  col_wint)) {
		if (last_row_win == global_df->row_win2 && last_col_win == global_df->col_win2) {
			global_df->row_win2 -= global_df->row_win;
			DrawCursor(2);
			global_df->row_win2 = row_wint; global_df->col_win2 = col_wint; global_df->col_chr2 = col_chrt;
		} else {
			rt = global_df->row_win2; ct = global_df->col_win2;
			global_df->row_win2 = row_wint-global_df->row_win; global_df->col_win2 = col_wint;
			DrawCursor(2);
			global_df->row_win2 = rt; global_df->col_win2 = ct;
		}
	} else if (global_df->row_win == global_df->row_win2 && global_df->col_win == global_df->col_win2) {
		global_df->row_win2 = row_wint - global_df->row_win;
		global_df->col_win2 = col_wint;
		if (global_df->row_win2 == 0L && global_df->col_win == global_df->col_win2) global_df->col_win2 = -2;
		DrawCursor(1);
		global_df->row_win2 = row_wint;
		global_df->col_win2 = col_wint; global_df->col_chr2 = col_chrt;
	}
	last_row_win = global_df->row_win;
	last_col_win = global_df->col_win;
}

void matchCursorColToGivenCol(long col) {
	if (global_df->row_txt == global_df->cur_line) {
		global_df->col_win = ComColWin(FALSE, NULL, global_df->head_row_len);
		if (global_df->col_win < col)
			col = global_df->col_win;
		global_df->col_win = 0L;
		global_df->col_chr = 0L;
		global_df->col_txt = global_df->head_row->next_char;
		while (global_df->col_txt != global_df->tail_row && global_df->col_win < col) {
			global_df->col_chr++;
			global_df->col_win = ComColWin(FALSE, NULL, global_df->col_chr);
			global_df->col_txt = global_df->col_txt->next_char;
		}
		if (global_df->col_txt->prev_char != global_df->head_row &&
				global_df->col_txt->prev_char->c == NL_C && global_df->ShowParags != '\001') {
			global_df->col_chr--;
			global_df->col_win = ComColWin(FALSE, NULL, global_df->col_chr);
			global_df->col_txt = global_df->col_txt->prev_char;
		}
	} else {
		global_df->col_win = ComColWin(FALSE, global_df->row_txt->line, strlen(global_df->row_txt->line));
		if (global_df->col_win < col)
			col = global_df->col_win;
		global_df->col_win = 0L;
		global_df->col_chr = 0L;
		while (global_df->row_txt->line[global_df->col_chr] != EOS && global_df->col_win < col) {
			global_df->col_chr++;
			global_df->col_win = ComColWin(FALSE, global_df->row_txt->line, global_df->col_chr);
		}
		if (global_df->col_chr > 0 && global_df->row_txt->line[global_df->col_chr-1] == NL_C &&
															global_df->ShowParags != '\001') {
			global_df->col_chr--;
			global_df->col_win = ComColWin(FALSE, global_df->row_txt->line, global_df->col_chr);
		}
	}
}

static char isAnyBulletsOnLine(void) {
	register int offset;
	long index, ti;
	char hc_found;
	ROWS *tr;
	LINE *tl, *ttl;
	
	offset = 0;
	tr = global_df->row_txt;
	if (global_df->row_txt == global_df->cur_line) {
		hc_found = FALSE;
		ttl = NULL;
		for (tl=global_df->col_txt; tl != global_df->tail_row; tl=tl->next_char) {
			if (tl->c == HIDEN_C) {
				hc_found = !hc_found;
				if (hc_found && ttl != NULL)
					return(FALSE);
				ttl = tl;
				if (!hc_found && ttl != NULL)
					break;
			}
		}
		if (ttl != NULL) {
			return(TRUE);
		}
	} else {
		hc_found = FALSE;
		ti = -1;
		for (index=global_df->col_chr; global_df->row_txt->line[index]; index++) {
			if (global_df->row_txt->line[index] == HIDEN_C) {
				hc_found = !hc_found;
				if (hc_found && ti != -1)
					return(FALSE);
				ti = index;
				if (!hc_found && ti != -1)
					break;
			}
		}
		if (ti != -1) {
			return(TRUE);
		}
	}
	return(FALSE);
}

static void UpdateCursorPos(int row, int col, char cnv, int extend) {
	long old_row_win = global_df->row_win;
	long row_wint, col_wint, col_chrt;

	if (cnv < 2) {
		long t;

		if (global_df->row_win2 == 0L && global_df->col_win2 == -2) {
			global_df->row_win2 = global_df->row_win;
			global_df->col_win2 = global_df->col_win;
			global_df->col_chr2 = global_df->col_chr;
		} else {
			global_df->row_win2 += global_df->row_win;
		}
		row_wint = global_df->row_win;
		col_wint = global_df->col_win;
		col_chrt = global_df->col_chr;
		if (row_wint < global_df->row_win2) {
			t = row_wint; row_wint = global_df->row_win2; global_df->row_win2 = t;
			t = col_wint; col_wint = global_df->col_win2; global_df->col_win2 = t;
			t = col_chrt; col_chrt = global_df->col_chr2; global_df->col_chr2 = t;
		} else if (row_wint == global_df->row_win2 && col_wint < global_df->col_win2) {
			t = col_wint; col_wint = global_df->col_win2; global_df->col_win2 = t;
			t = col_chrt; col_chrt = global_df->col_chr2; global_df->col_chr2 = t;
		}
	}
		
	global_df->row_win = 0;
	global_df->row_txt = global_df->top_win;
	while (!AtBotEnd(global_df->row_txt, global_df->tail_text, FALSE) && row > 0) {
		global_df->row_txt = ToNextRow(global_df->row_txt, FALSE);
		row--;
		global_df->row_win++;
	}

	if (last_row_win != global_df->row_win || extend != 2)
		matchCursorColToGivenCol(col);

	if (cnv == 3) {
		GetCurCode();
		if ((col=uS.partcmp(sp, "%gra:", FALSE, FALSE))) {
			isPlayS = -8;
		} else if ((col=uS.partcmp(sp, "%grt:", FALSE, FALSE))) {
			isPlayS = -8;
		} else if (global_df->SoundWin && (GetCurHidenCode(TRUE, NULL) || isAnyBulletsOnLine())) {
			char isTopWinChanged;
			char tDataChanged = global_df->DataChanged;
			long old_col_win = global_df->col_win;
			long old_col_chr = global_df->col_chr;
			long old_row_win = global_df->row_win;
			long old_col_win2 = global_df->col_win2;
			long old_col_chr2 = global_df->col_chr2;
			long old_row_win2 = global_df->row_win2;
			long old_lineno  = global_df->lineno;
			long old_LeftCol = global_df->LeftCol;
			ROWS *old_row_txt = global_df->row_txt;
			ROWS *old_top_win = global_df->top_win;
			FindAnyBulletsOnLine();
			if (FindTextCodeLine(cl_T(SOUNDTIER), NULL)) {
				if ((col=uS.partcmp(sp, SOUNDTIER, FALSE, FALSE))) {
					global_df->SnTr.SSource = 0;
					RSoundPlay(-2, FALSE);
				}
			}
			isTopWinChanged = (global_df->top_win != old_top_win);
			global_df->col_win = old_col_win;
			global_df->col_chr = old_col_chr;
			global_df->row_win = old_row_win;
			global_df->col_win2 = old_col_win2;
			global_df->col_chr2 = old_col_chr2;
			global_df->row_win2 = old_row_win2;
			global_df->lineno  = old_lineno;
			global_df->LeftCol = old_LeftCol;
			global_df->row_txt = old_row_txt;
			global_df->top_win = old_top_win;
			if (global_df->row_txt == global_df->cur_line) {
				long j;
				for (global_df->col_txt=global_df->head_row->next_char, j=global_df->col_chr; j > 0; j--)
					global_df->col_txt = global_df->col_txt->next_char;
			}
			wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
			if (isTopWinChanged)
				DisplayTextWindow(NULL, 1);
			global_df->DataChanged = tDataChanged;
		}
		global_df->col_win2 = 0L;
		global_df->col_chr2 = 0L;
		if (!AtBotEnd(global_df->row_txt, global_df->tail_text, FALSE)) {
			global_df->row_win2++;
			global_df->col_win = 0L;
			global_df->col_chr = 0L;
		} else {
			if (global_df->row_txt == global_df->cur_line) {
				for (; global_df->col_txt->c != NL_C && global_df->col_txt != global_df->tail_row;
								global_df->col_chr++, global_df->col_txt=global_df->col_txt->next_char) ;
				global_df->col_win = ComColWin(FALSE, NULL, global_df->col_chr);
			} else {
				for (global_df->col_chr++; global_df->row_txt->line[global_df->col_chr] != NL_C &&
							global_df->row_txt->line[global_df->col_chr]; global_df->col_chr++) ;
				global_df->col_win = ComColWin(FALSE, global_df->row_txt->line, global_df->col_chr);
			}
		}
		if (FindFileLine(TRUE, NULL)) {
			isPlayS = -5;
			InKey = (int)'v';
			global_df->LeaveHighliteOn = TRUE;
		}
	} else if (cnv == 2) {
		unCH *s;
		LINE *col_txt2;
		GrafPtr oldPort;

		GetPort(&oldPort);
		SetPortWindowPort(global_df->wind);
		TextFont(global_df->row_txt->Font.FName);
		TextSize(global_df->row_txt->Font.FSize);
		cedDFnt.isUTF = global_df->isUTF;
		if (global_df->row_txt == global_df->cur_line) {
			if (global_df->col_txt != global_df->tail_row && isword(global_df->col_txt->c)) {
#ifdef _UNICODE
		 		for (; isword(global_df->col_txt->c) && global_df->col_txt != global_df->tail_row; global_df->col_chr++, global_df->col_txt=global_df->col_txt->next_char) ;
#else
				CpHead_rowToTempSt();
		 		for (; (isword(global_df->col_txt->c) || my_CharacterByteType(tempSt,global_df->col_chr,&cedDFnt)) && global_df->col_txt != global_df->tail_row; 
						global_df->col_chr++, global_df->col_txt=global_df->col_txt->next_char) ;
#endif
				global_df->col_win = ComColWin(FALSE, NULL, global_df->col_chr);
#ifdef _UNICODE
		 		for (global_df->col_chr2=global_df->col_chr, col_txt2=global_df->col_txt->prev_char; isword(col_txt2->c) && col_txt2 != global_df->head_row;
						global_df->col_chr2--, col_txt2=col_txt2->prev_char) ;
#else
		 		for (global_df->col_chr2=global_df->col_chr, col_txt2=global_df->col_txt->prev_char; 
						(isword(col_txt2->c) || my_CharacterByteType(tempSt,global_df->col_chr2,&cedDFnt)) && col_txt2 != global_df->head_row;
						global_df->col_chr2--, col_txt2=col_txt2->prev_char) ;
#endif
				global_df->col_win2 = ComColWin(FALSE, NULL, global_df->col_chr2);
			}
		} else {
			s = global_df->row_txt->line;
			if (isword(s[global_df->col_chr])) {
#ifdef _UNICODE
		 		for (global_df->col_chr++; isword(s[global_df->col_chr]); global_df->col_chr++) ;
#else
		 		for (global_df->col_chr++; isword(s[global_df->col_chr]) || my_CharacterByteType(s,global_df->col_chr,&cedDFnt); global_df->col_chr++) ;
#endif
				global_df->col_win = ComColWin(FALSE, global_df->row_txt->line, global_df->col_chr);
#ifdef _UNICODE
				for (global_df->col_chr2=global_df->col_chr-1; isword(s[global_df->col_chr2]) && global_df->col_chr2 >= 0; global_df->col_chr2--) ;
#else
				for (global_df->col_chr2=global_df->col_chr-1; 
					(isword(s[global_df->col_chr2]) || my_CharacterByteType(s,global_df->col_chr2,&cedDFnt)) && global_df->col_chr2 >= 0; 
					 global_df->col_chr2--) ;
#endif
				global_df->col_chr2++;
				global_df->col_win2 = ComColWin(FALSE, global_df->row_txt->line, global_df->col_chr2);
			}
		}
		SetPort(oldPort);
	} else if (cnv < 2) {
		SortOutPos(row_wint, col_wint, col_chrt, cnv);
		global_df->row_win2 -= global_df->row_win;
	}

	if (global_df->row_win2 == 0L && global_df->col_win == global_df->col_win2) {
		global_df->col_win2 = -2L;
		global_df->col_chr2 = -2L;
	}
	global_df->lineno += (global_df->row_win - old_row_win);
	global_df->window_rows_offset = 0;
	PosAndDispl();
}

void MoveRegionDownAndAddFirstTier(WINDOW *w) {
	register long i;
	GrafPtr oldPort;
	Rect	theRect;
	RgnHandle tempRgn;

	if (!AtTopEnd(global_df->top_win, global_df->head_text, FALSE)) {
		global_df->top_win = ToPrevRow(global_df->top_win, FALSE);
		global_df->row_win++;
		last_row_win++;
		global_df->WinChange = FALSE;
		DisplayTextWindow(NULL, 0);
		global_df->WinChange = TRUE;
	} else
		return;

	GetPort(&oldPort);
	SetPortWindowPort(global_df->wind);

	theRect.left = LEFTMARGIN;
#if (TARGET_API_MAC_CARBON == 1)
	Rect rect;
	GetWindowPortBounds(global_df->wind, &rect);
	theRect.right = rect.right - SCROLL_BAR_SIZE;
#else
	theRect.right = global_df->wind->portRect.right - SCROLL_BAR_SIZE;
#endif
	theRect.top = w->LT_row;
	theRect.bottom = theRect.top + global_df->TextWinSize;

	tempRgn = NewRgn();
	ScrollRect(&theRect, 0, ComputeHeight(w, 0, 1), tempRgn);
	DisposeRgn(tempRgn);
	SetPort(oldPort);
	global_df->WinChange = FALSE;
	wUntouchWin(w, 0);
	wrefresh(w);
	global_df->WinChange = TRUE;
	if (global_df->row_win2 < 0) {
		i = global_df->col_win;
		global_df->col_win = 0L;
		DrawCursor(2);
		global_df->col_win = i;
	}
}

void MoveRegionUpAndAddLastTier(WINDOW *w) {
	register long i;
	int	NumRows;
	GrafPtr oldPort;
	Rect	theRect;
	RgnHandle tempRgn;
	ROWS   *tt;

	if (!AtBotEnd(global_df->top_win, global_df->tail_text, FALSE)) {
		for (i=ActualWinSize(w), tt=global_df->top_win; i > 0; i--) {
			if (!AtBotEnd(tt, global_df->tail_text, FALSE)) 
				tt = ToNextRow(tt, FALSE);
			else return;
		}
		NumRows = ActualWinSize(w);
		global_df->top_win = ToNextRow(global_df->top_win, FALSE);
		global_df->row_win--;
		last_row_win--;

  		GetPort(&oldPort);
		SetPortWindowPort(global_df->wind);
		theRect.left = LEFTMARGIN;
#if (TARGET_API_MAC_CARBON == 1)
		Rect rect;
		GetWindowPortBounds(global_df->wind, &rect);
		theRect.right = rect.right - SCROLL_BAR_SIZE;
#else
		theRect.right = global_df->wind->portRect.right - SCROLL_BAR_SIZE;
#endif
		theRect.top = w->LT_row;
		theRect.bottom = theRect.top + global_df->TextWinSize;
		tempRgn = NewRgn();
		ScrollRect(&theRect, 0, -(ComputeHeight(w, 0, 1)), tempRgn);
		DisposeRgn(tempRgn);
		SetPort(oldPort);

		global_df->WinChange = FALSE;
		DisplayTextWindow(NULL, 0);
		global_df->WinChange = TRUE;
	} else
		return;

	global_df->WinChange = FALSE;
	if (NumRows > ActualWinSize(w)) 
		wUntouchWin(w, NumRows-1);
	else
		wUntouchWin(w, ActualWinSize(w)-1);
	wrefresh(w);
	global_df->WinChange = TRUE;
	if (global_df->row_win2 > 0) {
		i = global_df->col_win;
		global_df->row_win2--;
		global_df->row_win++; global_df->col_win = 0L;
		DrawCursor(2);
		global_df->row_win--; global_df->col_win = i;
		global_df->row_win2++;
	}
}

void SelectCursorPosition(EventRecord *event, int extend) {
	int h = event->where.h;
	int v = event->where.v;
	int row, col, lrow;
	long tn;
	Point LocalPtr;
	ROWS *trow_txt;
	WINDOW *curWin;

	curWin = global_df->RootWindow;
	while (curWin != NULL) {
		if (curWin->LT_row <= v &&
			((curWin == global_df->w1 && v < global_df->TextWinSize) ||
			 (curWin == global_df->wm && v < curWin->LT_row+ComputeHeight(curWin,0,1)) ||
			 (curWin != global_df->wm && v < curWin->LT_row+ComputeHeight(curWin,0,ActualWinSize(curWin))))) {
			row = 0;
			v -= curWin->LT_row;
			do {
				v -= ComputeHeight(curWin,row,row+1);
				if (v <= 0) break;
				if (row == curWin->num_rows)
					break;
				row++;
			} while (TRUE) ;
			break;
		}
		curWin = curWin->NextWindow;
	}
	if (curWin == NULL) return;

	if (global_df->EditorMode && curWin == global_df->w2) {
		if (global_df->SoundWin == NULL && !global_df->NoCodes && global_df->cod_fname != NULL) {
			InKey = (int)'e';
			isPlayS = 4;
			global_df->LeaveHighliteOn = TRUE;
			return;
		}
	}

	if (global_df->ScrollBar == '\001' && curWin == global_df->w1) {
		col = FindRightColumn(h, curWin->win_data[row], row, curWin);
		if (!global_df->EditorMode && extend != 2) {
			InKey = (int)'e';
			isPlayS = 4;
			global_df->LeaveHighliteOn = TRUE;
		} else {
			isPlayS = 0;
			InKey = 0;
		}
/* take care of the mouse */
		if (global_df->row_win2 == 0L && global_df->col_win2 == -2)
			DrawCursor(0);
		else if (extend == 1) {
			if (global_df->row_win2 > 0) {
				global_df->row_win2 += global_df->row_win;
				tn = global_df->row_win; global_df->row_win = global_df->row_win2; global_df->row_win2 = tn;
				global_df->row_win2 -= global_df->row_win;
				tn = global_df->col_win; global_df->col_win = global_df->col_win2; global_df->col_win2 = tn;
				tn = global_df->col_chr; global_df->col_chr = global_df->col_chr2; global_df->col_chr2 = tn;
			} else if (global_df->row_win2 == 0 && global_df->col_win < global_df->col_win2) {
				tn = global_df->col_win; global_df->col_win = global_df->col_win2; global_df->col_win2 = tn;
				tn = global_df->col_chr; global_df->col_chr = global_df->col_chr2; global_df->col_chr2 = tn;
			}
		} else {
			DrawCursor(0);
			global_df->row_win2 = 0L;
			global_df->col_win2 = -2L;
			global_df->col_chr2 = -2L;
		}
		last_row_win = global_df->row_win;
		last_col_win = global_df->col_win;
		LocalPtr.h = event->where.h;
		LocalPtr.v = event->where.v;

		if ((((event->when - lastWhen) < GetDblTime()) &&
			(abs(LocalPtr.h - lastWhere.h) < 5) &&
			(abs(LocalPtr.v - lastWhere.v) < 5)) || extend == 4) {
			if (++DoubleClickCount > 3)
				DoubleClickCount = 3;
			else if (extend == 4)
				DoubleClickCount = 3;
			UpdateCursorPos(row, col+1, DoubleClickCount, extend);
			if (isPlayS == -5 && InKey == (int)'v') { // if file is to be openned then leave fake highlight
				FakeSelectWholeTier(TRUE);
				DrawFakeHilight(1);
			} else
				DrawCursor(2);
			while (StillDown() == TRUE) ;
		} else {
			DoubleClickCount = 1;
			SaveUndoState(FALSE);
			if (extend == 1)
				UpdateCursorPos(row, col+1, 1, extend);
			else
				UpdateCursorPos(row, col+1, 4, extend);
		}
		SetScrollControl();
		if (extend == 2)
			lastWhen  = 0L;
		else
			lastWhen  = event->when;
		lastWhere = LocalPtr;
/* main mouse down loop begin */
		while (StillDown() == TRUE) {
			GetMouse(&LocalPtr);
			lrow = 0;
			v = LocalPtr.v - curWin->LT_row;
			do {
				if (lrow >= global_df->EdWinSize) {
					lrow = global_df->EdWinSize - 1;
					break;
				}
				v -= ComputeHeight(curWin,lrow,lrow+1);
				if (v <= 0) break;
				lrow++;
			} while (TRUE) ;
			v = LocalPtr.v - curWin->LT_row;
			if (v >= global_df->TextWinSize) {
				MoveRegionUpAndAddLastTier(curWin);
				row = global_df->EdWinSize - 1;
				col = FindRightColumn(LocalPtr.h, curWin->win_data[row], row, curWin);
				SetScrollControl();
			} else if (v < 0) {
				MoveRegionDownAndAddFirstTier(curWin);
				row = 0;
				col = FindRightColumn(LocalPtr.h, curWin->win_data[row], row, curWin);
				SetScrollControl();
			} else {
				row = lrow;
				col = FindRightColumn(LocalPtr.h, curWin->win_data[row], row, curWin);
			} 
			UpdateCursorPos(row, col+1, 0, extend);
		}
		GetCurCode();
		FindRightCode(1);
		if (isPlayS != -5 && isPlayS != -8 && isPlayS != 4) {
			isPlayS = 0;
			InKey = 0;
		}
		PosAndDispl();
		if (extend == 2) {
			DrawCursor(0);
			global_df->row_win2 = 0L;
			global_df->col_win2 = -2L;
			global_df->col_chr2 = -2L;
			InKey = (int)'r';
			if (global_df->SoundWin == NULL)
				isPlayS = -4;
			else
				isPlayS = -7;
		}
		if (global_df->row_win2 < 0L && global_df->col_win <= 0) trow_txt= ToPrevRow(global_df->row_txt, FALSE);
		else trow_txt = global_df->row_txt;
		SetCurrentFontParams(trow_txt->Font.FName, trow_txt->Font.FSize);
		SetTextWinMenus(TRUE);
	} else if (global_df->SoundWin && (curWin == global_df->SoundWin || curWin == global_df->w2 || extend == 2)) {
		int left_lim;
		Size right_lim;
		Rect rect;

		GetWindowPortBounds(global_df->wind, &rect);
		right_lim = rect.right - SCROLL_BAR_SIZE - global_df->SnTr.SNDWccol + 1;
		left_lim = global_df->SnTr.SNDWccol;
		if (global_df->row_win2 != 0L || global_df->col_win2 != -2L) {
			DrawCursor(0);
			global_df->row_win2 = 0L;
			global_df->col_win2 = -2L;
			global_df->col_chr2 = -2L;
		}
		if (h >= left_lim && h <= right_lim) {
			if (global_df->SnTr.BegF == global_df->SnTr.EndF || global_df->SnTr.EndF == 0L)
				DrawSoundCursor(0);
			else if (extend != 1 && extend != 2) {
				DrawSoundCursor(0);
				global_df->SnTr.EndF = 0L;
			}
		} else
			DrawSoundCursor(0);
		v = row;
		LocalPtr.h = event->where.h;
		LocalPtr.v = event->where.v;
		row = 0;
		lrow = LocalPtr.v - global_df->SoundWin->LT_row;
		do {
			if (row >= global_df->SoundWin->num_rows) {
				row = global_df->SoundWin->num_rows - 1;
				break;
			}
			lrow -= ComputeHeight(global_df->SoundWin,row,row+1);
			if (lrow <= 0) break;
			row++;
		} while (TRUE) ;

		if (((event->when - lastWhen) < GetDblTime()) &&
			(abs(LocalPtr.h - lastWhere.h) < 5) &&
			(abs(LocalPtr.v - lastWhere.v) < 5) &&
			(h >= left_lim && h <= right_lim)) {
			InKey = (int)'s';
			isPlayS = 50;
		}
		if (extend == 2)
			lastWhen  = 0L;
		else
			lastWhen  = event->when;
		lastWhere = LocalPtr;

		if (AdjustSound(row, LocalPtr.h, extend, right_lim)) {
			delay_mach(25);
			while (StillDown() == TRUE) {
				GetMouse(&LocalPtr);
				row = 0;
				lrow = LocalPtr.v - global_df->SoundWin->LT_row;
				do {
					if (row >= global_df->SoundWin->num_rows) {
						row = global_df->SoundWin->num_rows - 1;
						break;
					}
					lrow -= ComputeHeight(global_df->SoundWin,row,row+1);
					if (lrow <= 0) break;
					row++;
				} while (TRUE) ;
				if ((LocalPtr.h >= left_lim && LocalPtr.h <= right_lim &&
					 h >= left_lim && h <= right_lim) ||
					((LocalPtr.h < left_lim || LocalPtr.h > right_lim) &&
					 (h < left_lim || h > right_lim))) {
					if (!AdjustSound(row, LocalPtr.h, extend, right_lim)) {
						while (StillDown() == TRUE) ;
						break;
					}
				}
			}
		} else
			while (StillDown() == TRUE) ;
		if (global_df->SnTr.EndF < global_df->SnTr.BegF && global_df->SnTr.EndF != 0L) {
			tn = global_df->SnTr.EndF; global_df->SnTr.EndF = global_df->SnTr.BegF; global_df->SnTr.BegF = tn;
			SetPBCglobal_df(false, 0L);
		}
		if ((h < left_lim || h > right_lim) && (v == 1 || v == 6 || v == 5 || v == 10))
			DisplayEndF(TRUE);
		DrawCursor(0);
	} else if (!global_df->EditorMode && curWin == global_df->w2 && extend != 2) {
		col = FindRightColumn(h, curWin->win_data[row], row, curWin);
		LocalPtr.h = event->where.h;
		LocalPtr.v = event->where.v;
		if (((event->when - lastWhen) < GetDblTime()) &&
			(abs(LocalPtr.h - lastWhere.h) < 5) &&
			(abs(LocalPtr.v - lastWhere.v) < 5)) {
			if (UpdateCodesCursorPos(row, col+1)) {
				InKey = (int)'c';
				isPlayS = 39;
				DrawCursor(0);
			}
		} else {
			UpdateCodesCursorPos(row, col+1);
		}
		while (StillDown() == TRUE) ;
		lastWhen  = event->when;
		lastWhere = LocalPtr;
	} else if (curWin == global_df->wm) {
		while (StillDown() == TRUE) ;
		if (global_df->SoundWin) {
			global_df->SnTr.dtype = (++global_df->SnTr.dtype) % 3;
			PutSoundStats(global_df->SnTr.WEndF - global_df->SnTr.WBegF);
			if (global_df->SnTr.dtype == 0)
				PrintSoundWin(0,0,0L);
		} else {
			blinkCursorLine();
		}
	} else if (!global_df->EditorMode && curWin == global_df->w1 && extend == 2) {
		SaveUndoState(FALSE);
		col = FindRightColumn(h, curWin->win_data[row], row, curWin);
		LocalPtr.h = event->where.h;
		LocalPtr.v = event->where.v;
		UpdateCursorPos(row, col+1, 1, extend);
		PosAndDispl();
		DrawCursor(0);
		global_df->row_win2 = 0L;
		global_df->col_win2 = -2L;
		global_df->col_chr2 = -2L;
		InKey = (int)'r';
		isPlayS = -6;
		while (StillDown() == TRUE) ;
		lastWhen  = 0L;
		lastWhere = LocalPtr;
	} else {
		if (global_df->ScrollBar != '\001') {
			DrawCursor(0);
			DrawSoundCursor(0);
			return;
		}
		if (global_df->row_win2 != 0L || global_df->col_win2 != -2L) {
			DrawCursor(0);
			global_df->row_win2 = 0L;
			global_df->col_win2 = -2L;
			global_df->col_chr2 = -2L;
		}
		while (StillDown() == TRUE) ;
		DrawCursor(0);
	}
}
