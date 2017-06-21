#include "ced.h"
#include "c_clan.h"
#include "mac_MUPControl.h"
#include "MMedia.h"
//#include "CurrentEvent.h"

enum {
	kShiftKeyCode = 56
};

extern short mScript;

/* Univerals Procedure Pointer variables used by the
	mUP Control.  These variables are set up
	the first time that mUPOpenControl is called. */
ControlUserPaneDrawUPP gTPDrawProc = NULL;
ControlUserPaneHitTestUPP gTPHitProc = NULL;
ControlUserPaneTrackingUPP gTPTrackProc = NULL;
ControlUserPaneIdleUPP gTPIdleProc = NULL;
ControlUserPaneKeyDownUPP gTPKeyProc = NULL;
ControlUserPaneActivateUPP gTPActivateProc = NULL;
ControlUserPaneFocusUPP gTPFocusProc = NULL;

	/* events handled by our focus advance override routine */
static const EventTypeSpec gMLTEEvents[] = { { kEventClassTextInput, kEventTextInputUnicodeForKeyEvent } };
#define kMLTEEventCount (sizeof( gMLTEEvents ) / sizeof( EventTypeSpec ))


short IsArrowKeyCode(short code) {
	if ((code == left_key) || (code == right_key) || (code == down_key) || (code == up_key))
		return TRUE;
	return FALSE;
}

void GetMLTETextSelection(ControlRef iCtrl, ControlEditTextSelectionRec *selection) {
	STPTextPaneVars **tpvars, *varsp;
	char		state;
	TXNOffset	oStartOffset, oEndOffset;

	tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
	if (tpvars != NULL) {
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		varsp = *tpvars;
		TXNGetSelection(varsp->fTXNRec, &oStartOffset, &oEndOffset); 
		HUnlock((Handle) tpvars);
		HSetState((Handle) tpvars, state);
		selection->selStart = oStartOffset;
		selection->selEnd   = oEndOffset;
	}
}

void SetMLTETextSelection(ControlRef iCtrl, ControlEditTextSelectionRec *selection) {
	STPTextPaneVars **tpvars, *varsp;
	char		state;
	TXNOffset	oStartOffset, oEndOffset;

	tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
	if (tpvars != NULL) {
		oStartOffset = selection->selStart;
		oEndOffset   = selection->selEnd;
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		varsp = *tpvars;
		TXNSetSelection(varsp->fTXNRec, oStartOffset, oEndOffset);
		HUnlock((Handle) tpvars);
		HSetState((Handle) tpvars, state);
	}
}

int AgmentCurrentMLTEText(ControlRef iCtrl, EventRecord *event, char *str, short var) {
	int  i, len;
	char varSt[128];
	ControlEditTextSelectionRec selection;

	if (event != NULL) {
		if ((event->modifiers & optionKey) || (event->modifiers & cmdKey))
			return(0);
	}
	if (var == 8) {
		varSt[0] = EOS;
		GetMLTETextSelection(iCtrl, &selection);
		if (selection.selStart < selection.selEnd)
			strcpy(str+selection.selStart, str+selection.selEnd);
		else if (selection.selStart > 0)
			strcpy(str+selection.selStart-1, str+selection.selStart);
	} else if (var == 0x7F) {
		varSt[0] = EOS;
		GetMLTETextSelection(iCtrl, &selection);
		if (selection.selStart < selection.selEnd)
			strcpy(str+selection.selStart, str+selection.selEnd);
		else if (str[selection.selStart] != EOS)
			strcpy(str+selection.selStart, str+selection.selStart+1);
	} else if (var >= 0x20) {
		UnicodeToUTF8((wchar_t *)&var, 1, (unsigned char *)varSt, NULL, 128);
		GetMLTETextSelection(iCtrl, &selection);
		if (selection.selStart < selection.selEnd)
			strcpy(str+selection.selStart, str+selection.selEnd);

		len = strlen(varSt);
		if (len > 0) {
			uS.shiftright(str+selection.selStart, len);
			for (i=0; i < len; i++)
				str[selection.selStart++] = varSt[i];
		}
	}
	return(1);
}

void SetWindowMLTEText(const char *text, WindowPtr win, SInt32 id) {
	HIViewID	viewID;
	ControlRef	iCtrl;
	OSStatus	err;
	char		state;
	long		len;
	TXNOffset	oStartOffset, oEndOffset;
	STPTextPaneVars **tpvars, *varsp;

	viewID.signature = 0;
	viewID.id = id;
	err = HIViewFindByID(HIViewGetRoot(win), viewID, &iCtrl);
	tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
	if (tpvars != NULL) {
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		varsp = *tpvars;
		len = strlen(text);
		if (len > 0) {
			TXNSelectAll(varsp->fTXNRec);
			TXNClear(varsp->fTXNRec);
			TXNGetSelection(varsp->fTXNRec, &oStartOffset, &oEndOffset); 
			TXNSetData(varsp->fTXNRec, kTXNTextData, text, len, oStartOffset, oEndOffset);
		}
		HUnlock((Handle) tpvars);
		HSetState((Handle) tpvars, state);
	}
}

