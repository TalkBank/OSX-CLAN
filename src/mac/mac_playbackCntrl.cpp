#include "ced.h"
#include "mac_MUPControl.h"
#include "MMedia.h"
#include "mac_dial.h"

struct PBCs PBC = {0, 0, false, 0, 0, 0, 0, 0, 0, 0};

// static char  firstAct = TRUE;
static myFInfo *PBCglobal_df = NULL;
static WindowPtr PBCReady = nil;

static void CleanupPBC(WindowPtr wind) {
	ControlRef iCtrl;

//	firstAct = TRUE;
	PBCglobal_df = NULL;
	PBCReady = nil;
	PBC.enable = false;
	iCtrl = GetWindowItemAsControl(wind, 1);
	mUPCloseControl(iCtrl);
	iCtrl = GetWindowItemAsControl(wind, 3);
	mUPCloseControl(iCtrl);
	iCtrl = GetWindowItemAsControl(wind, 6);
	mUPCloseControl(iCtrl);
	iCtrl = GetWindowItemAsControl(wind, 8);
	mUPCloseControl(iCtrl);
	iCtrl = GetWindowItemAsControl(wind, 15);
	mUPCloseControl(iCtrl);
	iCtrl = GetWindowItemAsControl(wind, 18);
	mUPCloseControl(iCtrl);
}

static pascal void PBCControlActionCallback(ControlRef theControl, ControlPartCode partCode) {
	double	val;

	if (PBCReady != nil && partCode != 0) {
		val = (double)GetControlValue(theControl);
		val = (double)PBC.total_len * (val / 100.0000);
		PBC.cur_pos = (long)val;
		if (PBC.cur_pos < 0L)
			PBC.cur_pos = -PBC.cur_pos;
		sprintf(templineC3, "%ld", PBC.cur_pos);
		SetWindowMLTEText(templineC3, PBCReady, 18);
	}
}

void UpdatePBCDialog(WindowPtr win) {
    Rect		theRect;
	ControlRef	theControl;
    GrafPtr		oldPort;

	GetPort(&oldPort);
	SetPortWindowPort(win);
	GetWindowPortBounds(win, &theRect);
	EraseRect(&theRect);
	DrawControls(win);

	sprintf(templineC3, "%d", PBC.LoopCnt);
	SetWindowMLTEText(templineC3, win, 1);

	sprintf(templineC3, "%ld", PBC.backspace);
	SetWindowMLTEText(templineC3, win, 3);

	sprintf(templineC3, "%d", PBC.speed);
	SetWindowMLTEText(templineC3, win, 6);

	sprintf(templineC3, "%ld", PBC.step_length);
	SetWindowMLTEText(templineC3, win, 8);

	sprintf(templineC3, "%ld", PBC.pause_len);
	SetWindowMLTEText(templineC3, win, 15);

	if (!PBC.enable)
		ControlCTRL(win, 24, HideCtrl, 0);
	else	
		ControlCTRL(win, 24, ShowCtrl, 0);

	if (PBCglobal_df == NULL || MovieReady || PBCglobal_df->SnTr.SoundFile[0] == EOS) {
		ControlCTRL(win, 17, HideCtrl, 0);
		ControlCTRL(win, 18, HideCtrl, 0);
		ControlCTRL(win, 19, HideCtrl, 0);
		ControlCTRL(win, 20, HideCtrl, 0);
		ControlCTRL(win, 21, HideCtrl, 0);
		ControlCTRL(win, 22, HideCtrl, 0);
		ControlCTRL(win, 23, HideCtrl, 0);
	} else {
		ControlCTRL(win, 17, ShowCtrl, 0);
		ControlCTRL(win, 18, ShowCtrl, 0);
		ControlCTRL(win, 19, ShowCtrl, 0);
		ControlCTRL(win, 20, ShowCtrl, 0);
		ControlCTRL(win, 21, ShowCtrl, 0);
		ControlCTRL(win, 22, ShowCtrl, 0);
		ControlCTRL(win, 23, ShowCtrl, 0);
		
		sprintf(templineC3, "%ld", PBC.cur_pos);
		SetWindowMLTEText(templineC3, win, 18);

		sprintf(templineC3, "%ld", PBC.total_len);
		SetDialogItemUTF8(templineC3, win, 21, FALSE);

		theControl = GetWindowItemAsControl(win, 23);
		if (PBC.total_len == 0L) {
			HiliteControl(theControl, 255);
		} else {
			short tVal;
			double max;

			max = 100.0000 / (double)PBC.total_len;
			max = (double)PBC.cur_pos * max;
			tVal = (short)roundUp(max);
			HiliteControl(theControl, 0);
			SetControlMaximum(theControl, 100);
			SetControlValue(theControl, tVal);
		}
	}

	SetPort(oldPort);
}

