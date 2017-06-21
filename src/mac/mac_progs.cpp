#include "ced.h"
#include "cu.h"
#include "mac_commands.h"

extern void (*clan_main[])(int argc, char *argv[]);

#define NUMPROGS 300

static int   local_num_rows;
static const char  *progs[NUMPROGS];
static char  DoubleClickCount = 1;
static long  lastWhen = 0;
static short progIndex = -1, maxFiles = 0, topIndex = 0;
static FONTINFO uFont;
static Point lastWhere = {0,0};
static WindowPtr ProgWin = NULL;

static void CleanupProgs(WindowPtr win) {
	topIndex = 0;
	maxFiles = 0;
}

static void DrawProgsHighlight(WindowPtr win) {
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
	for (i=topIndex; i < maxFiles; i++) {
		print_row += uFont.FHeight;
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
	theRect.bottom = print_row + FLINESPACE + 2;
	theRect.top = theRect.bottom - uFont.FHeight + 2;
	PaintRect(&theRect);
	SetPort(oldPort);
}

static void SetProgsScrollControl(WindowPtr win) {
	GrafPtr			savePort;
	WindowProcRec	*windProc;

	GetPort(&savePort);
 	SetPortWindowPort(win);
 	windProc = WindowProcs(win);
 	if (windProc != NULL && windProc->VScrollHnd != NULL) {
		if (topIndex == 0 && local_num_rows >= maxFiles) {
			HiliteControl(windProc->VScrollHnd, 255);
		} else {
			HiliteControl(windProc->VScrollHnd,0);
			SetControlMaximum(windProc->VScrollHnd, maxFiles-1);
			SetControlValue(windProc->VScrollHnd, topIndex);
		}
	}
	SetPort(savePort);
}

static void UpdateProgs(WindowPtr win) {
    register int i;
    register int print_row;
    Rect theRect;
	Rect rect;
    GrafPtr oldPort;

    GetPort(&oldPort);
    SetPortWindowPort(win);

	if (uS.mStricmp(dFnt.fontName, "Ascender Uni Duo") == 0) {
		TextFont(0);
		TextSize(0);
	} else {
		TextFont(dFnt.fontId);
		TextSize(dFnt.fontSize);
	}
	TextFace(0);
	
	GetWindowPortBounds(win, &rect);
	theRect.bottom = rect.bottom;
	theRect.top = rect.top;
	theRect.left = rect.left;
	theRect.right = rect.right - SCROLL_BAR_SIZE;

	local_num_rows = NumOfRowsInAWindow(win, &i, 0);
	EraseRect(&theRect);
	print_row = uFont.FHeight;
	for (i=topIndex; i < maxFiles && i < local_num_rows+topIndex; i++) {
		MoveTo(10, print_row);
		u_strcpy(templine, progs[i], UTTLINELEN);
		DrawUTFontMac(templine, strlen(templine), &uFont, 0);
		print_row += uFont.FHeight;
	}
	DrawProgsHighlight(win);
	SetProgsScrollControl(win);
	DrawGrowIcon(win);
	DrawControls(win);
    SetPort(oldPort);
}

void  HandleProgsVScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    long  MaxTick;

	do {
		HiliteControl(theControl, code);
		switch (code) {
			case kControlUpButtonPart:
				if (topIndex > 0) {
					topIndex--;
					UpdateProgs(win);
				}
				break;

			case kControlDownButtonPart:
				if (topIndex < maxFiles-1) {
					topIndex++;
					UpdateProgs(win);
				}
				break;

			case kControlPageUpPart:
				topIndex -= (local_num_rows / 2);
				if (topIndex < 0)
					topIndex = 0;
				UpdateProgs(win);
				break;

			case kControlPageDownPart:
				topIndex += (local_num_rows / 2);
				if (topIndex >= maxFiles) 
					topIndex = maxFiles - 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateProgs(win);
				break;

			case kControlIndicatorPart:
				code = TrackControl(theControl, myPt, NULL);
				topIndex = GetControlValue(theControl);
				if (topIndex >= maxFiles)
					topIndex = maxFiles - 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateProgs(win);
				break;	
		}

		SetProgsScrollControl(win);
//		if (code != kControlUpButtonPart && code != kControlDownButtonPart) {
			MaxTick = TickCount() + 10;
			do {  
				if (TickCount() > MaxTick) break;		 
			} while ( Button() == TRUE) ;
//		}
	} while (StillDown() == TRUE) ;
}

static void Do_Progs_ScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    short RefCon;

	RefCon = GetControlReference(theControl);
	switch  (RefCon) {
		case I_VScroll_bar:
			HandleProgsVScrollBar(win,code,theControl,myPt);
			break;
		default:
			break;
	}
}