void GetWindowMLTEText(WindowPtr win, SInt32 id, int maxLen, char *text) {
	HIViewID	viewID;
	ControlRef	iCtrl;
	OSStatus	err;
	char		state;
    TXNOffset	oStartOffset, oEndOffset;
    TXNOffset	cStartOffset, cEndOffset;
    Handle		DataHandle;
	STPTextPaneVars **tpvars, *varsp;

	text[0] = EOS;
	viewID.signature = 0;
	viewID.id = id;
	err = HIViewFindByID(HIViewGetRoot(win), viewID, &iCtrl);
	if (err == noErr) {
		tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
		if (tpvars != NULL) {
			state = HGetState((Handle) tpvars);
			HLock((Handle) tpvars);
			varsp = *tpvars;
			TXNGetSelection(varsp->fTXNRec, &oStartOffset, &oEndOffset);
			TXNSelectAll(varsp->fTXNRec);
			TXNGetSelection(varsp->fTXNRec, &cStartOffset, &cEndOffset);
			TXNGetDataEncoded(varsp->fTXNRec,cStartOffset,cEndOffset,&DataHandle,kTXNTextData);
			TXNSetSelection(varsp->fTXNRec, oStartOffset, oEndOffset);
			if (DataHandle == NULL) {
				HUnlock((Handle) tpvars);
				HSetState((Handle) tpvars, state);
				return;
			}

			HLock(DataHandle);
			strncpy(text, (char *)*DataHandle, cEndOffset);
			text[cEndOffset] = EOS;
			HUnlock(DataHandle);

			DisposeHandle(DataHandle);
			HUnlock((Handle) tpvars);
			HSetState((Handle) tpvars, state);
		}
	}
}

void SelectWindowMLTEText(WindowPtr win, SInt32 id, SInt16 beg, SInt16 end) {
	HIViewID	viewID;
	ControlRef	iCtrl;
    TXNOffset	oStartOffset, oEndOffset;
	OSStatus	err;
	char		state;
	STPTextPaneVars **tpvars, *varsp;

	viewID.signature = 0;
	viewID.id = id;
	err = HIViewFindByID(HIViewGetRoot(win), viewID, &iCtrl);
	if (err == noErr) {
		tpvars = (STPTextPaneVars **) GetControlReference(iCtrl);
		if (tpvars != NULL) {
			state = HGetState((Handle) tpvars);
			HLock((Handle) tpvars);
			varsp = *tpvars;
			oStartOffset = beg;
			oEndOffset   = end;
			TXNSetSelection(varsp->fTXNRec, oStartOffset, oEndOffset);
			HUnlock((Handle) tpvars);
			HSetState((Handle) tpvars, state);
		}
	}
}

/* TPActivatePaneText activates or deactivates the text edit record
	according to the value of setActive.  The primary purpose of this
	routine is to ensure each call is only made once. */
static void TPActivatePaneText(STPTextPaneVars **tpvars, Boolean setActive) {
	STPTextPaneVars *varsp;
	varsp = *tpvars;
	if (varsp->fTEActive != setActive) {
	
		varsp->fTEActive = setActive;
		
		TXNActivate(varsp->fTXNRec, varsp->fTXNFrame, varsp->fTEActive);
		
		if (varsp->fInFocus)
			TXNFocus( varsp->fTXNRec, varsp->fTEActive);
	}
}


/* TPFocusPaneText set the focus state for the text record. */
static void TPFocusPaneText(STPTextPaneVars **tpvars, Boolean setFocus) {
	STPTextPaneVars *varsp;
	varsp = *tpvars;
	if (varsp->fInFocus != setFocus) {
		varsp->fInFocus = setFocus;
		TXNFocus( varsp->fTXNRec, varsp->fInFocus);
	}
}


/* TPPaneDrawProc is called to redraw the control and for update events
	referring to the control.  This routine erases the text area's background,
	and redraws the text.  This routine assumes the scroll bar has been
	redrawn by a call to DrawControls. */
static pascal void TPPaneDrawProc(ControlRef theControl, ControlPartCode thePart) {
	STPTextPaneVars **tpvars, *varsp;
	char state;
	Rect bounds;
		/* set up our globals */
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
	if (tpvars != NULL) {
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		varsp = *tpvars;
			
			/* save the drawing state */
		SetPort((**tpvars).fDrawingEnvironment);
			/* verify our boundary */
		GetControlBounds(theControl, &bounds);
		if ( ! EqualRect(&bounds, &varsp->fRTextArea) ) {
			SetRect(&varsp->fRFocusOutline, bounds.left, bounds.top, bounds.right, bounds.bottom);
			SetRect(&varsp->fRTextOutline, bounds.left, bounds.top, bounds.right, bounds.bottom);
			SetRect(&varsp->fRTextArea, bounds.left, bounds.top, bounds.right, bounds.bottom);
			RectRgn(varsp->fTextBackgroundRgn, &varsp->fRTextOutline);
			TXNSetFrameBounds(  varsp->fTXNRec, bounds.top, bounds.left, bounds.bottom, bounds.right, varsp->fTXNFrame);
		}

			/* update the text region */
		EraseRgn(varsp->fTextBackgroundRgn);
		TXNDraw(varsp->fTXNRec, NULL);
			/* restore the drawing environment */
			/* draw the text frame and focus frame (if necessary) */
		DrawThemeEditTextFrame(&varsp->fRTextOutline, varsp->fIsActive ? kThemeStateActive: kThemeStateInactive);
//		if ((**tpvars).fIsActive && varsp->fInFocus) DrawThemeFocusRect(&varsp->fRFocusOutline, true);
			/* release our globals */
		HSetState((Handle) tpvars, state);
	}
}


