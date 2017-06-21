#include "ced.h"
#include "mac_MUPControl.h"
#include "MMedia.h"
#include "ids.h"

/* xcode
#ifndef _MSL_ANSI_PARMS_H
#error "NO _MSL_ANSI_PARMS_H"
#endif
#ifdef _MSL_ANSI_PARMS_H
#error "YES _MSL_ANSI_PARMS_H"
#endif
*/

#include "my_ctype.h"
#include <Quickdraw.h>
//#include <Windows.h>
#include "c_clan.h"

// 21-4-99 #define hiword(x)		(((short *) &(x))[0])
// 21-4-99 #define loword(x)		(((short *) &(x))[1])

#define WINDASHNUM 6

extern char  DefClan;
extern char  DefWindowDims;
extern char  isPlayAudioFirst;
extern char  isPlayAudioMenuSet;
extern short mScript;
extern FNType warning_str[];
extern struct DefWin defWinSize;
extern struct DefWin MovieWinSize;
extern struct DefWin PictWinSize;
extern struct DefWin TextWinSize;
extern struct DefWin ClanOutSize;
extern struct DefWin ClanWinSize;
extern struct DefWin ThumbnailWin;
extern struct DefWin RecallWinSize;
extern struct DefWin ProgsWinSize;
extern struct DefWin FolderWinSize;
extern struct DefWin WebItemsWinSize;
extern struct DefWin SpCharsWinSize;
extern struct DefWin HlpCmdsWinSize;
extern struct DefWin MVHLPWinSize;
extern struct DefWin PBCWinSize;
extern struct DefWin WarningWinSize;
extern struct DefWin ProgresWinSize;
extern MACWINDOWS *RootWind;

extern void  SetWinStats(WindowPtr wp);

static void  InstallAEProcs(void);
static void  GetWorldRect(void);
// 2009-10-13 static pascal void BombResume(void);

static Rect MainLimit = { 170, 200, 9999, 9999 };
static Rect MovieLimit = { 170, 200, 9999, 9999 };
Point SFGwhere = { 90, 82 };

static MenuRef appleMenu;
static MenuRef FileMenu;
static MenuRef EditMenu;
static MenuRef TiersMenu;
static MenuRef ModeMenu;
static MenuRef WindowMenu;
MenuRef FontMenu;
MenuRef SizeMenu;
static MenuRef HelpMenu;

static int   isFileFound = 1;
static char  OptionKeyFound;
static long  lastWhen = 0L;
static long  NumberOfReads = 0L;
static Rect	 theTotalScreen;
static short screenCnt = 0;
static short HMenu_ID;
static MenuItemIndex HMenuItemOffset;
static CTabHandle mycolors;

// Unicode handler
#if (TARGET_API_MAC_CARBON == 1)
static long lastKeyLayoutID;
static Handle uchrHandle;
static UInt32 deadKeyState;
UnicodeInput UniInputBuf;
#endif
short currentKeyScript, saveCurrentKeyScript;

char	  isMovieAvialable;
char	  isCloseProgressDialog = FALSE;
short	  NonModal = 0;
short	  isBackground = FALSE;
long	  QDTVersion;
Rect	  theScreen[5];
Rect	  mainScreenRect;
myFInfo   *lastGlobal_df = NULL;
struct SelectedFilesList *SelectedFiles;
struct MacWorld theWorld;

EventRecord *gCurrentEvent;

EventRecord *GetCurrentEventRecord(void) {
	return gCurrentEvent;
}

void SetCurrentEventRecord(EventRecord *theEvent) {
	gCurrentEvent = theEvent;
}

void OpenSelectedFiles(void) {
	InstallAEProcs();
}

#define Gestalttest	0xA1AD
#define NoTrap	0xA89F

static char Movie_Init(void) {
	OSErr err;
	long QDfeature, OSfeature;
	extern PaletteHandle srcPalette;

	mycolors = GetCTable(72);
	if (mycolors == nil)
		return 0;
	srcPalette = NewPalette(((**mycolors).ctSize) + 1, mycolors, pmTolerant, 0);

//	Use Gestalt to find if QuickDraw and QuickTime is available.
#if !TARGET_API_MAC_CARBON
	if (NGetTrapAddress(Gestalttest, ToolTrap) != NGetTrapAddress(NoTrap, ToolTrap)) {
#endif
		err = Gestalt(gestaltQuickdrawVersion, &QDfeature);
		if (err)
			return 0;
		err = Gestalt(gestaltSystemVersion, &OSfeature);
		if (err)
			return 0;
		if ((QDfeature & 0x0f00) != 0x0200 && OSfeature < 0x0607)
			return 0;
		err = Gestalt(gestaltQuickTime, &QDfeature);
		if (err)
			return 0;
#if !TARGET_API_MAC_CARBON
	}
	else
		return 0;
#endif
	/*      Open QuickTime last. */
	if (EnterMovies()) {
		return 0;
	}
	return 1;
}

static OSStatus InitMLTE(void) {
	OSStatus							status;
	TXNMacOSPreferredFontDescription	defaults;  // fontID, pointSize, encoding, and fontStyle
    TXNInitOptions options;

	defaults.fontID = (unsigned long)0L;// kTXNDefaultFontName;
  	defaults.pointSize = kTXNDefaultFontSize;
  	defaults.encoding = CreateTextEncoding(kTextEncodingMacRoman, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat);
  	defaults.fontStyle 	= kTXNDefaultFontStyle;

	options = kTXNWantMoviesMask | kTXNWantSoundMask | kTXNWantGraphicsMask;

	status = TXNInitTextension(&defaults, 1, options);
	return(status);
}

void MacintoshInit(void){
    extern Boolean IsColorMonitor;
    extern Boolean isHaveURLAccess;
	long	tlong;
    UInt32 	urlaVersion;
    long	carbonVersion;
    OSStatus err;
#if (TARGET_API_MAC_CARBON == 1)
	InitCursor();
	if (TXNVersionInformation != (void*)kUnresolvedCFragSymbolAddress) // Check for availability of MLTE api
		InitMLTE(); // default settings for MLTE
#else
	register short i;
	THz		zone;

	zone = GetZone();
	tlong = ((long)LMGetCurStackBase()-(*(long *)zone)-sizeof(Zone))*9/10;
	if (tlong % 2)
		tlong++;
	SetApplLimit((Ptr)((*(long *)zone) + sizeof(Zone) + tlong));
	MaxApplZone();

	InitGraf(&qd.thePort);		/* initialize Mac stuff */
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs((void *)BombResume);
	InitCursor();
	for (i=0; i < 15; i++) MoreMasters(); 
#endif

	isMovieAvialable = Movie_Init();

    isHaveURLAccess = FALSE;
    if (Gestalt(gestaltCarbonVersion, &carbonVersion) != noErr)
    	carbonVersion = 0;
    if (URLAccessAvailable() && (carbonVersion >= 0x130)) {	// Must have CarbonLib 1.3 or greater.
        err = URLGetURLAccessVersion(&urlaVersion);
        if (urlaVersion >= 0x200 || urlaVersion == 0)		// Check to make sure this is at least URL Access 2.0.
			isHaveURLAccess = TRUE;
    }


	IBNibRef mNibs;
	err = CreateNibReference(CFSTR("CLAN"), &mNibs);
	if (err == noErr) {
		err = SetMenuBarFromNib(mNibs, CFSTR("MenuBar"));
		DisposeNibReference(mNibs);
	}
	
	appleMenu = GetMenuHandle(appleID);
	FileMenu  = GetMenuHandle(File_Menu_ID);
	EditMenu  = GetMenuHandle(Edit_Menu_ID);
	FontMenu  = GetMenuHandle(Font_Menu_ID);
//	AppendResMenu(FontMenu, 'FONT');
	OSStatus status;
	status = CreateStandardFontMenu(FontMenu, 0, 0, 0, NULL);
	SizeMenu  = GetMenuHandle(Size_Menu_ID);
	TiersMenu = GetMenuHandle(Tier_Menu_ID);
	ModeMenu  = GetMenuHandle(Mode_Menu_ID);
	SetItemMark(ModeMenu,2,checkMark);
	if (isChatLineNums)
		SetItemMark(ModeMenu,3,checkMark);
	WindowMenu= GetMenuHandle(Windows_Menu_ID);
	HMenu_ID = 11;
	HelpMenu = NewMenu(HMenu_ID, "\pHelp");
	HMenuItemOffset = 0;
	AppendMenu(HelpMenu,"\pCommands and Shortcuts");
	AppendMenu(HelpMenu,"\pSave Them to File ...");
	InsertMenu(HelpMenu, 0);
//	HelpMenu = GetMenuHandle(HMenu_ID);
	
	currentKeyScript = GetScriptManagerVariable(smKeyScript);
	if ((Gestalt(gestaltQuickdrawFeatures,&tlong) == noErr) && (tlong & (1L << gestaltHasColor)))
		IsColorMonitor = TRUE;
	else
		IsColorMonitor = FALSE;
    theWorld.isOldMac     = false;
    theWorld.hasColorQD   = true;
    theWorld.isOldVersion = false;

// Unicode handler
	lastKeyLayoutID = GetScriptVariable(currentKeyScript, smScriptKeys);
	deadKeyState = 0L;
	uchrHandle = GetResource('uchr', (short)lastKeyLayoutID);

	GetWorldRect();

//	DrawMenuBar();
	if (Gestalt(gestaltQDTextVersion, &QDTVersion) != noErr)
		QDTVersion = 0L;
// if (QDTVersion >= gestaltMacOSXQDText) then in OS X

}

