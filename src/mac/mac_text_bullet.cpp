/*
 
 search for: text_bullet
	1966
 
	get movie frames and draw them:
	aPicHandle = GetMoviePict(theMovie, time);
	DrawPicture(myPicture, dstRect);
	
	call this only when very much done
	KillPicture(myPicture);
*/
#include "mac_text_bullet.h"

#ifdef TEXT_BULLETS_PICTS // text_bullet

#include "ced.h"
#include "cu.h"
#include "MMedia.h"

extern char fgets_cr_lc;

static int topRow;
static int maxRows;
static int max_num_rows;
static unCH txtMovieFile[FILENAME_MAX];
static COLORTEXTLIST *text_bullets_RootColorText = NULL;
static FNType txtrMovieFile[FNSize];
static MovieTXTInfo *txtPicts = NULL;

static MovieTXTInfo *freeText(MovieTXTInfo *p) {
	MovieTXTInfo *t;

	while (p != NULL) {
		t = p;
		p = p->nextPict;
		if (t->text)
			free(t->text);
		if (t->atts)
			free(t->atts);
		if (t->pict != NULL)
			KillPicture(t->pict);
		free(t);
	}
	FreeColorText(text_bullets_RootColorText);
	text_bullets_RootColorText = NULL;
	return(NULL);
}

static void CleanupText(WindowPtr wind) {
	topRow = 0;
	maxRows = 0;
	max_num_rows = 0;
	txtPicts = freeText(txtPicts);
}

static void SetTextScrollControl(WindowPtr win) {
	GrafPtr			savePort;
	WindowProcRec	*windProc;

	GetPort(&savePort);
 	SetPortWindowPort(win);
 	windProc = WindowProcs(win);
 	if (windProc != NULL && windProc->VScrollHnd != NULL) {
		if (topRow == 0 && max_num_rows == maxRows) {
			HiliteControl(windProc->VScrollHnd, 255);
		} else {
			HiliteControl(windProc->VScrollHnd,0);
			SetControlMaximum(windProc->VScrollHnd, maxRows-1);
			SetControlValue(windProc->VScrollHnd, topRow);
		}
		HiliteControl(windProc->HScrollHnd, 255);
	}
	SetPort(savePort);
}