/* TPPaneHitTestProc is called when the control manager would
	like to determine what part of the control the mouse resides over.
	We also call this routine from our tracking proc to determine how
	to handle mouse clicks. */
static pascal ControlPartCode TPPaneHitTestProc(ControlRef theControl, Point where) {
	STPTextPaneVars **tpvars;
	ControlPartCode result;
	char state;
		/* set up our locals and lock down our globals*/
	result = 0;
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
	if (tpvars != NULL) {
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
			/* find the region where we clicked */
		if (PtInRect(where, &(**tpvars).fRTextArea)) {
			result = kmUPTextPart;
		} else result = 0;
			/* release oure globals */
		HSetState((Handle) tpvars, state);
	}
	return result;
}





/* TPPaneTrackingProc is called when the mouse is being held down
	over our control.  This routine handles clicks in the text area
	and in the scroll bar. */
static pascal ControlPartCode TPPaneTrackingProc(ControlRef theControl, Point startPt, ControlActionUPP actionProc) {
	STPTextPaneVars **tpvars, *varsp;
	char state;
	ControlPartCode partCodeResult;
	ControlEditTextSelectionRec selection;

		/* make sure we have some variables... */
	partCodeResult = 0;
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
	if (tpvars != NULL) {
			/* lock 'em down */
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		varsp = *tpvars;
			/* we don't do any of these functions unless we're in focus */
		if ( ! varsp->fInFocus) {
			WindowPtr owner;
			ControlID  outID;
			WindowProcRec *windProc;

			owner = GetControlOwner(theControl);
			ClearKeyboardFocus(owner);
			SetKeyboardFocus(owner, theControl, kUserClickedToFocusPart);
			windProc = WindowProcs(owner);
			if (windProc != NULL && windProc->id == 500 && GetControlID(theControl, &outID) == noErr) {
				SelectWindowMLTEText(owner, movieActive, HUGE_INTEGER, HUGE_INTEGER);
				if (movieActive != outID.id) {
					GetMLTETextSelection(theControl, &selection);
					saveMovieUndo(owner, selection.selStart, selection.selEnd, 1);
				}
				movieActive = outID.id;
				setMovieCursor(owner);
			}
		}
			/* find the location for the click */
		switch (TPPaneHitTestProc(theControl, startPt)) {
				
				/* handle clicks in the text part */
			case kmUPTextPart:
				{
					EventRecord *event;
					SetPort(varsp->fDrawingEnvironment);
					event = GetCurrentEventRecord();
					LocalToGlobal(&event->where);
					TXNClick(varsp->fTXNRec, event);
					GlobalToLocal(&event->where);
				}
				break;
			
		}
		HUnlock((Handle) tpvars);
		HSetState((Handle) tpvars, state);
	}
	return partCodeResult;
}


/* TPPaneIdleProc is our user pane idle routine.  When our text field
	is active and in focus, we use this routine to set the cursor. */
static pascal void TPPaneIdleProc(ControlRef theControl) {
	STPTextPaneVars **tpvars, *varsp;
		/* set up locals */
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
	if (tpvars != NULL) {
			/* if we're not active, then we have nothing to say about the cursor */
		if ((**tpvars).fIsActive) {
			char state;
			Rect bounds;
			Point mousep;
				/* lock down the globals */
			state = HGetState((Handle) tpvars);
			HLock((Handle) tpvars);
			varsp = *tpvars;
				/* get the current mouse coordinates (in our window) */
			SetPort(GetWindowPort(GetControlOwner(theControl)));
			GetMouse(&mousep);
				/* there's a 'focus thing' and an 'unfocused thing' */
			if (varsp->fInFocus) {
					/* flash the cursor */
				SetPort((**tpvars).fDrawingEnvironment);
				TXNIdle(varsp->fTXNRec);
				/* set the cursor */
				if (PtInRect(mousep, &varsp->fRTextArea)) {
					RgnHandle theRgn;
					RectRgn((theRgn = NewRgn()), &varsp->fRTextArea);
					TXNAdjustCursor(varsp->fTXNRec, theRgn);
					DisposeRgn(theRgn);
				 } else SetThemeCursor(kThemeArrowCursor);
			} else {
				/* if it's in our bounds, set the cursor */
				GetControlBounds(theControl, &bounds);
				if (PtInRect(mousep, &bounds))
					SetThemeCursor(kThemeArrowCursor);
			}
			
			HSetState((Handle) tpvars, state);
		}
	}
}


/* TPPaneKeyDownProc is called whenever a keydown event is directed
	at our control.  Here, we direct the keydown event to the text
	edit record and redraw the scroll bar and text field as appropriate. */
static pascal ControlPartCode TPPaneKeyDownProc(ControlRef theControl, SInt16 keyCode, SInt16 charCode, SInt16 modifiers) {
	STPTextPaneVars **tpvars;
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
	if (tpvars != NULL) {
		if ((**tpvars).fInFocus) {
				/* turn autoscrolling on and send the key event to text edit */
			SetPort((**tpvars).fDrawingEnvironment);
			TXNKeyDown( (**tpvars).fTXNRec, GetCurrentEventRecord());
		}
	}
	return kControlEntireControl;
}