static void Text_MovieWindow(WindowProcRec *windProc) {
	if (!windProc)
		return;

	if (windProc->id == 1962)
		isWinExists(500, Movie_sound_str, false);
	else if (windProc->id == 500) {
		if (theMovie != NULL && (theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
			strcpy(FileName1, theMovie->df->fileName);
			isWinExists(1962, FileName1, false);
		}
	}
}

static void DoWindowsMenu(short theItem, WindowProcRec *windProc) {
	CFStringRef outString;
	FNType		ItemStr[FNSize];
	WindowPtr	wind;

	if (theItem == 1) {
		OpenCommandsWindow(FALSE);
	} else if (theItem == 2) {
		Text_MovieWindow(windProc);
	} else if (theItem == 3) {
		PlaybackControlWindow();
	} else if (theItem == 4) {
		OpenCharsWindow();
	} else if (theItem == 5) {
		OpenWebWindow();
	} else if (theItem == WINDASHNUM) {
		return;
	} else {
		CopyMenuItemTextAsCFString(WindowMenu, theItem, &outString);
		my_CFStringGetBytes(outString, ItemStr, FNSize);
		CFRelease(outString);
		wind = FindAWindowNamed(ItemStr);
		if (wind) {
			changeCurrentWindow(FrontWindow(), wind, true);
		}
	}
}
/* 2009-10-13
static pascal void BombResume(void) {
	exit(1);
}
*/
char isInsideTotalScreen(Rect *r) {
	if (r->top >= theTotalScreen.top && r->bottom <= theTotalScreen.bottom &&
		r->left >= theTotalScreen.left && r->right <= theTotalScreen.right)
		return(TRUE);
	else
		return(FALSE);
}

char isInsideScreen(short top, short left, Rect *res) {
	int i;
	Rect *s;
	
	for (i=0; i < screenCnt; i++) {
		s = &theScreen[i];
		if (left > (*s).left && left < (*s).right && 
			top  > (*s).top && top   < (*s).bottom) {
			if (res) *res = *s;
			return(TRUE);
		}
	}
	return(FALSE);
}

void FinishMainLoop(void) {
	if (global_df == NULL)
		return;
    global_df->WinChange = FALSE;
	if (!global_df->err_message[0] && global_df->SoundWin != NULL) 
		PutSoundStats(global_df->SnTr.WEndF - global_df->SnTr.WBegF);
	else
		draw_mid_wm();
	if (CheckLeftCol(global_df->col_win)) 
		DisplayTextWindow(NULL, 1);

    global_df->WinChange = TRUE;
	wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
	wrefresh(global_df->w1);
	if (global_df->LastCommand ==  7 || global_df->LastCommand == 12 || 
	    global_df->LastCommand == 16 || global_df->LastCommand == 17 ||
	    global_df->LastCommand == 18 || global_df->LastCommand == 19 ||
	    global_df->LastCommand == 23 || global_df->LastCommand == 35 ||
	    global_df->LastCommand == 36 || global_df->LastCommand == 41 ||
	    global_df->LastCommand == 44 || global_df->LastCommand == 45 ||
	    global_df->LastCommand == 76 || global_df->LastCommand == 78) {
		SetCurrentFontParams(global_df->row_txt->Font.FName, global_df->row_txt->Font.FSize);
		SetTextWinMenus(TRUE);
	}
}

void ControlAllControls(int on, short id) {
	if (global_df)
		EnableMenuItem(FileMenu, Select_Media);
	else
		DisableMenuItem(FileMenu, Select_Media);

	if (id == -1) {
		if (global_df)
			global_df->ScrollBar = 0;
 		DisableMenuItem(FileMenu, 0);
 
 		EnableMenuItem(EditMenu, 0);
 		DisableMenuItem(EditMenu, undo_id);
 		DisableMenuItem(EditMenu, redo_id);
  		EnableMenuItem(EditMenu, cut_id);
 		EnableMenuItem(EditMenu, copy_id);
 		EnableMenuItem(EditMenu, paste_id);
		EnableMenuItem(EditMenu, selectall_id);
 		DisableMenuItem(EditMenu, find_id);
 		DisableMenuItem(EditMenu, enterselct_id);
 		DisableMenuItem(EditMenu, findsame_id);
 		DisableMenuItem(EditMenu, replace_id);
 		DisableMenuItem(EditMenu, replacefind_id);
 		DisableMenuItem(EditMenu, goto_id);
 		DisableMenuItem(EditMenu, toupper_id);
 		DisableMenuItem(EditMenu, tolower_id);
 		DisableMenuItem(EditMenu, clanoptions_id);
 		DisableMenuItem(EditMenu, setdefsndana_id);
 		DisableMenuItem(EditMenu, setF5option_id);
 		DisableMenuItem(EditMenu, setlinenumsize_id);
 		DisableMenuItem(EditMenu, setURLaddress_id);
 		DisableMenuItem(EditMenu, setThumbnails_id);
 		DisableMenuItem(EditMenu, setStreamingSpeed_id);

		DisableMenuItem(FontMenu, 0);
		DisableMenuItem(SizeMenu, 0);
 		DisableMenuItem(TiersMenu, 0);
 		DisableMenuItem(ModeMenu, 0);
		DisableMenuItem(WindowMenu, 0);
 		DisableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+1));
 		DisableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+2));
 		DrawMenuBar();
 		return;
	}
	if (id == 139 || id == 155 || id == 145) {
 		DisableMenuItem(FileMenu, 0);
 		DisableMenuItem(EditMenu, 0);
		DisableMenuItem(FontMenu, 0);
		DisableMenuItem(SizeMenu, 0);
 		DisableMenuItem(TiersMenu, 0);
 		DisableMenuItem(ModeMenu, 0);
 		DisableMenuItem(WindowMenu, 0);
 		DisableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+1));
 		DisableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+2));
 		DrawMenuBar();
 		return;
	}
	if (id == 501 || id == 506) {
 		DisableMenuItem(TiersMenu, 0);
 		DisableMenuItem(ModeMenu, 0);
		EnableMenuItem(EditMenu, 0);
 		DisableMenuItem(EditMenu, undo_id);
 		DisableMenuItem(EditMenu, redo_id);
  		EnableMenuItem(EditMenu, cut_id);
 		EnableMenuItem(EditMenu, copy_id);
 		EnableMenuItem(EditMenu, paste_id);
		EnableMenuItem(EditMenu, selectall_id);
 		DisableMenuItem(EditMenu, find_id);
 		DisableMenuItem(EditMenu, enterselct_id);
 		DisableMenuItem(EditMenu, findsame_id);
 		DisableMenuItem(EditMenu, replace_id);
 		DisableMenuItem(EditMenu, replacefind_id);
 		DisableMenuItem(EditMenu, goto_id);
 		DisableMenuItem(EditMenu, toupper_id);
 		DisableMenuItem(EditMenu, tolower_id);
 		EnableMenuItem(EditMenu, clanoptions_id);
 		EnableMenuItem(EditMenu, setdefsndana_id);
 		EnableMenuItem(EditMenu, setF5option_id);
 		EnableMenuItem(EditMenu, setlinenumsize_id);
 		EnableMenuItem(EditMenu, setURLaddress_id);
 		EnableMenuItem(EditMenu, setThumbnails_id);
 		EnableMenuItem(EditMenu, setStreamingSpeed_id);

 		EnableMenuItem(FontMenu, 0);
		EnableMenuItem(SizeMenu, 0);
 		DisableMenuItem(SizeMenu, Font_Plain);
 		DisableMenuItem(SizeMenu, Font_Underline);
 		DisableMenuItem(SizeMenu, Font_Italic);
 		DisableMenuItem(SizeMenu, Font_Bold);
 		DisableMenuItem(SizeMenu, Font_Color_Keywords);
 // 18-07-2008			DisableMenuItem(SizeMenu, Font_Color_Words);
 		EnableMenuItem(FileMenu, 0);
 		EnableMenuItem(WindowMenu, 0);
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+1));
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+2));
 		DrawMenuBar();
 		return;
	}
	if (id == 500) {
 		DisableMenuItem(TiersMenu, 0);
 		EnableMenuItem(ModeMenu, 0);
 		DisableMenuItem(ModeMenu, 1);
 		DisableMenuItem(ModeMenu, 2);
 		EnableMenuItem(ModeMenu, 3);
 		DisableMenuItem(ModeMenu, 4);
 		EnableMenuItem(ModeMenu, 5);
 		DisableMenuItem(ModeMenu, 7);
 		EnableMenuItem(ModeMenu, 8);
 		DisableMenuItem(ModeMenu, 9);
 		DisableMenuItem(ModeMenu, 10);
 		DisableMenuItem(ModeMenu, 11);
 		DisableMenuItem(ModeMenu, 12);
 		DisableMenuItem(ModeMenu, 13);
 		DisableMenuItem(ModeMenu, 14);
 		DisableMenuItem(ModeMenu, 15);
 		DisableMenuItem(ModeMenu, 16);
 		DisableMenuItem(ModeMenu, 17);
 		DisableMenuItem(ModeMenu, 18);
// 		DisableMenuItem(ModeMenu, 19);
		EnableMenuItem(EditMenu, 0);
 		EnableMenuItem(EditMenu, undo_id);
 		EnableMenuItem(EditMenu, redo_id);
  		EnableMenuItem(EditMenu, cut_id);
 		EnableMenuItem(EditMenu, copy_id);
 		EnableMenuItem(EditMenu, paste_id);
		EnableMenuItem(EditMenu, selectall_id);
 		DisableMenuItem(EditMenu, find_id);
 		DisableMenuItem(EditMenu, enterselct_id);
 		DisableMenuItem(EditMenu, findsame_id);
 		DisableMenuItem(EditMenu, replace_id);
 		DisableMenuItem(EditMenu, replacefind_id);
 		DisableMenuItem(EditMenu, goto_id);
 		DisableMenuItem(EditMenu, toupper_id);
 		DisableMenuItem(EditMenu, tolower_id);
 		EnableMenuItem(EditMenu, clanoptions_id);
 		EnableMenuItem(EditMenu, setdefsndana_id);
 		EnableMenuItem(EditMenu, setF5option_id);
 		EnableMenuItem(EditMenu, setlinenumsize_id);
		EnableMenuItem(EditMenu, setURLaddress_id);
 		EnableMenuItem(EditMenu, setThumbnails_id);
 		EnableMenuItem(EditMenu, setStreamingSpeed_id);

 		DisableMenuItem(FontMenu, 0);
 		DisableMenuItem(SizeMenu, 0);
 		EnableMenuItem(FileMenu, 0);
 		EnableMenuItem(WindowMenu, 0);
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+1));
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+2));
 		DrawMenuBar();
 		return;
	}
	if (id == 504) {
 		EnableMenuItem(EditMenu, 0);
 		DisableMenuItem(EditMenu, undo_id);
 		DisableMenuItem(EditMenu, redo_id);
  		DisableMenuItem(EditMenu, cut_id);
 		DisableMenuItem(EditMenu, copy_id);
 		DisableMenuItem(EditMenu, paste_id);
		DisableMenuItem(EditMenu, selectall_id);
 		DisableMenuItem(EditMenu, find_id);
 		DisableMenuItem(EditMenu, enterselct_id);
 		DisableMenuItem(EditMenu, findsame_id);
 		DisableMenuItem(EditMenu, replace_id);
 		DisableMenuItem(EditMenu, replacefind_id);
 		DisableMenuItem(EditMenu, goto_id);
 		DisableMenuItem(EditMenu, toupper_id);
 		DisableMenuItem(EditMenu, tolower_id);
 		EnableMenuItem(EditMenu, clanoptions_id);
 		EnableMenuItem(EditMenu, setdefsndana_id);
 		EnableMenuItem(EditMenu, setF5option_id);
 		EnableMenuItem(EditMenu, setlinenumsize_id);
		EnableMenuItem(EditMenu, setURLaddress_id);
 		EnableMenuItem(EditMenu, setThumbnails_id);
 		EnableMenuItem(EditMenu, setStreamingSpeed_id);

		DisableMenuItem(FontMenu, 0);
		DisableMenuItem(SizeMenu, 0);
 		DisableMenuItem(SizeMenu, Font_Plain);
 		DisableMenuItem(SizeMenu, Font_Underline);
 		DisableMenuItem(SizeMenu, Font_Italic);
  		DisableMenuItem(SizeMenu, Font_Bold);
 		DisableMenuItem(SizeMenu, Font_Color_Keywords);
// 18-07-2008			DisableMenuItem(SizeMenu, Font_Color_Words);
 		DisableMenuItem(TiersMenu, 0);
 		DisableMenuItem(ModeMenu, 0);
 		EnableMenuItem(FileMenu, 0);
 		EnableMenuItem(WindowMenu, 0);
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+1));
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+2));
 		DrawMenuBar();
 		return;
	}
	if (global_df == NULL) {
 		EnableMenuItem(EditMenu, 0);
 		DisableMenuItem(EditMenu, undo_id);
 		DisableMenuItem(EditMenu, redo_id);
  		DisableMenuItem(EditMenu, cut_id);
 		DisableMenuItem(EditMenu, copy_id);
 		DisableMenuItem(EditMenu, paste_id);
		DisableMenuItem(EditMenu, selectall_id);
 		DisableMenuItem(EditMenu, find_id);
 		DisableMenuItem(EditMenu, enterselct_id);
 		DisableMenuItem(EditMenu, findsame_id);
 		DisableMenuItem(EditMenu, replace_id);
 		DisableMenuItem(EditMenu, replacefind_id);
 		DisableMenuItem(EditMenu, goto_id);
 		DisableMenuItem(EditMenu, toupper_id);
 		DisableMenuItem(EditMenu, tolower_id);
 		EnableMenuItem(EditMenu, clanoptions_id);
 		EnableMenuItem(EditMenu, setdefsndana_id);
 		EnableMenuItem(EditMenu, setF5option_id);
 		EnableMenuItem(EditMenu, setlinenumsize_id);
		EnableMenuItem(EditMenu, setURLaddress_id);
 		EnableMenuItem(EditMenu, setThumbnails_id);
 		EnableMenuItem(EditMenu, setStreamingSpeed_id);

		EnableMenuItem(FontMenu, 0);
		EnableMenuItem(SizeMenu, 0);
 		DisableMenuItem(SizeMenu, Font_Plain);
 		DisableMenuItem(SizeMenu, Font_Underline);
 		DisableMenuItem(SizeMenu, Font_Italic);
 		DisableMenuItem(SizeMenu, Font_Bold);
 		DisableMenuItem(SizeMenu, Font_Color_Keywords);
// 18-07-2008	 		DisableMenuItem(SizeMenu, Font_Color_Words);
 		DisableMenuItem(TiersMenu, 0);
 		DisableMenuItem(ModeMenu, 0);
 		EnableMenuItem(FileMenu, 0);
 		EnableMenuItem(WindowMenu, 0);
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+1));
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+2));
 		DrawMenuBar();
 		return;
	}
 	global_df->ScrollBar = (char)on;
 	if (global_df->ScrollBar == '\001') {
		EnableMenuItem(FileMenu, 0);
		EnableMenuItem(EditMenu, 0);
 		EnableMenuItem(EditMenu, undo_id);
 		EnableMenuItem(EditMenu, redo_id);
  		EnableMenuItem(EditMenu, cut_id);
 		EnableMenuItem(EditMenu, copy_id);
 		EnableMenuItem(EditMenu, paste_id);
		EnableMenuItem(EditMenu, selectall_id);
 		EnableMenuItem(EditMenu, find_id);
 		EnableMenuItem(EditMenu, enterselct_id);
 		EnableMenuItem(EditMenu, findsame_id);
 		EnableMenuItem(EditMenu, replace_id);
 		EnableMenuItem(EditMenu, replacefind_id);
 		EnableMenuItem(EditMenu, goto_id);
 		EnableMenuItem(EditMenu, toupper_id);
 		EnableMenuItem(EditMenu, tolower_id);
 		EnableMenuItem(EditMenu, clanoptions_id);
 		EnableMenuItem(EditMenu, setdefsndana_id);
 		EnableMenuItem(EditMenu, setF5option_id);
 		EnableMenuItem(EditMenu, setlinenumsize_id);
		EnableMenuItem(EditMenu, setURLaddress_id);
 		EnableMenuItem(EditMenu, setThumbnails_id);
 		EnableMenuItem(EditMenu, setStreamingSpeed_id);

		EnableMenuItem(FontMenu, 0);
		EnableMenuItem(SizeMenu, 0);
 		EnableMenuItem(SizeMenu, Font_Plain);
 		EnableMenuItem(SizeMenu, Font_Underline);
 		EnableMenuItem(SizeMenu, Font_Italic);
 		EnableMenuItem(SizeMenu, Font_Bold);
 		EnableMenuItem(SizeMenu, Font_Color_Keywords);
// 18-07-2008	 		EnableMenuItem(SizeMenu, Font_Color_Words);
 		EnableMenuItem(TiersMenu, 0);
 		EnableMenuItem(ModeMenu, 0);
 		EnableMenuItem(ModeMenu, 1);
 		EnableMenuItem(ModeMenu, 2);
 		EnableMenuItem(ModeMenu, 3);
 		EnableMenuItem(ModeMenu, 4);
 		EnableMenuItem(ModeMenu, 5);
 		EnableMenuItem(ModeMenu, 7);
 		EnableMenuItem(ModeMenu, 8);
 		EnableMenuItem(ModeMenu, 9);
 		EnableMenuItem(ModeMenu, 10);
 		EnableMenuItem(ModeMenu, 11);
 		EnableMenuItem(ModeMenu, 12);
 		EnableMenuItem(ModeMenu, 13);
 		EnableMenuItem(ModeMenu, 14);
 		EnableMenuItem(ModeMenu, 15);
 		EnableMenuItem(ModeMenu, 16);
 		EnableMenuItem(ModeMenu, 17);
 		EnableMenuItem(ModeMenu, 18);
// 		EnableMenuItem(ModeMenu, 19);
 		EnableMenuItem(WindowMenu, 0);
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+1));
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+2));
		DrawMenuBar();
 	} else {
 		EnableMenuItem(FileMenu, 0);
		if (global_df->UndoList->NextUndo && (id == -1 || on == 2)) {
 			EnableMenuItem(EditMenu, 0);
	 		EnableMenuItem(EditMenu, undo_id);
	 		EnableMenuItem(EditMenu, redo_id);
	  		DisableMenuItem(EditMenu, cut_id);
	 		DisableMenuItem(EditMenu, copy_id);
	 		DisableMenuItem(EditMenu, paste_id);
			DisableMenuItem(EditMenu, selectall_id);
	 		DisableMenuItem(EditMenu, find_id);
	 		DisableMenuItem(EditMenu, enterselct_id);
	 		DisableMenuItem(EditMenu, findsame_id);
	 		DisableMenuItem(EditMenu, replace_id);
 			DisableMenuItem(EditMenu, replacefind_id);
	 		DisableMenuItem(EditMenu, goto_id);
	 		DisableMenuItem(EditMenu, toupper_id);
	 		DisableMenuItem(EditMenu, tolower_id);
	 		EnableMenuItem(EditMenu, clanoptions_id);
			EnableMenuItem(EditMenu, setdefsndana_id);
 			EnableMenuItem(EditMenu, setF5option_id);
 			EnableMenuItem(EditMenu, setlinenumsize_id);
			EnableMenuItem(EditMenu, setURLaddress_id);
			EnableMenuItem(EditMenu, setThumbnails_id);
			EnableMenuItem(EditMenu, setStreamingSpeed_id);
		} else
			DisableMenuItem(EditMenu, 0);
 		DisableMenuItem(FontMenu, 0);
 		DisableMenuItem(SizeMenu, 0);
 		DisableMenuItem(SizeMenu, Font_Plain);
 		DisableMenuItem(SizeMenu, Font_Underline);
 		DisableMenuItem(SizeMenu, Font_Italic);
 		DisableMenuItem(SizeMenu, Font_Bold);
 		DisableMenuItem(SizeMenu, Font_Color_Keywords);
// 18-07-2008	 		DisableMenuItem(SizeMenu, Font_Color_Words);
 		DisableMenuItem(TiersMenu, 0);
 		DisableMenuItem(ModeMenu, 0);
 		EnableMenuItem(WindowMenu, 0);
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+1));
 		EnableMenuItem(HelpMenu, (MenuItemIndex)(HMenuItemOffset+2));
 	}
}