static const char *SelectProgsPosition(WindowPtr win, EventRecord *event) {
    register short i;
    register int print_row;
	register int h = event->where.h;
	register int v = event->where.v;
    Point LocalPtr;
	Rect rect;

	GetWindowPortBounds(win, &rect);
	if (v < 0 || h > rect.right - rect.left - SCROLL_BAR_SIZE)
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
		for (i=topIndex; i < maxFiles; i++) {
			print_row += uFont.FHeight;
			if (v < print_row)
				break;
		}
		if (i >= local_num_rows+topIndex || i >= maxFiles)
			return(NULL);

		DrawProgsHighlight(win);
		progIndex = i;
		DrawProgsHighlight(win);
		return(progs[i]);
	}
	DoubleClickCount = 1;
	lastWhen  = event->when;
	lastWhere = LocalPtr;

	print_row = 0;
	for (i=topIndex; i < maxFiles; i++) {
		print_row += uFont.FHeight;
		if (v < print_row)
			break;
	}
	if (i >= local_num_rows+topIndex || i >= maxFiles)
		return(NULL);

	DrawProgsHighlight(win);
	progIndex = i;
	DrawProgsHighlight(win);
	return(NULL);
}

static short ProgsEvent(WindowPtr win, EventRecord *event) {
	int				code;
	const char		*line;
	char			alreadyHighlighted;
	short			key;
	ControlRef		theControl;
	WindowProcRec	*windProc;
	PrepareStruct	saveRec;

	windProc = WindowProcs(win);
	if (windProc == NULL)
		return(-1);
	
	alreadyHighlighted = FALSE;
	if (event->what == keyDown || event->what == autoKey) {
		key = (event->message & keyCodeMask) / 256;
		if (key == up_key) {
			DrawProgsHighlight(win);
			progIndex--;
			if (progIndex < 0)
				progIndex = maxFiles - 1;
			if (progIndex < topIndex) {
				topIndex--;
				if (topIndex < 0)
					topIndex = 0;
				UpdateProgs(win);
				alreadyHighlighted = TRUE;
			}
			if (progIndex >= local_num_rows+topIndex) {
				topIndex = progIndex - local_num_rows + 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateProgs(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawProgsHighlight(win);
		} else if (key == down_key) {
			DrawProgsHighlight(win);
			progIndex++;
			if (progIndex >= maxFiles)
				progIndex = 0;
			if (progIndex < topIndex) {
				topIndex = 0;
				UpdateProgs(win);
				alreadyHighlighted = TRUE;
			}
			if (progIndex >= local_num_rows+topIndex) {
				topIndex++;
				UpdateProgs(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawProgsHighlight(win);
		} else {
			key = event->message & charCodeMask;
			if (key == RETURN_CHAR || key == ENTER_CHAR) {
				PrepareWindA4(win, &saveRec);
				mCloseWindow(win);
				RestoreWindA4(&saveRec);
				strcpy(fbuffer, progs[progIndex]);
				strcat(fbuffer, " ");
				AddComStringToComWin(fbuffer, 0);
			}
		}
	} else if (event->what == mouseDown) {
		GlobalToLocal(&event->where);
		code = FindControl(event->where, win, &theControl);
		if (code != 0) {
			if (code == kControlUpButtonPart || code == kControlDownButtonPart || 
				code == kControlIndicatorPart || code == kControlPageDownPart || 
				code == kControlPageUpPart) {
				Do_Progs_ScrollBar(win,code,theControl,event->where);
			}
		} else {
			line = SelectProgsPosition(win, event);
			if (line != NULL) {
				PrepareWindA4(win, &saveRec);
				mCloseWindow(win);
				RestoreWindA4(&saveRec);
				strcpy(fbuffer, line);
				strcat(fbuffer, " ");
				AddComStringToComWin(fbuffer, 0);
			}
		}
	}
	return(-1);
}

void OpenProgsWindow(void) {
	int  i;
	char ProgsWinCreated;

	ProgsWinCreated = (FindAWindowNamed(CLAN_Programs_str) != NULL);
	if (OpenWindow(2001,CLAN_Programs_str,0L,false,1,ProgsEvent,UpdateProgs,CleanupProgs)) {
		do_warning("Can't open CLAN Programs window", 0);
		return;
	}
	if (!ProgsWinCreated) {
		for (i=maxFiles=0; i < MEGRASP; i++) {
			if (clan_main[i] && maxFiles < NUMPROGS)
				progs[maxFiles++] = clan_name[i];
		}
		if (progIndex == -1)
			progIndex = 0;
		ProgWin = FindAWindowNamed(CLAN_Programs_str);
	}
	uFont.FName = dFnt.fontId;
	uFont.FSize = dFnt.fontSize;
	uFont.CharSet = my_FontToScript(uFont.FName, 0);
	uFont.Encod = uFont.CharSet;
	uFont.FHeight = GetFontHeight(&uFont, NULL, ProgWin);
}
