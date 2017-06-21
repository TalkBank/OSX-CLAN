#include "ced.h"
#include "c_clan.h"

static int   num_hlp_commands;
static int   local_num_rows, char_height;
static char  DoubleClickCount = 1;
static char  hlp_commands[256][SPEAKERLEN], userCom[33];
static long  lastWhen = 0;
static short  progIndex = -1, topIndex = 0;
static FNType gFileName[FNSize];
static Point lastWhere = {0,0};

static void CleanupHlp(WindowPtr wind) {
	topIndex = 0;
	progIndex = -1;
	DoubleClickCount = 1;
	gFileName[0] = EOS;
	userCom[0] = EOS;
}

static void DrawHlpHighlight(WindowPtr win) {
	register short i;
    register int print_row;
	Rect		theRect;
	Rect		rect;
	GrafPtr		oldPort;
	extern Boolean IsColorMonitor;

	GetPort(&oldPort);
	SetPortWindowPort(win);
	PenMode(((IsColorMonitor) ? 50 : notPatCopy));
	print_row = 0;
	for (i=topIndex; i < num_hlp_commands; i++) {
		print_row += char_height;
		if (progIndex == i)
			break;
	}
	if (progIndex != i || i >= local_num_rows+topIndex) {
		SetPort(oldPort);
		return;
	}
	theRect.left  = LEFTMARGIN;
	GetWindowPortBounds(win, &rect);
	theRect.right = rect.right - SCROLL_BAR_SIZE;
	theRect.bottom = print_row;
	theRect.top = theRect.bottom - char_height + FLINESPACE;
	theRect.bottom += FLINESPACE;
	PaintRect(&theRect);
	SetPort(oldPort);
}

static void SetHlpScrollControl(WindowPtr win) {
	GrafPtr			savePort;
	WindowProcRec	*windProc;

	GetPort(&savePort);
 	SetPortWindowPort(win);
 	windProc = WindowProcs(win);
 	if (windProc != NULL && windProc->VScrollHnd != NULL) {
		if (topIndex == 0 && local_num_rows >= num_hlp_commands) {
			HiliteControl(windProc->VScrollHnd, 255);
		} else {
			HiliteControl(windProc->VScrollHnd,0);
			SetControlMaximum(windProc->VScrollHnd, num_hlp_commands-1);
			SetControlValue(windProc->VScrollHnd, topIndex);
		}
	}
	SetPort(savePort);
}

static void UpdateHlp(WindowPtr win) {
    register int i;
    register int print_row;
    Rect theRect;
	Rect rect;
    GrafPtr oldPort;
	Point p = { 1, 1 };

    GetPort(&oldPort);
    SetPortWindowPort(win);

	TextFont(dFnt.fontId);
	TextSize(dFnt.fontSize);
	TextFace(0);

	GetWindowPortBounds(win, &rect);
	theRect.bottom = rect.bottom;
	theRect.top = rect.top;
	theRect.left = rect.left;
	theRect.right = rect.right - SCROLL_BAR_SIZE;

	local_num_rows = NumOfRowsInAWindow(win, &char_height, 0);
	EraseRect(&theRect);
	print_row = char_height;
	for (i=topIndex; i < num_hlp_commands && i < local_num_rows+topIndex; i++) {
		MoveTo(10, print_row);
		DrawJustified(hlp_commands[i], strlen(hlp_commands[i]), 0L, onlyStyleRun, p, p);
		print_row += char_height;
	}

	DrawHlpHighlight(win);
	SetHlpScrollControl(win);
	DrawGrowIcon(win);
	DrawControls(win);
    SetPort(oldPort);
}

