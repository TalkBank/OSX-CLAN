#include "ced.h"

extern FNType CLAN_Folder_str[FNSize];

#define NUMFOLDERS 15
FNType  *wd_folders[NUMFOLDERS]; // 1
FNType  *lib_folders[NUMFOLDERS];// 2
FNType  *mor_folders[NUMFOLDERS];// 3

static int   cur_which;
static int   local_num_rows, char_height;
static char  DoubleClickCount = 1;
static long  lastWhen = 0;
static short folderIndex = -1, maxFiles = 0, topIndex = 0;
static Point lastWhere = {0,0};

void initFoldersList(void) {
	register short i;

	for (i=0; i < NUMFOLDERS; i++)
		wd_folders[i] = NULL;
	for (i=0; i < NUMFOLDERS; i++)
		lib_folders[i] = NULL;
	for (i=0; i < NUMFOLDERS; i++)
		mor_folders[i] = NULL;
}

void set_folders(int which, FNType *text) {
	register int i;

	if (text == NULL)
		return;

	if (which == 1) {
		for (i=0; i < NUMFOLDERS; i++) {
			if (wd_folders[i] == NULL) {
				wd_folders[i] = (FNType *)malloc((strlen(text)+1)*sizeof(FNType));
				if (wd_folders[i] != NULL)
					strcpy(wd_folders[i], text);
				return;
			}
		}
	} else if (which == 2) {
		for (i=0; i < NUMFOLDERS; i++) {
			if (lib_folders[i] == NULL) {
				lib_folders[i] = (FNType *)malloc((strlen(text)+1)*sizeof(FNType));
				if (lib_folders[i] != NULL)
					strcpy(lib_folders[i], text);
				return;
			}
		}
	} else if (which == 3) {
		for (i=0; i < NUMFOLDERS; i++) {
			if (mor_folders[i] == NULL) {
				mor_folders[i] = (FNType *)malloc((strlen(text)+1)*sizeof(FNType));
				if (mor_folders[i] != NULL)
					strcpy(mor_folders[i], text);
				return;
			}
		}
	}
}

void addToFolders(int which, FNType *dir) {
	register int i;
	FNType **cfolders;

	if (dir[0] == EOS)
		return;

	if (which == 1)
		cfolders = wd_folders;
	else if (which == 2)
		cfolders = lib_folders;
	else if (which == 3)
		cfolders = mor_folders;

	for (i=0; i < NUMFOLDERS; i++) {
		if (cfolders[i] == NULL)
			break;
		if (strcmp(cfolders[i], dir) == 0)
			return;
	}
	if (i >= NUMFOLDERS) {
		free(cfolders[0]);
		for (i=0; i < NUMFOLDERS-1; i++)
			cfolders[i] = cfolders[i+1];
		cfolders[i] = NULL;
	}
	cfolders[i] = (FNType *)malloc((strlen(dir)+1)*sizeof(FNType));
	if (cfolders[i] != NULL)
		strcpy(cfolders[i], dir);
	WriteClanPreference();
}

void write_folders_2011_2013(FILE *fp) {
	register int i;

	for (i=0; i < NUMFOLDERS; i++) {
		if (wd_folders[i] != NULL)
			fprintf(fp, "%d=%s\n", 2011, wd_folders[i]);
	}
	for (i=0; i < NUMFOLDERS; i++) {
		if (lib_folders[i] != NULL)
			fprintf(fp, "%d=%s\n", 2012, lib_folders[i]);
	}
	for (i=0; i < NUMFOLDERS; i++) {
		if (mor_folders[i] != NULL)
			fprintf(fp, "%d=%s\n", 2013, mor_folders[i]);
	}
}

static void CleanupFolders(WindowPtr win) {
#pragma unused(win)
	NonModal = 0;
	topIndex = 0;
	maxFiles = 0;
	cur_which = 0;
	folderIndex = -1;
}

