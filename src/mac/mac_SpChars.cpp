#include "ced.h"
#include "cu.h"
#include "check.h"

static int   local_num_rows, char_height;
static char  DoubleClickCount = 1;
static unCH  chars[NUMSPCHARS+2][50];
static long  lastWhen = 0;
static short charIndex = -1, maxChars = 0, topIndex = 0;
static Point lastWhere = {0,0};
static FONTINFO uFont;

wchar_t Char2SpChar(int key, char num) { // CA CHARS
	if (num == 1) {
		if (key == 0x7e00) { 				 // up-arrow
			return(0x2191); // 0xe2 86 91
		} else if (key == 0x7d00) {			 // down-arrow
			return(0x2193); // 0xe2 86 93
		} else if (key == '1') {			 // rise to high
			return(0x21D7); // 0xe2 87 97
		} else if (key == '2') {			 // rise to mid
			return(0x2197); // 0xe2 86 97
		} else if (key == '3') {			 // level
			return(0x2192); // 0xe2 86 92
		} else if (key == '4') {			 // fall to mid
			return(0x2198); // 0xe2 86 98
		} else if (key == '5') {			 // fall to low
			return(0x21D8); // 0xe2 87 98
		} else if (key == '6') {			 // unmarked ending
			return(0x221E); // 0xe2 88 9e
		} else if (key == '+') {			 // continuation - wavy triple-line equals sign
			return(0x224B); // 0xe2 89 8b
		} else if (key == '.') {			 // inhalation - raised period
			return(0x2219); // 0xe2 88 99
		} else if (key == '=') {			 // latching
			return(0x2248); // 0xe2 89 88
		} else if (key == 'u' || key == 'U'){// uptake
			return(0x2261); // 0xe2 89 88
		} else if (key == '[') {			 // raised [
			return(0x2308); // 0xe2 8c 88
		} else if (key == ']') {			 // raised ]
			return(0x2309); // 0xe2 8c 89
		} else if (key == '{') {			 // lowered [
			return(0x230A); // 0xe2 8c 8a
		} else if (key == '}') {			 // lowered ]
			return(0x230B); // 0xe2 8c 8b
		} else if (key == 0x7c00) {			 // faster
			return(0x2206); // 0xe2 88 86
		} else if (key == 0x7b00) {			 // slower
			return(0x2207); // 0xe2 88 87
		} else if (key == '*'){				 // creaky
			return(0x204E); // 0xe2 81 8e
		} else if (key == '/') {			 // unsure
			return(0x2047); // 0xe2 81 87
		} else if (key == '0') {			 // softer
			return(0x00B0); // 0xc2 b0
		} else if (key == ')') {			 // louder
			return(0x25C9); // 0xe2 97 89
		} else if (key == 'd'){				 // low pitch - low bar
			return(0x2581); // 0xe2 96 81
		} else if (key == 'h') {			 // high pitch - high bar
			return(0x2594); // 0xe2 96 94
		} else if (key == 'l' || key == 'L'){// smile voice
			return(0x263A); // 0xe2 98 ba
		} else if (key == 'b'){				 // breathy-voice -♋- NOT CA
			return(0x264b); // 0xe2 99 8b
		} else if (key == 'w' || key == 'W'){// whisper
			return(0x222C); // 0xe2 88 ac
		} else if (key == 'y' || key == 'Y'){// yawn
			return(0x03AB); // ce ab
		} else if (key == 's' || key == 'S'){// singing
			return(0x222E); // 0xe2 88 ae
		} else if (key == 'p' || key == 'P'){// precise
			return(0x00A7); // c2 a7
		} else if (key == 'n' || key == 'N'){// constriction
			return(0x223E); // 0xe2 88 be
		} else if (key == 'r' || key == 'R'){// pitch reset
			return(0x21BB); // 0xe2 86 bb
		} else if (key == 'c' || key == 'C'){// laugh in a word
			return(0x1F29); // 0xe1 bc a9
		} else
			return(0);
	} else if (num == 2) {
		if (key == 'H') {					 // raised h - NOT CA
			return(0x02B0); // ca b0
		} else if (key == ',') {			 // dot diacritic - NOT CA
			return(0x0323); // cc a3
		} else if (key == '<') {			 // Group start marker - NOT CA
			return(0x2039); // 0xe2 80 B9
		} else if (key == '>') {			 // Group end marker - NOT CA
			return(0x203A); // 0xe2 80 BA
		} else if (key == 't' || key == 'T'){// Tag or sentence final particle; „ - NOT CA
			return(0x201E); // 0xe2 80 9E
		} else if (key == 'v' || key == 'V'){// Vocative or summons - ‡ - NOT CA
			return(0x2021); // 0xe2 80 A1
		} else if (key == '-'){				 // Stress - ̄ - NOT CA
			return(0x0304); // cc 84
		} else if (key == 'q'){				 // Glottal stop - ʔ - NOT CA
			return(0x0294); // ca 94
		} else if (key == 'Q'){				 // Hebrew glottal - ʕ - NOT CA
			return(0x0295); // ca 95
		} else if (key == ';'){				 // caron - ̌- NOT CA
			return(0x030C); // cc 8c
		} else if (key == '1'){				 // raised stroke - NOT CA
			return(0x02C8); // cb 88
		} else if (key == '2'){				 // lowered stroke - NOT CA
			return(0x02CC); // cb 8c
		} else if (key == '{'){				 // sign group start marker - NOT CA
			return(0x3014); // 0xe3 80 94
		} else if (key == '}'){				 // sign group end marker - NOT CA
			return(0x3015); // 0xe3 80 95
		} else if (key == 'm'){				 // %pho missing word -…- NOT CA
			return(0x2026); // 0xe2 80 a6
		} else if (key == '_'){				 // Uderline - NOT CA
			return(0x0332); // 0xe2 80 a6
		} else if (key == '\''){			 // open quote “ - NOT CA
			return(0x201C); // 0xe2 80 9c - NOTCA_OPEN_QUOTE
		} else if (key == '"'){				 // close quote ” - NOT CA
			return(0x201D); // 0xe2 80 a6 - NOTCA_CLOSE_QUOTE
		} else if (key == '='){				 // crossed equal ≠ - NOT CA
			return(0x2260); // 0xe2 89 a0 - NOTCA_CROSSED_EQUAL
		} else if (key == '/'){				 // left arrow with circle ↫ - NOT CA
			return(0x21AB); // 0xe2 86 ab - NOTCA_LEFT_ARROW_CIRCLE
		} else
			return(0);
	} else
		return(0);
}