static int proccessHits(WindowPtr win, short ModalHit, EventRecord *event, short var) {
	ControlRef		theControl;

	if (ModalHit == 1) {
		GetWindowMLTEText(win, 1, UTTLINELEN, templineC3);
		if (var != 0) {
			theControl = GetWindowItemAsControl(win, 1);
			if (!AgmentCurrentMLTEText(theControl, event, templineC3, var))
				return(0);
		}
		PBC.LoopCnt = atoi(templineC3);
		if (PBC.LoopCnt < 1)
			PBC.LoopCnt = 1;
		WriteCedPreference();
		return(0);
	} else if (ModalHit == 3) {
		GetWindowMLTEText(win, 3, UTTLINELEN, templineC3);
		if (var != 0) {
			theControl = GetWindowItemAsControl(win, 3);
			if (!AgmentCurrentMLTEText(theControl, event, templineC3, var))
				return(0);
		}
		PBC.backspace = atol(templineC3);
		if (PBC.backspace < 0)
			PBC.backspace = -PBC.backspace;
		WriteCedPreference();
		return(0);
	} else if (ModalHit == 6) {
		GetWindowMLTEText(win, 6, UTTLINELEN, templineC3);
		if (var != 0) {
			theControl = GetWindowItemAsControl(win, 6);
			if (!AgmentCurrentMLTEText(theControl, event, templineC3, var))
				return(0);
		}
		PBC.speed = atoi(templineC3);
		if (PBC.speed < 0)
			PBC.speed = 100;
		WriteCedPreference();
		return(0);
	} else if (ModalHit == 8) {
		GetWindowMLTEText(win, 8, UTTLINELEN, templineC3);
		if (var != 0) {
			theControl = GetWindowItemAsControl(win, 8);
			if (!AgmentCurrentMLTEText(theControl, event, templineC3, var))
				return(0);
		}
		PBC.step_length = atol(templineC3);
		if (PBC.step_length < 0L)
			PBC.step_length = -PBC.step_length;
		WriteCedPreference();
		return(0);
	} else if (ModalHit == 15) {
		GetWindowMLTEText(win, 15, UTTLINELEN, templineC3);
		if (var != 0) {
			theControl = GetWindowItemAsControl(win, 15);
			if (!AgmentCurrentMLTEText(theControl, event, templineC3, var))
				return(0);
		}
		PBC.pause_len = atol(templineC3);
		if (PBC.pause_len < 0L)
			PBC.pause_len = -PBC.pause_len;
		WriteCedPreference();
		return(0);
	} else if (ModalHit == 18 && PBCglobal_df != NULL && !MovieReady && PBCglobal_df->SnTr.SoundFile[0] != EOS) {
		int res = 0;
		myFInfo *tGlobal_df;

		GetWindowMLTEText(win, 18, UTTLINELEN, templineC3);
		if (var != 0) {
			theControl = GetWindowItemAsControl(win, 18);
			if (!AgmentCurrentMLTEText(theControl, event, templineC3, var))
				return(0);
		}
		PBC.cur_pos = atol(templineC3);
		if (PBC.cur_pos < 0L)
			PBC.cur_pos = -PBC.cur_pos;
		if (PBC.cur_pos > PBC.total_len) {
			PBC.cur_pos = PBC.total_len;
			sprintf(templineC3, "%ld", PBC.cur_pos);
			SetWindowMLTEText(templineC3, win, 18);
			res = 1;
		}
		tGlobal_df = global_df;
		global_df = PBCglobal_df;
		PBCglobal_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(PBC.cur_pos),'+');
		global_df = tGlobal_df;
		theControl = GetWindowItemAsControl(win, 23);
		if (PBC.total_len == 0L) {
			HiliteControl(theControl,255);
		} else {
			short tVal;
			double max;

			max = 100.0000 / (double)PBC.total_len;
			max = (double)PBC.cur_pos * max;
			tVal = (short)roundUp(max);
			HiliteControl(theControl,0);
			SetControlMaximum(theControl, 100);
			SetControlValue(theControl, tVal);
		}
		return(res);
	} else if (ModalHit == 23 && PBCglobal_df != NULL && !MovieReady && PBCglobal_df->SnTr.SoundFile[0] != EOS) {
		GrafPtr		port;
		myFInfo		*tGlobal_df;
		ControlActionUPP myActionUPP; 

		myActionUPP = NewControlActionUPP(PBCControlActionCallback);
		GetPort(&port);
		SetPortWindowPort(win);
		GlobalToLocal(&event->where);
		theControl = GetWindowItemAsControl(win, 23);
		if (TrackControl(theControl, event->where, myActionUPP)) {
			tGlobal_df = global_df;
			global_df = PBCglobal_df;
			PBCglobal_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(PBC.cur_pos),'+');
			global_df = tGlobal_df;
			UpdatePBCDialog(win);
		}
		LocalToGlobal(&event->where);
		SetPort(port);
		DisposeControlActionUPP(myActionUPP);
		return(1);
	} else if (ModalHit == 24) {
		char ret;

		if (PBCglobal_df == NULL) {
			isAjustCursor = TRUE;
			OpenAnyFile(NEWFILENAME, 1962, FALSE);
		} else {
			changeCurrentWindow(win, PBCglobal_df->wind, true);
		}

		if ((ret=GetNewMediaFile(FALSE, 3))) {
			if (ret == isAudio) { /* sound */
			} else if (ret == isVideo) { /* movie */
				if (global_df->SnTr.SoundFile[0] != EOS) {
					global_df->SnTr.SoundFile[0] = EOS;
					if (global_df->SnTr.isMP3 == TRUE) {
						global_df->SnTr.isMP3 = FALSE;
						if (global_df->SnTr.mp3.hSys7SoundData)
							DisposeHandle(global_df->SnTr.mp3.hSys7SoundData);
						global_df->SnTr.mp3.theSoundMedia = NULL;
						global_df->SnTr.mp3.hSys7SoundData = NULL;
					} else {
						fclose(global_df->SnTr.SoundFPtr);
					}
					global_df->SnTr.SoundFPtr = 0;
					if (global_df->SoundWin)
						DisposeOfSoundWin();
				}
			} else {
				strcpy(global_df->err_message, "+Unsupported media type.");
			}
			if (!findMediaTiers()) {
				sprintf(global_df->err_message, "+Please add \"%s\" tier with media file name to headers section at the top of the file.", MEDIAHEADER);
			}
		}
		SetPBCglobal_df(false, 0L);

		if (global_df->err_message[0] == '+') {
			strcpy(ced_lineC, global_df->err_message+1);
			strcpy(global_df->err_message, DASHES);
			do_warning(ced_lineC, 0);
		}
		UpdatePBCDialog(PBCReady);
		if (ret == isVideo) { // movie
			if (PBCglobal_df != NULL) {
				PlayMovie(&PBCglobal_df->MvTr, PBCglobal_df, TRUE);
			}
		}
		return(1);
	}
	return(0);
}

