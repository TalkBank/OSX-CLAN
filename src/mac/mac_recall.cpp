#include "ced.h"
#include "c_clan.h"
#include "mac_commands.h"

#define NUM_COMMANDS 50
#define MAX_COM_LEN  512

extern FNType Recall_str[];

static int   local_num_rows;
static char  *clan_commands[NUM_COMMANDS];
static char  DoubleClickCount = 1;
static long  lastWhen = 0;
static short curCommand;
static short progIndex = -1, topIndex = 0;
static FONTINFO uFont;
static Point lastWhere = {0,0};
static WindowPtr RecallWin = NULL;

short lastCommand;

void init_commands(void) {
	register int i;

	curCommand = 0;
	lastCommand = 0;
	for (i=0; i < NUM_COMMANDS; i++) {
		clan_commands[i] = (char *)malloc(MAX_COM_LEN+1);
		if (clan_commands[i] != NULL)
			*clan_commands[i] = EOS;
	}
}

void free_commands(void) {
	register int i;

	for (i=0; i < NUM_COMMANDS; i++) {
		if (clan_commands[i] != NULL)
			free(clan_commands[i]);
	}
}

void set_commands(char *text) {
	register int i;

	for (i=0; i < NUM_COMMANDS; i++) {
		if (clan_commands[i] != NULL && *clan_commands[i] == EOS) {
			strncpy(clan_commands[i], text, MAX_COM_LEN);
			clan_commands[i][MAX_COM_LEN] = EOS;
			break;
		}
	}
}

void set_lastCommand(short num) {
	lastCommand = num;
	if (lastCommand < 0 || lastCommand >= NUM_COMMANDS)
		lastCommand = 0;
	curCommand  = lastCommand;
}

void write_commands1977(FILE *fp) {
	register int i;

	for (i=0; i < NUM_COMMANDS; i++) {
		if (clan_commands[i] != NULL && *clan_commands[i] != EOS)
			fprintf(fp, "%d=%s\n", 1977, clan_commands[i]);
	}
}

static void CleanupRecall(WindowPtr wind) {
	RecallWin = NULL;
	topIndex = 0;
}

static void DrawRecallHighlight(WindowPtr win) {
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
	for (i=topIndex; i < NUM_COMMANDS; i++) {
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

static void SetRecallScrollControl(WindowPtr win) {
	GrafPtr			savePort;
	WindowProcRec	*windProc;

	GetPort(&savePort);
 	SetPortWindowPort(win);
 	windProc = WindowProcs(win);
 	if (windProc != NULL && windProc->VScrollHnd != NULL) {
		if (topIndex == 0 && local_num_rows >= NUM_COMMANDS) {
			HiliteControl(windProc->VScrollHnd, 255);
		} else {
			HiliteControl(windProc->VScrollHnd,0);
			SetControlMaximum(windProc->VScrollHnd, NUM_COMMANDS-1);
			SetControlValue(windProc->VScrollHnd, topIndex);
		}
	}
	SetPort(savePort);
}

static void UpdateRecall(WindowPtr win) {
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
	for (i=topIndex; i < NUM_COMMANDS && i < local_num_rows+topIndex; i++) {
		if (clan_commands[i] != NULL && *clan_commands[i] != EOS) {
			MoveTo(10, print_row);
			u_strcpy(templine, clan_commands[i], UTTLINELEN);
			DrawUTFontMac(templine, strlen(templine), &uFont, 0);
		}
		print_row += uFont.FHeight;
	}
	DrawRecallHighlight(win);
	SetRecallScrollControl(win);
	DrawGrowIcon(win);
	DrawControls(win);
	SetPort(oldPort);
}

void  HandleRecallVScrollBar(WindowPtr win, short code, ControlRef theControl, Point   myPt) {
    long  MaxTick;

	do {
		HiliteControl(theControl, code);
		switch (code) {
			case kControlUpButtonPart:
				if (topIndex > 0) {
					topIndex--;
					UpdateRecall(win);
				}
				break;

			case kControlDownButtonPart:
				if (topIndex < NUM_COMMANDS-1) {
					topIndex++;
					UpdateRecall(win);
				}
				break;

			case kControlPageUpPart:
				topIndex -= (local_num_rows / 2);
				if (topIndex < 0)
					topIndex = 0;
				UpdateRecall(win);
				break;

			case kControlPageDownPart:
				topIndex += (local_num_rows / 2);
				if (topIndex >= NUM_COMMANDS) 
					topIndex = NUM_COMMANDS - 1;
				UpdateRecall(win);
				break;

			case kControlIndicatorPart:
				code = TrackControl(theControl, myPt, NULL);
				topIndex = GetControlValue(theControl);
				if (topIndex >= NUM_COMMANDS)
					topIndex = NUM_COMMANDS - 1;
				UpdateRecall(win);
				break;	
		}

		SetRecallScrollControl(win);
//		if (code != kControlUpButtonPart && code != kControlDownButtonPart) {
			MaxTick = TickCount() + 10;
			do {  
				if (TickCount() > MaxTick) break;		 
			} while ( Button() == TRUE) ;
//		}
	} while (StillDown() == TRUE) ;
}

static void Do_Recall_ScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    short RefCon;

	RefCon = GetControlReference(theControl);
	switch  (RefCon) {
		case I_VScroll_bar:
			HandleRecallVScrollBar(win,code,theControl,myPt);
			break;
		default:
			break;
	}
}

static char *SelectRecallPosition(WindowPtr win, EventRecord *event) {
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
		for (i=topIndex; i < NUM_COMMANDS; i++) {
			print_row += uFont.FHeight;
			if (v < print_row)
				break;
		}
		if (i >= local_num_rows+topIndex || i >= NUM_COMMANDS)
			return(NULL);

		DrawRecallHighlight(win);
		progIndex = i;
		DrawRecallHighlight(win);
		return(clan_commands[i]);
	}
	DoubleClickCount = 1;
	lastWhen  = event->when;
	lastWhere = LocalPtr;

	print_row = 0;
	for (i=topIndex; i < NUM_COMMANDS; i++) {
		print_row += uFont.FHeight;
		if (v < print_row)
			break;
	}
	if (i >= local_num_rows+topIndex || i >= NUM_COMMANDS)
		return(NULL);

	DrawRecallHighlight(win);
	progIndex = i;
	DrawRecallHighlight(win);
	return(NULL);
}