static void UpdateText(WindowPtr win) {
	register int	col;
	register int	cCol;
	int				top, left, hight, width;
	int				lastViewCol;
	int				i, len, num_rows;
	unsigned int	row, maxRowHight;
	double			ThumbnailsWidth;
	unCH			*sData;
	AttTYPE			*sAtts;
	AttTYPE			oldState;
    Rect			theRect;
    GrafPtr			oldPort;
	COLORTEXTLIST	*tierColor = NULL;
	RGBColor		oldColor, theColor;
	MovieTXTInfo	*Picts, *tPicts;
	Rect			rect;

	if (txtPicts == NULL)
		return;
	GetPort(&oldPort);
    SetPortWindowPort(win);
	GetWindowPortBounds(win, &rect);
	theRect.bottom = rect.bottom;
	theRect.top = rect.top;
	theRect.left = rect.left;
	theRect.right = rect.right - SCROLL_BAR_SIZE;
	EraseRect(&theRect);
	hight = rect.bottom - rect.top - SCROLL_BAR_SIZE;
	width = rect.right - rect.left - SCROLL_BAR_SIZE;
	maxRows = 0;
	row = 0;
	for (Picts=txtPicts; Picts != NULL; Picts=Picts->nextPict) {
		Picts->row = row;
		if (Picts->isNewLine)
			row++;		
	}
	maxRows = row;
	if (topRow >= maxRows)
		topRow = maxRows - 1;

	GetForeColor(&oldColor);
	Picts = txtPicts;
	for (; Picts != NULL; Picts=Picts->nextPict) {
		if (topRow == Picts->row)
			break;
	}
	num_rows = 0;
	for (top=0; top < hight && Picts != NULL; ) {
		maxRowHight = 0;
		for (tPicts=Picts; tPicts != NULL; tPicts=tPicts->nextPict) {
			if (tPicts->text != NULL) {
				TextFont(tPicts->Font.FName);
				TextSize(tPicts->Font.FSize);
				tPicts->Font.FHeight = GetFontHeight(&tPicts->Font, NULL, win);
				if (maxRowHight < tPicts->Font.FHeight)
					maxRowHight = tPicts->Font.FHeight;
			}
			if (tPicts->pict != NULL) {
				if (maxRowHight < (int)(ThumbnailsHight))
					maxRowHight = (int)(ThumbnailsHight);
			}
			if (tPicts->isNewLine)
				break;
		}
		left = 0;
		do {
			if (Picts->text != NULL) {
				TextFont(Picts->Font.FName);
				TextSize(Picts->Font.FSize);
				MoveTo(left, top+maxRowHight);
				cCol = 0;
				lastViewCol = strlen(Picts->text);
				sData = Picts->text;
				sAtts = Picts->atts;
				len = strlen(sData);
				for (i=0; i < len; i++) {
					if (is_colored(sAtts[i]))
						sAtts[i] = set_color_to_0(sAtts[i]);
				}
				if (text_bullets_RootColorText != NULL)
					tierColor = FindColorKeywordsBounds(text_bullets_RootColorText,sAtts,sData,0,len,tierColor);
				oldState = sAtts[0];
				for (col=0; col <= lastViewCol; col++) {
					if (oldState != sAtts[col] || col == lastViewCol) {
						if (is_colored(sAtts[cCol])) {
							if (SetKeywordsColor(text_bullets_RootColorText, cCol, &theColor))
								RGBForeColor(&theColor);
						}
						DrawUTFontMac(sData+cCol, col-cCol, &Picts->Font, sAtts[cCol]);
						if (is_colored(sAtts[cCol]))
							RGBForeColor(&oldColor);
						cCol = col;
						oldState = sAtts[col];
					}
				}
				len = TextWidthInPix(Picts->text, 0L, strlen(Picts->text), &Picts->Font, 0);
				if (Picts->pict == NULL) {
					Picts->dstRect.top		= top;
					Picts->dstRect.bottom	= top + maxRowHight;
					Picts->dstRect.left		= left;
					Picts->dstRect.right	= left + len;
				}
				left = left + len;
			}
			if (Picts->pict != NULL) {
				Picts->dstRect.top		= top;
				Picts->dstRect.bottom	= top + maxRowHight;
				Picts->dstRect.left		= left;
				ThumbnailsWidth = (double)(maxRowHight) * Picts->aspectRetio;
				Picts->dstRect.right	= left + (int)(ThumbnailsWidth);
				left = Picts->dstRect.right + 2;
				DrawPicture(Picts->pict, &Picts->dstRect);
			}
			if (Picts->isNewLine) {
				Picts = Picts->nextPict;
				break;
			} else
				Picts = Picts->nextPict;
		} while (left < width && Picts != NULL) ;
		top += maxRowHight + 2;
		num_rows++;
	}
	if (topRow == 0 || top >= hight || num_rows > max_num_rows)
		max_num_rows = num_rows;
	SetTextScrollControl(win);
	DrawGrowIcon(win);
	DrawControls(win);
	SetPort(oldPort);
}

void HandleTextVScrollBar(WindowPtr win, short code, ControlRef theControl, Point   myPt) {
    long  MaxTick;

	do {
		HiliteControl(theControl, code);
		switch (code) {
			case kControlUpButtonPart:
				if (topRow > 0) {
					topRow--;
					UpdateText(win);
				}
				break;

			case kControlDownButtonPart:
				if (topRow < maxRows - 1) {
					topRow++;
					UpdateText(win);
				}
				break;

			case kControlPageUpPart:
				topRow -= (max_num_rows / 2);
				if (topRow < 0)
					topRow = 0;
				UpdateText(win);
				break;

			case kControlPageDownPart:
				topRow += (max_num_rows / 2);
				if (topRow >= maxRows) 
					topRow = maxRows - 1;
				UpdateText(win);
				break;

			case kControlIndicatorPart:
				code = TrackControl(theControl, myPt, NULL);
				topRow = GetControlValue(theControl);
				if (topRow >= maxRows)
					topRow = maxRows - 1;
				UpdateText(win);
				break;	
		}

		SetTextScrollControl(win);
//		if (code != kControlUpButtonPart && code != kControlDownButtonPart) {
			MaxTick = TickCount() + 5;
			do {  
				if (TickCount() > MaxTick) break;		 
			} while ( Button() == TRUE) ;
//		}
	} while (StillDown() == TRUE) ;
}