/* TPPaneActivateProc is called when the window containing
	the user pane control receives activate events. Here, we redraw
	the control and it's text as necessary for the activation state. */
static pascal void TPPaneActivateProc(ControlRef theControl, Boolean activating) {
	Rect bounds;
	STPTextPaneVars **tpvars, *varsp;
	char state;
		/* set up locals */
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
	if (tpvars != NULL) {
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		varsp = *tpvars;
			/* de/activate the text edit record */
		SetPort((**tpvars).fDrawingEnvironment);
		GetControlBounds(theControl, &bounds);
		varsp->fIsActive = activating;
		TPActivatePaneText(tpvars, varsp->fIsActive && varsp->fInFocus);
			/* redraw the frame */
		DrawThemeEditTextFrame(&varsp->fRTextOutline, varsp->fIsActive ? kThemeStateActive: kThemeStateInactive);
//		if (varsp->fInFocus) DrawThemeFocusRect(&varsp->fRFocusOutline, varsp->fIsActive);
		HSetState((Handle) tpvars, state);
	}
}


/* TPPaneFocusProc is called when ever the focus changes to or
	from our control.  Herein, switch the focus appropriately
	according to the parameters and redraw the control as
	necessary.  */
static pascal ControlPartCode TPPaneFocusProc(ControlRef theControl, ControlFocusPart action) {
	ControlPartCode focusResult;
	STPTextPaneVars **tpvars, *varsp;
	char state;
		/* set up locals */
	focusResult = kControlFocusNoPart;
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
	if (tpvars != NULL) {
		state = HGetState((Handle) tpvars);
		HLock((Handle) tpvars);
		varsp = *tpvars;
			/* if kControlFocusPrevPart and kControlFocusNextPart are received when the user is
			tabbing forwards (or shift tabbing backwards) through the items in the dialog,
			and kControlFocusNextPart will be received.  When the user clicks in our field
			and it is not the current focus, then the constant kUserClickedToFocusPart will
			be received.  The constant kControlFocusNoPart will be received when our control
			is the current focus and the user clicks in another control.  In your focus routine,
			you should respond to these codes as follows:

			kControlFocusNoPart - turn off focus and return kControlFocusNoPart.  redraw
				the control and the focus rectangle as necessary.

			kControlFocusPrevPart or kControlFocusNextPart - toggle focus on or off
				depending on its current state.  redraw the control and the focus rectangle
				as appropriate for the new focus state.  If the focus state is 'off', return the constant
				kControlFocusNoPart, otherwise return a non-zero part code.
			kUserClickedToFocusPart - is a constant defined for this example.  You should
				define your own value for handling click-to-focus type events. */
			/* save the drawing state */
		SetPort((**tpvars).fDrawingEnvironment);
			/* calculate the next highlight state */
		switch (action) {
			default:
			case kControlFocusNoPart:
				TPFocusPaneText(tpvars, false);
				focusResult = kControlFocusNoPart;
				break;
			case kUserClickedToFocusPart:
				TPFocusPaneText(tpvars, true);
				focusResult = 1;
				break;
			case kControlFocusPrevPart:
			case kControlFocusNextPart:
				TPFocusPaneText(tpvars, ( ! varsp->fInFocus));
				focusResult = varsp->fInFocus ? 1 : kControlFocusNoPart;
				break;
		}
		TPActivatePaneText(tpvars, varsp->fIsActive && varsp->fInFocus);
			/* redraw the text fram and focus rectangle to indicate the
			new focus state */
		DrawThemeEditTextFrame(&varsp->fRTextOutline, varsp->fIsActive ? kThemeStateActive: kThemeStateInactive);
//		DrawThemeFocusRect(&varsp->fRFocusOutline, varsp->fIsActive && varsp->fInFocus);
			/* done */
		HSetState((Handle) tpvars, state);
	}
	return focusResult;
}


