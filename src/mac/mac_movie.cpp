/*
	get movie frames and draw them:
	aPicHandle = GetMoviePict(theMovie, time);
	DrawPicture(myPicture, dstRect);
	
	call this only when very much done
	KillPicture(myPicture);
*/
#include "ced.h"
#include "my_ctype.h"
#include <Palettes.h>
#include "MMedia.h"
#include "mac_MUPControl.h"
#include "mac_dial.h"

#define COUNTLIMIT 7 // how many auto repeats before increament goes from 1 to 50

static void movieHelp(void);

static short rptCnt = 0;
static int  MoviePlayLoopCnt;
static int   oRptPTime;
static char  oRptMark;
short movieActive = 0;
PaletteHandle srcPalette;
static Boolean controlChangingBounds = false;

char MovieReady = false;
MovieInfo *theMovie = NULL;

extern char  rptMark;
extern int	 rptPTime;

short whichActive(WindowPtr win) {
	ControlRef	theControl;
	ControlID	outID;

	movieActive = 0;
	if (GetKeyboardFocus(win, &theControl) == noErr) {
		if (GetControlID(theControl, &outID) == noErr)
			movieActive = outID.id;
	}
	return(movieActive);
}

long conv_to_msec_mov(long num, long ts) {
	double dNum, dTs;
	
	dNum = (double)num;
	dTs = (double)ts;
	dNum /= dTs;
	dNum *= 1000.0000;
	num = (long)dNum;
	return(num);
}

long conv_from_msec_mov(long num, long ts) {
	double dNum, dTs;

	dNum = (double)num;
	dTs = (double)ts;
	dNum /= 1000.0000;
	dNum *= dTs;
	num = (long)dNum;
	return(num);
}