static void Do_Text_ScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    short RefCon;

	RefCon = GetControlReference(theControl);
	switch  (RefCon) {
		case I_VScroll_bar:
			HandleTextVScrollBar(win,code,theControl,myPt);
			break;
		default:
			break;
	}
}

static MovieTXTInfo *SelectTextPosition(WindowPtr win, EventRecord *event) {
	register int	h = event->where.h;
	register int	v = event->where.v;
	register int	offset;
	MovieTXTInfo	*Picts;
	Rect			rect;

	if (txtPicts == NULL)
		return(NULL);

	GetWindowPortBounds(win, &rect);
	if (v < 0 || h > rect.right - rect.left - SCROLL_BAR_SIZE)
		return(NULL);

	while (StillDown() == TRUE) ;
	Picts = txtPicts;
	for (; Picts != NULL; Picts=Picts->nextPict) {
		if (topRow == Picts->row)
			break;
	}
	if (Picts == NULL)
		return(NULL);
	offset = Picts->dstRect.top;
	for (; Picts != NULL; Picts=Picts->nextPict) {
		if ((v > (Picts->dstRect.top-offset) && v < (Picts->dstRect.bottom-offset)) &&
			(h > Picts->dstRect.left && h < Picts->dstRect.right))
			break;
	}
	return(Picts);
}

static short TextEvent(WindowPtr win, EventRecord *event) {
	int				code;
	short			key;
	ControlRef		theControl;
	WindowProcRec	*windProc;
	MovieTXTInfo	*Pict;
	movInfo			mvRec;

	windProc = WindowProcs(win);
	if (windProc == NULL)
		return(-1);
	
	if (event->what == keyDown || event->what == autoKey) {
		key = (event->message & keyCodeMask) / 256;
		if (key == up_key) {
			if (topRow > 0) {
				topRow--;
				UpdateText(win);
			}
		} else if (key == down_key) {
			if (topRow < maxRows - 1) {
				topRow++;
				UpdateText(win);
			}
		}
	} else if (event->what == mouseDown) {
		GlobalToLocal(&event->where);
		code = FindControl(event->where, win, &theControl);
		if (code != 0) {
			if (code == kControlUpButtonPart || code == kControlDownButtonPart || 
				code == kControlIndicatorPart || code == kControlPageDownPart || 
				code == kControlPageUpPart) {
				Do_Text_ScrollBar(win,code,theControl,event->where);
			}
		} else {
			Pict = SelectTextPosition(win, event);
			if (Pict != NULL && (Pict->MBeg != 0L || Pict->MEnd != 0L)) {
/* 2011-12-08 never happens and create a problem
				if (global_df != NULL) {
					DrawCursor(0);
					SaveUndoState(FALSE);
					move_cursor(Pict->MBeg+1, txtMovieFile, TRUE, FALSE);
					global_df->LeaveHighliteOn = TRUE;
					DrawCursor(1);
				}
*/
				strcpy(mvRec.MovieFile, txtMovieFile);
				strcpy(mvRec.rMovieFile, txtrMovieFile);
				mvRec.MBeg = Pict->MBeg;
				mvRec.MEnd = Pict->MEnd;
				PlayMovie(&mvRec, NULL, FALSE);
			}
		}
	}
	return(-1);
}