//This our carbon event handler for unicode key downs
static pascal OSStatus FocusAdvanceOverride(EventHandlerCallRef myHandler, EventRef event, void* userData) {
	WindowPtr window;
	STPTextPaneVars **tpvars;
	OSStatus err;
	EventRef eventRaw;
	EventRecord myEvent;
	unsigned short mUnicodeText;
	unsigned long charCounts=0L;
	ACT_FUNC_INFO *keyActions;

		/* get our window pointer */
	tpvars = (STPTextPaneVars **) userData;
	window = (**tpvars).fOwner;
	if ( ! (**tpvars).fInFocus)
		return eventNotHandledErr;

		//find out how many bytes are needed
	err = GetEventParameter(event, kEventParamTextInputSendText,
				typeUnicodeText, NULL, 0, &charCounts, NULL);
	if (err != noErr) goto bail;
		/* we're only looking at single characters */
	if (charCounts != 2) { err = eventNotHandledErr; goto bail; }
		/* get the character */
	err = GetEventParameter(event, kEventParamTextInputSendText, 
				typeUnicodeText, NULL, sizeof(mUnicodeText),
				&charCounts, (char*) &mUnicodeText);
	if (err != noErr) goto bail;
	err = GetEventParameter(event, kEventParamTextInputSendKeyboardEvent, 
				typeEventRef, NULL, sizeof(eventRaw), NULL, (char*) &eventRaw);

	keyActions = (**tpvars).keyActions;

		/* if it's not the tab key, forget it... */
	if (mUnicodeText == '\t') {
			/* advance the keyboard focus */
		if (keyActions[0].id == 500) {
			ControlRef iCtrl;
			ControlID  outID;

			GetKeyboardFocus(window, &iCtrl);
			if (iCtrl != NULL && GetControlID(iCtrl, &outID) == noErr) {
				SelectWindowMLTEText(window, outID.id, HUGE_INTEGER, HUGE_INTEGER);
				if (outID.id == 4 || outID.id == 8 || outID.id == 0 || outID.id == 5)
					outID.id = 3;
				else
					outID.id = 4;
				SetKeyboardFocus(window, GetWindowItemAsControl(window, outID.id), kUserClickedToFocusPart);
				SelectWindowMLTEText(window, outID.id, 0, HUGE_INTEGER);
				movieActive = outID.id;
				if (!ConvertEventRefToEventRecord(eventRaw, &myEvent)) {
					myEvent.message = mUnicodeText;
					myEvent.what = keyDown;
					myEvent.modifiers = GetCurrentEventKeyModifiers();
				}
				if ((*keyActions->act_func_ptr)(keyActions->win, &myEvent, mUnicodeText))
					return noErr;
			} else
				AdvanceKeyboardFocus(window);
		} else
			AdvanceKeyboardFocus(window);
		return noErr;
	} else {
		if (keyActions != nil) {
			if (!ConvertEventRefToEventRecord(eventRaw, &myEvent)) {
				myEvent.message = mUnicodeText;
				myEvent.what = keyDown;
				myEvent.modifiers = GetCurrentEventKeyModifiers();
			}
			if ((*keyActions->act_func_ptr)(keyActions->win, &myEvent, mUnicodeText))
				return noErr;
		}

		err = eventNotHandledErr;
		goto bail;
	}
bail:
	return eventNotHandledErr;
}

/*
			UInt32 eventClass;
			UInt32 eventKind;

			eventClass = GetEventClass( eventRaw );
			eventKind = GetEventKind( eventRaw );
			if ( eventClass == kEventClassKeyboard && (eventKind == kEventRawKeyDown || eventKind == kEventRawKeyRepeat)) {
				UInt32 keyCode;
				unsigned char charCode;
				UInt32 modifiers;

				//  Extract the key code parameter (kEventParamKeyCode).

				GetEventParameter( event, kEventParamKeyCode, typeUInt32, nil, sizeof (keyCode),
				                   nil, &keyCode );

				//  Extract the character code parameter (kEventParamKeyMacCharCodes).

				GetEventParameter( event, kEventParamKeyMacCharCodes, typeChar, nil,
				                   sizeof( charCode ), nil, &charCode );

				//  Extract the modifiers parameter (kEventParamKeyModifiers).

				GetEventParameter( event, kEventParamKeyModifiers, typeUInt32, nil,
				                   sizeof( modifiers ), nil, &modifiers );
			}
*/


/* mUPOpenControl initializes a user pane control so it will be drawn
	and will behave as a scrolling text edit field inside of a window.
	This routine performs all of the initialization steps necessary,
	except it does not create the user pane control itself.  theControl
	should refer to a user pane control that you have either created
	yourself or extracted from a dialog's control heirarchy using
	the GetDialogItemAsControl routine.  */