static void DrawFoldersHighlight(WindowPtr win) {
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
		print_row += char_height;
		if (folderIndex == i)
			break;
	}
	if (folderIndex != i || i >= local_num_rows+topIndex) {
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

static void SetFoldersScrollControl(WindowPtr win) {
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

static void UpdateFolders(WindowPtr win) {
    register int i;
    Rect theRect, box;
    GrafPtr oldPort;
	TXNTextBoxOptionsData iOptions;

    GetPort(&oldPort);
    SetPortWindowPort(win);

	iOptions.optionTags = kTXNDontWrapTextMask;
	iOptions.flushness = kATSUStartAlignment;
	iOptions.justification = kATSUNoJustification;
	iOptions.rotation = 0;
	iOptions.options = NULL;
	if (uS.mStricmp(dFnt.fontName, "Ascender Uni Duo") == 0) {
		TextFont(0);
		TextSize(0);
	} else {
		TextFont(dFnt.fontId);
		TextSize(dFnt.fontSize);
	}
	TextFace(0);

	GetWindowPortBounds(win, &theRect);
	theRect.left = LEFTMARGIN;
	theRect.right = theRect.right - SCROLL_BAR_SIZE;
	EraseRect(&theRect);

	local_num_rows = NumOfRowsInAWindow(win, &char_height, 0);
	theRect.top = 0;
	theRect.bottom = theRect.top + char_height;
	for (i=topIndex; i < maxFiles && i < local_num_rows+topIndex; i++) {
		box.left   = theRect.left;
		box.right  = theRect.right;
		box.top    = theRect.top;
		box.bottom = theRect.bottom;
		if (cur_which == 1) {
			u_strcpy(templineW, wd_folders[i], UTTLINELEN);
			TXNDrawUnicodeTextBox((UniChar *)templineW, strlen(templineW), &box, NULL, &iOptions); 
		} else if (cur_which == 2) {
			u_strcpy(templineW, lib_folders[i], UTTLINELEN);
			TXNDrawUnicodeTextBox((UniChar *)templineW, strlen(templineW), &box, NULL, &iOptions); 
		} else if (cur_which == 3) {
			u_strcpy(templineW, mor_folders[i], UTTLINELEN);
			TXNDrawUnicodeTextBox((UniChar *)templineW, strlen(templineW), &box, NULL, &iOptions); 
		}
		theRect.top = theRect.bottom;
		theRect.bottom = theRect.top + char_height;
	}

	DrawFoldersHighlight(win);
	SetFoldersScrollControl(win);
	DrawGrowIcon(win);
	DrawControls(win);
    SetPort(oldPort);
}

void HandleFoldersVScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    long  MaxTick;

	do {
		HiliteControl(theControl, code);
		switch (code) {
			case kControlUpButtonPart:
				if (topIndex > 0) {
					topIndex--;
					UpdateFolders(win);
				}
				break;

			case kControlDownButtonPart:
				if (topIndex < maxFiles-1) {
					topIndex++;
					UpdateFolders(win);
				}
				break;

			case kControlPageUpPart:
				topIndex -= (local_num_rows / 2);
				if (topIndex < 0)
					topIndex = 0;
				UpdateFolders(win);
				break;

			case kControlPageDownPart:
				topIndex += (local_num_rows / 2);
				if (topIndex >= maxFiles) 
					topIndex = maxFiles - 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateFolders(win);
				break;

			case kControlIndicatorPart:
				code = TrackControl(theControl, myPt, NULL);
				topIndex = GetControlValue(theControl);
				if (topIndex >= maxFiles)
					topIndex = maxFiles - 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateFolders(win);
				break;	
		}

		SetFoldersScrollControl(win);
//		if (code != kControlUpButtonPart && code != kControlDownButtonPart) {
			MaxTick = TickCount() + 10;
			do {  
				if (TickCount() > MaxTick)
					break;		 
			} while ( Button() == TRUE) ;
//		}
	} while (StillDown() == TRUE) ;
}

static void Do_Folders_ScrollBar(WindowPtr win, short code, ControlRef theControl, Point myPt) {
    short RefCon;

	RefCon = GetControlReference(theControl);
	switch  (RefCon) {
		case I_VScroll_bar:
			HandleFoldersVScrollBar(win,code,theControl,myPt);
			break;
		default:
			break;
	}
}

static FNType *SelectFoldersPosition(WindowPtr win, EventRecord *event) {
    register short i;
    register int print_row;
	register int h = event->where.h;
	register int v = event->where.v;
	FNType *s;
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
			print_row += char_height;
			if (v < print_row)
				break;
		}
		if (i >= local_num_rows+topIndex || i >= maxFiles)
			return(NULL);

		DrawFoldersHighlight(win);
		folderIndex = i;
		DrawFoldersHighlight(win);

		if (cur_which == 1)
			s = wd_folders[i];
		else if (cur_which == 2)
			s = lib_folders[i];
		else if (cur_which == 3)
			s = mor_folders[i];
		return(s);
	}
	DoubleClickCount = 1;
	lastWhen  = event->when;
	lastWhere = LocalPtr;

	print_row = 0;
	for (i=topIndex; i < maxFiles; i++) {
		print_row += char_height;
		if (v < print_row)
			break;
	}
	if (i >= local_num_rows+topIndex || i >= maxFiles)
		return(NULL);

	DrawFoldersHighlight(win);
	folderIndex = i;
	DrawFoldersHighlight(win);
	return(NULL);
}