static int ShowText(FNType *name, long totalPicts) {
	float			aspectRetio, t1, t2;
	TimeScale		ts;
	TimeValue		pos;
	WindowPtr		windPtr;
	MovieInfo		*thisMovie;
	movInfo			mvRec;
	MovieTXTInfo	*Picts;
	FNType			oldfName[FNSize];
	long			cPict;

	thisMovie = NULL;
	aspectRetio = 0.0;
	if (!isMovieAvialable) {
		do_warning("QuickTime failure!", 0);
		txtPicts = freeText(txtPicts);
		return(1);
	}
	if (!PlayingSound && !PlayingContSound && !PlayingContMovie) {
		if (totalPicts > 0) {
			sprintf(templineC, "Loading %ld movie clips...", totalPicts);
			if (!OpenProgressDialog(templineC)) {
				txtPicts = freeText(txtPicts);
				return(1);
			}
		}
		cPict = 0;
		oldfName[0] = EOS;
		for (Picts=txtPicts; Picts != NULL; Picts=Picts->nextPict) {
			if ((Picts->MBeg != 0L || Picts->MEnd != 0L) && Picts->isShowPict) {
				if (strcmp(oldfName, txtrMovieFile)) {
					if (thisMovie) {
						if (thisMovie->MvMovie != NULL)
							DisposeMovie(thisMovie->MvMovie);
						DisposePtr((Ptr)thisMovie);
					}
					strcpy(mvRec.rMovieFile, txtrMovieFile);
					thisMovie = Movie_Open(&mvRec);
					if (thisMovie == NULL) {
						sprintf(templineC3, "Error opening Movie: %s", txtrMovieFile);
						do_warning(templineC3, 0);
						txtPicts = freeText(txtPicts);
						return(1);
					}
					GetMovieBox(thisMovie->MvMovie, &thisMovie->MvBounds);
					t1 = thisMovie->MvBounds.right - thisMovie->MvBounds.left;
					t2 = thisMovie->MvBounds.bottom - thisMovie->MvBounds.top;
					aspectRetio = (float)(t1 / t2);
				}
				strcpy(oldfName, txtrMovieFile);
				ts = GetMovieTimeScale(thisMovie->MvMovie);
				Picts->aspectRetio = aspectRetio;
				pos = conv_from_msec_mov(Picts->MBeg, ts);
				Picts->pict = GetMoviePict(thisMovie->MvMovie, pos);
				cPict++;
				if (totalPicts > 0) {
					if (!UpdateProgressDialog((short)((100L*cPict)/totalPicts))) {
						txtPicts = freeText(txtPicts);
						return(1);
					}
				}
			}
		}
		if (totalPicts > 0) {
			CloseProgressDialog();
		}
		if (thisMovie) {
			if (thisMovie->MvMovie != NULL)
				DisposeMovie(thisMovie->MvMovie);
			DisposePtr((Ptr)thisMovie);
		}
	}
	if (OpenWindow(1966, name, 0L, false, 1, TextEvent, UpdateText, CleanupText)) {
		do_warning("Error opening Text window", 0);
		txtPicts = freeText(txtPicts);
		return(1);
	}
	windPtr = FindAWindowNamed(name);
	if (windPtr == NULL) {
		do_warning("Error opening Text window", 0);
		txtPicts = freeText(txtPicts);
		return(1);
	}
	UpdateText(windPtr);
	topRow = 0;
	maxRows = 0;
	max_num_rows = 0;
	return(1);
}

static char FindMovieInfo(unCH *line, int index, movInfo *tempMovie) {
	register int i;
	long beg, end;
	unCH buf[FILENAME_MAX];

	index++;
	if (is_unCH_digit(line[index])) {
		if (tempMovie->MovieFile[0] == EOS)
			return(-2);
	} else if (!uS.partwcmp(line+index, SOUNDTIER) && !uS.partwcmp(line+index, REMOVEMOVIETAG))
		return(0);
	else
		return(-1);
	if (line[index] == EOS || line[index] == HIDEN_C)
		return(0);
	if (mOpenMovieFile(tempMovie) != 0)
		return(-3);
	for (; line[index] && !is_unCH_digit(line[index]) && line[index] != HIDEN_C; index++) ;
	if (!is_unCH_digit(line[index]))
		return(-1);
	for (i=0; line[index] && is_unCH_digit(line[index]); index++)
		buf[i++] = line[index];
	buf[i] = EOS;
	beg = atol(buf);
	for (; line[index] && !is_unCH_digit(line[index]) && line[index] != HIDEN_C; index++) ;
	if (!is_unCH_digit(line[index]))
		return(-1);
	for (i=0; line[index] && is_unCH_digit(line[index]); index++)
		buf[i++] = line[index];
	buf[i] = EOS;
	end = atol(buf);
	tempMovie->MBeg = beg;
	tempMovie->MEnd = end;
	tempMovie->nMovie = NULL;
	return(1);
}