OSStatus mUPOpenControl(ControlRef theControl, short FName, short FSize, char isUTF, ACT_FUNC_INFO *keyActions) {
	Rect bounds;
	WindowPtr theWindow;
	STPTextPaneVars **tpvars, *varsp;
	OSStatus err;
	RGBColor rgbWhite = {0xFFFF, 0xFFFF, 0xFFFF};
	TXNBackground tback;

		/* set up our globals */
	if (gTPDrawProc == NULL) gTPDrawProc = NewControlUserPaneDrawUPP(TPPaneDrawProc);
	if (gTPHitProc == NULL) gTPHitProc = NewControlUserPaneHitTestUPP(TPPaneHitTestProc);
	if (gTPTrackProc == NULL) gTPTrackProc = NewControlUserPaneTrackingUPP(TPPaneTrackingProc);
	if (gTPIdleProc == NULL) gTPIdleProc = NewControlUserPaneIdleUPP(TPPaneIdleProc);
	if (gTPKeyProc == NULL) gTPKeyProc = NewControlUserPaneKeyDownUPP(TPPaneKeyDownProc);
	if (gTPActivateProc == NULL) gTPActivateProc = NewControlUserPaneActivateUPP(TPPaneActivateProc);
	if (gTPFocusProc == NULL) gTPFocusProc = NewControlUserPaneFocusUPP(TPPaneFocusProc);
		
		/* allocate our private storage */
	tpvars = (STPTextPaneVars **) NewHandleClear(sizeof(STPTextPaneVars));
	SetControlReference(theControl, (long) tpvars);
	HLock((Handle) tpvars);
	varsp = *tpvars;
		/* set the initial settings for our private data */
	varsp->fInFocus = false;
	varsp->fIsActive = true;
	varsp->fTEActive = false;
	varsp->keyActions = keyActions;

	varsp->fUserPaneRec = theControl;
	theWindow = varsp->fOwner = GetControlOwner(theControl);
	varsp->fDrawingEnvironment = GetWindowPort(varsp->fOwner);
	varsp->fInDialogWindow = ( GetWindowKind(varsp->fOwner) == kDialogWindowKind );
		/* set up the user pane procedures */
	SetControlData(theControl, kControlEntireControl, kControlUserPaneDrawProcTag, sizeof(gTPDrawProc), &gTPDrawProc);
	SetControlData(theControl, kControlEntireControl, kControlUserPaneHitTestProcTag, sizeof(gTPHitProc), &gTPHitProc);
	SetControlData(theControl, kControlEntireControl, kControlUserPaneTrackingProcTag, sizeof(gTPTrackProc), &gTPTrackProc);
	SetControlData(theControl, kControlEntireControl, kControlUserPaneIdleProcTag, sizeof(gTPIdleProc), &gTPIdleProc);
	SetControlData(theControl, kControlEntireControl, kControlUserPaneKeyDownProcTag, sizeof(gTPKeyProc), &gTPKeyProc);
	SetControlData(theControl, kControlEntireControl, kControlUserPaneActivateProcTag, sizeof(gTPActivateProc), &gTPActivateProc);
	SetControlData(theControl, kControlEntireControl, kControlUserPaneFocusProcTag, sizeof(gTPFocusProc), &gTPFocusProc);
		/* calculate the rectangles used by the control */
	GetControlBounds(theControl, &bounds);
	SetRect(&varsp->fRFocusOutline, bounds.left, bounds.top, bounds.right, bounds.bottom);
	SetRect(&varsp->fRTextOutline, bounds.left, bounds.top-1, bounds.right, bounds.bottom);
	SetRect(&varsp->fRTextArea, bounds.left, bounds.top, bounds.right, bounds.bottom);
		/* calculate the background region for the text.  In this case, it's kindof
		and irregular region because we're setting the scroll bar a little ways inside
		of the text area. */
	RectRgn((varsp->fTextBackgroundRgn = NewRgn()), &varsp->fRTextOutline);

		/* set up the drawing environment */
	SetPort(varsp->fDrawingEnvironment);

		/* create the new edit field */
/*
extern OSStatus 
TXNCreateObject(
  const HIRect *    iFrameRect,
  TXNFrameOptions   iFrameOptions,
  TXNObject *       oTXNObject)

extern OSStatus 
TXNAttachObjectToWindowRef(
  TXNObject   iTXNObject,
  WindowPtr   iWindowRef)
*/

	TXNNewObject(NULL, varsp->fOwner, &varsp->fRTextArea,
		((keyActions->id == 501) ? kTXNAlwaysWrapAtViewEdgeMask : kTXNSingleLineOnlyMask),
		kTXNTextEditStyleFrameType,
		kTXNTextensionFile,
		kTXNUnicodeEncoding, // kTXNSystemDefaultEncoding, 
		&varsp->fTXNRec, &varsp->fTXNFrame, (TXNObjectRefcon) tpvars);

/* set the font */
	TXNTypeAttributes iAttributes[2];

	Str255 pFontName;
	char FontName[256];
	GetFontName(FName, pFontName);
/*
	iAttributes[1].tag  = kTXNQDFontNameAttribute;
	iAttributes[1].size = kTXNQDFontNameAttributeSize;
	iAttributes[1].data.dataPtr = temp;
*/
	p2cstrcpy(FontName, pFontName);
	if (strcmp(FontName, "Ascender Uni Duo") == 0) {
		iAttributes[0].tag  = kTXNQDFontSizeAttribute;
		iAttributes[0].size = kTXNQDFontSizeAttributeSize;
		iAttributes[0].data.dataValue = FSize;

		err = TXNSetTypeAttributes (varsp->fTXNRec, 1, iAttributes, kTXNUseCurrentSelection, kTXNUseCurrentSelection); 
	} else {
/*
		iAttributes[1].tag  = kTXNQDFontFamilyIDAttribute;
		iAttributes[1].size = kTXNQDFontFamilyIDAttributeSize;
		iAttributes[1].data.dataValue = FName;
*/
		iAttributes[0].tag  = kTXNQDFontSizeAttribute;
		iAttributes[0].size = kTXNQDFontSizeAttributeSize;
		iAttributes[0].data.dataValue = FSize;

		err = TXNSetTypeAttributes (varsp->fTXNRec, 1, iAttributes, kTXNUseCurrentSelection, kTXNUseCurrentSelection); 
	}
/*
	ItemCount ioCount;
	TXNMacOSPreferredFontDescription tFontDefaults[25];
	TXNGetFontDefaults(varsp->fTXNRec, &ioCount, NULL);
	if (ioCount > 0 && ioCount < 25) {
		TXNGetFontDefaults(varsp->fTXNRec, &ioCount, tFontDefaults);
		tFontDefaults[0].pointSize = Long2Fix(FSize);
		err = TXNSetFontDefaults(varsp->fTXNRec, ioCount, tFontDefaults); 
	}
*/
/* set default font
	TXNMacOSPreferredFontDescription iFontDefaults[1];
	unsigned short t = (unsigned short)FName;
	iFontDefaults[0].fontID = (UInt32)t;
	
//	ATSUFontID theFontID;
//	err = ATSUFONDtoFontID(FName, NULL, &theFontID);
	iFontDefaults[0].fontID = kTXNUseScriptDefaultValue ;//theFontID;
	iFontDefaults[0].pointSize = kTXNUseScriptDefaultValue ;//Long2Fix(FSize);
//	if (isUTF)
		iFontDefaults[0].encoding = smUnicodeScript;
//	else
//		iFontDefaults[0].encoding = FontToScript(FName);
	iFontDefaults[0].fontStyle = kTXNUseScriptDefaultValue; //0;
	err = TXNSetFontDefaults(varsp->fTXNRec, 1, iFontDefaults); 
*/


		/* set the field's background */
	tback.bgType = kTXNBackgroundTypeRGB;
	tback.bg.color = rgbWhite;
	TXNSetBackground( varsp->fTXNRec, &tback);
	
		/* install our focus advance override routine */
	varsp->handlerUPP = NewEventHandlerUPP(FocusAdvanceOverride);
	err = InstallWindowEventHandler( varsp->fOwner, varsp->handlerUPP,
		kMLTEEventCount, gMLTEEvents, tpvars, &varsp->handlerRef );

		/* unlock our storage */
	HUnlock((Handle) tpvars);
		/* perform final activations and setup for our text field.  Here,
		we assume that the window is going to be the 'active' window. */
	TPActivatePaneText(tpvars, varsp->fIsActive && varsp->fInFocus);
		/* all done */

	return noErr;
}



