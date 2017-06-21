/*
	get movie frames and draw them:
	aPicHandle = GetMoviePict(theMovie, time);
	DrawPicture(myPicture, dstRect);
	
	call this only when very much done
	KillPicture(myPicture);
*/

#include "ced.h"
#include "cu.h"
#include "MMedia.h"

extern FNType ThumbNails_str[];

static long			lastWhen = 0;
static Point		lastWhere = {0,0};
static unsigned int topRow;
static unsigned int maxRows;
static unsigned int local_num_rows;
static myFInfo		*Odf;

MovieTNInfo *moviePics = NULL;

void freeThumbnails(MovieTNInfo *Picts) {
	MovieTNInfo *t;

	while (Picts != NULL) {
		t = Picts;
		Picts = Picts->nextPict;
		if (t->orgFName)
			free(t->orgFName);
		if (t->fName)
			free(t->fName);
		if (t->pict != NULL)
			KillPicture(t->pict);
		free(t);
	}
}

static void CleanupThumbnails(WindowPtr wind) {
	topRow = 0;
	maxRows = 0;
	Odf = NULL;
	freeThumbnails(moviePics);
	moviePics = NULL;
}

static void SetThumbScrollControl(WindowPtr win) {
	GrafPtr			savePort;
	WindowProcRec	*windProc;

	GetPort(&savePort);
 	SetPortWindowPort(win);
 	windProc = WindowProcs(win);
 	if (windProc != NULL && windProc->VScrollHnd != NULL) {
		if (topRow == 0 && local_num_rows >= maxRows) {
			HiliteControl(windProc->VScrollHnd, 255);
		} else {
			HiliteControl(windProc->VScrollHnd,0);
			SetControlMaximum(windProc->VScrollHnd, maxRows-1);
			SetControlValue(windProc->VScrollHnd, topRow);
		}
	}
	SetPort(savePort);
}

static void UpdateThumbnail(WindowPtr win) {
	int				top, left, hight, width;
	unsigned int	row, maxRowHight;
	double			ThumbnailsWidth;
    Rect			theRect;
    GrafPtr			oldPort;
	MovieTNInfo		*Picts;
	Rect			rect;

	if (moviePics == NULL)
		return;
	GetPort(&oldPort);
    SetPortWindowPort(win);

	GetWindowPortBounds(win, &rect);
	theRect.bottom = rect.bottom;
	theRect.top = rect.top;
	theRect.left = rect.left;
	theRect.right = rect.right - SCROLL_BAR_SIZE;
	EraseRect(&theRect);

	hight = rect.bottom - rect.top - SCROLL_BAR_SIZE - 54;
	width = rect.right - rect.left - SCROLL_BAR_SIZE;
	row = 0;
	Picts = moviePics;
	while (Picts != NULL) {
		for (left=0; left < width && Picts != NULL; ) {
			if (Picts->pict) {
				ThumbnailsWidth = ThumbnailsHight * Picts->aspectRetio;
				left = left + (int)(ThumbnailsWidth) + 4;
				if (left >= width)
					break;
			}
			Picts->row = row;
			Picts = Picts->nextPict;
		}
		row++;
	}
	maxRows = row;
	if (topRow >= maxRows)
		topRow = maxRows - 1;

	Picts = moviePics;
	for (; Picts != NULL; Picts=Picts->nextPict) {
		if (topRow == Picts->row)
			break;
	}
	local_num_rows = 0;
	for (top=0; top < hight && Picts != NULL; ) {
		maxRowHight = 0;
		for (left=0; left < width && Picts != NULL; ) {
			if (Picts->pict) {
				Picts->dstRect.top		= top;
				Picts->dstRect.bottom	= top + ThumbnailsHight;
				Picts->dstRect.left		= left;
				ThumbnailsWidth = ThumbnailsHight * Picts->aspectRetio;
				Picts->dstRect.right	= left + (int)(ThumbnailsWidth);
				left = Picts->dstRect.right + 4;
				if (left >= width)
					break;
				DrawPicture(Picts->pict, &Picts->dstRect);
				TextFont(Picts->Font.FName);
				TextSize(Picts->Font.FSize);
				if (Picts->Font.FHeight > maxRowHight)
					maxRowHight = Picts->Font.FHeight;
				MoveTo(Picts->dstRect.left+5, top+ThumbnailsHight+Picts->Font.FHeight);
				DrawUTFontMac(Picts->text, Picts->textLen, &Picts->Font, 0);
			} else
				Picts->dstRect.top = Picts->dstRect.bottom = Picts->dstRect.left = Picts->dstRect.right = 0;
			Picts = Picts->nextPict;
		}
		top += ThumbnailsHight + 4 + maxRowHight;
		local_num_rows++;
	}
	SetThumbScrollControl(win);
	DrawGrowIcon(win);
	DrawControls(win);
	SetPort(oldPort);
}