void  HandleHlpVScrollBar(WindowPtr win, short code, ControlRef theControl, Point   myPt) {
    long  MaxTick;

	do {
		HiliteControl(theControl, code);
		switch (code) {
			case kControlUpButtonPart:
				if (topIndex > 0) {
					topIndex--;
					UpdateHlp(win);
				}
				break;

			case kControlDownButtonPart:
				if (topIndex < num_hlp_commands-1) {
					topIndex++;
					UpdateHlp(win);
				}
				break;

			case kControlPageUpPart:
				topIndex -= (local_num_rows / 2);
				if (topIndex < 0)
					topIndex = 0;
				UpdateHlp(win);
				break;

			case kControlPageDownPart:
				topIndex += (local_num_rows / 2);
				if (topIndex >= num_hlp_commands) 
					topIndex = num_hlp_commands - 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateHlp(win);
				break;

			case kControlIndicatorPart:
				code = TrackControl(theControl, myPt, NULL);
				topIndex = GetControlValue(theControl);
				if (topIndex >= num_hlp_commands)
					topIndex = num_hlp_commands - 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateHlp(win);
				break;	
		}

		SetHlpScrollControl(win);
//		if (code != kControlUpButtonPart && code != kControlDownButtonPart) {
			MaxTick = TickCount() + 10;
			do {  
				if (TickCount() > MaxTick) break;		 
			} while ( Button() == TRUE) ;
//		}
	} while (StillDown() == TRUE) ;
}

static void Do_Hlp_ScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    short RefCon;

	RefCon = GetControlReference(theControl);
	switch  (RefCon) {
		case I_VScroll_bar:
			HandleHlpVScrollBar(win,code,theControl,myPt);
			break;
/*	
		case I_HScroll_bar:
			HandleProgsHScrollBar(code,theControl,myPt);
			break;
*/
		default:
			break;
	}
}

static char *SelectHlpPosition(WindowPtr win, EventRecord *event) {
    register short i;
    register int print_row;
	register int h = event->where.h;
	register int v = event->where.v;
    Point LocalPtr;
	Rect rect;

	GetWindowPortBounds(win, &rect);
	if (v < 0 || h > rect.right - rect.left-SCROLL_BAR_SIZE)
		return(NULL);
	LocalPtr.h = h;
	LocalPtr.v = v;
	if (((event->when - lastWhen) < GetDblTime()) &&
	    (abs(LocalPtr.h - lastWhere.h) < 5) &&
	    (abs(LocalPtr.v - lastWhere.v) < 5)) {
	    if (++DoubleClickCount > 2)
	    	DoubleClickCount = 2;
	    while (StillDown() == TRUE) ;

		print_row = 0;
		for (i=topIndex; i < num_hlp_commands; i++) {
			print_row += char_height;
			if (v < print_row)
				break;
		}
		if (i >= local_num_rows+topIndex || i >= num_hlp_commands)
			return(NULL);

		DrawHlpHighlight(win);
		progIndex = i;
		DrawHlpHighlight(win);
		return(hlp_commands[i]);
	}
	DoubleClickCount = 1;
	lastWhen  = event->when;
	lastWhere = LocalPtr;

	print_row = 0;
	for (i=topIndex; i < num_hlp_commands; i++) {
		print_row += char_height;
		if (v < print_row)
			break;
	}
	if (i >= local_num_rows+topIndex || i >= num_hlp_commands)
		return(NULL);

	DrawHlpHighlight(win);
	progIndex = i;
	DrawHlpHighlight(win);
	return(NULL);
}