/* mUPCloseControl deallocates all of the structures allocated
	by mUPOpenControl.  */
OSStatus mUPCloseControl(ControlRef theControl) {
	STPTextPaneVars **tpvars;
		/* set up locals */
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
		/* release our sub records */
	TXNDeleteObject((**tpvars).fTXNRec);
		/* remove our focus advance override */
	RemoveEventHandler((**tpvars).handlerRef);
	DisposeEventHandlerUPP((**tpvars).handlerUPP);
		/* delete our private storage */
	DisposeHandle((Handle) tpvars);
		/* zero the control reference */
	SetControlReference(theControl, 0);
	return noErr;
}




	/* mUPSetText replaces the contents of the selection with the unicode
	text described by the text and count parameters:.
		text = pointer to unicode text buffer
		count = number of bytes in the buffer.  */
OSStatus mUPSetText(ControlRef theControl, char* text, long count) {
	STPTextPaneVars **tpvars;
		/* set up locals */
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
		/* set the text in the record */
	return TXNSetData( (**tpvars).fTXNRec, kTXNUnicodeTextData, text, count,
		kTXNUseCurrentSelection, kTXNUseCurrentSelection);

	return noErr;
}


/* mUPSetSelection sets the text selection and autoscrolls the text view
	so either the cursor or the selction is in the view. */
void mUPSetSelection(ControlRef theControl, long selStart, long selEnd) {
	STPTextPaneVars **tpvars;
		/* set up our locals */
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
		/* and our drawing environment as the operation
		may force a redraw in the text area. */
	SetPort((**tpvars).fDrawingEnvironment);
		/* change the selection */
	TXNSetSelection( (**tpvars).fTXNRec, selStart, selEnd);
}





/* mUPGetText returns the current text data being displayed inside of
	the mUPControl.  When noErr is returned, *theText contain a new
	handle containing all of the Unicode text copied from the current
	selection.  It is the caller's responsibiliby to dispose of this handle. */
OSStatus mUPGetText(ControlRef theControl, Handle *theText) {
	STPTextPaneVars **tpvars;
	OSStatus err;
		/* set up locals */
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
		/* extract the text from the record */
	err = TXNGetData( (**tpvars).fTXNRec, kTXNUseCurrentSelection, kTXNUseCurrentSelection, theText);
		/* all done */
	return err;
}



/* mUPCreateControl creates a new user pane control and then it passes it
	to mUPOpenControl to initialize it as a scrolling text user pane control. */
OSStatus mUPCreateControl(WindowPtr theWindow, Rect *bounds, ControlRef *theControl) {
//	short featurSet;
		/* the following feature set can be specified in CNTL resources by using
		the value 1214.  When creating a user pane control, we pass this value
		in the 'value' parameter. */
/*
	featurSet = kControlSupportsEmbedding | kControlSupportsFocus | kControlWantsIdle
			| kControlWantsActivate | kControlHandlesTracking | kControlHasSpecialBackground
			| kControlGetsFocusOnClick | kControlSupportsLiveFeedback;
*/
		// create the control
//	*theControl = NewControl(theWindow, bounds, "\p", true, featurSet, 0, featurSet, kControlUserPaneProc, 0);
		// set up the mUP specific features and data
//	mUPOpenControl(*theControl, nil);
		// all done....
	return noErr;
}


/* mUPDisposeControl calls mUPCloseControl and then it calls DisposeControl. */
OSStatus mUPDisposeControl(ControlRef theControl) {
		/* deallocate the mUP specific data */
	mUPCloseControl(theControl);
		/* deallocate the user pane control itself */
	DisposeControl(theControl);
	return noErr;
}




/* IsmUPControl returns true if theControl is not NULL
	and theControl refers to a mUP Control.  */
Boolean IsmUPControl(ControlRef theControl) {
	Size theSize;
	ControlUserPaneFocusUPP localFocusProc;
		/* a NULL control is not a mUP control */
	if (theControl == NULL) return false;
		/* check if the control is using our focus procedure */
	theSize = sizeof(localFocusProc);
	if (GetControlData(theControl, kControlEntireControl, kControlUserPaneFocusProcTag,
		sizeof(localFocusProc), &localFocusProc, &theSize) != noErr) return false;
	if (localFocusProc != gTPFocusProc) return false;
		/* all tests passed, it's a mUP control */
	return true;
}