static int comKeys(WindowPtr win, EventRecord *event, short var) {
	short			key;
	short			ModalHit;
	ControlID		outID;
	ControlRef		theControl;

	key = event->message & keyCodeMask;
	if ((event->modifiers & cmdKey) || IsArrowKeyCode(key/256) || 
			key == 0x6000 || key == 0x6100 || key == 0x6200 || key == 0x6300 || key == 0x6400 || key == 0x6500) {
		return(0);
	} else if (key == 0x7C00 && ((event->modifiers & controlKey) || (event->modifiers & cmdKey))) {//right arrow
		return(1);
	} else if (key == 0x7B00 && ((event->modifiers & controlKey) || (event->modifiers & cmdKey))) {//left arrow
		return(1);
	}

	ModalHit = 0;
	key = (event->message & keyCodeMask)/256;
	if ((var < '0' || var > '9') && var != '+' && var != '-' && var != DELETE_CHAR && var != 8) {
		return(1);
	} else {
		GetKeyboardFocus(win, &theControl);
		if (GetControlID(theControl, &outID) == noErr)
			ModalHit = outID.id;
	}

	return(proccessHits(win, ModalHit, event, var));
}

static short PBCEvent(WindowPtr win, EventRecord *event) {
	short ModalHit = 0;

	PBCReady = win;
	if (event->what == keyDown || event->what == autoKey) {
		int key;

		key = event->message & keyCodeMask;
		if (key == 0x6100) {
			if ((event->modifiers & shiftKey) || (event->modifiers & rightShiftKey))
				F_key = 12+6;
			else
				F_key = 6;
			if (PBCglobal_df != NULL) {
				changeCurrentWindow(win, PBCglobal_df->wind, true);
			}
			return key;
		} else if (key == 0x6200) {
			if ((event->modifiers & shiftKey) || (event->modifiers & rightShiftKey))
				F_key = 12+7;
			else
				F_key = 7;
			if (PBCglobal_df != NULL) {
				changeCurrentWindow(win, PBCglobal_df->wind, true);
			}
			return key;
		} else if (key == 0x6400) {
			if ((event->modifiers & shiftKey) || (event->modifiers & rightShiftKey))
				F_key = 12+8;
			else
				F_key = 8;
			if (PBCglobal_df != NULL) {
				changeCurrentWindow(win, PBCglobal_df->wind, true);
			}
			return key;
		} else if (key == 0x6500) {
			if ((event->modifiers & shiftKey) || (event->modifiers & rightShiftKey))
				F_key = 12+9;
			else
				F_key = 9;
			if (PBCglobal_df != NULL) {
				changeCurrentWindow(win, PBCglobal_df->wind, true);
			}
			return key;
		} else if (key == 0x7C00 && ((event->modifiers & controlKey) || (event->modifiers & cmdKey))) {//right arrow
			return(-1);
		} else if (key == 0x7B00 && ((event->modifiers & controlKey) || (event->modifiers & cmdKey))) {//left arrow
			return(-1);
		}
	} else if (event->what == mouseDown) {
		ControlRef	theControl;
		ControlID	outID;
		ControlPartCode	code;

		GlobalToLocal(&event->where);
		theControl = FindControlUnderMouse(event->where,win,&code);
		if (theControl != NULL) {
			code = HandleControlClick(theControl,event->where,event->modifiers,NULL); 
			if (GetControlID(theControl, &outID) == noErr)
				ModalHit = outID.id;
			if (code == 0 && ModalHit != 1 && ModalHit != 3 && ModalHit != 6 && ModalHit != 8 && ModalHit != 15 && ModalHit != 18)
				ModalHit = 0;
		}
		LocalToGlobal(&event->where);
	}

	proccessHits(win, ModalHit, event, 0);
	return -1;
}