void ChangeSpeakerMenuItem(void) {
	int		i;
	wchar_t	buf[256];
	CFStringRef	theStr = NULL;

	for (i=0; i < 10; i++) {
		if (global_df) {
			if (!GetSpeakerNames(buf, i, 254)) {
				strcpy(buf, " ");
			}
		} else
			strcpy(buf, " ");

		theStr = CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar *)buf, strlen(buf));
		if (i == 0)
			SetMenuItemTextWithCFString(TiersMenu, 10, theStr);
		else
			SetMenuItemTextWithCFString(TiersMenu, i, theStr);
		CFRelease(theStr);
	}
}

void ChangeMacroMenuItem(char *temp, int i) {
    if (i == 0)
    	i = 10;
	if (*temp == EOS)
		temp = " ";
}

void ChangeWindowsMenuItem(FNType *temp, int add) {
	MenuItemIndex i, j, fc;
	CFStringRef outString;
	CFStringRef	theStr;

	theStr = my_CFStringCreateWithBytes(temp);
	if (theStr == NULL)
		return;

	fc = CountMenuItems(WindowMenu);
	for (i=1; i <= fc; i++) {
		if (i != WINDASHNUM) {
			CopyMenuItemTextAsCFString(WindowMenu, i, &outString);
			if (CFStringCompare(theStr, outString, kCFCompareCaseInsensitive) == 0) 
				break;
			CFRelease(outString);
		}
	}
	if (add) {
		for (j=1; j <= fc; j++) {
			if (j != WINDASHNUM)
				SetItemMark(WindowMenu,j,noMark);
		}
		if (i > fc) {
			AppendMenuItemTextWithCFString(WindowMenu, theStr, kMenuItemAttrNotPreviousAlternate, 0, &i);
		}
		if (i != 2 && add != '\002')
			SetItemMark(WindowMenu,i,checkMark);
	} else if (i > WINDASHNUM) {
		DeleteMenuItem(WindowMenu, i);
	} else if (i != WINDASHNUM)
		SetItemMark(WindowMenu,i,noMark);
	CFRelease(theStr);
}

void SetTextWinMenus(char isSampleAtt) {
	if (global_df == NULL) {
		SetItemMark(ModeMenu,2,noMark);
		if (isChatLineNums)
			SetItemMark(ModeMenu,3,checkMark);
		else
			SetItemMark(ModeMenu,3,noMark);
	    if (isPlayAudioFirst) {
			SetItemMark(ModeMenu,5,checkMark);
	    } else {
			SetItemMark(ModeMenu,5,noMark);
	    }
    } else {
	    if (global_df->EditorMode) {
			SetItemMark(ModeMenu,1,noMark);
	    } else {
			SetItemMark(ModeMenu,1,checkMark);
	    }
	    if (global_df->ChatMode) {
			DisableMenuItem(SizeMenu, Font_Italic);
			DisableMenuItem(SizeMenu, Font_Bold);
			SetItemMark(ModeMenu,2,checkMark);
	    } else {
			EnableMenuItem(SizeMenu, Font_Italic);
			EnableMenuItem(SizeMenu, Font_Bold);
	    	SetItemMark(ModeMenu,2,noMark);
		}
		if (isChatLineNums)
			SetItemMark(ModeMenu,3,checkMark);
		else
			SetItemMark(ModeMenu,3,noMark);
	    if (global_df->SoundWin) {
			SetItemMark(ModeMenu,4,checkMark);
	    } else {
			SetItemMark(ModeMenu,4,noMark);
	    }
	    if (isPlayAudioFirst) {
			SetItemMark(ModeMenu,5,checkMark);
	    } else {
			SetItemMark(ModeMenu,5,noMark);
	    }
		if (global_df->headtier != NULL) {
			SetMenuItemText(ModeMenu, 16, "\pShow all tiers");
		} else {
			SetMenuItemText(ModeMenu, 16, "\pHide tiers in \"0hide.cut\"");
		}
	    if (global_df->ShowParags != '\002') {
			SetMenuItemText(ModeMenu, 18, "\pExpand bullets {Esc-a}");
		} else {
			SetMenuItemText(ModeMenu, 18, "\pHide bullets {Esc-a}");
		}

		if (isSampleAtt) {
			if (global_df->row_txt == global_df->cur_line) {
				if (global_df->col_txt->prev_char != global_df->head_row)
					global_df->gAtt = global_df->col_txt->prev_char->att;
				else
					global_df->gAtt = 0;
			} else if (global_df->row_txt->att != NULL) {
				if (global_df->col_chr > 0)
					global_df->gAtt = global_df->row_txt->att[global_df->col_chr-1];
				else
					global_df->gAtt = 0;
			} else
				global_df->gAtt = 0;
		}
	    if (global_df->gAtt == 0) {
			SetItemMark(SizeMenu,Font_Plain,checkMark);
	    } else {
			SetItemMark(SizeMenu,Font_Plain,noMark);
	    }
	    if (is_underline(global_df->gAtt)) {
			SetItemMark(SizeMenu,Font_Underline,checkMark);
	    } else {
			SetItemMark(SizeMenu,Font_Underline,noMark);
	    }
	    if (is_italic(global_df->gAtt)) {
			SetItemMark(SizeMenu,Font_Italic,checkMark);
	    } else {
			SetItemMark(SizeMenu,Font_Italic,noMark);
	    }
	    if (is_bold(global_df->gAtt)) {
			SetItemMark(SizeMenu,Font_Bold,checkMark);
	    } else {
			SetItemMark(SizeMenu,Font_Bold,noMark);
	    }
/* 18-07-2008	
	    if (is_word_color(global_df->gAtt)) {
			SetItemMark(SizeMenu,Font_Color_Words,checkMark);
	    } else {
			SetItemMark(SizeMenu,Font_Color_Words,noMark);
	    }
*/
	}
}