static void delFromFolders(WindowPtr win) {
	char  alreadyHighlighted;
	short i;
	
	if (folderIndex >= 0 && folderIndex < NUMFOLDERS) {
		alreadyHighlighted = FALSE;
		DrawFoldersHighlight(win);
		if (cur_which == 1) {
			if (wd_folders[folderIndex] != NULL) {
				free(wd_folders[folderIndex]);
				wd_folders[folderIndex] = NULL;
				maxFiles--;
				for (i=folderIndex; i < maxFiles; i++) {
					wd_folders[i] = wd_folders[i+1];
				}
				if (wd_folders[i] != NULL)
					wd_folders[i] = NULL;
				if (folderIndex >= maxFiles) {
					folderIndex = maxFiles - 1;
					if (folderIndex < 0)
						folderIndex = maxFiles - 1;
					if (folderIndex < topIndex) {
						topIndex--;
						if (topIndex < 0)
							topIndex = 0;
					}
					if (folderIndex >= local_num_rows+topIndex) {
						topIndex = folderIndex - local_num_rows + 1;
						if (topIndex < 0)
							topIndex = 0;
					}
				}
				UpdateFolders(win);
				alreadyHighlighted = TRUE;
			}
		} else if (cur_which == 2) {
			if (lib_folders[folderIndex] != NULL) {
				free(lib_folders[folderIndex]);
				lib_folders[folderIndex] = NULL;
				maxFiles--;
				for (i=folderIndex; i < maxFiles; i++) {
					lib_folders[i] = lib_folders[i+1];
				}
				if (lib_folders[i] != NULL)
					lib_folders[i] = NULL;
				if (folderIndex >= maxFiles) {
					folderIndex = maxFiles - 1;
					if (folderIndex < 0)
						folderIndex = maxFiles - 1;
					if (folderIndex < topIndex) {
						topIndex--;
						if (topIndex < 0)
							topIndex = 0;
					}
					if (folderIndex >= local_num_rows+topIndex) {
						topIndex = folderIndex - local_num_rows + 1;
						if (topIndex < 0)
							topIndex = 0;
					}
				}
				UpdateFolders(win);
				alreadyHighlighted = TRUE;
			}
		} else if (cur_which == 3) {
			if (mor_folders[folderIndex] != NULL) {
				free(mor_folders[folderIndex]);
				mor_folders[folderIndex] = NULL;
				maxFiles--;
				for (i=folderIndex; i < maxFiles; i++) {
					mor_folders[i] = mor_folders[i+1];
				}
				if (mor_folders[i] != NULL)
					mor_folders[i] = NULL;
				if (folderIndex >= maxFiles) {
					folderIndex = maxFiles - 1;
					if (folderIndex < 0)
						folderIndex = maxFiles - 1;
					if (folderIndex < topIndex) {
						topIndex--;
						if (topIndex < 0)
							topIndex = 0;
					}
					if (folderIndex >= local_num_rows+topIndex) {
						topIndex = folderIndex - local_num_rows + 1;
						if (topIndex < 0)
							topIndex = 0;
					}
				}
				UpdateFolders(win);
				alreadyHighlighted = TRUE;
			}
		}
		if (maxFiles == 0)
			folderIndex = -1;
		if (!alreadyHighlighted)
			DrawFoldersHighlight(win);
		WriteClanPreference();
	}
}