void SetPBCglobal_df(char isReset, long cur_pos) {
	if (isReset) {
		if (PBCglobal_df == global_df)
			PBCglobal_df = NULL;
	} else {
		PBCglobal_df = global_df;
		if (PBCglobal_df != NULL) {
			if (cur_pos == 0L)
				PBC.cur_pos = conv_to_msec_rep(PBCglobal_df->SnTr.BegF);
			else
				PBC.cur_pos = conv_to_msec_rep(cur_pos);
			PBC.total_len = conv_to_msec_rep(PBCglobal_df->SnTr.SoundFileSize);
		}
	}
	if (PBCReady != nil)
		UpdatePBCDialog(PBCReady);
}

char isPBCWindowOpen(void) {
	return((char)(PBCReady != nil));
}

ACT_FUNC_INFO keyControllerActions = {
	comKeys, nil, 506,
} ;

void PlaybackControlWindow(void) {
	char		isControllerExist;

	PBCglobal_df = global_df;
	isControllerExist = (FindAWindowNamed(Walker_Controller_str) != NULL);
	if (OpenWindow(506, Walker_Controller_str, 0L, false, 0, PBCEvent, UpdatePBCDialog, CleanupPBC)) {
		do_warning("Error opening Walker Control window", 0);
		return;
	}
	PBCReady = FindAWindowNamed(Walker_Controller_str);
	if (PBCReady != NULL) {
		if (!isControllerExist) {
			ControlRef iCtrl;
	
			keyControllerActions.win = PBCReady;
			iCtrl = GetWindowItemAsControl(PBCReady, 1);
			mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyControllerActions);
			iCtrl = GetWindowItemAsControl(PBCReady, 3);
			mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyControllerActions);
			iCtrl = GetWindowItemAsControl(PBCReady, 6);
			mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyControllerActions);
			iCtrl = GetWindowItemAsControl(PBCReady, 8);
			mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyControllerActions);
			iCtrl = GetWindowItemAsControl(PBCReady, 15);
			mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyControllerActions);
			iCtrl = GetWindowItemAsControl(PBCReady, 18);
			mUPOpenControl(iCtrl, dFnt.fontId, dFnt.fontSize, dFnt.isUTF, &keyControllerActions);
			iCtrl = GetWindowItemAsControl(PBCReady, 1);
			SetKeyboardFocus(PBCReady, iCtrl, kUserClickedToFocusPart);
		}
		PBC.enable = true;
		UpdatePBCDialog(PBCReady);
	}
}