char GetPrefSize(short id, FNType *name, long *height, long *width, long *top, long *left) {
    Handle InfoHand;
	FSRef  ref;
	short ResRefNum;
	short oldResRefNum;
	struct MPSR1010 *WindowSizeInfo;

	if ((id == 139 || id == 155) && (WarningWinSize.top || WarningWinSize.width || WarningWinSize.height || WarningWinSize.left)) {
		*height = WarningWinSize.height;
		*width = WarningWinSize.width;
		*top = WarningWinSize.top;
		*left = WarningWinSize.left;
		return(TRUE);
	} else if (id == 145 && (ProgresWinSize.top || ProgresWinSize.width || ProgresWinSize.height || ProgresWinSize.left)) {
		*height = ProgresWinSize.height;
		*width = ProgresWinSize.width;
		*top = ProgresWinSize.top;
		*left = ProgresWinSize.left;
		return(TRUE);
	} else if (id == 500 && (MovieWinSize.top || MovieWinSize.left)) {
		*top = MovieWinSize.top;
		*left = MovieWinSize.left;
		return(TRUE);
	} else if (id == 501 && (ClanWinSize.top || ClanWinSize.width || ClanWinSize.height || ClanWinSize.left)) {
		*height = ClanWinSize.height;
		*width = ClanWinSize.width;
		*top = ClanWinSize.top;
		*left = ClanWinSize.left;
		return(TRUE);
	} else if (id == 502) {
		return(TRUE);
	} else if (id == 503) {
		return(TRUE);
	} else if (id == 504 && (PictWinSize.top || PictWinSize.left)) {
		*top = PictWinSize.top;
		*left = PictWinSize.left;
		return(TRUE);
	} else if (id == 505 && (MVHLPWinSize.top || MVHLPWinSize.width || MVHLPWinSize.height || MVHLPWinSize.left)) {
		*height = MVHLPWinSize.height;
		*width = MVHLPWinSize.width;
		*top = MVHLPWinSize.top;
		*left = MVHLPWinSize.left;
		return(TRUE);
	} else if (id == 506 && (PBCWinSize.top || PBCWinSize.width || PBCWinSize.height || PBCWinSize.left)) {
		*height = PBCWinSize.height;
		*width = PBCWinSize.width;
		*top = PBCWinSize.top;
		*left = PBCWinSize.left;
		return(TRUE);
	} else if (id == 1964 && (ClanOutSize.top || ClanOutSize.width || ClanOutSize.height || ClanOutSize.left)) {
		*height = ClanOutSize.height;
		*width = ClanOutSize.width;
		*top = ClanOutSize.top;
		*left = ClanOutSize.left;
		return(TRUE);
	} else if (id == 1965 && (ThumbnailWin.top || ThumbnailWin.width || ThumbnailWin.height || ThumbnailWin.left)) {
		*height = ThumbnailWin.height;
		*width = ThumbnailWin.width;
		*top = ThumbnailWin.top;
		*left = ThumbnailWin.left;
		return(TRUE);
	} else if (id == 1966 && (TextWinSize.top || TextWinSize.width || TextWinSize.height || TextWinSize.left)) {
		*height = TextWinSize.height;
		*width = TextWinSize.width;
		*top = TextWinSize.top;
		*left = TextWinSize.left;
		return(TRUE);
	} else if (id == 2000 && (RecallWinSize.top || RecallWinSize.width || RecallWinSize.height || RecallWinSize.left)) {
		*height = RecallWinSize.height;
		*width = RecallWinSize.width;
		*top = RecallWinSize.top;
		*left = RecallWinSize.left;
		return(TRUE);
	} else if (id == 2001 && (ProgsWinSize.top || ProgsWinSize.width || ProgsWinSize.height || ProgsWinSize.left)) {
		*height = ProgsWinSize.height;
		*width = ProgsWinSize.width;
		*top = ProgsWinSize.top;
		*left = ProgsWinSize.left;
		return(TRUE);
	} else if (id == 2002 && (HlpCmdsWinSize.top || HlpCmdsWinSize.width || HlpCmdsWinSize.height || HlpCmdsWinSize.left)) {
		*height = HlpCmdsWinSize.height;
		*width = HlpCmdsWinSize.width;
		*top = HlpCmdsWinSize.top;
		*left = HlpCmdsWinSize.left;
		return(TRUE);
	} else if (id == 2007 && (SpCharsWinSize.top || SpCharsWinSize.width || SpCharsWinSize.height || SpCharsWinSize.left)) {
		*height = SpCharsWinSize.height;
		*width = SpCharsWinSize.width;
		*top = SpCharsWinSize.top;
		*left = SpCharsWinSize.left;
		return(TRUE);
	} else if (id == 2009 && (WebItemsWinSize.top || WebItemsWinSize.width || WebItemsWinSize.height || WebItemsWinSize.left)) {
		*height = WebItemsWinSize.height;
		*width = WebItemsWinSize.width;
		*top = WebItemsWinSize.top;
		*left = WebItemsWinSize.left;
		return(TRUE);
	} else if (id == 2010 && (FolderWinSize.top || FolderWinSize.width || FolderWinSize.height || FolderWinSize.left)) {
		*height = FolderWinSize.height;
		*width = FolderWinSize.width;
		*top = FolderWinSize.top;
		*left = FolderWinSize.left;
		return(TRUE);
	} else if (id == 1962) {
		InfoHand = NULL;
		if (!DefWindowDims) {
			oldResRefNum = CurResFile();
			my_FSPathMakeRef(name, &ref); 
			if ((ResRefNum=FSOpenResFile(&ref, fsRdPerm)) != -1) {
				UseResFile(ResRefNum);
				if ((InfoHand=GetResource('MPSR',1010))) {
					HLock(InfoHand);
					WindowSizeInfo = (struct MPSR1010 *)*InfoHand;
					*height = CFSwapInt16BigToHost(WindowSizeInfo->height);
					*width = CFSwapInt16BigToHost(WindowSizeInfo->width);
					*top = CFSwapInt16BigToHost(WindowSizeInfo->top);
					*left = CFSwapInt16BigToHost(WindowSizeInfo->left);
					HUnlock(InfoHand);
				}
				CloseResFile(ResRefNum);
			}
			UseResFile(oldResRefNum);
		}
		if (InfoHand == NULL || !isInsideScreen(*top, *left, NULL) || !isInsideScreen(*height, *width, NULL)) {
			if (defWinSize.top || defWinSize.width || defWinSize.height || defWinSize.left) {
				*height = defWinSize.height;
				*width = defWinSize.width;
				*top = defWinSize.top;
				*left = defWinSize.left;
			} else
				return(FALSE);
		}
		return(TRUE);
	}
	return(FALSE);
}

void SetWinStats(WindowPtr wp) {
    Handle InfoHand;
	char  dateFound;
	UInt32  dateValue;
	short ResRefNum;
	short oldResRefNum;
	FSRef ref;
	FSSpec fss;
	short width, height, left, top;
	WindowProcRec	*windProc;
	struct MPSR1010 *WindowSizeInfo;

	InfoHand = NULL;
	if ((windProc=WindowProcs(wp)) == NULL)
		return;
	GetWindTopLeft(wp, &left, &top);
	width = windWidth(wp);
	height = windHeight(wp);
	if (windProc->id == 139 || windProc->id == 155) {
		WarningWinSize.height = height;
		WarningWinSize.width = width;
		WarningWinSize.top = top;
		WarningWinSize.left = left;
	} else if (windProc->id == 145) {
		ProgresWinSize.height = height;
		ProgresWinSize.width = width;
		ProgresWinSize.top = top;
		ProgresWinSize.left = left;
	} else if (windProc->id == 500) {
		MovieWinSize.top = top;
		MovieWinSize.left = left;
	} else if (windProc->id == 501) {
		ClanWinSize.height = height;
		ClanWinSize.width = width;
		ClanWinSize.top = top;
		ClanWinSize.left = left;
	} else if (windProc->id == 502) {
	} else if (windProc->id == 503) {
	} else if (windProc->id == 504) {
		PictWinSize.top = top;
		PictWinSize.left = left;
	} else if (windProc->id == 505) {
		MVHLPWinSize.height = height;
		MVHLPWinSize.width = width;
		MVHLPWinSize.top = top;
		MVHLPWinSize.left = left;
	} else if (windProc->id == 506) {
		PBCWinSize.height = height;
		PBCWinSize.width = width;
		PBCWinSize.top = top;
		PBCWinSize.left = left;
	} else if (windProc->id == 1964) {
		ClanOutSize.height = height;
		ClanOutSize.width = width;
		ClanOutSize.top = top;
		ClanOutSize.left = left;
	} else if (windProc->id == 1965) {
		ThumbnailWin.height = height;
		ThumbnailWin.width = width;
		ThumbnailWin.top = top;
		ThumbnailWin.left = left;
	} else if (windProc->id == 1966) {
		TextWinSize.height = height;
		TextWinSize.width = width;
		TextWinSize.top = top;
		TextWinSize.left = left;
	} else if (windProc->id == 2000) {
		RecallWinSize.height = height;
		RecallWinSize.width = width;
		RecallWinSize.top = top;
		RecallWinSize.left = left;
	} else if (windProc->id == 2001) {
		ProgsWinSize.height = height;
		ProgsWinSize.width = width;
		ProgsWinSize.top = top;
		ProgsWinSize.left = left;
	} else if (windProc->id == 2002) {
		HlpCmdsWinSize.height = height;
		HlpCmdsWinSize.width = width;
		HlpCmdsWinSize.top = top;
		HlpCmdsWinSize.left = left;
	} else if (windProc->id == 2007) {
		SpCharsWinSize.height = height;
		SpCharsWinSize.width = width;
		SpCharsWinSize.top = top;
		SpCharsWinSize.left = left;
	} else if (windProc->id == 2009) {
		WebItemsWinSize.height = height;
		WebItemsWinSize.width = width;
		WebItemsWinSize.top = top;
		WebItemsWinSize.left = left;
	} else if (windProc->id == 2010) {
		FolderWinSize.height = height;
		FolderWinSize.width = width;
		FolderWinSize.top = top;
		FolderWinSize.left = left;
	} else if (windProc->id == 1962) {
		InfoHand = NULL;
		if (!DefWindowDims) {
			oldResRefNum = CurResFile();
			if (getFileDate(windProc->wname,&dateValue))
				dateFound = TRUE;
			else
				dateFound = FALSE;
			my_FSPathMakeRef(windProc->wname, &ref);
			FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, &fss, NULL);
			if ((ResRefNum=HOpenResFile(fss.vRefNum,fss.parID,fss.name,fsRdWrShPerm)) == -1) {
				if (!my_access(windProc->wname,0)) {
					HCreateResFile(fss.vRefNum,fss.parID,fss.name);
					ResRefNum=HOpenResFile(fss.vRefNum,fss.parID,fss.name,fsRdWrShPerm);
				}
			}
/*
			if ((ResRefNum=FSOpenResFile(&ref, fsRdWrShPerm)) == -1) {
				if (!my_access(windProc->wname,0)) {
					err = FSCreateResourceFork(&ref, 0, NULL, 0);
					ResRefNum = FSOpenResFile(&ref, fsRdWrShPerm);
				} else
					ResRefNum = -1;
			}
*/
			if (ResRefNum != -1) {
				UseResFile(ResRefNum);
				InfoHand = Get1Resource('MPSR',1010);
				if (InfoHand) {
					HLock(InfoHand);
					WindowSizeInfo = (struct MPSR1010 *)*InfoHand;
					WindowSizeInfo->top = CFSwapInt16HostToBig(top);
					WindowSizeInfo->left = CFSwapInt16HostToBig(left);
					WindowSizeInfo->height = CFSwapInt16HostToBig(height);
					WindowSizeInfo->width = CFSwapInt16HostToBig(width);
					HUnlock(InfoHand);
					ChangedResource(InfoHand);
				} else {
					InfoHand = NewHandle(sizeof(struct MPSR1010));
					if (InfoHand) {
						HLock(InfoHand);
						WindowSizeInfo = (struct MPSR1010 *)*InfoHand;
						WindowSizeInfo->top = top;
						WindowSizeInfo->left = left;
						WindowSizeInfo->height = height;
						WindowSizeInfo->width = width;
						HUnlock(InfoHand);
						AddResource(InfoHand, 'MPSR', 1010, "\p");
					}
				}
				CloseResFile(ResRefNum);
			}
			if (dateFound && InfoHand != NULL)
				setFileDate(windProc->wname,dateValue);
			UseResFile(oldResRefNum);
		}
		if (InfoHand == NULL || windProc->FileInfo->isTempFile == 1 || (!defWinSize.top && !defWinSize.width && !defWinSize.height && !defWinSize.left)) {
			defWinSize.height = height;
			defWinSize.width = width;
			defWinSize.top = top;
			defWinSize.left = left;
			InfoHand = NULL;
		}
	}
	if (InfoHand == NULL) {
		WriteCedPreference();
	}
}