/* mUPDoEditCommand performs the editing command specified
	in the editCommand parameter.  The mUPControl's text
	and scroll bar are redrawn and updated as necessary. */
void mUPDoEditCommand(ControlRef theControl, short editCommand) {
	STPTextPaneVars **tpvars;
		/* set up our locals */
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
		/* and our drawing environment as the operation
		may force a redraw in the text area. */
	SetPort((**tpvars).fDrawingEnvironment);
		/* perform the editing command */
	switch (editCommand) {
		case kmUPCut:
			ClearCurrentScrap();
			TXNCut((**tpvars).fTXNRec); 
			TXNConvertToPublicScrap();
			break;
		case kmUPCopy:
			ClearCurrentScrap();
			TXNCopy((**tpvars).fTXNRec);
			TXNConvertToPublicScrap();
			break;
		case kmUPPaste:
			TXNConvertFromPublicScrap();
			TXNPaste((**tpvars).fTXNRec);
			break;
		case kmUPClear:
			TXNClear((**tpvars).fTXNRec);
			break;
	}
}




/* mUPGetContents returns the entire contents of the control including the text
	and the formatting information. */
OSStatus mUPGetContents(ControlRef theControl, Handle *theContents) {
	STPTextPaneVars **tpvars;
	OSStatus err;
	short vRefNum;
	long dirID;
	FSSpec tspec;
	short trefnum;
	Boolean texists;
	long bytecount;
	Handle localdata;
		/* set up locals */
	trefnum = 0;
	texists = false;
	localdata = NULL;
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
	if (theContents == NULL) return paramErr;
		/* create a temporary file */
	err = FindFolder(kOnSystemDisk, kTemporaryFolderType, true, &vRefNum, &dirID);
	if (err != noErr) goto bail;
	FSMakeFSSpec(vRefNum, dirID, "\pmUPGetContents", &tspec);
	err = FSpCreate(&tspec, 'trsh', 'trsh', smSystemScript);
	if (err != noErr) goto bail;
	texists = true;
		/* open the file */
	err = FSpOpenDF(&tspec, fsRdWrPerm, &trefnum);
	if (err != noErr) goto bail;
		/* save the data */
	err = TXNSave( (**tpvars).fTXNRec, kTXNTextensionFile, 0, kTXNSystemDefaultEncoding, &tspec, trefnum, 0);
	if (err != noErr) goto bail;
		/* get the file length and set the position */
	err = GetEOF(trefnum, &bytecount);
	if (err != noErr) goto bail;
	err = SetFPos(trefnum, fsFromStart, 0);
	if (err != noErr) goto bail;
		/* copy the data fork to a handle */
	localdata = NewHandle(bytecount);
	if (localdata == NULL) { err = memFullErr; goto bail; }
	HLock(localdata);
	err = FSRead(trefnum, &bytecount, *localdata);
	HUnlock(localdata);
	if (err != noErr) goto bail;
		/* store result */
	*theContents = localdata;
		/* clean up */
	FSClose(trefnum);
	FSpDelete(&tspec);
		/* all done */
	return noErr;
bail:
	if (trefnum != 0) FSClose(trefnum);
	if (texists) FSpDelete(&tspec);
	if (localdata != NULL) DisposeHandle(localdata);
	return err;
}




/* mUPSetContents replaces the contents of the selection with the data stored in the handle. */
OSStatus mUPSetContents(ControlRef theControl, Handle theContents) {
	STPTextPaneVars **tpvars;
	OSStatus err;
	short vRefNum;
	long dirID;
	FSSpec tspec;
	short trefnum;
	Boolean texists;
	long bytecount;
	char state;
		/* set up locals */
	trefnum = 0;
	texists = false;
	tpvars = (STPTextPaneVars **) GetControlReference(theControl);
	if (theContents == NULL) return paramErr;
		/* create a temporary file */
	err = FindFolder(kOnSystemDisk,  kTemporaryFolderType, true, &vRefNum, &dirID);
	if (err != noErr) goto bail;
	FSMakeFSSpec(vRefNum, dirID, "\pmUPSetContents", &tspec);
	err = FSpCreate(&tspec, 'trsh', 'trsh', smSystemScript);
	if (err != noErr) goto bail;
	texists = true;
		/* open the file */
	err = FSpOpenDF(&tspec, fsRdWrPerm, &trefnum);
	if (err != noErr) goto bail;
		/* save the data to the temporary file */
	state = HGetState(theContents);
	HLock(theContents);
	bytecount = GetHandleSize(theContents);
	err = FSWrite(trefnum, &bytecount, *theContents);
	HSetState(theContents, state);
	if (err != noErr) goto bail;
		/* reset the file position */
	err = SetFPos(trefnum, fsFromStart, 0);
	if (err != noErr) goto bail;
		/* load the data */
	err = TXNSetDataFromFile((**tpvars).fTXNRec, trefnum, kTXNTextensionFile, bytecount, kTXNUseCurrentSelection, kTXNUseCurrentSelection);
	if (err != noErr) goto bail;
		/* clean up */
	FSClose(trefnum);
	FSpDelete(&tspec);
		/* all done */
	return noErr;
bail:
	if (trefnum != 0) FSClose(trefnum);
	if (texists) FSpDelete(&tspec);
	return err;
}