static void delFromRecall(WindowPtr win) {
	char  alreadyHighlighted;
	short i;
	
	if (progIndex >= 0 && progIndex < NUM_COMMANDS) {
		alreadyHighlighted = FALSE;
		DrawRecallHighlight(win);
		if (clan_commands[progIndex] != NULL && clan_commands[progIndex][0] != EOS) {
			for (i=progIndex; i < NUM_COMMANDS-1; i++) {
				if (clan_commands[i] == NULL)
					clan_commands[i] = (char *)malloc(MAX_COM_LEN+1);
				if (clan_commands[i] != NULL)
					strcpy(clan_commands[i], clan_commands[i+1]);
			}
			if (clan_commands[i] != NULL && clan_commands[i][0] != EOS) {
				clan_commands[i][0] = EOS;
			}
			while ((clan_commands[progIndex] == NULL || clan_commands[progIndex][0] == EOS) && progIndex > 0) {
				progIndex--;
				lastCommand--;
			}
			curCommand = lastCommand;
			if (progIndex < 0)
				progIndex = NUM_COMMANDS - 1;
			if (progIndex < topIndex) {
				topIndex--;
				if (topIndex < 0)
					topIndex = 0;
			}
			if (progIndex >= local_num_rows+topIndex) {
				topIndex = progIndex - local_num_rows + 1;
				if (topIndex < 0)
					topIndex = 0;
			}
			UpdateRecall(win);
			alreadyHighlighted = TRUE;
		}
		if (!alreadyHighlighted)
			DrawRecallHighlight(win);
		WriteClanPreference();
	}
}