static void extractMediaName(unCH *line) {
	char qf;
	unCH *s;
	int i;
	
	strcpy(line, line+strlen(MEDIAHEADER));
	for (i=0; isSpace(line[i]); i++) ;
	if (i > 0)
		strcpy(line, line+i);
	qf = FALSE;
	for (i=0; line[i] != EOS; i++) {
		if (line[i] == '"')
			qf = !qf;
		if (line[i] == ',' || (!qf && isSpace(line[i]))) {
			line[i] = EOS;
			break;
		}
	}
	uS.remblanks(line);
	s = strrchr(line, '.');
	if (s != NULL)
		*s = EOS;
}

static char SetTextAttW(wchar_t *st, AttTYPE *att) {
	char c;
	char found = FALSE;

	c = (char)st[1];
	if (c == underline_start) {
		*att = set_underline_to_1(*att);
		found = TRUE;
	} else if (c == underline_end) {
		*att = set_underline_to_0(*att);
		found = TRUE;
	} else if (c == italic_start) {
		*att = set_italic_to_1(*att);
		found = TRUE;
	} else if (c == italic_end) {
		*att = set_italic_to_0(*att);
		found = TRUE;
	} else if (c == bold_start) {
		*att = set_bold_to_1(*att);
		found = TRUE;
	} else if (c == bold_end) {
		*att = set_bold_to_0(*att);
		found = TRUE;
	} else if (c == error_start) {
		*att = set_error_to_1(*att);
		found = TRUE;
	} else if (c == error_end) {
		*att = set_error_to_0(*att);
		found = TRUE;
	} else if (c == blue_start) {
		*att = set_color_num(blue_color, *att);
		found = TRUE;
	} else if (c == red_start) {
		*att = set_color_num(red_color, *att);
		found = TRUE;
	} else if (c == green_start) {
		*att = set_color_num(green_color, *att);
		found = TRUE;
	} else if (c == magenta_start) {
		*att = set_color_num(magenta_color, *att);
		found = TRUE;
	} else if (c == color_end) {
		*att = zero_color_num(*att);
		found = TRUE;
	}
	if (found) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}

static void convertTabs(unCH *templine, unCH *templine1) {
	register long i;
	register long lineChr;
	register long colWin;
	long	newcol;
	AttTYPE att;

	i = 0L;
	att = 0;
	colWin = 0L;
	lineChr = 0L;
	while (templine1[lineChr]) {
		if (templine1[lineChr] == '\t') {
			newcol = (((colWin / TabSize) + 1) * TabSize);
			for (; colWin < newcol; colWin++) {
				templine[i] = ' ';
				tempAtt[i] = att;
				i++;
			}
		} else {
			colWin++;
			if (templine1[lineChr] == ATTMARKER && SetTextAttW(templine1+lineChr, &att)) {
				lineChr++;
			} else {
				templine[i] = templine1[lineChr];
				tempAtt[i] = att;
				i++;
			}
		}
		lineChr++;
	}
	templine[i] = EOS;
}