static short FoldersEvent(WindowPtr win, EventRecord *event) {
	int				code;
	FNType			*line;
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
			DrawFoldersHighlight(win);
			folderIndex--;
			if (folderIndex < 0)
				folderIndex = maxFiles - 1;
			if (folderIndex < topIndex) {
				topIndex--;
				if (topIndex < 0)
					topIndex = 0;
				UpdateFolders(win);
				alreadyHighlighted = TRUE;
			}
			if (folderIndex >= local_num_rows+topIndex) {
				topIndex = folderIndex - local_num_rows + 1;
				if (topIndex < 0)
					topIndex = 0;
				UpdateFolders(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawFoldersHighlight(win);
		} else if (key == down_key) {
			DrawFoldersHighlight(win);
			folderIndex++;
			if (folderIndex >= maxFiles)
				folderIndex = 0;
			if (folderIndex < topIndex) {
				topIndex = 0;
				UpdateFolders(win);
				alreadyHighlighted = TRUE;
			}
			if (folderIndex >= local_num_rows+topIndex) {
				topIndex++;
				UpdateFolders(win);
				alreadyHighlighted = TRUE;
			}
			if (!alreadyHighlighted)
				DrawFoldersHighlight(win);
		} else {
			key = event->message & charCodeMask;
			if (key == DELETE_CHAR || key == BACKSPACE_CHAR) {
				delFromFolders(win);
			} else if (key == RETURN_CHAR || key == ENTER_CHAR) {
				if (folderIndex >= 0 && folderIndex < NUMFOLDERS) {
					if (cur_which == 1)
						changeCommandFolder(cur_which, wd_folders[folderIndex]);
					else if (cur_which == 2)
						changeCommandFolder(cur_which, lib_folders[folderIndex]);
					else if (cur_which == 3)
						changeCommandFolder(cur_which, mor_folders[folderIndex]);
				}
				PrepareWindA4(win, &saveRec);
				mCloseWindow(win);
				RestoreWindA4(&saveRec);
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
				Do_Folders_ScrollBar(win,code,theControl,event->where);
			}
		} else {
			line = SelectFoldersPosition(win, event);
			if (line != NULL) {
				code = cur_which;
				PrepareWindA4(win, &saveRec);
				mCloseWindow(win);
				RestoreWindA4(&saveRec);
				changeCommandFolder(code, line);
			}
		}
	}
	return(-1);
}

void OpenFoldersWindow(int which) {
	int				t;
	int				saveIsPlayS;
	int				saveF_key;
	myFInfo			*saveGlobal_df;
	WindowPtr		win;
	PrepareStruct	saveRec;

	saveGlobal_df = global_df;
	saveIsPlayS = isPlayS;
	saveF_key   = F_key;
	if (OpenWindow(2010,CLAN_Folder_str,0L,false,1,FoldersEvent,UpdateFolders,CleanupFolders)) {
		do_warning("Can't open CLAN Folders window", 0);
		return;
	}
	maxFiles = 0;
	cur_which = which;
	if (cur_which == 1) {
		for (maxFiles=0; wd_folders[maxFiles] != NULL && maxFiles < NUMFOLDERS; maxFiles++) ;
	} else if (cur_which == 2) {
		for (maxFiles=0; lib_folders[maxFiles] != NULL && maxFiles < NUMFOLDERS; maxFiles++) ;
	} else if (cur_which == 3) {
		for (maxFiles=0; mor_folders[maxFiles] != NULL && maxFiles < NUMFOLDERS; maxFiles++) ;
	}
	if (maxFiles > 0)
		folderIndex = 0;
	else
		folderIndex = -1;
	NonModal = 2010;
	while (NonModal && RealMainEvent(&t)) {
	}
	if ((win=FindAWindowNamed(CLAN_Folder_str)) != NULL) {
		PrepareWindA4(win, &saveRec);
		mCloseWindow(win);
		RestoreWindA4(&saveRec);
	}
	global_df = saveGlobal_df;
	isPlayS = saveIsPlayS;
	F_key	= saveF_key;
}