static short RecallEvent(WindowPtr win, EventRecord *event) {
	int				code;
	char			*line, alreadyHighlighted;
	short			key;
	ControlRef		theControl;
	WindowProcRec	*windProc;
	PrepareStruct	saveRec;

	windProc = WindowProcs(win);
	if (windProc == NULL)
		return(-1);
	
	alreadyHighlighted = FALSE;
	if (event->what == keyDown || event->what == autoKey) {
		key = (event->message & keyCodeMask)/256;
		if (key == up_key) {
			DrawRecallHighlight(win);
			progIndex--;
			if (progIndex < 0)
				progIndex = NUM_COMMANDS - 1;
			if (progIndex < topIndex) {
				topIndex--;
				if (topIndex < 0)
					topIndex = 0;
				UpdateRecall(win);
				alreadyHighlighted = TRUE;
			}
			if (progIndex >= local_num_rows+topIndex) {
				topIndex = progIndex - local_num_rows + 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateRecall(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawRecallHighlight(win);
		} else if (key == down_key) {
			DrawRecallHighlight(win);
			progIndex++;
			if (progIndex >= NUM_COMMANDS)
				progIndex = 0;
			if (progIndex < topIndex) {
				topIndex = 0;
				UpdateRecall(win);
				alreadyHighlighted = TRUE;
			}
			if (progIndex >= local_num_rows+topIndex) {
				topIndex++;
				UpdateRecall(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawRecallHighlight(win);
		} else {
			key = event->message & charCodeMask;
			if (key == DELETE_CHAR || key == BACKSPACE_CHAR) {
				delFromRecall(win);
			} else if (key == RETURN_CHAR || key == ENTER_CHAR) {
				AddComStringToComWin(clan_commands[progIndex], 0);
			} else if (key == ESC_CHAR) {
				PrepareWindA4(win, &saveRec);
				mCloseWindow(win);
				RestoreWindA4(&saveRec);
			}
		}
	} else if (event->what == mouseDown) {
		GlobalToLocal(&event->where);
		code = FindControl(event->where, win, &theControl);
		if (code != 0) {
			if (code == kControlUpButtonPart || code == kControlDownButtonPart || 
				code == kControlIndicatorPart || code == kControlPageDownPart || 
				code == kControlPageUpPart) {
				Do_Recall_ScrollBar(win,code,theControl,event->where);
			}
		} else {
			line = SelectRecallPosition(win, event);
			if (line != NULL && *line != EOS) {
				AddComStringToComWin(line, 0);
			}
		}
	}
	return(-1);
}

void OpenRecallWindow(void) {
	char RecallWinCreated;

	RecallWinCreated = (char)(FindAWindowNamed(Recall_str) != NULL);
	if (OpenWindow(2000,Recall_str,0L,false,1,RecallEvent,UpdateRecall,CleanupRecall))
		ProgExit("Can't open recall window");
	if (!RecallWinCreated) {
		RecallWin = FindAWindowNamed(Recall_str);
//			UpdateRecall(RecallWin);
	}
	uFont.FName = dFnt.fontId;
	uFont.FSize = dFnt.fontSize;
	uFont.CharSet = my_FontToScript(uFont.FName, 0);
	uFont.Encod = uFont.CharSet;
	uFont.FHeight = GetFontHeight(&uFont, NULL, RecallWin);
	if (progIndex == -1)
		progIndex = lastCommand;
}

void AddToClan_commands(char *st) {
	register int i;

	if (strlen(st) >= MAX_COM_LEN)
		return;

	i = lastCommand - 1;
	if (i < 0)
		i = NUM_COMMANDS - 1;
	if (clan_commands[i] != NULL && !strcmp(clan_commands[i], st)) {
		curCommand = lastCommand;
		return;
	}

	i = lastCommand;
	if (clan_commands[lastCommand] == NULL) {
		for (lastCommand++; clan_commands[lastCommand] == NULL && lastCommand != i; lastCommand++) {
			if (lastCommand >= NUM_COMMANDS)
				lastCommand = 0;
		}
	}
	if (clan_commands[lastCommand] != NULL) {
		strncpy(clan_commands[lastCommand], st, MAX_COM_LEN);
		clan_commands[lastCommand][MAX_COM_LEN] = EOS;
		if (RecallWin != NULL) {
			UpdateRecall(RecallWin);
		}
		lastCommand++;
		if (lastCommand >= NUM_COMMANDS)
			lastCommand = 0;
		curCommand = lastCommand;
		WriteClanPreference();
	}
}

void RecallCommand(WindowPtr win, short type) {
	if (type == up_key) {
		type = curCommand;
		if (--curCommand < 0)
			curCommand = NUM_COMMANDS - 1;
		while ((clan_commands[curCommand] == NULL || *clan_commands[curCommand] == EOS) && curCommand != type) {
			if (--curCommand < 0)
				curCommand = NUM_COMMANDS - 1;
		}
	} else {
		type = curCommand;
		if (curCommand == lastCommand) {
			set_fbuffer(win, FALSE);
			if (fbuffer != NULL && fbuffer[0] == EOS)
				curCommand--;
		}
		if (++curCommand >= NUM_COMMANDS)
			curCommand = 0;
		while ((clan_commands[curCommand] == NULL || *clan_commands[curCommand] == EOS) && curCommand != type) {
			if (++curCommand >= NUM_COMMANDS)
				curCommand = 0;
		}
	}

	if (clan_commands[curCommand] != NULL)
		AddComStringToComWin(clan_commands[curCommand], 0);
}