void drawFakeHilight(char isOn, WindowPtr win) {
	myFInfo *saveGlobal_df;

	if (theMovie == NULL)
		return;
	if ((theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
		saveGlobal_df = global_df;
		global_df = theMovie->df;
		if (isOn)
			FakeSelectWholeTier(F5Option == EVERY_LINE);
		DrawFakeHilight(isOn);
		global_df = saveGlobal_df;
	}
}


void stopMoviePlaying(void) {
	if (theMovie != NULL) {
		theMovie->isPlaying = -1;
		MCDoAction(theMovie->MvController, mcActionPlay, 0);
		PBC.isPC = 0;
		PBC.walk = 0;
	}
}

void replayMovieWithOffset(int offset) {
	long		t;
	TimeScale	ts;
	TimeValue	tc;

	if (theMovie == NULL)
		return;

	if (offset < 0) {
		tc = MCGetCurrentTime(theMovie->MvController, &ts);
		t = conv_to_msec_mov(tc, ts) - PBC.step_length;
		if (t < 0L)
			t = 0L;
		global_df->MvTr.MBeg = t;
		theMovie->MBeg = conv_from_msec_mov(global_df->MvTr.MBeg, ts);
		if (theMovie->MBeg >= theMovie->MDur) {
			theMovie->isPlaying = -1;
			MCDoAction(theMovie->MvController, mcActionPlay, 0);
			PBC.isPC = 0;
			PBC.walk = 0;
		} else {
			global_df->MvTr.MEnd = global_df->MvTr.MBeg + PBC.step_length;
			sprintf(templineC3, "%ld", global_df->MvTr.MBeg);
			SetWindowMLTEText(templineC3, theMovie->win, 3);
			SetWindowMLTEText(templineC3, theMovie->win, 5);
			sprintf(templineC3, "%ld", global_df->MvTr.MEnd);
			SetWindowMLTEText(templineC3, theMovie->win, 4);
			theMovie->isPlaying = TRUE;
			MCDoAction(theMovie->MvController, mcActionPlay, (void *)-2);
			theMovie->toOrgWind = FALSE;
		}
	} else if (offset > 0) {
		tc = MCGetCurrentTime(theMovie->MvController, &ts);
		t = conv_to_msec_mov(tc, ts) + PBC.step_length;
		if (t < 0L)
			t = 0L;
		global_df->MvTr.MBeg = t;
		theMovie->MBeg = conv_from_msec_mov(global_df->MvTr.MBeg, ts);
		if (theMovie->MBeg >= theMovie->MDur) {
			theMovie->isPlaying = -1;
			MCDoAction(theMovie->MvController, mcActionPlay, 0);
			PBC.isPC = 0;
			PBC.walk = 0;
		} else {
			global_df->MvTr.MEnd = global_df->MvTr.MBeg + PBC.step_length;
			sprintf(templineC3, "%ld", global_df->MvTr.MBeg);
			SetWindowMLTEText(templineC3, theMovie->win, 3);
			SetWindowMLTEText(templineC3, theMovie->win, 5);
			sprintf(templineC3, "%ld", global_df->MvTr.MEnd);
			SetWindowMLTEText(templineC3, theMovie->win, 4);
			theMovie->isPlaying = TRUE;
			MCDoAction(theMovie->MvController, mcActionPlay, (void *)-2);
			theMovie->toOrgWind = FALSE;
		}
	}
}

static void Movie_Close(MovieInfo *Movie) {
	MCSetActionFilterWithRefCon(Movie->MvController, NULL, (long)NULL);
	if (Movie->MvController != NULL)
		DisposeMovieController(Movie->MvController);
	if (Movie->MvMovie != NULL)
		DisposeMovie(Movie->MvMovie);
	DisposePtr((Ptr)Movie);
}

static void CleanupMovie(WindowPtr wind) {
	ControlRef		iCtrl;

	if (theMovie != NULL) {
		if (theMovie->isPlaying)
			StopMovie(theMovie->MvMovie);
		theMovie->isPlaying = FALSE;
		Movie_Close(theMovie);
		theMovie = NULL;
	}
	MovieReady = false;
	movieActive = 0;
	rptCnt = 0;
	iCtrl = GetWindowItemAsControl(wind, 3);
	mUPCloseControl(iCtrl);
	iCtrl = GetWindowItemAsControl(wind, 4);
	mUPCloseControl(iCtrl);
	iCtrl = GetWindowItemAsControl(wind, 5);
	mUPCloseControl(iCtrl);
	iCtrl = GetWindowItemAsControl(wind, 8);
	mUPCloseControl(iCtrl);
}

static void UpdateMovieWindow(WindowPtr win) {
    GrafPtr			oldPort;
	Rect			rect;
//	short startSelect, endSelect;

	if (theMovie != NULL && !theMovie->isPlaying) {
		if (win != theMovie->win && theMovie->win != NULL)
			win = theMovie->win;

		GetPort(&oldPort);
  	  	SetPortWindowPort(win);
		GetWindowPortBounds(win, &rect);
		EraseRect(&rect);
//		InvalWindowRect(win, &rect);

		MCDraw(theMovie->MvController, win);
		DrawGrowIcon(win);
		DrawControls(win);
		UpdateMovie(theMovie->MvMovie);
		MoviesTask(theMovie->MvMovie, 0);
		SetPort(oldPort);
	}
}

static void UpdateMovieWindowBeforePlayback(WindowPtr win) {
    GrafPtr			oldPort;
//	Rect			rect;
//	short startSelect, endSelect;

	if (theMovie != NULL) {
		if (win != theMovie->win && theMovie->win != NULL)
			win = theMovie->win;

		GetPort(&oldPort);
  	  	SetPortWindowPort(win);
//		GetWindowPortBounds(win, &rect);
//		InvalWindowRect(win, &rect);

		MCDraw(theMovie->MvController, win);
		DrawGrowIcon(win);
		DrawControls(win);
		SetPort(oldPort);
	}
}

void setMovieCursorToValue(long tTime, myFInfo *df) {
	TimeScale	ts;
	TimeValue	tc;
	WindowPtr	win;

	win = FindAWindowID(500);
	if (win == NULL || theMovie == NULL)
		return;
	if (theMovie->df != df)
		return;
	
	MCGetCurrentTime(theMovie->MvController, &ts);
	tc = GetMovieDuration(theMovie->MvMovie);
	if (tTime > tc)
		tTime = tc;
	SetMovieTimeValue(theMovie->MvMovie, tTime);
	MoviesTask(theMovie->MvMovie, 0);
	sprintf(templineC3, "%ld", tTime);
	SetWindowMLTEText(templineC3, win, 5);
}

void setMovieCursor(WindowPtr win) {
	long			tTime;
	TimeScale		ts;
	TimeValue		tc;

	if (win == NULL || theMovie == NULL)
		return;
	
	MCGetCurrentTime(theMovie->MvController, &ts);
	GetWindowMLTEText(win, 5, UTTLINELEN, templineC3);
	tTime = conv_from_msec_mov(atol(templineC3), ts);
	tc = GetMovieDuration(theMovie->MvMovie);
	if (tTime > tc)
		tTime = tc;
	SetMovieTimeValue(theMovie->MvMovie, tTime);
	MoviesTask(theMovie->MvMovie, 0);
}

static void moveMovieCursor(WindowPtr win, EventModifiers modifiers, MovieInfo *cMovie, short ModalHit, short direct) {
	long			tTime, bTime, aTime;
	long			inc;
	TimeScale		ts;
	TimeValue		tc;
	ControlRef		iCtrl;
	ControlEditTextSelectionRec selection;

	GetKeyboardFocus(win, &iCtrl);
	GetMLTETextSelection(iCtrl, &selection);
	saveMovieUndo(win, selection.selStart, selection.selEnd, 1);
	if (modifiers & cmdKey)
		rptCnt = COUNTLIMIT;

	if (rptCnt >= COUNTLIMIT) {
		if (direct == -1)
			inc = -300L;
		else
			inc = 300L;
	} else {
		if (direct == -1)
			inc = -1L;
		else
			inc = 1L;
	}
	if (ModalHit == 5) {
		tTime = MCGetCurrentTime(cMovie->MvController, &ts);
		bTime = conv_to_msec_mov(tTime, ts);
		if (PBC.enable && (cMovie->df=WindowFromGlobal_df(cMovie->df)) != NULL) {
			cMovie->df->MvTr.MBeg = bTime;
			PBC.cur_pos = bTime;
		}
	} else {
		MCGetCurrentTime(cMovie->MvController, &ts);
		GetWindowMLTEText(win, ModalHit, UTTLINELEN, templineC3);
		bTime = atol(templineC3);
		tTime = conv_from_msec_mov(bTime, ts);
	}
	tc = GetMovieDuration(cMovie->MvMovie);
	do {
		tTime += inc;
		if (tTime < 0)
			tTime = 0;
		if (movieActive == 8)
			aTime = tTime;
		else
			aTime = conv_to_msec_mov(tTime, ts);
	} while (bTime == aTime && tTime > 0 && tTime < tc) ;
	if (tTime >= tc)
		aTime = conv_to_msec_mov(tc, ts);

	SetMovieTimeValue(cMovie->MvMovie, tTime);
	MoviesTask(cMovie->MvMovie, 0);
	sprintf(templineC3, "%ld", aTime);
	SetWindowMLTEText(templineC3, win, ModalHit);
	if (ModalHit != 5) {
		SetWindowMLTEText(templineC3, win, 5);
	}
}

static int proccessHits(WindowPtr win, MovieInfo *cMovie, short ModalHit, EventRecord *event, short var) {
	TimeScale		ts;
	TimeValue 		CurFP;
	long			timeBeg, timeEnd;
	myFInfo 		*saveGlobal_df;
	ControlRef		theControl;
	ControlEditTextSelectionRec selection;

	if (ModalHit == -1 || ModalHit == -2) {
		moveMovieCursor(win, event->modifiers, cMovie, (short)5, ModalHit);
		if (movieActive == 5)
			SelectWindowMLTEText(win, 5, 0, HUGE_INTEGER);
		return(1);
	} else if (ModalHit == -4) {
		GetWindowMLTEText(win, movieActive, UTTLINELEN, templineC3);
		SetWindowMLTEText(templineC3, win, 5);
		ts = GetMovieTimeScale(cMovie->MvMovie);
		timeBeg = atol(templineC3);
		SetMovieTimeValue(cMovie->MvMovie, conv_from_msec_mov(timeBeg, ts));
		MoviesTask(cMovie->MvMovie, 0);
		SetWaveTimeValue(WindowProcs(win), timeBeg, 0L);
		return(1);
	} else if (ModalHit == -5) {
		if (cMovie->isPlaying) {
			cMovie->isPlaying = -1;
			MCDoAction(cMovie->MvController, mcActionPlay, (void *)0);
		} else
			MCDoAction(cMovie->MvController, mcActionPlay, (void *)0x00ff);
		return(1);
	} else if (ModalHit == 1) {
		if (PBC.enable && PBC.isPC && PBC.walk) {
			if (cMovie->isPlaying) {
				cMovie->isPlaying = -1;
				MCDoAction(cMovie->MvController, mcActionPlay, (void *)0);
			}
			do_warning("To repeat use F7 or F8 command. This buttons stops playback in Walker Controller.", 0);
		} else if (cMovie->isPlaying) {
			cMovie->isPlaying = -1;
			MCDoAction(cMovie->MvController, mcActionPlay, (void *)0);
		} else if (event->modifiers & optionKey)
			MCDoAction(cMovie->MvController, mcActionPlay, (void *)-2);
		else
			MCDoAction(cMovie->MvController, mcActionPlay, (void *)-1);
		return(1);
	} else if (ModalHit == 5) {
		int res = 0;

		theControl = GetWindowItemAsControl(win, 7);
		SetControlTitle(theControl, "\p^"); 
		theControl = GetWindowItemAsControl(win, 2);
		SetControlTitle(theControl, "\p^");
		UpdateMovieWindowBeforePlayback(win);
		if (var != 0) {
			theControl = GetWindowItemAsControl(win, 5);
			GetWindowMLTEText(win, 5, UTTLINELEN, templineC3);
			if (AgmentCurrentMLTEText(theControl, event, templineC3, var)) {
				TimeValue tTime;

				ts = GetMovieTimeScale(cMovie->MvMovie);
				CurFP = GetMovieDuration(cMovie->MvMovie);
				timeBeg = atol(templineC3);
				tTime = conv_from_msec_mov(timeBeg, ts);
				if (tTime > CurFP) {
					tTime = conv_to_msec_mov(CurFP, ts);
					sprintf(templineC3, "%ld", tTime);
					SetWindowMLTEText(templineC3, win, 5);
					res = 1;
					tTime = CurFP;
				}
				SetMovieTimeValue(cMovie->MvMovie, tTime);
				MoviesTask(cMovie->MvMovie, 0);
				if (PBC.enable && (cMovie->df=WindowFromGlobal_df(cMovie->df)) != NULL) {
					tTime = conv_to_msec_mov(tTime, ts);
					cMovie->df->MvTr.MBeg = tTime;
					PBC.cur_pos = tTime;
				}
				SetWaveTimeValue(WindowProcs(win), timeBeg, 0L);
			}
		}
		return(res);
	} else if (ModalHit == 3 || ModalHit == 4) {
		theControl = GetWindowItemAsControl(win, 7);
		SetControlTitle(theControl, "\pv"); 
		theControl = GetWindowItemAsControl(win, 2);
		SetControlTitle(theControl, "\pv"); 
		UpdateMovieWindowBeforePlayback(win);
		if (var != 0) {
			if (ModalHit == 3 && movieActive == 3) {
				theControl = GetWindowItemAsControl(win, 3);
				GetWindowMLTEText(win, 3, UTTLINELEN, templineC3);
				if (AgmentCurrentMLTEText(theControl, event, templineC3, var)) {
					timeBeg = atol(templineC3);
					GetWindowMLTEText(win, 4, UTTLINELEN, templineC3);
					timeEnd = atol(templineC3);
					SetWaveTimeValue(WindowProcs(win), timeBeg, timeEnd);
				}
			} else {
				theControl = GetWindowItemAsControl(win, 4);
				GetWindowMLTEText(win, 4, UTTLINELEN, templineC3);
				if (AgmentCurrentMLTEText(theControl, event, templineC3, var)) {
					timeEnd = atol(templineC3);
					GetWindowMLTEText(win, 3, UTTLINELEN, templineC3);
					timeBeg = atol(templineC3);
					SetWaveTimeValue(WindowProcs(win), timeBeg, timeEnd);
				}
			}
		}
		return(0);
	} else if ((ModalHit == 2 || ModalHit == 7) && movieActive == 5) {
		GetWindowMLTEText(win, ((ModalHit == 7) ? 3 : 4), UTTLINELEN, templineC3);
		SetWindowMLTEText(templineC3, win, movieActive);
		SelectWindowMLTEText(win, movieActive, 0, HUGE_INTEGER);
		UpdateMovieWindowBeforePlayback(win);
		return(1);
	} else if (((movieActive == 3 || movieActive == 4) && ModalHit == 7) || (movieActive == 3 && ModalHit == -3)) {
		GetKeyboardFocus(win, &theControl);
		GetMLTETextSelection(theControl, &selection);
		saveMovieUndo(win, selection.selStart, selection.selEnd, 1);
		GetWindowMLTEText(win, 5, UTTLINELEN, templineC3);
		SetWindowMLTEText(templineC3, win, 3);
		if (movieActive == 3)
			SelectWindowMLTEText(win, 3, 0, HUGE_INTEGER);
		CurFP = atol(templineC3);
		ts = GetMovieTimeScale(cMovie->MvMovie);
		cMovie->MBeg = conv_from_msec_mov(CurFP, ts);
		if ((cMovie->df=WindowFromGlobal_df(cMovie->df)) != NULL)
			cMovie->df->MvTr.MBeg = CurFP;
		return(1);
	} else if (((movieActive == 3 || movieActive == 4) && ModalHit == 2) || (movieActive == 4 && ModalHit == -3)) {
		GetKeyboardFocus(win, &theControl);
		GetMLTETextSelection(theControl, &selection);
		saveMovieUndo(win, selection.selStart, selection.selEnd, 1);
		GetWindowMLTEText(win, 5, UTTLINELEN, templineC3);
		SetWindowMLTEText(templineC3, win, 4);
		if (movieActive == 4)
			SelectWindowMLTEText(win, 4, 0, HUGE_INTEGER);
		CurFP = atol(templineC3);
		ts = GetMovieTimeScale(cMovie->MvMovie);
		cMovie->MEnd = conv_from_msec_mov(CurFP, ts);
		return(1);
	} else if (ModalHit == 6) {
		if ((cMovie->df=WindowFromGlobal_df(cMovie->df)) != NULL) {
			saveGlobal_df = global_df;
			global_df = cMovie->df;
			DrawFakeHilight(0);
			GetWindowMLTEText(win, 3, UTTLINELEN, templineC3);
			cMovie->MBeg = atol(templineC3);
			GetWindowMLTEText(win, 4, UTTLINELEN, templineC3);
			cMovie->MEnd = atol(templineC3);
			addBulletsToText(SOUNDTIER, cMovie->fName, cMovie->MBeg, cMovie->MEnd);
			FakeSelectWholeTier(F5Option == EVERY_LINE);
			DrawFakeHilight(1);
			global_df = saveGlobal_df;
		}
		return(1);
	} else if (ModalHit == 10) {
		movieHelp();
		return(1);
	} else if (ModalHit == 8) {
		theControl = GetWindowItemAsControl(win, 7);
		SetControlTitle(theControl, "\p-"); 
		theControl = GetWindowItemAsControl(win, 2);
		SetControlTitle(theControl, "\p-"); 
		UpdateMovieWindowBeforePlayback(win);
		GetWindowMLTEText(win, 8, UTTLINELEN, templineC3);
		if (var != 0) {
			theControl = GetWindowItemAsControl(win, 8);
			AgmentCurrentMLTEText(theControl, event, templineC3, var);
		}

		if (*templineC3=='-' || *templineC3=='+' || (char)toupper((unsigned char)*templineC3)=='B' || (char)toupper((unsigned char)*templineC3)=='E') {
			rptMark = *templineC3;
			rptPTime = atoi(templineC3);
		} else {
			rptMark = 0;
			rptPTime = atoi(templineC3);
		}
		if (oRptPTime != rptPTime || oRptMark != rptMark) {
			oRptPTime = rptPTime;
			oRptMark = rptMark;
			WriteCedPreference();
		}
		return(0);
	} else if (ModalHit == 7 || ModalHit == 2) {
		UpdateMovieWindowBeforePlayback(win);
		return(1);
	}
	return(0);
}

static int comKeys(WindowPtr win, EventRecord *event, short var) {
	char			res;
	short			type;
	short			ModalHit;
	TimeScale		ts;
	TimeValue 		CurFP;
	ControlID		outID;
	ControlRef		theControl;
	myFInfo 		*saveGlobal_df;
	ControlEditTextSelectionRec selection;

	type = event->message & keyCodeMask;
	if ((event->modifiers & cmdKey && !IsArrowKeyCode(type/256)) || type == 0x6100 || type == 0x6200 || type == 0x6400 || type == 0x6500) {
		return(0);
	}
	if (theMovie == NULL)
		return(1);

	theControl = GetWindowItemAsControl(win, 7);
	if (event->what == keyDown)
		rptCnt = 0;
	else if (event->what == autoKey && rptCnt < COUNTLIMIT)
		rptCnt++;
	if (PlayingContMovie == '\003') {
		if (theMovie != NULL && (theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
			if (type == 0x6000 && F5_Offset == 0L) {
				theMovie->isPlaying = -1;
				MCDoAction(theMovie->MvController, mcActionPlay, 0);
				return(1);
			}
			if (type == 0x6000 || type == 0x7A00 || type == 0x7800) {
				saveGlobal_df = global_df;
				global_df = theMovie->df;
				DrawFakeHilight(0);
				CurFP = MCGetCurrentTime(theMovie->MvController, &ts);
				CurFP = conv_to_msec_mov(CurFP, ts);
				if (global_df->MvTr.MBeg != CurFP && CurFP != 0L) {
					global_df->row_win2 = 0L;
					global_df->col_win2 = -2L;
					global_df->col_chr2 = -2L;
					if ((res=findStartMediaTag(TRUE, F5Option == EVERY_LINE)) != TRUE)
						findEndOfSpeakerTier(res, F5Option == EVERY_LINE);
					SaveUndoState(FALSE);
					addBulletsToText(SOUNDTIER, theMovie->fName, global_df->MvTr.MBeg, CurFP);
					global_df->MvTr.MBeg = CurFP;
				}
				selectNextSpeaker();
				wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
				wrefresh(global_df->w1);
				DrawFakeHilight(1);
				global_df = saveGlobal_df;
				theMovie->isPlaying = -1;
				MCDoAction(theMovie->MvController, mcActionPlay, 0);
			}
			type = event->message & charCodeMask;
			if (type == 'i' || type == 'I' || type == ' ') {
				saveGlobal_df = global_df;
				global_df = theMovie->df;
				DrawFakeHilight(0);
				CurFP = MCGetCurrentTime(theMovie->MvController, &ts);
				CurFP = conv_to_msec_mov(CurFP, ts);
				if (global_df->MvTr.MBeg != CurFP && CurFP != 0L) {
					global_df->row_win2 = 0L;
					global_df->col_win2 = -2L;
					global_df->col_chr2 = -2L;
					if ((res=findStartMediaTag(TRUE, F5Option == EVERY_LINE)) != TRUE)
						findEndOfSpeakerTier(res, F5Option == EVERY_LINE);
					SaveUndoState(FALSE);
					addBulletsToText(SOUNDTIER, theMovie->fName, global_df->MvTr.MBeg, CurFP);
					global_df->MvTr.MBeg = CurFP;
				}
				selectNextSpeaker();
				wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
				wrefresh(global_df->w1);
				DrawFakeHilight(1);
				global_df = saveGlobal_df;
			}
		}
		return(1);
	} else {
		if (theMovie != NULL) {
			if (theMovie->isPlaying) {
				theMovie->isPlaying = -1;
				MCDoAction(theMovie->MvController, mcActionPlay, 0);
			}
		}
		type = (event->message & keyCodeMask)/256;
		if ((type == right_key || type == left_key) && 
			   ((event->modifiers & controlKey) || (event->modifiers & cmdKey)) && 
			   (theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
			if (type == right_key && event->modifiers & controlKey) { //lw-right
				moveMovieCursor(win, 0, theMovie, (short)3, (short)-2);
			} else if (type == left_key && event->modifiers & controlKey) { //lw-left
				moveMovieCursor(win, 0, theMovie, (short)3, (short)-1);
			} else if (type == right_key && event->modifiers & cmdKey) { //rw-right
				moveMovieCursor(win, 0, theMovie, (short)4, (short)-2);
			} else if (type == left_key && event->modifiers & cmdKey) { //rw-left
				moveMovieCursor(win, 0, theMovie, (short)4, (short)-1);
			}
			SetCurrSoundTier(WindowProcs(win));
			return(1);
		}
	}

	ModalHit = 0;
// var = event->message & charCodeMask;
	type = (event->message & keyCodeMask)/256;
	if (var == '\t') {
		GetKeyboardFocus(win, &theControl);
		if (GetControlID(theControl, &outID) == noErr)
			ModalHit = outID.id;
	} else if (var == ' ') {
		ModalHit = -5;
	} else if (var == RETURN_CHAR || var == ENTER_CHAR) {
		ModalHit = 1;
	} else if ((movieActive == 3 || movieActive == 4 || movieActive == 5) && (event->modifiers & optionKey) && IsArrowKeyCode(type)) {
		if (type==left_key)
			ModalHit = -1;
		else if (type==right_key)
			ModalHit = -2;
		else if (type==down_key)
			ModalHit = -3;
		else if (type==up_key)
			ModalHit = -4;
	} else if ((movieActive == 3 || movieActive == 4 || movieActive == 5 || movieActive == 8) && IsArrowKeyCode(type)) {
		return(0);
	} else if ((var < '0' || var > '9') && var != '+' && var != '-' && 
						(char)toupper((unsigned char)var) != 'B' && (char)toupper((unsigned char)var) != 'E'  && 
						var != DELETE_CHAR && var != 8) {
		return(1);
	} else {
		GetKeyboardFocus(win, &theControl);
		if (GetControlID(theControl, &outID) == noErr)
			ModalHit = outID.id;
		GetMLTETextSelection(theControl, &selection);
		saveMovieUndo(win, selection.selStart, selection.selEnd, 1);
	}

	if (ModalHit != 1 && ModalHit != -5) {
		if (theMovie != NULL) {
			if (theMovie->isPlaying) {
				theMovie->isPlaying = -1;
				MCDoAction(theMovie->MvController, mcActionPlay, 0);
			}
		}
	}

	return(proccessHits(win, theMovie, ModalHit, event, var));
}

static short MovieEvent(WindowPtr win, EventRecord *event) {
	char			res;
	short			type;
	short			ModalHit;
	TimeScale		ts;
	TimeValue 		CurFP;
	myFInfo 		*saveGlobal_df;
	ControlRef		theControl;

	if (event->modifiers & cmdKey && event->what == mouseDown) {
		if (theMovie->MEnd < theMovie->MBeg) {
			do_warning("BEG mark must be smaller than END mark", 0);
			return(-1);
		}
		MCDoAction(theMovie->MvController, mcActionPlay, (void *)-1);
		ModalHit = -1;
	} else {
		ModalHit = 0;
		if (event->what == keyDown || event->what == autoKey) {
			theControl = GetWindowItemAsControl(win, 7);
			if (event->what == keyDown)
				rptCnt = 0;
			else if (event->what == autoKey && rptCnt < COUNTLIMIT)
				rptCnt++;
			if (PlayingContMovie == '\003') {
				type = event->message & charCodeMask;
				if ((theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
					if (type == 'i' || type == 'I' || type == ' ') {
						saveGlobal_df = global_df;
						global_df = theMovie->df;
						DrawFakeHilight(0);
						CurFP = MCGetCurrentTime(theMovie->MvController, &ts);
						CurFP = conv_to_msec_mov(CurFP, ts);
						if (global_df->MvTr.MBeg != CurFP && CurFP != 0L) {
							global_df->row_win2 = 0L;
							global_df->col_win2 = -2L;
							global_df->col_chr2 = -2L;
							if ((res=findStartMediaTag(TRUE, F5Option == EVERY_LINE)) != TRUE)
								findEndOfSpeakerTier(res, F5Option == EVERY_LINE);
							SaveUndoState(FALSE);
							addBulletsToText(SOUNDTIER, theMovie->fName, global_df->MvTr.MBeg, CurFP);
							global_df->MvTr.MBeg = CurFP;
						}
						selectNextSpeaker();
						wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
						wrefresh(global_df->w1);
						DrawFakeHilight(1);
						global_df = saveGlobal_df;
					}
				}
				return(-1);
			} else {
				type = event->message & keyCodeMask;
				if ((type == 0x6100 || type == 0x6200 || type == 0x6400 || type == 0x6500) && (theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
					theMovie->toOrgWind = -1;
					changeCurrentWindow(win, theMovie->df->wind, true);
					if (type == 0x6100) {
						F_key = 6;
						return(1);
					}
					if (type == 0x6200) {
						F_key = 7;
						return(1);
					}
					if (type == 0x6400) {
						F_key = 8;
						return(1);
					}
					if (type == 0x6500) {
						F_key = 9;
						return(1);
					}
				} else if ((type == 0x7C00 || type == 0x7B00) && 
						   ((event->modifiers & controlKey) || (event->modifiers & cmdKey)) && 
						   (theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
					if (type == 0x7C00 && event->modifiers & controlKey) { //lw-right
						moveMovieCursor(win, 0, theMovie, (short)3, (short)-2);
					} else if (type == 0x7B00 && event->modifiers & controlKey) { //lw-left
						moveMovieCursor(win, 0, theMovie, (short)3, (short)-1);
					} else if (type == 0x7C00 && event->modifiers & cmdKey) { //rw-right
						moveMovieCursor(win, 0, theMovie, (short)4, (short)-2);
					} else if (type == 0x7B00 && event->modifiers & cmdKey) { //rw-left
						moveMovieCursor(win, 0, theMovie, (short)4, (short)-1);
					}
					SetCurrSoundTier(WindowProcs(win));
					return(-1);
				}
			}
			if (ModalHit != 1 && ModalHit != -5) {
				if (theMovie != NULL) {
					if (theMovie->isPlaying) {
						theMovie->isPlaying = -1;
						MCDoAction(theMovie->MvController, mcActionPlay, 0);
					}
				}
			}
		}

		if (event->what == mouseDown) {
			ControlPartCode	code;
			ControlID outID;

			GlobalToLocal(&event->where);
			theControl = FindControlUnderMouse(event->where,win,&code); //code = FindControl(event->where, win, &theControl);
			if (theControl != NULL) {
				code = HandleControlClick(theControl,event->where,event->modifiers,NULL); 
				if (GetControlID(theControl, &outID) == noErr)
					ModalHit = outID.id;
				if (code == 0 && ModalHit != 3 && ModalHit != 4 && ModalHit != 5 && ModalHit != 8)
					ModalHit = 0;
			}
			LocalToGlobal(&event->where);
		}

	}

	proccessHits(win, theMovie, ModalHit, event, 0);
	return -1;
}

static char isStreamingMovieFile(FNType *rMovieFile, char *URL) {
	int c, i, j;
	FILE* fp;

	URL[0] = EOS;
	fp = fopen(rMovieFile, "rb");
	if (fp == NULL)
		return(FALSE);
	for (i=0; i < 44 && !feof(fp); i++)
		c = getc(fp);
	if (feof(fp)) {
		fclose(fp);
		return(FALSE);
	}
	j = 0;
	for (i=0; i < 7 && !feof(fp); i++)
		URL[j++] = (char)getc(fp);
	if (strncmp(URL, "rtsp://", 7)) {
		URL[0] = EOS;
		fclose(fp);
		return(FALSE);
	}
	while (!feof(fp)) {
		URL[j] = (char)getc(fp);
		if (!strncmp(URL+j-3, ".mov", 4)) {
			URL[j+1] = EOS;
			if (streamSpeedNumber == 1 && !strncmp(URL+j-8, "56k_S.mov", 9)) {
				fclose(fp);
				return(TRUE);
			} else if (streamSpeedNumber == 2 && !strncmp(URL+j-9, "256k_S.mov", 10)) {
				fclose(fp);
				return(TRUE);
			} else if (streamSpeedNumber == 3 && !strncmp(URL+j-9, "512k_S.mov", 10)) {
				fclose(fp);
				return(TRUE);
			} else if (streamSpeedNumber == 4 && !strncmp(URL+j-7, "T1_S.mov", 8)) {
				fclose(fp);
				return(TRUE);
			} else {
				j = 0;
			}
		} else if (j == 0) {
			if (URL[j] == 'r') {
				for (i=0; i < 6 && !feof(fp); i++)
					URL[++j] = (char)getc(fp);
				if (strncmp(URL, "rtsp://", 7))
					j = 0;
				
			}
		}
		if (j > 0 && (URL[j] < ' ' || URL[j] > '~')) {
			URL[0] = EOS;
			fclose(fp);
			return(FALSE);
		}
		if (j > 0)
			j++;
	}
	URL[0] = EOS;
	fclose(fp);
	return(FALSE);
}

/* OPEN AND LOAD MOVIE */
MovieInfo *Movie_Open(movInfo *mvRec) {
	char		URL[FNSize];
	wchar_t		wrMovieFile[FNSize];
	OSErr		err;
	OSType		DRType;
	Handle		DR = NULL;
	short		RID = 0;
	CFStringRef	theStr = NULL;
	Handle		outDataRef;
	OSType		outDataRefType;
	MovieInfo	*Movie;

	Movie = (MovieInfo *)NewPtr(sizeof(MovieInfo));
	if( Movie == NULL )
		return NULL;
	if (streamSpeedNumber == 0 || !isStreamingMovieFile(mvRec->rMovieFile, URL)) {
		u_strcpy(wrMovieFile, mvRec->rMovieFile, FNSize);
		// create the data reference
		theStr = CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar *)wrMovieFile, strlen(wrMovieFile));
		err = QTNewDataReferenceFromFullPathCFString(theStr, kQTNativeDefaultPathStyle, 0, &DR, &DRType);
		CFRelease(theStr);
		if (err != noErr) {
			DisposePtr((Ptr)Movie);
			return NULL;
		} 
		// get the Movie
		err = NewMovieFromDataRef(&Movie->MvMovie, newMovieActive, &RID, DR, DRType);
		if (err != noErr) {
			DisposePtr((Ptr)Movie);
			return NULL;
		} 
		// dispose the data reference handle - we no longer need it
		DisposeHandle(DR);
	} else {
		theStr = my_CFStringCreateWithBytes(URL);
		if (theStr == NULL) {
			DisposePtr((Ptr)Movie);
			return NULL;
		}
		err = QTNewDataReferenceFromURLCFString(theStr, 0, &outDataRef, &outDataRefType);
		if (err != noErr) {
			CFRelease(theStr);
			DisposePtr((Ptr)Movie);
			return NULL;
		}
		err = NewMovieFromDataRef(&Movie->MvMovie, newMovieActive, NULL, outDataRef, outDataRefType);
		if (err != noErr) {
			CFRelease(theStr);
			if (outDataRef != NULL)
				DisposeHandle((Handle) outDataRef);
			DisposePtr((Ptr)Movie);
			return NULL;
		}
		CFRelease(theStr);
		if (outDataRef != NULL)
			DisposeHandle((Handle) outDataRef);
	}	
	if (err != noErr) {
		DisposePtr((Ptr)Movie);
		return NULL;
	}
	Movie->isPlaying = FALSE;
	return Movie;
}

static void resizeMovieWindowItems(WindowPtr MvWin, MovieInfo *Movie) {
	int		offset, tOff, hight, hOff, rightOffset;
	Rect	ibox;

	GetControlBounds(GetWindowItemAsControl(MvWin, 5), &ibox);
	offset = (ibox.bottom - ibox.top) + 7;
	GetControlBounds(GetWindowItemAsControl(MvWin, 7), &ibox);
	offset += (ibox.bottom - ibox.top) + 11;
	GetControlBounds(GetWindowItemAsControl(MvWin, 3), &ibox);
	offset += (ibox.bottom - ibox.top) + 9;
	GetControlBounds(GetWindowItemAsControl(MvWin, 1), &ibox);
	offset += (ibox.bottom - ibox.top) + 5;
	GetControlBounds(GetWindowItemAsControl(MvWin, 11), &ibox);
	offset += (ibox.bottom - ibox.top) + 5;
	
	SizeWindow(MvWin, Movie->MvBounds.right, Movie->MvBounds.bottom+offset,true);
	ibox = Movie->MvBounds;
	ibox.bottom = ibox.bottom + offset;
	EraseRect(&ibox);
	
	rightOffset = 36;
	offset = Movie->MvBounds.bottom + 5;
	GetControlBounds(GetWindowItemAsControl(MvWin, 5), &ibox);
	ibox.bottom = offset + (ibox.bottom - ibox.top);
	ibox.top = offset;
	ibox.right = rightOffset + (ibox.right - ibox.left);
	ibox.left = rightOffset;
	Movie->posBox = ibox;
	//	MoveControl(GetWindowItemAsControl(MvWin, 5), rightOffset, offset);
	SetControlBounds(GetWindowItemAsControl(MvWin, 5), &ibox);
	offset += (ibox.bottom - ibox.top) + 7;
	
	GetControlBounds(GetWindowItemAsControl(MvWin, 7), &ibox);
	rightOffset = 29;
	MoveControl(GetWindowItemAsControl(MvWin, 7), rightOffset, offset);
	Movie->Beg = ibox;
	Movie->Beg.bottom = offset + (ibox.bottom - ibox.top);
	Movie->Beg.top = offset;
	Movie->Beg.right = rightOffset + (ibox.right - ibox.left);
	Movie->Beg.left = rightOffset;
	rightOffset += (ibox.right - ibox.left) + 12;
	
	GetControlBounds(GetWindowItemAsControl(MvWin, 2), &ibox);
	MoveControl(GetWindowItemAsControl(MvWin, 2), rightOffset, offset);
	Movie->End = ibox;
	Movie->End.bottom = offset + (ibox.bottom - ibox.top);
	Movie->End.top = offset;
	Movie->End.right = rightOffset + (ibox.right - ibox.left);
	Movie->End.left = rightOffset;
	offset += (ibox.bottom - ibox.top) + 11;
	
	GetControlBounds(GetWindowItemAsControl(MvWin, 3), &ibox);
	ibox.bottom = offset + (ibox.bottom - ibox.top);
	ibox.top = offset;
	ibox.right = 4 + (ibox.right - ibox.left);
	ibox.left = 4;
	Movie->begBox = ibox;
	//	MoveControl(GetWindowItemAsControl(MvWin, 3), 4, offset);
	SetControlBounds(GetWindowItemAsControl(MvWin, 3), &ibox);
	rightOffset = ibox.right + ibox.left + 3;
	
	GetControlBounds(GetWindowItemAsControl(MvWin, 4), &ibox);
	ibox.bottom = offset + (ibox.bottom - ibox.top);
	ibox.top = offset;
	ibox.right = rightOffset + (ibox.right - ibox.left);
	ibox.left = rightOffset;
	Movie->endBox = ibox;
	//	MoveControl(GetWindowItemAsControl(MvWin, 4), rightOffset, offset);
	SetControlBounds(GetWindowItemAsControl(MvWin, 4), &ibox);
	offset += (ibox.bottom - ibox.top) + 9;
	
	GetControlBounds(GetWindowItemAsControl(MvWin, 1), &ibox);
	MoveControl(GetWindowItemAsControl(MvWin, 1), 0, offset);
	rightOffset = (ibox.right - ibox.left) + 9;
	Movie->Repeat = ibox;
	Movie->Repeat.bottom = offset + (ibox.bottom - ibox.top);
	Movie->Repeat.top = offset;
	tOff = (ibox.bottom - ibox.top);
	
	GetControlBounds(GetWindowItemAsControl(MvWin, 8), &ibox);
	hight = ibox.bottom - ibox.top;
	hOff = (tOff - hight) / 2;
	ibox.bottom = offset + hight + hOff;
	ibox.top = offset + hOff;
	ibox.right = rightOffset + (ibox.right - ibox.left);
	ibox.left = rightOffset;
	rightOffset = ibox.right + 3;
	Movie->rptTime = ibox;
	//	MoveControl(GetWindowItemAsControl(MvWin, 8), ibox.left, ibox.top);
	SetControlBounds(GetWindowItemAsControl(MvWin, 8), &ibox);
	
	GetControlBounds(GetWindowItemAsControl(MvWin, 9), &ibox);
	hight = ibox.bottom - ibox.top;
	hOff = (tOff - hight) / 2;
	ibox.bottom = offset + hight + hOff;
	ibox.top = offset + hOff;
	ibox.right = rightOffset + (ibox.right - ibox.left);
	ibox.left = rightOffset;
	//	MoveControl(GetWindowItemAsControl(MvWin, 9), rightOffset, offset + hOff);
	SetControlBounds(GetWindowItemAsControl(MvWin, 9), &ibox);
	offset += tOff + 5;
	
	GetControlBounds(GetWindowItemAsControl(MvWin, 11), &ibox);
	ibox.bottom = offset + (ibox.bottom - ibox.top);
	ibox.top = offset;
	ibox.right = 2 + (ibox.right - ibox.left);
	ibox.left = 2;
	//	MoveControl(GetWindowItemAsControl(MvWin, 11), 2, offset);
	SetControlBounds(GetWindowItemAsControl(MvWin, 11), &ibox);
	offset += (ibox.bottom - ibox.top) + 5;
	
	tOff = Movie->MvBounds.bottom + 3;
	GetControlBounds(GetWindowItemAsControl(MvWin, 6), &ibox);
	MoveControl(GetWindowItemAsControl(MvWin, 6), Movie->MvBounds.right-(ibox.right-ibox.left), tOff);
	Movie->Save = ibox;
	Movie->Save.bottom = tOff + (ibox.bottom - ibox.top);
	Movie->Save.top = tOff;
	Movie->Save.left = Movie->MvBounds.right - (ibox.right - ibox.left);
	Movie->Save.right = Movie->MvBounds.right;
	
	tOff = Movie->Save.bottom + 5;
	GetControlBounds(GetWindowItemAsControl(MvWin, 12), &ibox);
	hight = ibox.bottom - ibox.top;
	ibox.top = tOff;
	ibox.bottom = ibox.top + hight;
	ibox.left =  Movie->MvBounds.right - (ibox.right - ibox.left);
	ibox.right = Movie->MvBounds.right;
	Movie->zoomRatio = ibox;
	//	MoveControl(GetWindowItemAsControl(MvWin, 12), Movie->MvBounds.right, tOff);
	SetControlBounds(GetWindowItemAsControl(MvWin, 12), &ibox);
	
	GetControlBounds(GetWindowItemAsControl(MvWin, 10), &ibox);
	hight = ibox.bottom - ibox.top;
	hOff = Movie->MvBounds.right - (ibox.right - ibox.left);
	ibox.bottom = offset - hight;
	tOff = ibox.bottom - hight;
	ibox.top = tOff;
	ibox.left =  hOff;
	ibox.right = Movie->MvBounds.right;
	Movie->help = ibox;
	//	MoveControl(GetWindowItemAsControl(MvWin, 10), hOff, tOff);
	SetControlBounds(GetWindowItemAsControl(MvWin, 10), &ibox);
}

static pascal Boolean MCActionFilter(MovieController mc, short action, void *params, long ref) {
	long			rptTime, tLong;
	short			type;
	short			active;
	TimeScale 		ts;
	TimeValue 		tc;
	TimeValue 		tc2;
	TimeValue 		tc3;
	WindowPtr		win;
	extern long		sEF;


	win = FindAWindowNamed(Movie_sound_str);
	if (win != NULL) {


/*
if (action == mcActionShowStatusString) {
	QTStatusStringPtr mess = (QTStatusStringPtr)params;
	long flag = mess->stringTypeFlags;
	if (flag & kStatusStringIsStreamingStatus) {
		char *str = mess->statusString;
		
		ts = 0;
	}
} else
		if (action == mcActionIdle || action == mcActionResume || action == mcActionSuspend ||
			action == mcActionGetVolume || action == mcActionPlay || action == mcActionDeactivate ||
			action == mcActionDraw || action == mcActionUseTrackForTimeTable) {
			tc2 = 5L;
		} else {
			tc2 = 6L;
		}
*/
		if (action == mcActionPlay) {
			if ((Fixed)params != 0) {
				if ((Fixed)params < 0) {
					tc = GetMovieDuration(theMovie->MvMovie);
					tc2 = MCGetCurrentTime(theMovie->MvController, &ts);
					active = whichActive(win);
					tLong = 0L;
					rptTime = 0L;
					if ((Fixed)params != -2L) {
						GetWindowMLTEText(win, 8, UTTLINELEN, templineC3);
						if ((char)toupper((unsigned char)*templineC3) == 'B') {
							active = 3;
							rptTime = atol(templineC3+1);
						} else if ((char)toupper((unsigned char)*templineC3) == 'E') {
							active = 4;
							rptTime = atol(templineC3+1);
						} else if ((char)toupper((unsigned char)*templineC3) == '+') {
							active = 4;
							tLong = atol(templineC3+1);
						} else if ((char)toupper((unsigned char)*templineC3) == '-') {
							active = 3;
							tLong = atol(templineC3+1);
						} else
							rptTime = atol(templineC3);
					}

					GetWindowMLTEText(win, 3, UTTLINELEN, templineC3);
					theMovie->MBeg = atol(templineC3);

					GetWindowMLTEText(win, 4, UTTLINELEN, templineC3);
					theMovie->MEnd = atol(templineC3);

					if (theMovie->MEnd == 0L) {
						if ((Fixed)params == -2L)
							theMovie->MBeg = conv_from_msec_mov(theMovie->MBeg,ts);
						else
							theMovie->MBeg = MCGetCurrentTime(theMovie->MvController,&ts);
						theMovie->MEnd = tc;
					} else if (tLong != 0L) {
						if (active == 3) {
							tLong = theMovie->MBeg - tLong;
							if (tLong < 0L)
								tLong = 0L;
							theMovie->MBeg = conv_from_msec_mov(tLong, ts);
							theMovie->MEnd = conv_from_msec_mov(theMovie->MEnd, ts);
						} else {
							theMovie->MBeg = conv_from_msec_mov(theMovie->MBeg, ts);
							tLong = theMovie->MEnd + tLong;
							theMovie->MEnd = conv_from_msec_mov(tLong, ts);
							if (theMovie->MEnd > tc)
								theMovie->MEnd = tc;
						}
					} else if (rptTime == 0L || active == 8 || active == 5) {
						theMovie->MBeg = conv_from_msec_mov(theMovie->MBeg, ts);
						theMovie->MEnd = conv_from_msec_mov(theMovie->MEnd, ts);
					} else if (active == 3) {
						rptTime = theMovie->MBeg + rptTime;
						theMovie->MBeg = conv_from_msec_mov(theMovie->MBeg, ts);
						theMovie->MEnd = conv_from_msec_mov(rptTime, ts);
					} else if (active == 4) {
						rptTime = theMovie->MEnd - rptTime;
						theMovie->MBeg = conv_from_msec_mov(rptTime, ts);
						theMovie->MEnd = conv_from_msec_mov(theMovie->MEnd, ts);
					}
					if (theMovie->MEnd > tc)
						theMovie->MEnd = tc;
					if (theMovie->MBeg < 0)
						theMovie->MBeg = 0;

					if (theMovie->MBeg < 0 || theMovie->MBeg > tc) {
						do_warning("Beginning time is invalid", 0);
						return(true);
					}
					if (theMovie->MEnd < 0 || theMovie->MEnd > tc) {
						do_warning("Ending time is invalid", 0);
						return(true);
					}
					if (theMovie->MBeg > theMovie->MEnd) {
						do_warning("BEG mark must be smaller than END mark", 0);
						return(true);
					}
					tc = theMovie->MBeg;
					SetMovieActiveSegment(theMovie->MvMovie, tc, theMovie->MEnd-theMovie->MBeg);
					GoToBeginningOfMovie(theMovie->MvMovie);
					theMovie->isPlaying = TRUE;


					SetMoviePlayHints(theMovie->MvMovie, hintsScrubMode, hintsScrubMode);
					if (PBC.speed != 100) {
						float speed;
						if (PBC.speed > 0 && PBC.speed < 100) {
							speed = (1.0000 * (float)PBC.speed) / 100.0000;
							SetMovieRate(theMovie->MvMovie, FloatToFixed(speed));
						} else if (PBC.speed > 100) {
							speed = (float)PBC.speed / 100.0000;
							SetMovieRate(theMovie->MvMovie, FloatToFixed(speed));
						} else
							StartMovie(theMovie->MvMovie);
					} else
						StartMovie(theMovie->MvMovie);
				} else {
					tc = MCGetCurrentTime(theMovie->MvController, &ts);
					if (tc < GetMovieDuration(theMovie->MvMovie)) {
						SetMovieActiveSegment(theMovie->MvMovie, tc, GetMovieDuration(theMovie->MvMovie)-tc);
						GoToBeginningOfMovie(theMovie->MvMovie);
						theMovie->isPlaying = 3;


						SetMoviePlayHints(theMovie->MvMovie, hintsScrubMode, hintsScrubMode);
						if (PBC.speed != 100) {
							float speed;
							if (PBC.speed > 0 && PBC.speed < 100) {
								speed = (1.0000 * (float)PBC.speed) / 100.0000;
								SetMovieRate(theMovie->MvMovie, FloatToFixed(speed));
							} else if (PBC.speed > 100) {
								speed = (float)PBC.speed / 100.0000;
								SetMovieRate(theMovie->MvMovie, FloatToFixed(speed));
							} else
								StartMovie(theMovie->MvMovie);
						} else
							StartMovie(theMovie->MvMovie);
					} else {
						if ((theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
							theMovie->df->MvTr.MovieFile[0] = EOS;
						}
						do_warning("You are at the end of the movie file.\rIf you want to start linking new media file, then place text cursor on a tier without a bullet.", 0);
					}
				}
			} else {
				tc = MCGetCurrentTime(theMovie->MvController, &ts);
				if (( tc >= theMovie->MEnd /* IsMovieDone(theMovie->MvMovie) */ && theMovie->isPlaying) || 
						theMovie->isPlaying == -1 || theMovie->isPlaying == -2) {
					if (theMovie->isPlaying)
						StopMovie(theMovie->MvMovie);
					if ((theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
						myFInfo *saveGlobal_df;
						extern long gCurF;

						saveGlobal_df = global_df;
						global_df = theMovie->df;
						if (theMovie->isPlaying == -1 || theMovie->isPlaying == -2)
							CopyAndFreeMovieData(TRUE);
						if (FreqCountLimit > 0 && global_df->FreqCount >= FreqCountLimit) {
							SaveCKPFile(TRUE);
							global_df->FreqCount = 0;
						}
						gCurF = conv_to_msec_mov(tc, ts);
						global_df = saveGlobal_df;
					}
				
					GetMovieActiveSegment(theMovie->MvMovie, &tc2, &tc3);
					if (tc2 != -1)
						SetMovieActiveSegment(theMovie->MvMovie, -1, 0);
					sprintf(templineC3, "%ld", conv_to_msec_mov(tc, ts));
					SetWindowMLTEText(templineC3, win, 5);
					PlayingContMovie = FALSE;

					if (theMovie->isPlaying != -1 && PBC.enable && PBC.isPC && PBC.LoopCnt > MoviePlayLoopCnt) {
						MoviePlayLoopCnt++;
						MCDoAction(theMovie->MvController, mcActionPlay, (void *)-2);
						theMovie->toOrgWind = FALSE;
					} else if (theMovie->df != NULL && theMovie->isPlaying != -1 && PBC.walk && PBC.isPC && PBC.backspace != PBC.step_length) {
						MoviePlayLoopCnt = 1;
						if (PBC.backspace > PBC.step_length) {
							tLong = theMovie->df->MvTr.MBeg - (PBC.backspace - PBC.step_length);
							if (tLong < 0L)
								goto fin;
							theMovie->df->MvTr.MBeg = tLong;

							if (PBC.walk == 2 && theMovie->df->MvTr.MBeg >= sEF)
								goto fin;

							theMovie->MBeg = conv_from_msec_mov(theMovie->df->MvTr.MBeg, ts);
							if (theMovie->MBeg >= theMovie->MDur)
								goto fin;

							theMovie->df->MvTr.MEnd = theMovie->df->MvTr.MEnd - (PBC.backspace - PBC.step_length);
							if (PBC.walk == 2 && theMovie->df->MvTr.MEnd > sEF)
								theMovie->df->MvTr.MEnd = sEF;
						} else {
							tLong = theMovie->df->MvTr.MBeg + (PBC.step_length - PBC.backspace);
							if (tLong < 0L)
								goto fin;
							theMovie->df->MvTr.MBeg = tLong;

							if (PBC.walk == 2 && theMovie->df->MvTr.MBeg >= sEF)
								goto fin;

							theMovie->MBeg = conv_from_msec_mov(theMovie->df->MvTr.MBeg, ts);
							if (theMovie->MBeg >= theMovie->MDur)
								goto fin;

							theMovie->df->MvTr.MEnd = theMovie->df->MvTr.MEnd + (PBC.step_length - PBC.backspace);
							if (PBC.walk == 2 && theMovie->df->MvTr.MEnd > sEF)
								theMovie->df->MvTr.MEnd = sEF;
						}
						theMovie->MEnd = conv_from_msec_mov(theMovie->df->MvTr.MEnd, ts);

						sprintf(templineC3, "%ld", theMovie->df->MvTr.MBeg);
						SetWindowMLTEText(templineC3, win, 3);
						SetWindowMLTEText(templineC3, win, 5);
						sprintf(templineC3, "%ld", theMovie->df->MvTr.MEnd);
						SetWindowMLTEText(templineC3, win, 4);
				/*
						if (PBC.pause_len > 0L) {
							double d;
							Size t;

							d = (double)PBC.pause_len;
							t = TickCount() + (Size)(d / (double)16.666666666);
							do {  
								if (TickCount() > t) 
									break;
								if ((key=ced_getc()) != -1) {
									if (isKeyEqualCommand(key, 91)) {
										isPlayS = 91;
										isAborted = 1;
										break;
									}
									if (((key > 0 && key < 256) || isPlayS != 0) && PBC.enable) {
										if (key != 1 || isPlayS != 0) {
											if (proccessKeys((unsigned int)key) == 1) {
												strcpy(global_df->err_message, DASHES);
												goto fin;
											}
										}
									} else {
										strcpy(global_df->err_message, DASHES);
										goto fin;
									}
								} else if (isPlayS != 0) {
									goto fin;
								}
							} while (true) ;	
						}
				*/
						theMovie->isPlaying = TRUE;
						MCDoAction(theMovie->MvController, mcActionPlay, (void *)-2);
						theMovie->toOrgWind = FALSE;
					} else {
						if (theMovie->isPlaying == -1 && theMovie->df != NULL && PBC.enable && PBC.walk && PBC.isPC) {
							tLong = conv_to_msec_mov(tc, ts);
							if (tLong < 0L)
								goto fin;
							theMovie->df->MvTr.MBeg = tLong;
							theMovie->MBeg = conv_from_msec_mov(theMovie->df->MvTr.MBeg, ts);
							if (theMovie->MBeg >= theMovie->MDur)
								goto fin;
							theMovie->df->MvTr.MEnd = theMovie->df->MvTr.MBeg + PBC.step_length;
						}
fin:
						if (((PBC.enable && PBC.walk && PBC.isPC) || !PBC.enable) && (theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
							myFInfo *saveGlobal_df;

							saveGlobal_df = global_df;
							global_df = theMovie->df;
							strcpy(theMovie->df->err_message, DASHES);
							if (!global_df->err_message[0] && global_df->SoundWin != NULL) 
								PutSoundStats(global_df->SnTr.WEndF - global_df->SnTr.WBegF);
							else
								draw_mid_wm();
							PrepWindow(0);
							global_df = saveGlobal_df;
						}


						theMovie->isPlaying = FALSE;
						PBC.isPC = 0;
						PBC.walk = 0;
					}


					if (theMovie->toOrgWind && (theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
						changeCurrentWindow(win, theMovie->df->wind, true);
					}
				}
			}
			if (theMovie->toOrgWind != -1)
				theMovie->toOrgWind = FALSE;
			return(true);
		} else if (action == mcActionGoToTime) {
			SetMovieTime(theMovie->MvMovie, (TimeRecord*)params);
			tc = MCGetCurrentTime(theMovie->MvController, &ts);
			tc = conv_to_msec_mov(tc, ts);
			sprintf(templineC3, "%ld", tc);
			SetWindowMLTEText(templineC3, win, 5);
			theMovie->toOrgWind = FALSE;
			if (PBC.enable && (theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
				theMovie->df->MvTr.MBeg = tc;
				PBC.cur_pos = tc;
			}
			SetWaveTimeValue(WindowProcs(win), tc, 0L);
			return(true);
		} else if (action == mcActionStep) {
			if ((long)params < 0)
				type = -1;
			else
				type = -2;
			moveMovieCursor(win, 0, theMovie, 5, type);
			tc = MCGetCurrentTime(theMovie->MvController, &ts);
			tc = conv_to_msec_mov(tc, ts);
			SetWaveTimeValue(WindowProcs(win), tc, 0L);
			return(true);
		}
	}
	return(false);
}

static Boolean myTestAndSet(Boolean *p) {
	Boolean val;
	
	val = *p;
	*p = true;
	return(val);
}

static Boolean myTestAndClear(Boolean *p) {
	Boolean val;
	
	val = *p;
	*p = false;
	return(!val);
}

static pascal Boolean ActionNotificationCallback(MovieController inMC, short inAction, void *params, UInt32 inFlags, UInt32 *outFlags, void *inRefCon) {
#pragma unused(params)
    WindowRef   win = NULL;
	GrafPtr		oldPort;

    win = (WindowRef)inRefCon;
    if (win == NULL) { *outFlags = kQTMCActionNotifyCancelled; return 0; }
	if (inFlags & kQTMCActionNotifyCancelled) return 0;
    if (inFlags & kQTMCActionNotifyAfter) {
        switch (inAction) {
			case mcActionControllerSizeChanged:
			{
				if (myTestAndSet(&controlChangingBounds) == false) {

					GetPort(&oldPort);
					SetPortWindowPort(win);
					
					GetMovieNaturalBoundsRect(theMovie->MvMovie, &theMovie->MvBounds);
					OffsetRect(&theMovie->MvBounds, -theMovie->MvBounds.left, -theMovie->MvBounds.top);
					SetMovieBox(theMovie->MvMovie, &theMovie->MvBounds);

					if ((theMovie->MvBounds.right-theMovie->MvBounds.left) < 200)
						theMovie->MvBounds.right = theMovie->MvBounds.left + 200;			

					theMovie->orgMvBounds = theMovie->MvBounds;
					MCGetControllerBoundsRect(theMovie->MvController,&theMovie->MvBounds);
					if ((theMovie->MvBounds.right-theMovie->MvBounds.left) < 200) {
						theMovie->MvBounds.right = theMovie->MvBounds.left + 200;			
						MCSetControllerBoundsRect(theMovie->MvController,&theMovie->MvBounds);
					}
					OffsetRect(&theMovie->MvBounds, -theMovie->MvBounds.left, -theMovie->MvBounds.top);			

					resizeMovieWindowItems(win, theMovie);
					
					SetPort(oldPort);

					myTestAndClear(&controlChangingBounds);
				}
				break;
			}
			default:
				break;
        }
    }
	
    return 0;
    
}   // ActionNotificationCallback

char isMovieStreaming(Movie MvMovie) {
    if (MvMovie == NULL)
		return(FALSE);
    return(GetMovieIndTrackType(MvMovie, 1, kQTSStreamMediaType, movieTrackMediaType | movieTrackEnabledOnly) != NULL);
}

static pascal void MyPPRollProc(Movie theMovie, OSErr prerollErr, void *refcon) {
	char *isDone = (char *)refcon;
	*isDone = TRUE;
}

static OSErr Movie_Load(WindowPtr MvWin, MovieInfo *Movie, TimeValue timeNow, char isSetOrg) {
	char isDone;
    GrafPtr	oldPort;
	OSErr   theError = noErr;
	Fixed	playRate;
	EventRecord		myEvent;
    QTMCActionNotificationRecord actionNotifier;

	//    MCActionFilterWithRefConUPP mFD = NewMCActionFilterWithRefConUPP((MCActionFilterWithRefConProcPtr)MCActionFilter);
    MCActionFilterWithRefConUPP mFD(MCActionFilter);

	GetPort(&oldPort);
  	SetPortWindowPort(MvWin);

	GetMovieBox(Movie->MvMovie, &Movie->MvBounds);
	OffsetRect(&Movie->MvBounds, -Movie->MvBounds.left, -Movie->MvBounds.top);
	SetMovieBox(Movie->MvMovie, &Movie->MvBounds);
	SetPalette(MvWin, srcPalette, true);
	SetMovieGWorld(Movie->MvMovie, GetWindowPort(MvWin), GetGWorldDevice(GetWindowPort(MvWin)));
	
	if ((Movie->MvBounds.right-Movie->MvBounds.left) < 200)
		Movie->MvBounds.right = Movie->MvBounds.left + 200;
	if (isSetOrg)
		Movie->orgMvBounds = Movie->MvBounds;
	Movie->MvController = NewMovieController(Movie->MvMovie, &Movie->MvBounds, 0L | mcTopLeftMovie | mcWithFrame);
	MCEnableEditing(Movie->MvController, false);
	MCDoAction(Movie->MvController, mcActionSetUseBadge, (void *)false);
	
	theError = MCGetControllerBoundsRect(Movie->MvController,&Movie->MvBounds);
	OffsetRect(&Movie->MvBounds, -Movie->MvBounds.left, -Movie->MvBounds.top);
	if (theError == noErr)
		theError = MCSetControllerPort(Movie->MvController,GetWindowPort(MvWin));
	// add grow box for the movie controller
	//	MCDoAction(myMC, mcActionSetGrowBoxBounds, &gLimitRect);
	MCSetActionFilterWithRefCon(Movie->MvController, mFD, (long)GetWRefCon(MvWin));
	
	resizeMovieWindowItems(MvWin, Movie);
	
	if (isSetOrg && isMovieStreaming(Movie->MvMovie)) {
		actionNotifier.returnSignature	= 0;                            // set to zero when passed to to the Movie Controller, on return will be set to 'noti' if mcActionAddActionNotification is implemented
		actionNotifier.notifyAction		= ActionNotificationCallback;   // function to be called at action time
		actionNotifier.refcon			= MvWin;                  // something to pass to the action function
		actionNotifier.flags			= kQTMCActionNotifyAfter;	// option flags
		MCDoAction(Movie->MvController, mcActionAddActionNotification, (void *)&actionNotifier);
		
		SetMoviePlayHints(Movie->MvMovie, hintsAllowDynamicResize, hintsAllowDynamicResize);
	}
	if (isSetOrg) {
		playRate = GetMoviePreferredRate(Movie->MvMovie);
		if (theError == noErr) {
			if (isMovieStreaming(Movie->MvMovie)) {
				isDone = FALSE;
				theError = PrePrerollMovie(Movie->MvMovie,timeNow,playRate,NewMoviePrePrerollCompleteUPP(MyPPRollProc),(void *)&isDone);
				if (theError == 1)
					theError = noErr;
			} else {
				isDone = TRUE;
				theError = PrePrerollMovie(Movie->MvMovie, timeNow, playRate, nil, nil);
			}
		}

		if (theError == noErr && isMovieStreaming(Movie->MvMovie)) {
			long state;
			while (1) {
				state = GetMovieLoadState(Movie->MvMovie);
				if (state <= kMovieLoadStateError) {
					theError = kMovieLoadStateError;
					break;
				} else if (state < kMovieLoadStatePlayable) {
					MoviesTask(Movie->MvMovie, 1);
				} else if (state == kMovieLoadStateComplete && isDone) {
					break;
				}
				MoviesTask(Movie->MvMovie, 1);
				WaitNextEvent(everyEvent, &myEvent, 4, nil);
			}
		}

		if (theError == noErr)
			theError = PrerollMovie(Movie->MvMovie, timeNow, playRate);		
	}
	
	if (theError == noErr) {
		GoToBeginningOfMovie(Movie->MvMovie);
		if (LoadMovieIntoRam(Movie->MvMovie, 0L/*GetMovieTime(Movie->MvMovie, 0L)*/, GetMovieDuration(Movie->MvMovie), unkeepInRam) == noErr)
			theError = UpdateMovie(Movie->MvMovie);
	}

	SetPort(oldPort);
	return(theError);
}

void ResizeMovie(WindowPtr MvWin, MovieInfo *tMovie, short w, short isRedo) {
	double ratio, th, tw;
	short			h;
    GrafPtr			oldPort;
	FNType			str[256];

	MCSetActionFilterWithRefCon(tMovie->MvController, NULL, (long)NULL);
	DisposeMovieController(tMovie->MvController);
	tMovie->MvController = NULL;
	if (w == 0) {
		do {
			if (MVWinWidth != 0) {
				MVWinZoom = 1;
				MVWinWidth = 0;
			} else if (isRedo != 0) {
				MVWinZoom++;
				if (MVWinZoom >= 4)
					MVWinZoom = -1;
			}
			tMovie->MvBounds.top = tMovie->orgMvBounds.top;
			tMovie->MvBounds.left = tMovie->orgMvBounds.left;
			if (MVWinZoom <= 0) {
				if (MVWinZoom == -1) {
					tMovie->MvBounds.right = tMovie->MvBounds.left + ((tMovie->orgMvBounds.right-tMovie->orgMvBounds.left) / 4);
					tMovie->MvBounds.bottom = tMovie->MvBounds.top + ((tMovie->orgMvBounds.bottom-tMovie->orgMvBounds.top) / 4);
				} else {
					tMovie->MvBounds.right = tMovie->MvBounds.left + ((tMovie->orgMvBounds.right-tMovie->orgMvBounds.left) / 2);
					tMovie->MvBounds.bottom = tMovie->MvBounds.top + ((tMovie->orgMvBounds.bottom-tMovie->orgMvBounds.top) / 2);
				}
			} else {
				tMovie->MvBounds.right = tMovie->MvBounds.left + ((tMovie->orgMvBounds.right-tMovie->orgMvBounds.left) * MVWinZoom);
				tMovie->MvBounds.bottom = tMovie->MvBounds.top + ((tMovie->orgMvBounds.bottom-tMovie->orgMvBounds.top) * MVWinZoom);
			}
			w = tMovie->MvBounds.right - tMovie->MvBounds.left;
			h = tMovie->MvBounds.bottom - tMovie->MvBounds.top;
			if (w >= 200 || isRedo == 0)
				break;
		} while (1) ;
	} else {
		th = (double)(tMovie->orgMvBounds.bottom - tMovie->orgMvBounds.top);
		tw = (double)(tMovie->orgMvBounds.right - tMovie->orgMvBounds.left);
		ratio = th / tw;
		tw = w;
		tMovie->MvBounds.right = tMovie->MvBounds.left + w;
		tMovie->MvBounds.bottom = tMovie->MvBounds.top + (short)(tw * ratio);
		MVWinWidth = w;
	}
	SetMovieBox(tMovie->MvMovie, &tMovie->MvBounds);
	Movie_Load(MvWin, tMovie, tMovie->MBeg, FALSE);
	GetWindowMLTEText(MvWin, 5, UTTLINELEN, templineC3);
	if (templineC3[0] == EOS)
		SetMovieTimeValue(tMovie->MvMovie, tMovie->MBeg);
	else
		SetMovieTimeValue(tMovie->MvMovie, conv_from_msec_mov(atol(templineC3), GetMovieTimeScale(tMovie->MvMovie)));
	MoviesTask(tMovie->MvMovie, 1);
	UpdateMovieWindow(MvWin);
	UpdateMovieWindow(MvWin);
	if (MVWinWidth != 0 || MVWinZoom != 1) {
		th = tMovie->orgMvBounds.right;
		tw = tMovie->MvBounds.right;
		uS.sprintf(str, "zoom: %.2lf", tw/th);
	} else
		strcpy(str, " ");
	GetPort(&oldPort);
  	SetPortWindowPort(MvWin);
	SetDialogItemUTF8(str, MvWin, 12, FALSE);
	SetPort(oldPort);
	WriteCedPreference();
}
/* OPEN AND LOAD MOVIE */

ACT_FUNC_INFO keyMovieActions = {
	comKeys, nil, 500,
} ;

int PlayMovie(movInfo *mvRec, myFInfo *df, char isJustOpen) {
	int				i;
	char			isMovieWinExist;
	TimeScale		ts;
	TimeValue		timeNow;
	WindowPtr		win, front;
	PrepareStruct	saveRec;
    GrafPtr			oldPort;
	OSErr			err = noErr;
	FNType			fileName[FNSize];
	CFStringRef		theStr;

	if (!MovieReady && !isMovieAvialable) {
		PlayingContMovie = FALSE;
		do_warning("QuickTime failure!", 0);
		return(1);
	}
	if (/*!PlayingContMovie || */PlayingContMovie == '\003') {
		FakeSelectWholeTier((PlayingContMovie == '\003' && F5Option == EVERY_LINE));
		DrawFakeHilight(1);
	}
	isMovieWinExist = (FindAWindowNamed(Movie_sound_str) != NULL);

	if (!PlayingContMovie || (PBC.walk && PBC.enable))
		front = FrontWindow();

	if (OpenWindow(500, Movie_sound_str, 0L, false, 0, MovieEvent, UpdateMovieWindow, CleanupMovie)) {
		MovieReady = false;
		PlayingContMovie = FALSE;
		do_warning("Error opening Movie File or Movie window", 0);
		return(1);
	}
	win = FindAWindowNamed(Movie_sound_str);
	if (win == NULL) {
		theMovie = NULL;
		PlayingContMovie = FALSE;
		do_warning("Error opening Movie window", 0);
		return(1);
	}
	if (WindowProcs(win) == NULL) {
		if (win != NULL) {
			PrepareWindA4(win, &saveRec);
			mCloseWindow(win);
			RestoreWindA4(&saveRec);
		}
		PlayingContMovie = FALSE;
		do_warning("Error opening Movie window", 0);
		return(1);
	}

	if (!isMovieWinExist) {
		ControlRef iCtrl;

		keyMovieActions.win = win;
		iCtrl = GetWindowItemAsControl(win, 3);
		mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyMovieActions);
		iCtrl = GetWindowItemAsControl(win, 4);
		mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyMovieActions);
		iCtrl = GetWindowItemAsControl(win, 5);
		mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyMovieActions);
		iCtrl = GetWindowItemAsControl(win, 8);
		mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyMovieActions);
	}

	if (theMovie != NULL) {
		if (strcmp(theMovie->fName, mvRec->MovieFile)) {
			if (theMovie->isPlaying)
				StopMovie(theMovie->MvMovie);
			theMovie->isPlaying = FALSE;
			Movie_Close(theMovie);
			theMovie = NULL;
			movieActive = 0;
			rptCnt = 0;
		}
	}
	if (theMovie == NULL) {
		DrawMouseCursor(2);
		theMovie = Movie_Open(mvRec);

		if (theMovie == NULL)	{
			PlayingContMovie = FALSE;
			do_warning("Error opening local Movie file", 0);
			PrepareWindA4(win, &saveRec);
			mCloseWindow(win);
			RestoreWindA4(&saveRec);
			return(1);
		}
		theMovie->win = win;
		theMovie->isPlaying = FALSE;
		strcpy(theMovie->fName, mvRec->MovieFile);

		ts = GetMovieTimeScale(theMovie->MvMovie);

		timeNow = conv_from_msec_mov(mvRec->MBeg, ts);
		err = Movie_Load(win, theMovie, timeNow, TRUE);
		DrawGrowIcon(win);
		DrawControls(win);

		ShowWindow(win);

		if (!PlayingContMovie || (PBC.walk && PBC.enable)) {
			if (front != NULL && WindowProcs(front) != NULL) {
				SendBehind(win, front);
				changeCurrentWindow(NULL, front, false);
			} else
				DrawMouseCursor(1);
		} else
			DrawMouseCursor(1);

		theMovie->MDur = GetMovieDuration(theMovie->MvMovie);
		theMovie->MBeg = timeNow;
		theMovie->MEnd = conv_from_msec_mov(mvRec->MEnd, ts);

		if (MVWinWidth != 0)
			ResizeMovie(win, theMovie, MVWinWidth, 0);
		else if (MVWinZoom != 1)
			ResizeMovie(win, theMovie, 0, 0);

	} else {
		Fixed playRate;

		theMovie = theMovie;
		ts = GetMovieTimeScale(theMovie->MvMovie);
		if (conv_from_msec_mov(mvRec->MBeg+300, ts) >= GetMovieDuration(theMovie->MvMovie)) {
			theMovie->isPlaying = FALSE;
			PlayingContMovie = FALSE;
			do_warning("You are at the end of the movie file.", 0);
			return(1);
		}

		timeNow = conv_from_msec_mov(mvRec->MBeg, ts);
		theMovie->MDur = GetMovieDuration(theMovie->MvMovie);
		theMovie->MBeg = timeNow;
		theMovie->MEnd = conv_from_msec_mov(mvRec->MEnd, ts);

		playRate = GetMoviePreferredRate(theMovie->MvMovie);
		err = PrePrerollMovie(theMovie->MvMovie, timeNow, playRate, nil, nil);
		//		PrePrerollMovie(theMovie->MvMovie,timeNow,playRate,NewMoviePrePrerollCompleteUPP(MyPPRollProc),(void *)theMovie); 
		if (err == noErr)
			err = PrerollMovie(theMovie->MvMovie, timeNow, playRate);
	}

// SetMoviePlayHints(theMovie->MvMovie, 0 | hintsScrubMode, 0);

	oRptPTime = rptPTime;
	oRptMark = rptMark;

	for (i=0; i < NUMMOVIEUNDO; i++) 
		theMovie->undo[i].type = 0;
	theMovie->undoIndex = 0;

	GetPort(&oldPort);
	SetPortWindowPort(win);

	if (rptMark != 0)
		sprintf(templineC3, "%c%d", rptMark, rptPTime);
	else
		sprintf(templineC3, "%d", rptPTime);
	SetWindowMLTEText(templineC3, win, 8);

	extractFileName(fileName, mvRec->rMovieFile);
	SetDialogItemUTF8(fileName, win, 11, FALSE);

	if (MVWinWidth != 0 || MVWinZoom != 1) {
		double t1, t2;

		t1 = theMovie->orgMvBounds.right;
		t2 = theMovie->MvBounds.right;
		uS.sprintf(fileName, "zoom: %.2lf", t2/t1);
	} else
		uS.str2FNType(fileName, 0L, " ");
	SetDialogItemUTF8(fileName, win, 12, FALSE);

	SetPort(oldPort);

	if (movieActive != 0)
		SelectWindowMLTEText(win, movieActive, HUGE_INTEGER, HUGE_INTEGER);
	movieActive = 3;
	SetKeyboardFocus(win, GetWindowItemAsControl(win, movieActive), kUserClickedToFocusPart);

	sprintf(templineC3, "%ld", mvRec->MBeg);
	SetWindowMLTEText(templineC3, win, movieActive);
	SelectWindowMLTEText(win, movieActive, 0, HUGE_INTEGER);

	theStr = my_CFStringCreateWithBytes(Movie_sound_str);
	if (theStr != NULL) {
		SetWindowTitleWithCFString(win, theStr);
		CFRelease(theStr);
	}

	if (err != noErr && isMovieStreaming(theMovie->MvMovie)) {
		PlayingContMovie = FALSE;
		sprintf(templineC, "Can't find movie file on a server, pointed to by: %s. Or having some network problems(%d).", mvRec->rMovieFile, err);
		do_warning(templineC, 0);
	} else if (isJustOpen) {
		theMovie->MEnd = theMovie->MDur;
		mvRec->MEnd = conv_to_msec_mov(mvRec->MEnd, ts);
		sprintf(templineC3, "%ld", conv_to_msec_mov(theMovie->MEnd, ts));
		SetWindowMLTEText(templineC3, win, 4);
		SetWindowMLTEText("0", win, 5);
	} else if (mvRec->MEnd == 0L) {
		SetMovieActiveSegment(theMovie->MvMovie, -1, 0);
		theMovie->MEnd = theMovie->MDur;
		theMovie->toOrgWind = FALSE;
		sprintf(templineC3, "%ld", conv_to_msec_mov(theMovie->MEnd, ts));
		SetWindowMLTEText(templineC3, win, 4);
		SetWindowMLTEText("0", win, 5);
		if (PlayingContMovie == '\003') {
			UpdateMovieWindowBeforePlayback(win);
			MCDoAction(theMovie->MvController, mcActionPlay, (void *)-2);
		} else
			UpdateMovieWindow(win);
	} else {
		if (PlayingContMovie) {
			if (PlayingContMovie == '\003')
				theMovie->MEnd = theMovie->MDur;
			else {
				if (theMovie->MEnd > theMovie->MDur)
					theMovie->MEnd = theMovie->MDur;
			}
		}
		sprintf(templineC3, "%ld", conv_to_msec_mov(theMovie->MEnd, ts));
		SetWindowMLTEText(templineC3, win, 4);
		sprintf(templineC3, "%ld", conv_to_msec_mov(theMovie->MBeg, ts));
		SetWindowMLTEText(templineC3, win, 5);
		if (theMovie->MEnd < theMovie->MBeg) {
			PlayingContMovie = FALSE;
			do_warning("BEG mark must be smaller than END mark", 0);
			return(1);
		}
		UpdateMovieWindowBeforePlayback(win);
		MCDoAction(theMovie->MvController, mcActionPlay, (void *)-2);
		if (theMovie->toOrgWind == -1)
			theMovie->toOrgWind = FALSE;
		else
			theMovie->toOrgWind = TRUE; // 21-05-07 FALSE;
	}
	MoviePlayLoopCnt = 1;
	theMovie->df = df;
	if (err == noErr)
		MovieReady = true;
	return(1);
}

/* MOVIE UNDO FUNCTIONS BEGIN */
void saveMovieUndo(WindowPtr win, short selBeg, short selEnd, char isFixRedo) {
	short pos;

	if (theMovie == NULL)
		return;
	if (movieActive == 8 || movieActive == 5)
		return;

	GetWindowMLTEText(win, movieActive, UTTLINELEN, templineC3);
	theMovie->undo[theMovie->undoIndex].type   = isFixRedo;
	theMovie->undo[theMovie->undoIndex].val    = atol(templineC3);
	theMovie->undo[theMovie->undoIndex].selBeg = selBeg;
	theMovie->undo[theMovie->undoIndex].selEnd = selEnd;
	theMovie->undo[theMovie->undoIndex].which  = movieActive;
	theMovie->undoIndex++;
	if (theMovie->undoIndex == NUMMOVIEUNDO)
		theMovie->undoIndex = 0;
	if (isFixRedo == 1) {
		pos = theMovie->undoIndex;
		while (theMovie->undo[pos].type == 2) {
			theMovie->undo[pos].type = 0;
			pos++;
			if (pos == NUMMOVIEUNDO)
				pos = 0;
		}
	}
}

void doMovieUndo(WindowProcRec *windProc) {
	char		isAC;
	short		pos;
	WindowPtr	win;
	ControlRef	iCtrl;
	ControlEditTextSelectionRec selection;

	if (theMovie == NULL)
		return;
	win = FindAWindowProc(windProc);
	pos = theMovie->undoIndex;
	if (theMovie->undo[pos].type != 2) {
		GetKeyboardFocus(win, &iCtrl);
		GetMLTETextSelection(iCtrl, &selection);
		saveMovieUndo(win, selection.selStart, selection.selEnd, 2);
	}
	if (pos == 0)
		pos = NUMMOVIEUNDO - 1;
	else
		pos--;
	if (theMovie->undo[pos].type == 1) {
		if (movieActive != theMovie->undo[pos].which) {
			isAC = TRUE;
			SelectWindowMLTEText(win, movieActive, HUGE_INTEGER, HUGE_INTEGER);
		}
		movieActive = theMovie->undo[pos].which;
		sprintf(templineC3, "%ld", theMovie->undo[pos].val);
		SetWindowMLTEText(templineC3, win, movieActive);
		if (isAC) {
			iCtrl = GetWindowItemAsControl(win, movieActive);
			SetKeyboardFocus(win, iCtrl, kUserClickedToFocusPart);
		} else
			GetKeyboardFocus(win, &iCtrl);
		selection.selStart = theMovie->undo[pos].selBeg;
		selection.selEnd = theMovie->undo[pos].selEnd;
		SetMLTETextSelection(iCtrl, &selection);
		if (isAC) {
			UpdateMovieWindow(win);
			UpdateMovieWindow(win);
		}
		theMovie->undo[pos].type = 2;
		theMovie->undoIndex = pos;
	} else {
		do_warning("Nothing more to undo", 0);
	}
}

void doMovieRedo(WindowProcRec *windProc) {
	char		isAC;
	short		pos;
	WindowPtr	win;
	ControlRef	iCtrl;
	ControlEditTextSelectionRec selection;

	if (theMovie == NULL)
		return;
	pos = theMovie->undoIndex;

	if (movieActive == theMovie->undo[pos].which && theMovie->undo[pos].type == 2) {
		win = FindAWindowProc(windProc);
		GetWindowMLTEText(win, movieActive, UTTLINELEN, templineC3);
		if (theMovie->undo[pos].val == atol(templineC3)) {
			theMovie->undo[pos].type = 1;
			pos++;
			if (pos == NUMMOVIEUNDO)
				pos = 0;
		}
	}

	if (theMovie->undo[pos].type == 2) {
		win = FindAWindowProc(windProc);
		if (movieActive != theMovie->undo[pos].which) {
			isAC = TRUE;
			SelectWindowMLTEText(win, movieActive, HUGE_INTEGER, HUGE_INTEGER);
		}
		movieActive = theMovie->undo[pos].which;
		sprintf(templineC3, "%ld", theMovie->undo[pos].val);
		SetWindowMLTEText(templineC3, win, movieActive);
		if (isAC) {
			iCtrl = GetWindowItemAsControl(win, movieActive);
			SetKeyboardFocus(win, iCtrl, kUserClickedToFocusPart);
		} else
			GetKeyboardFocus(win, &iCtrl);
		selection.selStart = theMovie->undo[pos].selBeg;
		selection.selEnd = theMovie->undo[pos].selEnd;
		SetMLTETextSelection(iCtrl, &selection);
		if (isAC) {
			UpdateMovieWindow(win);
			UpdateMovieWindow(win);
		}
		theMovie->undo[pos].type = 1;
		pos++;
		if (pos == NUMMOVIEUNDO)
			pos = 0;
		theMovie->undoIndex = pos;
	} else {
		do_warning("Nothing more to redo", 0);
	}
}
/* MOVIE UNDO FUNCTIONS END */

/* MOVIE HELP FUNCTIONS BEGIN */
static void UpdateMovieHelp(WindowPtr win) {
    GrafPtr	oldPort;
	Rect rect;

	GetPort(&oldPort);
    SetPortWindowPort(win);
	GetWindowPortBounds(win, &rect);
	EraseRect(&rect);
	DrawGrowIcon(win);
	DrawControls(win); 		/* Draw all the controls */
	SetPort(oldPort);
}

static void movieHelp(void) {
	if (OpenWindow(505,Movie_Help_str,0L,false,0,NULL,UpdateMovieHelp,NULL))
		ProgExit("Can't open Movie Help window");
}
/* MOVIE HELP FUNCTIONS END */