void HandleThumbVScrollBar(WindowPtr win, short code, ControlRef theControl, Point   myPt) {
    int  t;
    long MaxTick;

	do {
		HiliteControl(theControl, code);
		switch (code) {
			case kControlUpButtonPart:
				if (topRow > 0) {
					topRow--;
					UpdateThumbnail(win);
				}
				break;

			case kControlDownButtonPart:
				if (topRow < maxRows - 1) {
					topRow++;
					UpdateThumbnail(win);
				}
				break;

			case kControlPageUpPart:
				t = topRow - (local_num_rows / 2);
				if (t < 0)
					t = 0;
                topRow = t;
				UpdateThumbnail(win);
				break;

			case kControlPageDownPart:
				topRow += (local_num_rows / 2);
				if (topRow >= maxRows) 
					topRow = maxRows - 1;
				UpdateThumbnail(win);
				break;

			case kControlIndicatorPart:
				code = TrackControl(theControl, myPt, NULL);
				topRow = GetControlValue(theControl);
				if (topRow >= maxRows)
					topRow = maxRows - 1;
				UpdateThumbnail(win);
				break;	
		}

		SetThumbScrollControl(win);
//		if (code != kControlUpButtonPart && code != kControlDownButtonPart) {
			MaxTick = TickCount() + 5;
			do {  
				if (TickCount() > MaxTick) break;		 
			} while ( Button() == TRUE) ;
//		}
	} while (StillDown() == TRUE) ;
}

static void Do_Thumb_ScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    short RefCon;

	RefCon = GetControlReference(theControl);
	switch  (RefCon) {
		case I_VScroll_bar:
			HandleThumbVScrollBar(win,code,theControl,myPt);
			break;
		default:
			break;
	}
}