static short HlpEvent(WindowPtr win, EventRecord *event) {
	int				code;
	char			*line, *t, alreadyHighlighted;
	short			key;
	ControlRef		theControl;
	WindowPtr 		TextWin;
	WindowProcRec	*windProc;
	PrepareStruct	saveRec;

	windProc = WindowProcs(win);
	if (windProc == NULL)
		return(-1);
	
	alreadyHighlighted = FALSE;
	if (event->what == keyDown || event->what == autoKey) {
		key = (event->message & keyCodeMask)/256;
		if (key == up_key) {
			DrawHlpHighlight(win);
			progIndex--;
			if (progIndex < 0)
				progIndex = num_hlp_commands - 1;
			if (progIndex < topIndex) {
				topIndex--;
				if (topIndex < 0)
					topIndex = 0;
				UpdateHlp(win);
				alreadyHighlighted = TRUE;
			}
			if (progIndex >= local_num_rows+topIndex) {
				topIndex = progIndex - local_num_rows + 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateHlp(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawHlpHighlight(win);
		} else if (key == down_key) {
			DrawHlpHighlight(win);
			progIndex++;
			if (progIndex >= num_hlp_commands)
				progIndex = 0;
			if (progIndex < topIndex) {
				topIndex = 0;
				UpdateHlp(win);
				alreadyHighlighted = TRUE;
			}
			if (progIndex >= local_num_rows+topIndex) {
				topIndex++;
				UpdateHlp(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawHlpHighlight(win);
		} else {
			key = event->message & charCodeMask;
			if (key == RETURN_CHAR || key == ENTER_CHAR) {
				line = hlp_commands[progIndex];
				TextWin = FindAWindowNamed(gFileName);
				if (TextWin != NULL) {
					changeCurrentWindow(win, TextWin, true);
					PrepareWindA4(win, &saveRec);
					mCloseWindow(win);
					RestoreWindA4(&saveRec);
					t = strchr(line, ':');
					if (t != NULL)
						*t = EOS;
					isPlayS = getCommandNumber(line);
				}
			} else {
				code = strlen(userCom);
				if (key == DELETE_CHAR || key == BACKSPACE_CHAR) {
					userCom[code-1] = EOS;
				} else if (code < 32) {
					userCom[code++] = key;
					userCom[code] = EOS;
				}
				topIndex = 0;
				for (progIndex=0; progIndex < num_hlp_commands; progIndex++) {
					if (uS.mStrnicmp(hlp_commands[progIndex], userCom, strlen(userCom)) == 0) {
						break;
					}
				}
				DrawHlpHighlight(win);
				if (progIndex >= num_hlp_commands) {
					topIndex = 0;
					progIndex = 0;
					UpdateHlp(win);
					alreadyHighlighted = TRUE;
				}
				if (progIndex >= local_num_rows+topIndex) {
					topIndex = progIndex - (local_num_rows / 2);
					UpdateHlp(win);
					alreadyHighlighted = TRUE;
				}
				if (!alreadyHighlighted)
					DrawHlpHighlight(win);
			}
		}
	} else if (event->what == mouseDown) {
		GlobalToLocal(&event->where);
		code = FindControl(event->where, win, &theControl);
		if (code != 0) {
			if (code == kControlUpButtonPart || code == kControlDownButtonPart || 
				code == kControlIndicatorPart || code == kControlPageDownPart || 
				code == kControlPageUpPart) {
				Do_Hlp_ScrollBar(win,code,theControl,event->where);
			}
		} else {
			line = SelectHlpPosition(win, event);
			if (line != NULL && *line != EOS) {
				TextWin = FindAWindowNamed(gFileName);
				if (TextWin != NULL) {
					changeCurrentWindow(win, TextWin, true);
					PrepareWindA4(win, &saveRec);
					mCloseWindow(win);
					RestoreWindA4(&saveRec);
					t = strchr(line, ':');
					if (t != NULL)
						*t = EOS;
					isPlayS = getCommandNumber(line);
				}
			}
		}
	}
	return(-1);
}

void OpenHelpWindow(const FNType *fname) {
	int  i;
	int  sl[256];
	char st[SPEAKERLEN];
	char isHlpWinCreatedYet;

	userCom[0] = EOS;
	isHlpWinCreatedYet = (FindAWindowNamed(Commands_Shortcuts_str) != NULL);
	if (!isHlpWinCreatedYet) {
		SortCommands(sl,"",'\001');
		num_hlp_commands = 0;
		for (i=0; sl[i] != -1; i++) {
			AddKeysToCommand(sl[i], st);
			strcpy(hlp_commands[num_hlp_commands++], st);
		}
		strncpy(gFileName, fname, 255);
	}
	if (OpenWindow(2002,Commands_Shortcuts_str,0L,false,1,HlpEvent,UpdateHlp,CleanupHlp))
		ProgExit("Can't open Commands and Shortcuts window");
	if (!isHlpWinCreatedYet) {
		if (progIndex == -1)
			progIndex = 0;
	}
}