static void GetWorldRect(void) {
	GDHandle device;

#if (TARGET_API_MAC_CARBON != 1)
	if (theWorld.hasColorQD) {
#endif
		device = GetDeviceList();
		do {
			if (TestDeviceAttribute(device, screenDevice) && TestDeviceAttribute(device, mainScreen)) {
				mainScreenRect = (**device).gdRect;
			}
			if (TestDeviceAttribute(device, screenDevice) && TestDeviceAttribute(device, screenActive)) {
				theScreen[screenCnt] = (**device).gdRect;
				screenCnt++;
				UnionRect(&theTotalScreen, &(**device).gdRect, &theTotalScreen);
			}
		} while (screenCnt < 5 && (device=GetNextDevice(device)));
#if (TARGET_API_MAC_CARBON != 1)
	} else {
		theScreen[screenCnt] = qd.screenBits.bounds;
		(theScreen[screenCnt]).top -= LMGetMBarHeight();
		mainScreenRect = theScreen[screenCnt];
		screenCnt++;
		theTotalScreen = qd.screenBits.bounds;
		theTotalScreen.top -= LMGetMBarHeight();
	}
#endif
}

void PrepareWindA4(WindowPtr wind, PrepareStruct *rec){
	GetPort(&rec->port);
	if (rec->port == GetWindowPort(wind))
		rec->port = 0L;
	else
		SetPortWindowPort(wind);
}


void RestoreWindA4(PrepareStruct *rec) {
	if (rec->port)
		SetPort(rec->port);
}

void ActivateFrontWindow(short active) {
	WindowPtr		window;
	WindowProcRec	*windProc;

	if ((window=FrontWindow()) != NULL) {
		windProc = WindowProcs(window);
		if (windProc != NULL && windProc->idocID != nil) {
			if (active) {
				ActivateTSMDocument(windProc->idocID);
			} else {
				DeactivateTSMDocument(windProc->idocID);
			}
		}
		HiliteWindow(window, active);
	}
}

static short IsGhostWindow(WindowPtr which) {
#if (TARGET_API_MAC_CARBON == 1)
	return(FALSE);
#else // (TARGET_API_MAC_CARBON == 1)
	WindowPtr	wind, front;

	if (!LMGetGhostWindow())
		return FALSE;
	
	wind = LMGetGhostWindow();
	front = FrontWindow();
	
	while (StripAddress(wind) != StripAddress(front)) {
		if (StripAddress(wind) == StripAddress(which))
			return TRUE;
		wind = (WindowPtr)((WindowPeek)wind)->nextWindow;
		if (!wind) /* error */;
	}
	
	return FALSE;
#endif // (TARGET_API_MAC_CARBON == 1)
}

short DoCommand(long mResult, WindowProcRec *windProc) {
	int			i;
	char		isHilited;
	short		theItem;
	short		result;
	short		this_menuID;

	theItem = LoWord(mResult);	/* get the item number */
	this_menuID = HiWord(mResult);
	
	result = -1;
	isHilited = TRUE;
	if (this_menuID == 0) ;
	else if (this_menuID == HMenu_ID) {
		if (theItem == (MenuItemIndex)(HMenuItemOffset+1)) {
			if (global_df == NULL)
				OpenHlpWindow("");
			else
				OpenHlpWindow(global_df->fileName);
		} else if (theItem == (MenuItemIndex)(HMenuItemOffset+2)) {
			AproposFile(-1);
		}
	} else
	switch (this_menuID) {
		case appleID:
			if (theItem == 1)
				AboutCLAN(FALSE);
			break;
		case File_Menu_ID:
			result = DoFile(theItem);
			break;
		case Edit_Menu_ID:
			DoEdit(theItem, windProc);
			break;
		case Font_Menu_ID:
			if (theItem == Font_Auto_Script) {
				AutoScriptSelect = !AutoScriptSelect;
				SetTextKeyScript(TRUE);
				WriteCedPreference();
			} else if (theItem == Font_Blank_3) {
			} else if (global_df) {
				DoFont(theItem);
			} else
				DoDefFont(theItem);
			break;
		case Size_Menu_ID:
			if (global_df)
				DoSize_Style(theItem);
			else
				DoDefSize_Style(theItem);
			break;
		case Tier_Menu_ID:
			if (global_df != NULL) {
				if (theItem > 0 && theItem < 11) {
					if (global_df->row_win < 0 || global_df->row_win >= (long)global_df->EdWinSize)
						PutCursorInWindow(global_df->w1);
					SaveUndoState(FALSE);
					if (theItem == 10)
						AddSpeakerNames(0);
					else
						AddSpeakerNames(theItem);
				} else if (theItem == 12) {
					for (i=0; i < 10; i++)
						AllocSpeakerNames(cl_T(""), i);
					SetUpParticipants();
					ChangeSpeakerMenuItem();
				} else if (theItem == 13) {
					setIDs(1);
				}
				FinishMainLoop();
			}
			break;
		case Mode_Menu_ID:
			if (theItem == 1) {		/* Editor mode */
				isPlayS = 4;
			} else if (theItem == 2) {	/* chat mode */
				isPlayS = 58;
			} else if (theItem == 3) { /* Show Line Numbers */
				isChatLineNums = ((isChatLineNums == TRUE) ? FALSE : TRUE);
				if (global_df != NULL) {
					RefreshAllTextWindows(TRUE);
					SetTextWinMenus(TRUE);
				}
			    WriteCedPreference();
			} else if (theItem == 4) { // Sound mode
				isPlayS = 63;
			} else if (theItem == 5) { // Play audio media first
				isPlayAudioFirst = !isPlayAudioFirst;
				isPlayAudioMenuSet = 1;
				SetTextWinMenus(FALSE);
			} else if (theItem == 7) { // Continuous playback
				isPlayS = 71;
			} else if (theItem == 8) { // Continuous playback skip silence
				isPlayS = 97;
			} else if (theItem == 9) { // Insert bullet into text
				SetCurrSoundTier(windProc);
				HiliteMenu(0);
				isHilited = FALSE;
				FinishMainLoop();
			} else if (theItem == 10) {	// Sound to text sync
				isPlayS = 50;
			} else if (theItem == 11) {	// Play bullet media {Esc-4}
				isPlayS = 49;
			} else if (theItem == 12) {	// Transcribe sound or movie
				isPlayS = 89;
			} else if (theItem == 13) {	// Movie thumbnails
				isPlayS = 80;
			} else if (theItem == 14) {	// check mode
				isPlayS = 72;
			} else if (theItem == 15) {	// Disambiguate tier
				isPlayS = 69;
			} else if (theItem == 16) {	// Exclude tiers in 0hide.cut
				if (global_df != NULL) {
					strcpy(global_df->err_message, DASHES);
					if (global_df->headtier != NULL)
						ShowAllTiers(1);
					else
						checkForTiersToHide(global_df, TRUE);
					DisplayTextWindow(NULL, 1);
					ResetUndos();
					FinishMainLoop();
				}
			} else if (theItem == 17) {	// Exclude tiers
				isPlayS = 65;
			} else if (theItem == 18) {	// Expand bullets
				isPlayS = 73;
			} else if (theItem == 19) {	// Send to Praat
				getTextAndSendToSoundAnalyzer();
			}
			break;
		case Windows_Menu_ID:
			DoWindowsMenu(theItem, windProc);
			break;

		default:
			break;
	}
	if (isHilited)
		HiliteMenu(0);
	return(result);					/* return with plea for continuance */
}

void UpdateWindowNamed(FNType *name) {
	WindowPtr		wind;
	WindowProcRec	*windProc;
	PrepareStruct	saveRec;
	
	if ((wind=FindAWindowNamed(name)) != NULL) {
		if ((windProc=WindowProcs(wind)) != NULL) {
			PrepareWindA4(wind, &saveRec);
			if (windProc->UpdateProc)
				windProc->UpdateProc(wind);/* Update this window */
			RestoreWindA4(&saveRec);
		}
	}
}

void UpdateWindowProc(WindowProcRec *windProc) {
	WindowPtr		wind;
	PrepareStruct	saveRec;

	if (windProc != NULL) {
		if ((wind=FindAWindowProc(windProc)) != NULL) {
			PrepareWindA4(wind, &saveRec);
			if (windProc->UpdateProc)
				windProc->UpdateProc(wind);/* Update this window */
			RestoreWindA4(&saveRec);
		}
	}
}

int TotalNumOfRows(WindowProcRec *windProc) {
	WindowPtr		wind;
	PrepareStruct	saveRec;
	int char_height;
	int local_num_rows;

	local_num_rows = 0;
	if (windProc != NULL) {
		if ((wind=FindAWindowProc(windProc)) != NULL) {
			PrepareWindA4(wind, &saveRec);
			local_num_rows = NumOfRowsInAWindow(wind, &char_height, 1);
			RestoreWindA4(&saveRec);
		}
	}
	return(local_num_rows);
}

static void resize(WindowPtr whichWindow) {
	Rect bounds;
	GrafPtr savePort;
	WindowProcRec	*windProc;

	GetPort(&savePort);
	SetPortWindowPort(whichWindow);
	GetWindowPortBounds(whichWindow, &bounds);
	EraseRect(&bounds);

	windProc = WindowProcs(whichWindow);

	if (windProc != NULL) {
		if (windProc->FileInfo != NULL) {
			long t;
			myFInfo *saveGlobal_df;

			saveGlobal_df = global_df;
			global_df = windProc->FileInfo;
			t = global_df->row_win;
			Re_WrapLines(AddLineToRow, 0L, TRUE, NULL);
			global_df->row_win = 0L;
			if (!init_windows(true, 1, false))
				mem_err(TRUE, global_df);
			global_df->row_win = t;
			wmove(global_df->w1, global_df->row_win, global_df->col_win - global_df->LeftCol);
			if (global_df->row_win < 0L || global_df->row_win >= (long)global_df->EdWinSize) 
				global_df->w1->cur_row = -1;
			RefreshOtherCursesWindows(FALSE);
			SetScrollControl();
			global_df = saveGlobal_df;
		} else if (windProc->id == 1965) ;
		else if (windProc->UpdateProc)
			windProc->UpdateProc(whichWindow);

#if (TARGET_API_MAC_CARBON == 1)
		InvalWindowRect(whichWindow, &bounds);
#else
		InvalRect(&bounds);
#endif
		if (windProc->VScrollHnd != NULL) {
#if (TARGET_API_MAC_CARBON != 1)
			HLock((Handle)windProc->VScrollHnd);/* Lock handle while we use it */
#endif
			HideControl(windProc->VScrollHnd); /* Hide it during size and move */
			SizeControl(windProc->VScrollHnd, SCROLL_BAR_SIZE, bounds.bottom-bounds.top-SCROLL_BAR_SIZE);
			MoveControl(windProc->VScrollHnd, bounds.right-bounds.left-SCROLL_BAR_SIZE, -1);
			ShowControl(windProc->VScrollHnd);   /* Safe to show it now */ 
#if (TARGET_API_MAC_CARBON != 1)
			HUnlock((Handle)windProc->VScrollHnd);/* Let it float again */
#endif
		}
		if (windProc->HScrollHnd != NULL) {
#if (TARGET_API_MAC_CARBON != 1)
			HLock((Handle)windProc->HScrollHnd);/* Lock handle while we use it */
#endif
			HideControl(windProc->HScrollHnd); /* Hide it during size and move */
			SizeControl(windProc->HScrollHnd, bounds.right-bounds.left-SCROLL_BAR_SIZE, SCROLL_BAR_SIZE);
			MoveControl(windProc->HScrollHnd, -1, bounds.bottom-SCROLL_BAR_SIZE);
			ShowControl(windProc->HScrollHnd);   /* Safe to show it now */ 
#if (TARGET_API_MAC_CARBON != 1)
			HUnlock((Handle)windProc->HScrollHnd);/* Let it float again */
#endif
		}
	}

	DrawGrowIcon(whichWindow);
	DrawControls(whichWindow); 		/* Draw all the controls */

	SetPort(savePort);	
	SetWinStats(whichWindow);
}