static void CleanupChars(WindowPtr wind) {
	topIndex = 0;
	maxChars = 0;
}

static void DrawCharsHighlight(WindowPtr win) {
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
	for (i=topIndex; i < maxChars; i++) {
		print_row += char_height;
		if (charIndex == i)
			break;
	}
	if (charIndex != i || i >= local_num_rows+topIndex) {
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

static void SetCharsScrollControl(WindowPtr win) {
	GrafPtr			savePort;
	WindowProcRec	*windProc;

	GetPort(&savePort);
 	SetPortWindowPort(win);
 	windProc = WindowProcs(win);
 	if (windProc != NULL && windProc->VScrollHnd != NULL) {
		if (topIndex == 0 && local_num_rows >= maxChars) {
			HiliteControl(windProc->VScrollHnd, 255);
		} else {
			HiliteControl(windProc->VScrollHnd,0);
			SetControlMaximum(windProc->VScrollHnd, maxChars-1);
			SetControlValue(windProc->VScrollHnd, topIndex);
		}
	}
	SetPort(savePort);
}

static void UpdateChars(WindowPtr win) {
    register int i;
    register int print_row;
    Rect theRect;
	Rect rect;
    GrafPtr oldPort;

    GetPort(&oldPort);
    SetPortWindowPort(win);

	TextFont(4);
	TextSize(9);
	TextFace(0);

	GetWindowPortBounds(win, &rect);
	theRect.bottom = rect.bottom;
	theRect.top = rect.top;
	theRect.left = rect.left;
	theRect.right = rect.right - SCROLL_BAR_SIZE;

	local_num_rows = NumOfRowsInAWindow(win, &char_height, 0);
	i = local_num_rows * char_height;
	char_height += 3;
	local_num_rows = i / char_height;
	EraseRect(&theRect);
	print_row = char_height;
	for (i=topIndex; i < maxChars && i < local_num_rows+topIndex; i++) {
		MoveTo(LEFTMARGIN, print_row);
		DrawUTFontMac(chars[i], 1, &uFont, 0);
		MoveTo(35, print_row);
		DrawUTFontMac(chars[i]+1, strlen(chars[i]+1), &uFont, 0);
		print_row += char_height;
	}

	DrawCharsHighlight(win);
	SetCharsScrollControl(win);
	DrawGrowIcon(win);
	DrawControls(win);
    SetPort(oldPort);
}

void  HandleCharsVScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    long  MaxTick;
	UInt16 val;
	WindowProcRec *windProc;

  	windProc = WindowProcs(win);
	if (windProc != NULL && windProc->VScrollHnd != NULL) {
		val = GetControlHilite(windProc->VScrollHnd);
		if (val == 255)
			return;
	}	
	do {
		HiliteControl(theControl, code);
		switch (code) {
			case kControlUpButtonPart:
				if (topIndex > 0) {
					topIndex--;
					UpdateChars(win);
				}
				break;

			case kControlDownButtonPart:
				if (topIndex < maxChars-1) {
					topIndex++;
					UpdateChars(win);
				}
				break;

			case kControlPageUpPart:
				topIndex -= (local_num_rows / 2);
				if (topIndex < 0)
					topIndex = 0;
				UpdateChars(win);
				break;

			case kControlPageDownPart:
				topIndex += (local_num_rows / 2);
				if (topIndex >= maxChars) 
					topIndex = maxChars - 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateChars(win);
				break;

			case kControlIndicatorPart:
				code = TrackControl(theControl, myPt, NULL);
				topIndex = GetControlValue(theControl);
				if (topIndex >= maxChars)
					topIndex = maxChars - 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateChars(win);
				break;	
		}

		SetCharsScrollControl(win);
//		if (code != kControlUpButtonPart && code != kControlDownButtonPart) {
			MaxTick = TickCount() + 10;
			do {  
				if (TickCount() > MaxTick) break;		 
			} while ( Button() == TRUE) ;
//		}
	} while (StillDown() == TRUE) ;
}

static void Do_Chars_ScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    short RefCon;

	RefCon = GetControlReference(theControl);
	switch  (RefCon) {
		case I_VScroll_bar:
			HandleCharsVScrollBar(win,code,theControl,myPt);
			break;
		default:
			break;
	}
}