int DisplayText(FNType *fname) {
	int			i, bc, ec, len;
	char		res;
	char		txtMovieFileC[FILENAME_MAX];
	long		ln, totalPicts;
	FILE		*fp;
	myFInfo 	*saveGlobal_df;
	movInfo		tempMovie;
	MovieTXTInfo *cPict;
	FNType mDirPathName[FNSize];
	FNType mFileName[FNSize];
	NewFontInfo finfo;
	NewFontInfo tFInfo;

	txtPicts = freeText(txtPicts);
	extractPath(mDirPathName, fname);
	extractFileName(mFileName, fname);
	if ((fp=fopen(fname,"r")) == NULL) {
		DrawMouseCursor(1);
		sprintf(templineC, "Can't open file \"%s\".", fname);
		do_warning(templineC, 0);
		return(0);
	}
	strcpy(finfo.fontName, DEFAULT_FONT);
	finfo.fontSize = DEFAULT_SIZE;
	finfo.fontId = DEFAULT_ID;
#ifdef _MAC_CODE
	finfo.orgFType = getFontType(finfo.fontName, FALSE);
	finfo.CharSet = my_FontToScript(DEFAULT_ID, 0);
	finfo.Encod = finfo.orgEncod = finfo.CharSet;
#elif defined(_WIN32)
	finfo.orgFType = getFontType(finfo.fontName, TRUE);
	finfo.CharSet = NSCRIPT;
	finfo.Encod = finfo.orgEncod = 1252;
#endif
	finfo.fontTable = NULL;
#ifdef _MAC_CODE
	if (DefChatMode == 2 && isCHATFile(fname) != '\001' && isCEXFile(fname) != '\001') {
		if (GetFontOfTEXT(templineC, fname)) {
			if (SetNewFont(templineC, EOS, &tFInfo))
				copyNewFontInfo(&finfo, &tFInfo);
		}
	}
#endif
	DrawMouseCursor(2);
	DrawSoundCursor(0);
	totalPicts = 0L;
	txtMovieFile[0] = EOS;
	txtMovieFileC[0] = EOS;
	txtrMovieFile[0] = EOS;
	fgets_cr_lc = '\0';
	ln = 0;
	while (fgets_cr(templineC, 256, fp)) {
		if (uS.isUTF8(templineC) || uS.partcmp(templineC, FONTHEADER, FALSE, FALSE))
			continue;
		if (uS.partcmp(templineC, CKEYWORDHEADER, FALSE, FALSE)) {
			FreeColorText(text_bullets_RootColorText);
			text_bullets_RootColorText = NULL;
			text_bullets_RootColorText = createColorTextKeywordsList(text_bullets_RootColorText, templineC);
			continue;
		}
		if (uS.partcmp(templineC,FONTHEADER,FALSE,FALSE)) {
			strcpy(templineC, templineC+strlen(FONTHEADER));
			for (i=0; isSpace(templineC[i]); i++) ;
			if (SetNewFont(templineC+i, EOS, &tFInfo))
				copyNewFontInfo(&finfo, &tFInfo);
			continue;
		}
		ln++;
		u_strcpy(templine1, templineC, UTTLINELEN);
		if (uS.partcmp(templine1, MEDIAHEADER, FALSE, FALSE)) {
			extractMediaName(templine1);
			strcpy(txtMovieFile, templine1);
			u_strcpy(txtMovieFileC, txtMovieFile, FILENAME_MAX);
			continue;
		}
		uS.remblanks(templine1);
		bc = 0;
		ec = 0;
		convertTabs(templine, templine1);
		while (templine[ec]) {
			if (templine[ec] == HIDEN_C) {
				strcpy(tempMovie.MovieFile, txtMovieFile);
				strcpy(tempMovie.rMovieFile, mDirPathName);
				saveGlobal_df = global_df;
				global_df = NULL;
				res = FindMovieInfo(templine, ec, &tempMovie);
				global_df = saveGlobal_df;
				if (res == -1) {
					DrawMouseCursor(1);
					sprintf(templineC, "Media bullet is corrupted or old format on line %ld.", ln);
					do_warning(templineC, 0);
					return(0);
				} else if (res == -2) {
					DrawMouseCursor(1);
					sprintf(templineC, "Can't find \"%s\" header tier in %%txt: bullet file.", MEDIAHEADER);
					do_warning(templineC, 0);
					return(0);
				} else if (res == -3) {
					DrawMouseCursor(1);
					sprintf(templineC, "Can't locate media file \"%s\".", txtMovieFileC);
					do_warning(templineC, 0);
					return(0);
				} else if (res == 1) {
					if (txtPicts == NULL) {
						txtPicts = NEW(MovieTXTInfo);
						cPict = txtPicts;
					} else {
						cPict->nextPict = NEW(MovieTXTInfo);
						cPict = cPict->nextPict;
					}
					if (cPict == NULL) {
						txtPicts = freeText(txtPicts);
						DrawMouseCursor(1);
						sprintf(templineC, "Out of memory.");
						do_warning(templineC, 0);
						return(0);
					}
					cPict->text = NULL;
					cPict->isShowPict = FALSE;
					cPict->isNewLine = FALSE;
					cPict->atts = NULL;
					cPict->MBeg = tempMovie.MBeg;
					cPict->MEnd = tempMovie.MEnd;
					cPict->row = 0;
					cPict->pict = NULL;
					copyNewToFontInfo(&cPict->Font, &finfo);
					cPict->nextPict = NULL;
					strcpy(txtrMovieFile, tempMovie.rMovieFile);
					len = ec - bc + 1;
					if (!uS.partcmp(templine, PICTTIER, FALSE, FALSE)) {
						len++;
					}
					cPict->text = (unCH *)malloc((len)*sizeof(unCH));
					if (cPict->text == NULL) {
						txtPicts = freeText(txtPicts);
						DrawMouseCursor(1);
						sprintf(templineC, "Out of memory.");
						do_warning(templineC, 0);
						return(0);
					}
					cPict->text[0] = EOS;
					cPict->atts = (AttTYPE *)malloc((len)*sizeof(AttTYPE));
					if (cPict->atts == NULL) {
						txtPicts = freeText(txtPicts);
						DrawMouseCursor(1);
						sprintf(templineC, "Out of memory.");
						do_warning(templineC, 0);
						return(0);
					}
					for (i=0; bc < ec; bc++) {
						cPict->text[i] = templine[bc];
						cPict->atts[i] = tempAtt[bc];
						i++;
					}
					if (!uS.partcmp(templine, PICTTIER, FALSE, FALSE)) {
						cPict->text[i] = 0x2022;
						cPict->atts[i] = 0;
						i++;
						cPict->isShowPict = FALSE;
					} else if (PlayingSound || PlayingContSound || PlayingContMovie) {
						cPict->text[i] = 0x2022;
						cPict->atts[i] = 0;
						i++;
						cPict->isShowPict = FALSE;
					} else {
						totalPicts++;
						cPict->isShowPict = TRUE;
					}
					cPict->text[i] = EOS;
				}
				bc = ec + 1;
				for (; templine[bc] != HIDEN_C && templine[bc] != EOS; bc++) ;
				if (templine[bc] == EOS) {
					DrawMouseCursor(1);
					sprintf(templineC, "Unmatched bullets found in %%txt: bullet file.");
					do_warning(templineC, 0);
					return(0);
				}
				bc++;
				ec = bc;
			} else
				ec++;
		}
		if (bc != ec || bc == 0) {
			ec = strlen(templine);
			if (txtPicts == NULL) {
				txtPicts = NEW(MovieTXTInfo);
				cPict = txtPicts;
			} else {
				cPict->nextPict = NEW(MovieTXTInfo);
				cPict = cPict->nextPict;
			}
			if (cPict == NULL) {
				txtPicts = freeText(txtPicts);
				DrawMouseCursor(1);
				sprintf(templineC, "Out of memory.");
				do_warning(templineC, 0);
				return(0);
			}
			cPict->text = NULL;
			cPict->isShowPict = FALSE;
			cPict->isNewLine = TRUE;
			cPict->MBeg = 0L;
			cPict->MEnd = 0L;
			cPict->row = 0;
			cPict->pict = NULL;
			copyNewToFontInfo(&cPict->Font, &finfo);
			cPict->nextPict = NULL;
			cPict->atts = NULL;
			len = ec - bc + 1;
			cPict->text = (unCH *)malloc((len)*sizeof(unCH));
			if (cPict->text == NULL) {
				txtPicts = freeText(txtPicts);
				DrawMouseCursor(1);
				sprintf(templineC, "Out of memory.");
				do_warning(templineC, 0);
				return(0);
			}
			cPict->text[0] = EOS;
			cPict->atts = (AttTYPE *)malloc((len)*sizeof(AttTYPE));
			if (cPict->atts == NULL) {
				txtPicts = freeText(txtPicts);
				DrawMouseCursor(1);
				sprintf(templineC, "Out of memory.");
				do_warning(templineC, 0);
				return(0);
			}
			for (i=0; bc < ec; bc++) {
				cPict->text[i] = templine[bc];
				cPict->atts[i] = tempAtt[bc];
				i++;
			}
			cPict->text[i] = EOS;
		} else if (cPict != NULL) {
			cPict->isNewLine = TRUE;
		}
	}
	DrawMouseCursor(1);
	if (txtPicts != NULL) {
		ShowText(mFileName, totalPicts);
	}
	return(1);
}

#endif //TEXT_BULLETS_PICTS // text_bullet