void SysEventCheck(long reps) {
	EventRecord myEvent;

    if (NumberOfReads < TickCount()) {
		NumberOfReads = TickCount() + (Size)((double)reps / (double)16.666666666);
		if (EventAvail(mDownMask | keyDownMask | keyUpMask | osMask, &myEvent)) {
			if (myEvent.what == keyDown || myEvent.what == keyUp) {
				if ((char)myEvent.message == '.' && myEvent.modifiers & cmdKey) {
					isKillProgram = 1;
					FlushEvents(keyDownMask, 0);
					FlushEvents(keyUpMask, 0);
					return;
				}
				if (!isBackground) {
					FlushEvents(keyDownMask, 0);
					FlushEvents(keyUpMask, 0);
				}
			}
			if (myEvent.what == mouseDown || myEvent.what == osEvt) {
				WaitNextEvent(mDownMask | osMask, &myEvent, 0, nil);
			}
		}
    }
}

char sameKeyPressed(int key) {
	return(FALSE);
}

void changeCurrentWindow(WindowPtr fromWindow, WindowPtr toWindow, char isSelectWin) {
	short fromId;
	short toId;
	WindowProcRec *fromWindProc;
	WindowProcRec *toWindProc;
	
	if ((fromWindProc=WindowProcs(fromWindow)) != NULL) {
		fromId = fromWindProc->id;
	} else
		fromId = 0;

	if (fromId == 1962 && isSelectWin)
		SetPBCglobal_df(false, 0L);
	if ((toWindProc=WindowProcs(toWindow)) != NULL) {
		toId = toWindProc->id;
		if (toId == 500)
			drawFakeHilight(TRUE, toWindow);
//		if (fromId == 500)
//			drawFakeHilight(FALSE, fromWindow);
		global_df = toWindProc->FileInfo;
		ChangeWindowsMenuItem(toWindProc->wname, TRUE);
		if (toId == 1962 && isSelectWin)
			SetPBCglobal_df(false, 0L);
	} else {
		toId = 0;
		global_df = NULL;
	}

	if (isSelectWin) {
		SelectWindow(toWindow);
	}
	ControlAllControls('\001', toId);
	SetTextKeyScript(FALSE);
	PrepWindow(toId);
	ChangeSpeakerMenuItem();
	SetTextWinMenus(TRUE);
	if (global_df != NULL)
		SetCurrentFontParams(global_df->row_txt->Font.FName, global_df->row_txt->Font.FSize);
	else
		SetCurrentFontParams(dFnt.fontId, dFnt.fontSize);
}

/****************************************************************************/
/*								MainEvent routine

	Contains the body of the Main Event loop.  Virtually completely copped
	from somewhere else.  Straightforward stuff.  Returns 0 if program
	to exit, returns 1 if should continue.
	
*/
/****************************************************************************/