static unCH *SelectCharsPosition(WindowPtr win, EventRecord *event) {
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
		for (i=topIndex; i < maxChars; i++) {
			print_row += char_height;
			if (v < print_row)
				break;
		}
		if (i >= local_num_rows+topIndex || i >= maxChars)
			return(NULL);

		DrawCharsHighlight(win);
		charIndex = i;
		DrawCharsHighlight(win);
		return(chars[i]);
	}
	DoubleClickCount = 1;
	lastWhen  = event->when;
	lastWhere = LocalPtr;

	print_row = 0;
	for (i=topIndex; i < maxChars; i++) {
		print_row += char_height;
		if (v < print_row)
			break;
	}
	if (i >= local_num_rows+topIndex || i >= maxChars)
		return(NULL);

	DrawCharsHighlight(win);
	charIndex = i;
	DrawCharsHighlight(win);
	return(NULL);
}

static void Write2TextWin(unCH c) {
	myFInfo *saveGlobal_df;

	lastGlobal_df = WindowFromGlobal_df(lastGlobal_df);
	if (lastGlobal_df == NULL)
		return;

	saveGlobal_df = global_df;
	global_df = lastGlobal_df;

	AddText(NULL, c, -2, 1L);

	global_df = saveGlobal_df;
}

static short CharsEvent(WindowPtr win, EventRecord *event) {
	int				code;
	unCH			*line;
	char			alreadyHighlighted;
	short			key;
	ControlRef		theControl;
	WindowProcRec	*windProc;

	windProc = WindowProcs(win);
	if (windProc == NULL)
		return(-1);
	
	alreadyHighlighted = FALSE;
	if (event->what == keyDown || event->what == autoKey) {
		key = (event->message & keyCodeMask) / 256;
		if (key == up_key) {
			DrawCharsHighlight(win);
			charIndex--;
			if (charIndex < 0)
				charIndex = maxChars - 1;
			if (charIndex < topIndex) {
				topIndex--;
				if (topIndex < 0)
					topIndex = 0;
				UpdateChars(win);
				alreadyHighlighted = TRUE;
			}
			if (charIndex >= local_num_rows+topIndex) {
				topIndex = charIndex - local_num_rows + 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateChars(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawCharsHighlight(win);
		} else if (key == down_key) {
			DrawCharsHighlight(win);
			charIndex++;
			if (charIndex >= maxChars)
				charIndex = 0;
			if (charIndex < topIndex) {
				topIndex = 0;
				UpdateChars(win);
				alreadyHighlighted = TRUE;
			}
			if (charIndex >= local_num_rows+topIndex) {
				topIndex++;
				UpdateChars(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawCharsHighlight(win);
		} else {
			key = event->message & charCodeMask;
			if (key == RETURN_CHAR || key == ENTER_CHAR) {
#ifdef _UNICODE
				Write2TextWin(chars[charIndex][0]);
#endif
			}
		}
	} else if (event->what == mouseDown) {
		GlobalToLocal(&event->where);
		code = FindControl(event->where, win, &theControl);
		if (code != 0) {
			if (code == kControlUpButtonPart || code == kControlDownButtonPart || 
				code == kControlIndicatorPart || code == kControlPageDownPart || 
				code == kControlPageUpPart) {
				Do_Chars_ScrollBar(win,code,theControl,event->where);
			}
		} else {
			line = SelectCharsPosition(win, event);
			if (line != NULL) {
#ifdef _UNICODE
				Write2TextWin(line[0]);
#endif
			}
		}
	}
	return(-1);
}

#define isHex(x) (x == 'A' || x == 'B' || x == 'C' || x == 'D' || x == 'E' || x == 'F')

static void makeUnicodeString(unCH *dist, const char *src) {
	long f, t, i;
	unsigned long uc;
	char hexStr[256];

	f=0L;
	t=0L;
	while (src[f] != EOS) {
		if (src[f] == '0' && src[f+1] == 'x') {
			i = 0L;
			hexStr[i++] = src[f++];
			hexStr[i++] = src[f++];
			for (; isdigit(src[f]) || isHex(src[f]); i++)
				hexStr[i] = src[f++];
			hexStr[i] = EOS;
			sscanf(hexStr, "%lx", &uc);
			dist[t++] = uc;
		} else
			dist[t++] = src[f++];
	}
	dist[t] = EOS;
}

void OpenCharsWindow(void) {
	char CharsWinCreated;
	char fontName[50];
	short fID;
	PrepareStruct saveRec;

	lastGlobal_df = global_df;
	CharsWinCreated = (FindAWindowNamed(Special_Characters_str) != NULL);
	if (OpenWindow(2007,Special_Characters_str,0L,false,1,CharsEvent,UpdateChars,CleanupChars)) {
		do_warning("Can't open Special Characters window", 0);
		return;
	}
	if (!CharsWinCreated) {
#ifdef _UNICODE
		short FHeight;
		WindowPtr wind;

		wind = FindAWindowNamed(Special_Characters_str);

		strcpy(fontName, "CAfont");
		if (!GetFontNumber(fontName, &fID)) {
			do_warning("Missing \"CAfont\" font.", 0);
			PrepareWindA4(wind, &saveRec);
			mCloseWindow(wind);
			RestoreWindA4(&saveRec);
			return;
		}
		uFont.FName = fID;
		uFont.FSize = 15;
		uFont.CharSet = my_FontToScript(uFont.FName, 0);
		uFont.Encod = uFont.CharSet;
		FHeight = GetFontHeight(&uFont, NULL, wind);
		uFont.FHeight = FHeight;

		maxChars = 0; // CA CHARS
/* 1*/	makeUnicodeString(chars[maxChars++], "0x2191 shift to high pitch; F1 up-arrow"); // CA_BREATHY_VOICE
/* 2*/	makeUnicodeString(chars[maxChars++], "0x2193 shift to low pitch; F1 down-arrow");
/* 3*/	makeUnicodeString(chars[maxChars++], "0x21D7 rising to high; F1 1");
/* 4*/	makeUnicodeString(chars[maxChars++], "0x2197 rising to mid; F1 2");
/* 5*/	makeUnicodeString(chars[maxChars++], "0x2192 level; F1 3");
/* 6*/	makeUnicodeString(chars[maxChars++], "0x2198 falling to mid; F1 4");
/* 7*/	makeUnicodeString(chars[maxChars++], "0x21D8 falling to low; F1 5");
/* 8*/	makeUnicodeString(chars[maxChars++], "0x221E unmarked ending; F1 6");
/* 9*/	makeUnicodeString(chars[maxChars++], "0x224B 0x224Bcontinuation; F1 +");
/*10*/	makeUnicodeString(chars[maxChars++], "0x2219 inhalation; F1 .");
/*11*/	makeUnicodeString(chars[maxChars++], "0x2248 latching0x2248; F1 =");
/*12*/	makeUnicodeString(chars[maxChars++], "0x2261 0x2261uptake; F1 u");
/*13*/	makeUnicodeString(chars[maxChars++], "0x2308 top begin overlap; F1 [");
/*14*/	makeUnicodeString(chars[maxChars++], "0x2309 top end overlap; F1 ]");
/*15*/	makeUnicodeString(chars[maxChars++], "0x230A bottom begin overlap; F1 {");
/*16*/	makeUnicodeString(chars[maxChars++], "0x230B bottom end overlap; F1 }");
/*17*/	makeUnicodeString(chars[maxChars++], "0x2206 0x2206faster0x2206; F1 right-arrow");
/*18*/	makeUnicodeString(chars[maxChars++], "0x2207 0x2207slower0x2207; F1 left-arrow");
/*19*/	makeUnicodeString(chars[maxChars++], "0x204E 0x204Ecreaky0x204E; F1 *");
/*20*/	makeUnicodeString(chars[maxChars++], "0x2047 0x2047unsure0x2047; F1 /");
/*21*/	makeUnicodeString(chars[maxChars++], "0x00B0 0x00B0softer0x00B0; F1 0");
/*22*/	makeUnicodeString(chars[maxChars++], "0x25C9 0x25C9louder0x25C9; F1 )");
/*23*/	makeUnicodeString(chars[maxChars++], "0x2581 0x2581low pitch0x2581; F1 d");
/*24*/	makeUnicodeString(chars[maxChars++], "0x2594 0x2594high pitch0x2594; F1 h");
/*25*/	makeUnicodeString(chars[maxChars++], "0x263A 0x263Asmile voice0x263A; F1 l");
/*26*/	makeUnicodeString(chars[maxChars++], "0x264B 0x264Bbreathy voice0x264B marker; F1 b");
/*27*/	makeUnicodeString(chars[maxChars++], "0x222C 0x222Cwhisper0x222C; F1 w");
/*28*/	makeUnicodeString(chars[maxChars++], "0x03AB 0x03AByawn0x03AB; F1 y");
/*29*/	makeUnicodeString(chars[maxChars++], "0x222E 0x222Esinging0x222E; F1 s");
/*30*/	makeUnicodeString(chars[maxChars++], "0x00A7 0x00A7precise0x00A7; F1 p");
/*31*/	makeUnicodeString(chars[maxChars++], "0x223E constriction0x223E; F1 n");
/*32*/	makeUnicodeString(chars[maxChars++], "0x21BB 0x21BBpitch reset; F1 r");
/*33*/	makeUnicodeString(chars[maxChars++], "0x1F29 laugh in a word; F1 c");
/*34*/	makeUnicodeString(chars[maxChars++], "0x201E Tag or sentence final particle; F2 t");
/*35*/	makeUnicodeString(chars[maxChars++], "0x2021 0x2021 Vocative or summons; F2 v");
/*36*/	makeUnicodeString(chars[maxChars++], "0x0323 Arabic dot diacritic; F2 ,");
/*37*/	makeUnicodeString(chars[maxChars++], "0x02B0 Arabic raised h; F2 H");
/*38*/	makeUnicodeString(chars[maxChars++], "0x0304 Stress; F2 -");
/*39*/	makeUnicodeString(chars[maxChars++], "0x0294 Glottal stop0x0294; F2 q");
/*40*/	makeUnicodeString(chars[maxChars++], "0x0295 Reverse glottal0x0295; F2 Q");
/*41*/	makeUnicodeString(chars[maxChars++], "0x030C Caron; F2 ;");
/*42*/	makeUnicodeString(chars[maxChars++], "0x02C8 raised0x02C8 stroke; F2 1");
/*43*/	makeUnicodeString(chars[maxChars++], "0x02CC lowered0x02CC stroke; F2 2");
/*44*/	makeUnicodeString(chars[maxChars++], "0x2039 0x2039begin phono group0x203A marker; F2 <");
/*45*/	makeUnicodeString(chars[maxChars++], "0x203A 0x2039end phono group0x203A marker; F2 >");
/*46*/	makeUnicodeString(chars[maxChars++], "0x3014 0x3014begin sign group0x3015; F2 {");
/*47*/	makeUnicodeString(chars[maxChars++], "0x3015 0x3014end sign group0x3015; F2 }");
/*48*/	makeUnicodeString(chars[maxChars++], "0x2026 %pho missing word; F2 m");
/*49*/	makeUnicodeString(chars[maxChars++], "0x0332 und0x0332e0x0332r0x0332line; F2 _");
/*50*/	makeUnicodeString(chars[maxChars++], "0x201C open 0x201Cquote0x201D; F2 '");
/*51*/	makeUnicodeString(chars[maxChars++], "0x201D close 0x201Cquote0x201D; F2 \"");
/*52*/	makeUnicodeString(chars[maxChars++], "0x2018 open 0x2018quote0x2019;");
/*53*/	makeUnicodeString(chars[maxChars++], "0x2019 close 0x2018quote0x2019;");
/*54*/	makeUnicodeString(chars[maxChars++], "0x2260 0x2260row; F2 =");
/*55*/	makeUnicodeString(chars[maxChars++], "0x21AB 0x21ABr-r0x21ABrabbit; F2 /");

// if number of chars changes then change #define NUMSPCHARS at in check.h

#else // else _UNICODE
		strcpy(chars[maxChars++], "This ONLY works in Unicode CLAN");
#endif // else _UNICODE
		if (charIndex == -1)
			charIndex = 0;
	}
}