static MovieTNInfo *SelectThumbPosition(WindowPtr win, EventRecord *event) {
	register int	h = event->where.h;
	register int	v = event->where.v;
	register int	offset;
    Point			LocalPtr;
	MovieTNInfo		*Picts;
	Rect			rect;

	if (moviePics == NULL)
		return(NULL);

	GetWindowPortBounds(win, &rect);
	if (v < 0 || h > rect.right - rect.left - SCROLL_BAR_SIZE)
		return(NULL);

	LocalPtr.h = h;
	LocalPtr.v = v;
	if (((event->when - lastWhen) < GetDblTime()) &&
	    (abs(LocalPtr.h - lastWhere.h) < 5) && (abs(LocalPtr.v - lastWhere.v) < 5)) {
	    while (StillDown() == TRUE) ;
		Picts = moviePics;
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
	lastWhen  = event->when;
	lastWhere = LocalPtr;
	return(NULL);
}

static short ThumbnailEvent(WindowPtr win, EventRecord *event) {
	int				code;
	short			key;
	ControlRef		theControl;
	WindowProcRec	*windProc;
	MovieTNInfo		*Pict;
	movInfo			mvRec;
	myFInfo 		*saveGlobal_df;

	windProc = WindowProcs(win);
	if (windProc == NULL)
		return(-1);
	
	if (event->what == keyDown || event->what == autoKey) {
		key = (event->message & keyCodeMask) / 256;
		if (key == up_key) {
			if (topRow > 0) {
				topRow--;
				UpdateThumbnail(win);
			}
		} else if (key == down_key) {
			if (topRow < maxRows - 1) {
				topRow++;
				UpdateThumbnail(win);
			}
		}
	} else if (event->what == mouseDown) {
		GlobalToLocal(&event->where);
		code = FindControl(event->where, win, &theControl);
		if (code != 0) {
			if (code == kControlUpButtonPart || code == kControlDownButtonPart || 
				code == kControlIndicatorPart || code == kControlPageDownPart || 
				code == kControlPageUpPart) {
				Do_Thumb_ScrollBar(win,code,theControl,event->where);
			}
		} else {
			Pict = SelectThumbPosition(win, event);
			if (Pict != NULL) {
				saveGlobal_df = global_df;
				global_df = Odf;
				DrawCursor(0);
				SaveUndoState(FALSE);
				move_cursor(Pict->MBeg+1, Pict->orgFName, TRUE, FALSE);
				global_df->LeaveHighliteOn = TRUE;
				DrawCursor(1);
				global_df = saveGlobal_df;
				strcpy(mvRec.rMovieFile, Pict->fName);
				mvRec.MBeg = Pict->MBeg;
				mvRec.MEnd = Pict->MEnd;
				PlayMovie(&mvRec, Odf, FALSE);
			}
		}
	}
	return(-1);
}

int ShowThumbnails(MovieTNInfo *orgPicts, myFInfo *df, long totalPicts) {
	float			aspectRetio, t1, t2;
	TimeScale		ts;
	TimeValue		pos;
	WindowPtr		windPtr;
	MovieInfo		*thisMovie;
	movInfo			mvRec;
	MovieTNInfo		*Picts;
	FNType			oldfName[FNSize];
	long			cPict;

	*oldfName = EOS;
	thisMovie = NULL;
	aspectRetio = 0.0;
	if (!isMovieAvialable) {
		do_warning("QuickTime failure!", 0);
		freeThumbnails(orgPicts);
		return(1);
	}
	sprintf(templineC, "Loading %ld movie clips...", totalPicts);
	if (!OpenProgressDialog(templineC)) {
		freeThumbnails(orgPicts);
		return(1);
	}			
	cPict = 0;
	for (Picts=orgPicts; Picts != NULL; Picts=Picts->nextPict) {
		if (strcmp(oldfName, Picts->fName)) {
			if (thisMovie) {
				if (thisMovie->MvMovie != NULL)
					DisposeMovie(thisMovie->MvMovie);
				DisposePtr((Ptr)thisMovie);
			}
			strcpy(mvRec.rMovieFile, Picts->fName);
			thisMovie = Movie_Open(&mvRec);
			if (thisMovie == NULL) {
				sprintf(templineC3, "Error opening Movie: %s", Picts->fName);
				do_warning(templineC3, 0);
				freeThumbnails(orgPicts);
				return(1);
			}
			GetMovieBox(thisMovie->MvMovie, &thisMovie->MvBounds);
			t1 = thisMovie->MvBounds.right - thisMovie->MvBounds.left;
			t2 = thisMovie->MvBounds.bottom - thisMovie->MvBounds.top;
			aspectRetio = (float)(t1 / t2);
		}
		strcpy(oldfName, Picts->fName);
		ts = GetMovieTimeScale(thisMovie->MvMovie);
		Picts->aspectRetio = aspectRetio;
		pos = conv_from_msec_mov(Picts->MBeg, ts);
		Picts->pict = GetMoviePict(thisMovie->MvMovie, pos);
		cPict++;
		if (!UpdateProgressDialog((short)((100L*cPict)/totalPicts))) {
			freeThumbnails(orgPicts);
			return(1);
		}			
	}
	CloseProgressDialog();

	if (thisMovie) {
		if (thisMovie->MvMovie != NULL)
			DisposeMovie(thisMovie->MvMovie);
		DisposePtr((Ptr)thisMovie);
	}

	if (OpenWindow(1965, ThumbNails_str, 0L, false, 1, ThumbnailEvent, UpdateThumbnail, CleanupThumbnails)) {
		do_warning("Error opening ThumbNails window", 0);
		freeThumbnails(orgPicts);
		return(1);
	}
	windPtr = FindAWindowNamed(ThumbNails_str);
	if (windPtr == NULL) {
		do_warning("Error opening ThumbNails window", 0);
		freeThumbnails(orgPicts);
		return(1);
	}
	topRow = 0;
	maxRows = 0;
	Odf = df;
	freeThumbnails(moviePics);
	moviePics = orgPicts;
	return(1);
}