short RealMainEvent(int *key) {
	register Boolean res;
	EventRecord		myEvent;	 /* keeps track of event */	
	WindowPtr		whichWindow, front; /* keeps track of event window */
	Rect			tempRect;
	WindowProcRec	*windProc;
	long			size;
	short			part, retVal, sleep;
	Point			tWhere;
	PrepareStruct	saveRec;
	myFInfo 		*saveGlobal_df;
	TimeValue 		tc;
	TimeScale		ts;
    GrafPtr			oldPort;
	RGBColor 		oldColor,
					theColor;

	if (isCloseProgressDialog) {
		isCloseProgressDialog = FALSE;
		CloseProgressDialog();
	}
	front = FrontWindow();
	windProc = WindowProcs(front);
	if (windProc != NULL && windProc->id == 1962)
		lastGlobal_df = windProc->FileInfo;
	sleep = isBackground ? 120 : 5;
	currentKeyScript = GetScriptManagerVariable(smKeyScript);
	if (windProc != NULL) {
		if (NonModal != 0 && NonModal != windProc->id && !isBackground) {
			if ((whichWindow=FindAWindowID(NonModal)) != NULL)
				changeCurrentWindow(front, whichWindow, true);
		}
	}
	if (theMovie != NULL && theMovie->isPlaying) {
		char move_cursorRes;
		while (!(res=WaitNextEvent(everyEvent, &myEvent, 0, nil))) {
			SetCurrentEventRecord(&myEvent);

			if (MCIsPlayerEvent(theMovie->MvController,&myEvent))
				continue;

			if (theMovie->isPlaying == 2) {
				tc = MCGetCurrentTime(theMovie->MvController, &ts);
				if (tc >= theMovie->MBeg) {
					SetMovieActiveSegment(theMovie->MvMovie, theMovie->MBeg, theMovie->MEnd-theMovie->MBeg);
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
					SetMovieTimeValue(theMovie->MvMovie, theMovie->MBeg);

					GetPort(&oldPort);
					SetPortWindowPort(theMovie->win);
					GetForeColor(&oldColor);
					theColor.red = 57670;
					theColor.green = 1;
					theColor.blue = 1;
					RGBForeColor(&theColor);
					tc = MCGetCurrentTime(theMovie->MvController, &ts);
					sprintf(templineC2, "%ld", conv_to_msec_mov(tc, ts));
					SetWindowMLTEText(templineC2, theMovie->win, 5);
					RGBForeColor(&oldColor);
					SetPort(oldPort);
				}
			} else if (theMovie->isPlaying) {
				tc = MCGetCurrentTime(theMovie->MvController, &ts);
				if (tc >= theMovie->MEnd && theMovie->isPlaying == TRUE) {
					theMovie->isPlaying = -2;
					MCDoAction(theMovie->MvController, mcActionPlay, 0);
				}
				if (PlayingContMovie && PlayingContMovie != '\003') {
					size = conv_to_msec_mov(tc,ts);
					if ((theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
						saveGlobal_df = global_df;
						global_df = theMovie->df;
						if (size < global_df->MvTr.MBeg || size > global_df->MvTr.MEnd || PlayingContMovie == '\001') {
							DrawFakeHilight(0);
							SaveUndoState(FALSE);
							if ((move_cursorRes=move_cursor(size, global_df->MvTr.MovieFile, TRUE, FALSE)) == FALSE) {
								global_df->row_win2 = 0L;
								global_df->col_win2 = -2L;
								global_df->col_chr2 = -2L;
							} else if (move_cursorRes == '\001')
								PlayingContMovie = '\002';
							DrawFakeHilight(1);
						}
						global_df = saveGlobal_df;
					}
					tc = MCGetCurrentTime(theMovie->MvController, &ts);
					sprintf(templineC2, "%ld", conv_to_msec_mov(tc, ts));
					SetWindowMLTEText(templineC2, theMovie->win, 5);
				} else {
					GetPort(&oldPort);
					SetPortWindowPort(theMovie->win);
					tc = MCGetCurrentTime(theMovie->MvController, &ts);
					sprintf(templineC2, "%ld", conv_to_msec_mov(tc, ts));
					SetWindowMLTEText(templineC2, theMovie->win, 5);
					SetPort(oldPort);
				}
				if (!PlayingContMovie || (PBC.walk && PBC.enable)) {
					DrawCursor(1); // lll
				}
			} else
				break;
		}
	} else
		res = WaitNextEvent(everyEvent, &myEvent, sleep, 0L);
	
	if (theMovie != NULL && theMovie->isPlaying == 0 && theMovie->df != NULL && theMovie->df->cpMvTr != NULL) {
		if ((theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
			saveGlobal_df = global_df;
			global_df = theMovie->df;
			CopyAndFreeMovieData(FALSE);
			PlayingContMovie = '\001';
			PlayMovie(&global_df->MvTr,global_df, FALSE);
			global_df = saveGlobal_df;
		}
	}

	if (theMovie != NULL && theMovie->isPlaying == 0) {
		PBC.walk = 0;
		PBC.isPC = 0;
	}
	
	SetCurrentEventRecord(&myEvent);
	if (windProc != NULL && windProc->FileInfo != NULL) {
		GrafPtr	oldPort;

		GetPort(&oldPort);
		SetPortWindowPort(front);
		tWhere = myEvent.where;
		GlobalToLocal(&tWhere);
		SetPort(oldPort);
		GetWindowPortBounds(front, &tempRect);
		if (tWhere.v > 0 && tWhere.v < windProc->FileInfo->TextWinSize &&
			tWhere.h > 0 && tWhere.h < tempRect.right-SCROLL_BAR_SIZE)
			DrawMouseCursor(0);
		else
			DrawMouseCursor(1);
	} else
		DrawMouseCursor(1);
	/* wait for event */
	if (res) {
		if (theMovie != NULL) {
			if (!theMovie->isPlaying) {
				lastWhen = myEvent.when;
				if (MCIsPlayerEvent(theMovie->MvController,&myEvent))
					return -1;
			} else if (myEvent.what == mouseDown) {
				if (MCIsPlayerEvent(theMovie->MvController,&myEvent))
					return -1;
				if (PlayingContMovie || !PBC.walk || !PBC.enable || windProc == NULL || windProc->id != 1962) {
					theMovie->isPlaying = -1;
					MCDoAction(theMovie->MvController, mcActionPlay, 0);
				}
			} else if (myEvent.what == keyDown || myEvent.what == autoKey) {
				if (MCIsPlayerEvent(theMovie->MvController,&myEvent))
					return -1;
				if (PlayingContMovie || !PBC.walk || !PBC.enable || windProc == NULL || windProc->id != 1962) {
					theMovie->isPlaying = -1;
					MCDoAction(theMovie->MvController, mcActionPlay, 0);
					if (UniInputBuf.len > 0)
						UniInputBuf.len--;
					return -1;
				}
			} else {
				if (MCIsPlayerEvent(theMovie->MvController,&myEvent))
					return -1;
			}
		}

		OptionKeyFound = 0;
		switch (myEvent.what) {			/* process event */
			case osEvt:
				/* Is it a Suspend/Resume event? */
				if ((myEvent.message & 0xFF000000) == 0x01000000){
					if (myEvent.message & 1){
						InitCursor();
						currentKeyScript = saveCurrentKeyScript;
						isBackground = FALSE;
						SetTextKeyScript(FALSE);
						if (RootWind == NULL && isFileFound == 0)
							isFileFound = 1;
						if (currentKeyScript == kTextEncodingMacJapanese    ||
							currentKeyScript == kTextEncodingMacChineseTrad ||
							currentKeyScript == kTextEncodingMacChineseSimp)
							KeyScript(0);
						ActivateFrontWindow(1);
						KeyScript(currentKeyScript);
						if (windProc != NULL && windProc->FileInfo != NULL) {
							if (windProc->FileInfo->isSpelling) {
								windProc->FileInfo->isSpelling = FALSE;
								saveGlobal_df = global_df;
								global_df = windProc->FileInfo;
								doPaste();
								global_df = saveGlobal_df;
							}
						}
						/* Resume Event */
					} else {
						saveCurrentKeyScript = currentKeyScript;
						isBackground = TRUE;
						cedDFnt.Encod = mScript;
						if (theMovie != NULL && theMovie->isPlaying) {
							theMovie->isPlaying = -1;
							MCDoAction(theMovie->MvController, mcActionPlay, 0);
						}
						/* Suspend Event */
						ActivateFrontWindow(0);
						KeyScript(mScript);
// quit if in the background and all windows are closed
//						if (RootWind == NULL)
//							Quit_CED(1999);
					}
				}
				break;
			case updateEvt:				/* if updateEvt event */
				whichWindow = (WindowPtr)myEvent.message;/* Get which window to update */
				windProc = WindowProcs(whichWindow);
				if (windProc != NULL && global_df != NULL) {
					DrawCursor(0);
					DrawSoundCursor(0);
				}
				if (windProc != NULL) {
					if (windProc->id == 500 && theMovie != NULL) {
						MCIsPlayerEvent(theMovie->MvController,&myEvent);
					}
					BeginUpdate(whichWindow);
					PrepareWindA4(whichWindow, &saveRec);
					if (windProc->UpdateProc)
						windProc->UpdateProc(whichWindow);/* Update this window */
					RestoreWindA4(&saveRec);
					EndUpdate(whichWindow);
				}
				break;
			case activateEvt:
				if ((whichWindow=FindAWindowNamed(warning_str)) == NULL)
					whichWindow = (WindowPtr)myEvent.message;
				else if (whichWindow != (WindowPtr)myEvent.message)
					SelectWindow(whichWindow);

				windProc = WindowProcs(whichWindow);
				if (NonModal == 0) {
					if ((myEvent.modifiers & activeFlag) != 0)
						changeCurrentWindow(front, whichWindow, false);
				}
				saveGlobal_df = global_df;
				if (windProc != NULL)
					global_df = windProc->FileInfo;
				else
					global_df = NULL;
				if (windProc != NULL && global_df != NULL && (myEvent.modifiers & activeFlag) == 0) {
					DrawCursor(0);
					DrawSoundCursor(0);
				}
				if (windProc != NULL){
#if (TARGET_API_MAC_CARBON == 1)
					if (windProc->idocID != nil) {
						if ((myEvent.modifiers & activeFlag) != 0) {
							if (currentKeyScript == kTextEncodingMacJapanese    ||
								currentKeyScript == kTextEncodingMacChineseTrad ||
								currentKeyScript == kTextEncodingMacChineseSimp)
								KeyScript(0);
							ActivateTSMDocument(windProc->idocID);
							if (currentKeyScript == kTextEncodingMacJapanese    ||
								currentKeyScript == kTextEncodingMacChineseTrad ||
								currentKeyScript == kTextEncodingMacChineseSimp)
								KeyScript(currentKeyScript);
						} else {
							DeactivateTSMDocument(windProc->idocID);
						}
					}
#endif
					if ((windProc->id < 1900 || windProc->id > 1999) && 
												windProc->EventProc) {
						PrepareWindA4(whichWindow, &saveRec);
						windProc->EventProc(whichWindow, &myEvent);
						RestoreWindA4(&saveRec);
					}
				}
				if (windProc != NULL && global_df != NULL && (myEvent.modifiers & activeFlag) != 0) {
					DrawCursor(1);
					DrawSoundCursor(1);
				}
				global_df = saveGlobal_df;
				break;
			case kHighLevelEvent:
				globalWhichInput = 0;
				if (global_df != NULL) {
					DrawCursor(0);
					if ((myEvent.what != keyDown && myEvent.what != autoKey) || 
							myEvent.modifiers & cmdKey)
						DrawSoundCursor(0);
				}
				AEProcessAppleEvent(&myEvent);
				break;
			case mouseDown:		/* mouse event */
				globalWhichInput = 0;
				if (PlayingContSound == TRUE || PlayingContSound == '\002') {
// 2007-11-04		if (myEvent.when - lastWhen < GetDblTime())
						PlayingContSound = '\003';
// 2007-11-04		else
// 2007-11-04			PlayingContSound = '\002';
					*key = 1;
					lastWhen = myEvent.when;
					return 1;
				} else if (PlayingContSound == '\004') {
// 2007-11-04		if (myEvent.when - lastWhen < GetDblTime())
						PlayingContSound = '\003';
					*key = 1;
					lastWhen = myEvent.when;
					return 1;
				} else if (PlayingSound && !PlayingContSound && !PBC.enable) {
					PlayingSound = FALSE;
					*key = 0;
					return 1;
				}
/*
				if (PlayingContMovie && windProc && theMovie != NULL && theMovie->isPlaying) {
					theMovie->isPlaying = -1;
					MCDoAction(theMovie->MvController, mcActionPlay, 0);
					lastWhen = myEvent.when;
					break;
				}
*/
				part = FindWindow(myEvent.where, &whichWindow);
				if (part != inContent && global_df != NULL) {
					DrawCursor(0);
					DrawSoundCursor(0);
				}

				if (global_df != NULL)
					global_df->cursorCol = 0L;

				switch (part) {
					case inDesk:
						if (PlayingSound && !PlayingContSound) {
							PlayingSound = FALSE;
							*key = 0;
							return 1;
						}
						break;
					case inMenuBar:
						/* do menu command if in menu */
						if (NonModal == 0 || (global_df == NULL && windProc->id != NonModal) || NonModal == 2010) {
							retVal = DoCommand(MenuSelect(myEvent.where), windProc);
							SetScrollControl();
							return(retVal);
						}
						break;
					case inSysWindow:
						if (PlayingSound && !PlayingContSound) {
							PlayingSound = FALSE;
							*key = 0;
							return 1;
						}
						/* do system stuff if in system window */
#if (TARGET_API_MAC_CARBON != 1)
						if (NonModal == 0 && !PlayingSound)
							SystemClick(&myEvent, whichWindow);
#endif
						break;
					case inZoomIn:
					case inZoomOut:
						if ((windProc=WindowProcs(whichWindow)) == NULL)
							break;
						if (NonModal == 0 || (global_df == NULL && windProc->id != NonModal)) {
							if (windProc && windProc->id == 500) {
								ResizeMovie(whichWindow, theMovie, 0, 1);
							} else {
								InitCursor();
								if (TrackBox(whichWindow, myEvent.where, part)){
									PrepareWindA4(whichWindow, &saveRec);
									GetWindowPortBounds(whichWindow, &tempRect);
									EraseRect(&tempRect);
									ZoomWindow(whichWindow, part, 0);
									resize(whichWindow);
									RestoreWindA4(&saveRec);
								}
							}
						}
						break;
					case inGrow:
						if ((windProc=WindowProcs(whichWindow)) == NULL)
							break;
						if (NonModal == 0 || (global_df == NULL && windProc->id != NonModal) || NonModal == 2010) {
							if (windProc && windProc->id == 500) {
								if ((size = GrowWindow(whichWindow, myEvent.where, &MovieLimit)) != 0L) {
									ResizeMovie(whichWindow,theMovie,LoWord(size),HiWord(size));
								}
							} else if (windProc && windProc->id == 504) {
								if ((size = GrowWindow(whichWindow, myEvent.where, &MainLimit)) != 0L) {
									ResizePicture(whichWindow,LoWord(size),HiWord(size));
								}
							} else {
								PrepareWindA4(whichWindow, &saveRec);
								InitCursor();
								if ((size = GrowWindow(whichWindow, myEvent.where, &MainLimit)) != 0L) {
									SizeWindow(whichWindow, LoWord(size), HiWord(size), 0);
									resize(whichWindow);
								}
								RestoreWindA4(&saveRec);
							}
						}
						break;
					case inContent: /* ^^^ */ /* (<- indicates fall-through in case) */
						if ((front!=whichWindow) && !IsGhostWindow(whichWindow)) {
							WindowProcRec *new_windProc;
							if (NonModal == 0) {
								changeCurrentWindow(front, whichWindow, true);
							}
							new_windProc = WindowProcs(whichWindow);
							if (windProc == NULL) {
								break;
							} else if (windProc->id == 2000 || windProc->id == 2001 || windProc->id == 2009 || windProc->id == 2010) {
								if (new_windProc == NULL || new_windProc->id != 501)
									break;
								else
									windProc = new_windProc;
							} else if (windProc->id == 2007) {
								break;
							} else if (windProc->id == 1962) {
								if (new_windProc == NULL || 
									(new_windProc->id != 500 && new_windProc->id != 504  && 
									 new_windProc->id != 506 && new_windProc->id != 1964 &&
									 new_windProc->id != 2007))
									break;
								else
									windProc = new_windProc;
							} else if (windProc->id == 500 || windProc->id == 506) {
								if (new_windProc == NULL)
									break;
								else
									windProc = new_windProc;
							} else
								break;
						}
						if (windProc) {
							if (windProc->EventProc) {
								PrepareWindA4(whichWindow, &saveRec);
								retVal=windProc->EventProc(whichWindow, &myEvent);
								RestoreWindA4(&saveRec);
								return retVal;
							}
						}
						break;
					case inDrag:
						if ((myEvent.modifiers & cmdKey) || Button()) {
							if (NonModal && whichWindow != front)
								break;
							windProc = WindowProcs(whichWindow);
							tempRect = theTotalScreen;
							SetRect(&tempRect, tempRect.left + 5, tempRect.top + 25, tempRect.right - 5, tempRect.bottom - 5);/*  l,t,r,b, drag area */
							DragWindow(whichWindow, myEvent.where, &tempRect);/* Drag the window */
							SetWinStats(whichWindow);
							changeCurrentWindow(front, whichWindow, true);
						} else if (NonModal == 0) {
							changeCurrentWindow(front, whichWindow, true);
						}
						break;
					case inGoAway:
						if (PlayingSound && !PlayingContSound) {
							PlayingSound = FALSE;
							*key = 0;
							return 1;
						}
						if (StdInWindow != NULL && global_df != NULL && !strcmp(StdInWindow,global_df->fileName))
							do_warning(StdInErrMessage, 0);
						else if (NonModal == 0 || (global_df == NULL && windProc->id != NonModal) || NonModal == 2010) {
							if (TrackGoAway(whichWindow, myEvent.where)) {
								WindowProcRec *new_windProc;
								new_windProc = WindowProcs(whichWindow);
								if (StdInWindow != NULL && new_windProc->FileInfo != NULL && !strcmp(StdInWindow,new_windProc->FileInfo->fileName))
									do_warning(StdInErrMessage, 0);
								else if (StdInWindow != NULL && new_windProc->id == 501 && !strcmp(StdInWindow,CLAN_Output_str))
									do_warning(StdInErrMessage, 0);
								else {
									PrepareWindA4(whichWindow, &saveRec);
									mCloseWindow(whichWindow);
									RestoreWindA4(&saveRec);
								}
							}
						} else {
							*key = CTRL('G');
							return 1;
						}
						break;
					default: break;
				}
				/* end switch for mouseDown case */
				if (PlayingSound && !PlayingContSound) {
					*key = 1;
					return 1;
				}
				break;
			case keyDown:				/* if keyboard event */
			case autoKey:
				whichWindow = FrontWindow();
				windProc = WindowProcs(whichWindow);
				if (global_df != NULL) {
					DrawCursor(0);
					if (myEvent.modifiers & cmdKey)
						DrawSoundCursor(0);
				}
				if (myEvent.modifiers & cmdKey) {
#if (TARGET_API_MAC_CARBON == 1)
//					if (windProc && windProc->idocID)
//						FixTSMDocument(windProc->idocID);
					UniInputBuf.len = 0L;
#endif
					if (NonModal) {
						if (global_df != NULL) {
						 	*key = CTRL('G');
							return 1;
						} else if (!IsArrowKeyCode((myEvent.message & keyCodeMask)/256)) {
							if (myEvent.message == autoKey)
								return -1;
							retVal = DoCommand(MenuKey(myEvent.message & charCodeMask), windProc);
							return retVal;
						}
					} else if (!IsArrowKeyCode((myEvent.message & keyCodeMask)/256)) {
						if (myEvent.message == autoKey)
							return -1;
						*key = myEvent.message & charCodeMask;
//						1 2 3 4 5 6 7 8 9 0
//						& é " ' ( § è ! ç à

						if (*key == 0x26) { // 38 - & (French keyboard)
							myEvent.message = '1' & charCodeMask;
						} else if (*key == 0x8e) { // 142 - é (French keyboard)
							myEvent.message = '2' & charCodeMask;
						} else if (*key == 0x22) {// 34 - " (French keyboard)
							myEvent.message = '3' & charCodeMask;
						} else if (*key == 0x27) { // 39 - '  (French keyboard)
							myEvent.message = '4' & charCodeMask;
						} else if (*key == 0x28) { // 40 - ( (French keyboard)
							myEvent.message = '5' & charCodeMask;
						} else if (*key == 0xa4) { // 164 - § (French keyboard)
							myEvent.message = '6' & charCodeMask;
						} else if (*key == 0x8f) { // 143 - è (French keyboard)
							myEvent.message = '7' & charCodeMask;
						} else if (*key == 0x21) { // 33 - ! (French keyboard)
							myEvent.message = '8' & charCodeMask;
						} else if (*key == 0x8d) { // 141 - ç (French keyboard)
							myEvent.message = '9' & charCodeMask;
						} else if (*key == 0x88) { // 136 - à  (French keyboard)
							myEvent.message = '0' & charCodeMask;
						} else if (*key == ',' || *key == '<') {
							myEvent.message = '[' & charCodeMask;
						} else if (*key == '.' || *key == '>') {
							myEvent.message = ']' & charCodeMask;
						} else if (*key == '?') {
							myEvent.message = '/' & charCodeMask;
						}
/* 2007-03-28
						if (windProc && (windProc->id == 1962 || windProc->id == 500) && (*key == '/' || *key == '?')) {
							Text_MovieWindow(windProc);	// for Susanne switch text to movie - movie to text							
							*key = -1;
							return -1;
						} else
*/
							retVal = DoCommand(MenuKey(myEvent.message & charCodeMask), windProc);
						SetScrollControl();
						if (PlayingSound && !PlayingContSound) {
							*key = 1;
							return 1;
						} else
							return retVal;
					} else if (windProc && windProc->id == 500) {
						if (windProc->EventProc) {
							PrepareWindA4(whichWindow, &saveRec);
							retVal=windProc->EventProc(whichWindow, &myEvent);
							RestoreWindA4(&saveRec);
							if (retVal != -1) {
								*key = retVal;
								retVal = 1;
							}
							return retVal;
						}					
					} else if (IsArrowKeyCode((myEvent.message & keyCodeMask)/256)) {
						if ((myEvent.message & keyCodeMask) == 0x7C00)
							isPlayS = 92;
						else if ((myEvent.message & keyCodeMask) == 0x7B00)
							isPlayS = 93;
						if (PlayingSound && !PlayingContSound) {
							*key = 2;
							return 1;
						} else
							return -1;
					}
				} else {
					/* Pass the typing on to the frontmost window */
					if (windProc) {
						if (windProc->EventProc) {
							PrepareWindA4(whichWindow, &saveRec);
							retVal=windProc->EventProc(whichWindow, &myEvent);
							RestoreWindA4(&saveRec);
#if (TARGET_API_MAC_CARBON == 1)
							if (windProc->textHandler != nil) {
								if (isPlayS || retVal == -1)
									UniInputBuf.len = 0L;
								else if (UniInputBuf.len == 0L)
									retVal = -1;
							} else {
								UniInputBuf.len = 0L;
							}
#endif
							if (retVal != -1) {
								*key = retVal;
								retVal = 1;
							} else if (PlayingSound && !PlayingContSound) {
								*key = 1;
								return 1;
							}
							return retVal;
						}
#if (TARGET_API_MAC_CARBON == 1)
						   else
							UniInputBuf.len = 0L;
#endif
					}
#if (TARGET_API_MAC_CARBON == 1)
					  else
						UniInputBuf.len = 0L;
#endif
				}
				break;
			default:
				break;
		}
		SetScrollControl();
	} else { /* if (WaitNextEvent(everyEvent, &myEvent, sleep, 0L)) { */
		if (theMovie != NULL) {
			MCIsPlayerEvent(theMovie->MvController,&myEvent);
		}
		if (SelectedFiles != NULL) {
			if (NonModal == 0) {
				struct SelectedFilesList *t;
				isFileFound = 0;
				for (t=SelectedFiles; t != NULL; t=t->nextFile) {
					if (t->fname != NULL) {
						isAjustCursor = TRUE;
						OpenAnyFile(t->fname, 1962, FALSE);
						ControlAllControls('\001', 0);
						SetTextWinMenus(TRUE);
						PrepWindow(0);
					}
				}
				SelectedFiles = freeSelectedFiles(SelectedFiles);
			}
		}
		if (myEvent.what == nullEvent && windProc != NULL) {
			if (windProc->FileInfo == NULL) {
				if (myEvent.modifiers & optionKey && (OptionKeyFound==0 || OptionKeyFound==2))
					OptionKeyFound++;
				if (!(myEvent.modifiers & optionKey) && OptionKeyFound == 1)
					OptionKeyFound++;
				if (!(myEvent.modifiers & optionKey) && OptionKeyFound == 3) {
					OptionKeyFound = 0;
					AutoScriptSelect = !AutoScriptSelect;
					SetTextKeyScript(TRUE);
					WriteCedPreference();
				}
			}
			if (windProc->FileInfo != NULL) {
				saveGlobal_df = global_df;
				global_df = windProc->FileInfo;
#if (TARGET_API_MAC_CARBON == 1)
				if (UniInputBuf.len > 0L) {
					DrawCursor(0);
					if (myEvent.modifiers & cmdKey)
						DrawSoundCursor(0);
					whichWindow = FrontWindow();
					if (windProc->textHandler != nil) {
						if (windProc->EventProc) {
							if (UniInputBuf.inEvent.what == 0) {
								*key = 1;
								return 1;
							}
							PrepareWindA4(whichWindow, &saveRec);
							retVal=windProc->EventProc(whichWindow, &UniInputBuf.inEvent);
							RestoreWindA4(&saveRec);
							if (windProc->textHandler != nil) {
								if (isPlayS || retVal == -1)
									UniInputBuf.len = 0L;
								else if (UniInputBuf.len == 0L)
									retVal = -1;
							} else
								UniInputBuf.len = 0L;
							if (retVal != -1) {
								*key = retVal;
								retVal = 1;
							} else if (PlayingSound && !PlayingContSound) {
								*key = 1;
								return 1;
							}
							return retVal;
						} else
							UniInputBuf.len = 0L;
					} else {
						UniInputBuf.len = 0L;
					}
				} else {
					DrawCursor(1); // lll
					DrawSoundCursor(1);
				}
#else
				DrawCursor(1); // lll
				DrawSoundCursor(1);
#endif
				if (myEvent.modifiers & optionKey && (OptionKeyFound==0 || OptionKeyFound==2))
					OptionKeyFound++;
				if (!(myEvent.modifiers & optionKey) && OptionKeyFound == 1)
					OptionKeyFound++;
				if (!(myEvent.modifiers & optionKey) && OptionKeyFound == 3) {
					OptionKeyFound = 0;
					AutoScriptSelect = !AutoScriptSelect;
					SetTextKeyScript(TRUE);
					WriteCedPreference();
				}
				global_df = saveGlobal_df;
			}
		} else if (isFileFound != 0) {
			if (NonModal == 0) {
				if (isFileFound >= 4) {
					if (DefClan)
						OpenCommandsWindow(FALSE);
					else {
						isAjustCursor = TRUE;
						OpenAnyFile(NEWFILENAME, 1962, FALSE);
					}
					isFileFound = 0;
				} else
					isFileFound++;
			}
		}
	}

	if (PlayingSound) {
		if (isBackground) {
			PlayingSound = FALSE;
			if (PlayingContSound)
				PlayingContSound = '\003';
			*key = 1;
		} else
			*key = -1;
		return 1;
	} else
		return -1;
}

void SetTextKeyScript(char isForce) {
	if (!isBackground) {
		if (global_df != NULL)
			SetRdWKeyScript();
		else
			SetDefFontKeyScript();
	} else if (isForce)
		SetDefKeyScript();

	if (AutoScriptSelect)
		SetItemMark(FontMenu,Font_Auto_Script,checkMark);
	else
		SetItemMark(FontMenu,Font_Auto_Script,noMark);
}

extern char *sendMessage(char *mess);
char *sendMessage(char *mess) {
	AEDesc pDesc;
	AppleEvent myEvent, reply;
	OSErr err;

	err = AECreateDesc(typeApplSignature, "PHW0", 4, &pDesc);
	err = AECreateAppleEvent('PHW0', 0, &pDesc, kAutoGenerateReturnID, 1, &myEvent);
	AEPutParamPtr(&myEvent, typeChar, typeChar, mess, strlen(mess) + 1);
	err = AESend(&myEvent, &reply, kAENoReply, kAENormalPriority, kNoTimeOut, NULL, NULL);
//	err = AESend(&myEvent, &reply, kAEWaitReply, kAENormalPriority, kNoTimeOut, NULL, NULL);
//	err = AESend(&myEvent, &reply, kAEWaitReply, kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
//	err = AESend(&myEvent, &reply, kAEWaitReply, kAENormalPriority, 120L, NULL, NULL);

	mess = NULL;
	if (err != noErr) {
		if (err == connectionInvalid)
			mess = "Invalid connection to PitchWorks";
		else if (err == errAETimeout)
			mess = "Connection to PitchWorks timedout";
		else
			mess = "Unexpected error";
	}
	AEDisposeDesc(& pDesc);
	AEDisposeDesc(& myEvent);
	AEDisposeDesc(& reply);
	return(mess);
}

static pascal OSErr AEQuitCed(AppleEvent *evt, AppleEvent *reply, long ref) {
	if (NonModal == 0 && ExitCED(1) == 0) {
		isPlayS = 6;
		return(noErr);
	} else
		return(userCanceledErr);
}

struct SelectedFilesList *addToSelectedFiles(struct SelectedFilesList *root, FNType *fname) {
	struct SelectedFilesList *p;

	if (root == NULL) {
		root = NEW(struct SelectedFilesList);
		p = root;
	} else {
		for (p=root; p->nextFile != NULL; p=p->nextFile) ;
		p->nextFile = NEW(struct SelectedFilesList);
		p = p->nextFile;
	}
	if (p == NULL)
		return(root);
	p->nextFile = NULL;
	p->fname = (char *)malloc(strlen(fname)+1);
	if (p->fname != NULL)
		strcpy(p->fname, fname);
	return(root);
}

struct SelectedFilesList *freeSelectedFiles(struct SelectedFilesList *p) {
	struct SelectedFilesList *t;

	while (p) {
		t = p;
		p = p->nextFile;
		if (t->fname != NULL)
			free(t->fname);
		free(t);
	}
	return(NULL);
}

static pascal OSErr OpenFinderDoc(AppleEvent *evt, AppleEvent *reply, long ref) {
#pragma unused (reply, ref)
	AEDescList	docList;
	long		count, size;
	short		i;
	FNType		sTemp[FNSize];
	DescType	retType;
	AEKeyword	keywd;
	FSRef		fref;

	AEGetParamDesc(evt, keyDirectObject, typeAEList, &docList);
	AECountItems(&docList, &count);
	for (i=0; i < count; i++) {
		AEGetNthPtr(&docList, i+1, typeFSRef, &keywd, &retType, (Ptr)&fref, sizeof(FSRef), &size);
		my_FSRefMakePath(&fref, sTemp, FNSize);
		extractPath(defaultPath, sTemp);
		SelectedFiles = addToSelectedFiles(SelectedFiles, sTemp);
	}
	AEDisposeDesc(&docList);
	return 0;
}

static pascal OSErr OpenFilePos(AppleEvent *evt, AppleEvent *reply, long ref) {
#pragma unused (reply, ref)
	long		size;
	char		text[1024+1];
	DescType	actualType;

	if (NonModal)
		return 0;
	AEGetParamPtr (evt, 1, typeChar, &actualType, (Ptr)text, 1024, &size);
	FindFileLine(FALSE, text);
	return 0;
}

static void InstallAEProcs(void) {
	short err = 0;

	err = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, NewAEEventHandlerUPP((AEEventHandlerProcPtr)OpenFinderDoc), 0, false);
	err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP((AEEventHandlerProcPtr)AEQuitCed), 0, false);
	err = AEInstallEventHandler(758934755, 0, NewAEEventHandlerUPP((AEEventHandlerProcPtr)OpenFilePos), 0, false);
}
