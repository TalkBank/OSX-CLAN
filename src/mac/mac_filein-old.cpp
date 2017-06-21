/*
	for (i=1; i <= F_numfiles; i++)
		get_a_file(i, line, nil, FileList);
*/

#include "cu.h"
#include "ced.h"
#include "mac_dial.h"
#include <sys/stat.h>
#include <dirent.h>

struct f_list {
	FNType *fname;
	Boolean isDir;
	struct f_list *nextFile;
} ;

static int  lastfn;
static int  firstSelectFile;
static struct f_list *FileList = NULL;


#define SCROLL_BAR_WIDTH 16

static struct FDS {
	FNType dirPathName[FNSize];
	f_list *files;
	int  numfiles;
	MenuRef mref;
	ControlRef ctrlH;
	ControlRef theDirList;
} fd;

static struct fileinUserWindowData {
	WindowPtr window;
	ListHandle theFileList;
} userData;

static Handle fd_icons[5];
static IconRef file_iconRef[1002];

int  F_numfiles = 0;
char isAllFile;

void InitFileDialog(void) {
	FileList = NULL;
	firstSelectFile = true;
	lastfn = 0;
	F_numfiles = 0;
	isAllFile = FALSE;
	fd.dirPathName[0] = EOS;
	fd.files = NULL;
	fd.numfiles = 0;
	fd.theDirList = NULL;
}

static struct f_list *CleanupFileList(struct f_list *fl) {
	struct f_list *t;

	while (fl != NULL) {
		t = fl;
		fl = fl->nextFile;
		free(t->fname);
		free(t);
	}
	return(NULL);
}

static void get_a_file(int fnum, FNType *s, Boolean *isDir, struct f_list *fl) {	
	struct f_list *t;

	*s = EOS;
	if (fnum < 1 || fl == NULL)
		return;

	for (t=fl; t != NULL; fnum--, t=t->nextFile) {
		if (fnum == 1) {
			strncpy(s, t->fname, FNSize-1);
			s[FNSize-1] = EOS;
			if (isDir != nil)
				*isDir = t->isDir;
			break;
		}
	}	
}

static struct f_list *add_a_file(FNType *s, Boolean isDir, struct f_list *fl, int *fn) {
	struct f_list *tw, *t, *tt;

	tw = NEW(struct f_list);
	if (tw == NULL)
		ProgExit("Out of memory");
	if (fl == NULL) {
		fl = tw;
		tw->nextFile = NULL;
	} else if (uS.mStricmp(fl->fname, s) > 0 && (!fl->isDir || isDir)) {
		tw->nextFile = fl;
		fl = tw;
	} else {
		t = fl;
		tt = fl->nextFile;
		if (tt != NULL) {
			if (!isDir && tt->isDir) {
				while (tt != NULL) {
					if (!tt->isDir)
						break;
					t = tt;
					tt = tt->nextFile;
				}
			}
		}
		while (tt != NULL) {
			if (uS.mStricmp(tt->fname, s) > 0 || (isDir && !tt->isDir))
				break; 
			t = tt;
			tt = tt->nextFile;
		}
		if (tt == NULL) {
			t->nextFile = tw;
			tw->nextFile = NULL;
		} else {
			tw->nextFile = tt;
			t->nextFile = tw;
		}
    }
	tw->fname = (FNType *)malloc((strlen(s)+1)*sizeof(FNType));
	if (tw->fname == NULL)
		ProgExit("Out of memory");
	strcpy(tw->fname, s);
	tw->isDir = isDir;
	(*fn)++;
	return(fl);
}

static struct f_list *remove_a_file(int fnum, struct f_list *fl, int *fn) {
	struct f_list *t, *tt;

	if (fl == NULL) ;
	else if (fnum == 1) {
		t = fl;
		fl = fl->nextFile;
		free(t->fname);
		free(t);
		(*fn)--;
	} else {
		tt = fl;
		t  = fl->nextFile;
		for (fnum--; t != NULL; fnum--) {
			if (fnum == 1) {
				tt->nextFile = t->nextFile;
				free(t->fname);
				free(t);
				(*fn)--;
			}
			tt = t;
			t = t->nextFile;
		}	
	}
	return(fl);
}

static int isDuplicate_File(FNType *fs, struct f_list *fl) {
	struct f_list *t;

	for (t=fl; t != NULL; t=t->nextFile) {
		if (!strcmp(t->fname, fs))
			return(TRUE);
	}
	return(FALSE);
}

void get_selected_file(int fnum, FNType *s) {
	get_a_file(fnum, s, nil, FileList);
}

static void createPopupMenu(ControlRef ctrlH, MenuRef mref) {
	int			i, len;
	FNType		mDirPathName[FNSize], *p;
	CFStringRef cString;

	if ((i=CountMenuItems(mref)) > 0) {
		for (; i > 1; i--)
			DeleteMenuItem(mref, i-1);
	}

	i = 0;
	if (fd.dirPathName[0] != EOS) {
		strcpy(mDirPathName, fd.dirPathName);
		len = strlen(mDirPathName) - 1;
		if (mDirPathName[len] == PATHDELIMCHR)
			mDirPathName[len] = EOS;
		while ((p=strrchr(mDirPathName, PATHDELIMCHR)) != NULL) {
			cString = my_CFStringCreateWithBytes(p+1);
			if (cString != NULL) {
				InsertMenuItemTextWithCFString(mref, cString, i, kMenuItemAttrCustomDraw, i+1);
				CFRelease(cString);
				i++;
				SetItemCmd(mref,i,0x1E);
			}
			*p = EOS;
		}
	}

	i = CountMenuItems(mref);
	i--;
	for (; i > 0; i--)
		SetItemIcon(mref, i, 0) ; // 0 = 2

	Draw1Control(ctrlH);
}

static pascal void	theLDEF(short lMessage,Boolean lSelect,Rect *lRect,Cell lCell,
				short lDataOffset,short lDataLen,ListHandle lHandle) {
	FontInfo fontInfo;						/* font information (ascent/descent/etc) */
	ListPtr listPtr;						/* pointer to store dereferenced list */
	SignedByte hStateList,hStateCells;		/* state variables for HGetState/SetState */
	Ptr cellData;							/* points to start of cell data for list */
	short leftDraw,topDraw;					/* left/top offsets from topleft of cell */
	Rect tempRect;
	SInt32 savedPenMode;
	RgnHandle savedClipRegion;


	/* lock and dereference list mgr handles */	
	hStateList = HGetState((Handle)lHandle);
	HLock((Handle)lHandle);
	listPtr = *lHandle;
	hStateCells = HGetState(listPtr->cells);
	HLock(listPtr->cells);
	cellData = *(listPtr->cells);
	
	switch (lMessage) {
	  case lInitMsg:
	  	/* we don't need any initialization */
	  	break;

	  case lDrawMsg:
		//	Save the current clip region, and set the clip region to the area we are about
		//	to draw.
		savedClipRegion = NewRgn();
		GetClip( savedClipRegion );
		ClipRect( lRect );
		EraseRect(lRect);
		
	  	if (lDataLen > 0) {
	  		/* determine starting point for drawing */
	  		leftDraw =	lRect->left + listPtr->indent.h + 19;
	  		topDraw =	lRect->top + listPtr->indent.v;
	  		
			tempRect = *lRect;
			tempRect.bottom = tempRect.top + SCROLL_BAR_WIDTH;
			tempRect.left += 1;
			tempRect.right = tempRect.left + SCROLL_BAR_WIDTH;
			if ((short)cellData[lDataOffset] == 5) {
				PlotIconRef(&tempRect,kAlignAbsoluteCenter,ttNone,kIconServicesNormalUsageFlag,file_iconRef[lCell.v]);
			} else {
			    Handle InfoHand = fd_icons[(short)cellData[lDataOffset]];
				HLock(InfoHand);
				PlotSICNHandle(&tempRect, kAlignAbsoluteCenter, ttNone, InfoHand);
				HUnlock(InfoHand);
			}
			lDataOffset++;
			lDataLen--;
	  		
	  		/* plot text (offset 32 bytes onward) */
			GetFontInfo(&fontInfo);
			MoveTo(leftDraw,topDraw+fontInfo.ascent);
			
			/* set condensed mode if necessary (if the text doesn't fit otherwise) */
			TextFace(0);
			if (TextWidth(cellData,lDataOffset,lDataLen) > (lRect->right - leftDraw))
				TextFace(condense);

			DrawText(cellData,lDataOffset,lDataLen);

			if (lSelect) {
				savedPenMode = GetPortPenMode(listPtr->port);
				SetPortPenMode(listPtr->port, hilitetransfermode);
				PaintRect(lRect);
				SetPortPenMode(listPtr->port, savedPenMode);
			}
	  	}
		//	Restore the saved clip region.
		
		SetClip(savedClipRegion);
		DisposeRgn(savedClipRegion);
		break;

	  case lHiliteMsg:
		savedPenMode = GetPortPenMode(listPtr->port);
		SetPortPenMode(listPtr->port, hilitetransfermode);
		PaintRect(lRect);
		SetPortPenMode(listPtr->port, savedPenMode);
	  	break;

	  case lCloseMsg:
	  	break;
	}

	HUnlock(listPtr->cells);
	HUnlock((Handle)lHandle);
	
	HSetState((Handle)listPtr->cells,hStateCells);
	HSetState((Handle)lHandle,hStateList);
}

static void displayFDinList(void) {
	int i, len;
	Boolean isDir;
	Point csize;
	FNType sTemp[FNSize+1], mFileName[FNSize], *fname_where;
	Rect tempRect, dataRect;
	ControlRef 	itemCtrl;
	GrafPtr		oldPort;
	FSRef		tRef;

	if (fd.numfiles) { 
		fd.files = CleanupFileList(fd.files);
		fd.numfiles = 0;
	}
	if (fd.theDirList)
		RemoveDataBrowserItems(fd.theDirList, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty);

	for (i=0; i < 1002; i++) {
		if (file_iconRef[i] != nil)
			ReleaseIconRef(file_iconRef[i]);
		file_iconRef[i] = nil;
	}
	
	if (fd.dirPathName[0] == EOS) {
		FSVolumeInfoParam fsv;

		i = 1;
		fsv.ioVRefNum = kFSInvalidVolumeRefNum;
		fsv.volumeIndex = i;
		fsv.whichInfo = kFSVolInfoNone;
		fsv.volumeInfo = nil;
		fsv.volumeName = nil;
		fsv.ref = &tRef;
		while (PBGetVolumeInfoSync(&fsv) == noErr) {
			my_FSRefMakePath(fsv.ref, sTemp, FNSize);
			fd.files = add_a_file(sTemp, true, fd.files, &fd.numfiles);
			fsv.volumeIndex = ++i;
		}
		if (FSFindFolder(kOnAppropriateDisk,kDesktopFolderType,kDontCreateFolder,&tRef) == noErr) {
			my_FSRefMakePath(&tRef, mFileName, FNSize);
			SetNewVol(mFileName);
			len = strlen(mFileName);
			i = 1;
			while ((i=Get_Dir(sTemp, i)) != 0) {
				if (sTemp[0] != '.') {
					mFileName[len] = EOS;
					addFilename2Path(mFileName, sTemp);
					fd.files = add_a_file(mFileName, true, fd.files, &fd.numfiles);
				}
			}
			i = 1;
			while ((i=Get_File(sTemp, i)) != 0) {
				if (sTemp[0] != '.') {
					mFileName[len] = EOS;
					addFilename2Path(mFileName, sTemp);
					if (uS.fIpatmat(sTemp, "*.cha") || uS.fIpatmat(sTemp, "*.cex") || isAllFile) {
						if (!isDuplicate_File(mFileName, FileList))
							fd.files = add_a_file(mFileName, false, fd.files, &fd.numfiles);
					}
				}
			}
		}
	} else {
		i = 1;
		SetNewVol(fd.dirPathName);
		strcpy(mFileName, fd.dirPathName);
		len = strlen(mFileName);
		while ((i=Get_Dir(sTemp, i)) != 0) {
			if (sTemp[0] != '.') {
				mFileName[len] = EOS;
				addFilename2Path(mFileName, sTemp);
				fd.files = add_a_file(mFileName, true, fd.files, &fd.numfiles);
			}
		}

		i = 1;
		while ((i=Get_File(sTemp, i)) != 0) {
			if (sTemp[0] != '.') {
				mFileName[len] = EOS;
				addFilename2Path(mFileName, sTemp);
				if (uS.fIpatmat(sTemp, "*.cha") || uS.fIpatmat(sTemp, "*.cex") || isAllFile) {
					if (!isDuplicate_File(mFileName, FileList)) {
						fd.files = add_a_file(mFileName, false, fd.files, &fd.numfiles);
					}
				}
			}
		}
	}

	if (fd.numfiles && fd.theDirList != NULL) {
		AddDataBrowserItems(fd.theDirList, kDataBrowserNoItem, fd.numfiles, NULL, kDataBrowserItemNoProperty);
	}
}

static void displayFilesList(void) {
	int i;
	Boolean isDir;
	Point csize;
	FNType path[FNSize], *fname_where;
	Rect tempRect, dataRect;
	ControlRef 	itemCtrl;
	GrafPtr		oldPort;

	if (userData.theFileList) {
		LDispose(userData.theFileList);
		userData.theFileList = NULL;
	}
	
	GetPort(&oldPort);
	SetPortWindowPort(userData.window);
	itemCtrl = GetWindowItemAsControl(userData.window, 12);
	GetControlBounds(itemCtrl, &tempRect);
	SetRect(&dataRect, 0, 0, 1, 0);
	csize.h=0;
	csize.v=0;
	userData.theFileList=LNew(&tempRect, &dataRect, csize, 0, userData.window, TRUE, FALSE, FALSE, TRUE);
	(*userData.theFileList)->selFlags= lOnlyOne | lNoExtend;
	LSetDrawingMode(TRUE,userData.theFileList);	
	tempRect.top -= 1;
	tempRect.bottom += 1;
	tempRect.right += SCROLL_BAR_WIDTH;
	EraseRect(&tempRect);
	PenNormal();
	InsetRect(&tempRect, -1, -1);
	FrameRect(&tempRect);
	PenNormal();

	path[0] = EOS;
	if (F_numfiles) {
		for (i=1; i <= F_numfiles; i++) {
			get_a_file(i, path, &isDir, FileList);
			LAddRow(1, i-1, userData.theFileList);
			csize.h=0;
			csize.v = i-1;
			fname_where = strrchr(path,PATHDELIMCHR);
			if (!fname_where)
				fname_where	= path;
			else
				fname_where++;
			LSetCell(fname_where,strlen(fname_where),csize,userData.theFileList);
			LDraw(csize,userData.theFileList);
	 	}
//		LSetSelect(TRUE, csize, userData.theFileList); 
	}
	LAutoScroll(userData.theFileList);
	csize.h=0;
	csize.v=0;
	if (LGetSelect(false, &csize, userData.theFileList)) {
		get_a_file(csize.v+1, path, &isDir, FileList);
	} else
		path[0] = EOS;

	SetDialogItemUTF8(path, userData.window, 14, TRUE);

	SetPort(oldPort);
}

static pascal OSStatus DoFileinEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	int			i;
	char		mDirPathName[FNSize];
	Boolean		isDir;
	Cell		csize;
	HICommand	aCommand;
	ControlRef	itemCtrl;
	FSRef		tRef;
	CFStringRef	cString;

	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandOK:
/*
					csize.v = csize.h = 0;
					if (fd.theDirList && LGetSelect(true, &csize, fd.theDirList)) {
						get_a_file(csize.v+1, mDirPathName, &isDir, fd.files);
						if (isDir) {
							strcpy(fd.dirPathName, mDirPathName);
							createPopupMenu(fd.ctrlH, fd.mref);
							displayFDinList();
						} else {
							if (!isDuplicate_File(mDirPathName, FileList)) {
								FileList = add_a_file(mDirPathName, 0, FileList, &F_numfiles);
								LDelRow(1, csize.v, fd.theDirList);
								fd.files = remove_a_file(csize.v+1, fd.files, &fd.numfiles);
								displayFDinList();
								displayFilesList();
							}
						}
						DrawControls(userData.window);
					}
*/
					break;
				case 'ID04':
					if (FSFindFolder(kOnAppropriateDisk,kDesktopFolderType,kDontCreateFolder,&tRef) == noErr) {
						my_FSRefMakePath(&tRef, mDirPathName, FNSize);
						strcpy(fd.dirPathName, mDirPathName);
						createPopupMenu(fd.ctrlH, fd.mref);
						displayFDinList();
/*
						csize.v = csize.h = 0;
						if (LGetSelect(true, &csize, fd.theDirList)) {
							get_a_file(csize.v + 1, mDirPathName, &isDir, fd.files);
							if (mDirPathName[0] != EOS) {
								itemCtrl = GetWindowItemAsControl(userData.window, 1);
								if (isDir) {
									SetControlTitle(itemCtrl,"\pOpen");
								} else {
									SetControlTitle(itemCtrl,"\pAdd ->");
								}
							}
						} else {
							itemCtrl = GetWindowItemAsControl(userData.window, 1);
							SetControlTitle(itemCtrl,"\pAdd ->");
						}
*/
						DrawControls(userData.window);
					}
					break;
				case 'ID06':
					for (i=1; i <= fd.numfiles; i++) {
						get_a_file(i, mDirPathName, &isDir, fd.files);
						if (!isDir && !isDuplicate_File(mDirPathName, FileList))
							FileList = add_a_file(mDirPathName, 0, FileList, &F_numfiles);
				 	}			
					displayFDinList();
					displayFilesList();
					DrawControls(userData.window);
					break;
				case 'ID08':
					itemCtrl = ((HICommandExtended *)&aCommand)->source.control;
					if (GetControl32BitValue(itemCtrl) == 0L) {
						SetDialogItemValue(userData.window, 8, 1);
						isAllFile = FALSE;
					} else {
						SetDialogItemValue(userData.window, 8, 0);
						isAllFile = TRUE;
					}
					displayFDinList();
					break;
				case 'ID10':
					csize.h=0;
					csize.v=0;
					if (LGetSelect(true, &csize, userData.theFileList)) {
						FileList = remove_a_file(csize.v+1, FileList, &F_numfiles);
						LDelRow(1, csize.v, userData.theFileList);
						displayFDinList();
						csize.h=0; csize.v=0;
						if (LGetSelect(false, &csize, userData.theFileList)) {
							get_a_file(csize.v+1, mDirPathName, &isDir, FileList);
						} else
							mDirPathName[0] = EOS;

						SetDialogItemUTF8(mDirPathName, userData.window, 14, TRUE);
						DrawControls(userData.window);
					}
					break;
				case 'ID11':
					if (F_numfiles) { 
						FileList = CleanupFileList(FileList);
						F_numfiles = 0;
						LDelRow(0, 0, userData.theFileList);
						displayFDinList();
						displayFilesList();
						DrawControls(userData.window);
					}
					break;
				case kHICommandCancel:
					// we got a valid click on the OK button so let's quit our local run loop
					QuitAppModalLoopForWindow(userData.window);
					break;
				default:
					break;
			}
			break;
		case kEventClassControl:
			switch (GetEventKind(inEvent)) {
				case kEventControlClick:
					Boolean dbClick;
					Point theMouse;
					HIPoint hPoint;
					ControlID outID;

					GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &itemCtrl);
					if (GetControlID(itemCtrl, &outID) != noErr)
						return eventNotHandledErr;

/*
					if (outID.id == 3) {
						fd.dref.vRefNum = 0;
						fd.dref.parID   = 0L;
						createPopupMenu(fd.ctrlH, fd.mref);
						displayFDinList(userData.window);
						csize.v = csize.h = 0;
						if (LGetSelect(true, &csize, fd.theDirList)) {
							get_a_file(csize.v + 1, mDirPathName, &isDir, fd.files);
							if (mDirPathName[0] != EOS) {
								itemCtrl = GetWindowItemAsControl(userData.window, 1);
								if (isDir) {
									SetControlTitle(itemCtrl,"\pOpen");
								} else {
									SetControlTitle(itemCtrl,"\pAdd ->");
								}
							}
						} else {
							itemCtrl = GetWindowItemAsControl(userData.window, 1);
							SetControlTitle(itemCtrl,"\pAdd ->");
						}
						DrawControls(userData.window);
					} else
*/
/*
					if (outID.id == 5) {
						GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &hPoint);
						theMouse.v = hPoint.y;
						theMouse.h = hPoint.x;
						GlobalToLocal(&theMouse);
						dbClick = LClick(theMouse, 0, fd.theDirList);
						csize = LLastClick(fd.theDirList);
						mDirPathName[0] = EOS;
						if (fd.theDirList && LGetSelect(false, &csize, fd.theDirList)) {
							get_a_file(csize.v+1, mDirPathName, &isDir, fd.files);
							if (mDirPathName[0] != EOS && dbClick) {
								if (isDir) {
									strcpy(fd.dirPathName, mDirPathName);
									createPopupMenu(fd.ctrlH, fd.mref);
									displayFDinList();
								} else {
									if (!isDuplicate_File(mDirPathName, FileList)) {
										FileList = add_a_file(mDirPathName, 0, FileList, &F_numfiles);
										LDelRow(1, csize.v, fd.theDirList);
										fd.files = remove_a_file(csize.v+1, fd.files, &fd.numfiles);
										displayFDinList();
										displayFilesList();
									}
								}
							}
							csize.v = csize.h = 0;
							if (LGetSelect(true, &csize, fd.theDirList)) {
								get_a_file(csize.v + 1, mDirPathName, &isDir, fd.files);
								if (mDirPathName[0] != EOS) {
									itemCtrl = GetWindowItemAsControl(userData.window, 1);
									if (isDir) {
										SetControlTitle(itemCtrl,"\pOpen");
									} else {
										SetControlTitle(itemCtrl,"\pAdd ->");
									}
								}
							} else {
								itemCtrl = GetWindowItemAsControl(userData.window, 1);
								SetControlTitle(itemCtrl,"\pAdd ->");
							}
						}
						DrawControls(userData.window);
					} else
					if (outID.id == 12) {
						GetEventParameter(inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &hPoint);
						theMouse.v = hPoint.y;
						theMouse.h = hPoint.x;
						GlobalToLocal(&theMouse);
						dbClick = LClick(theMouse, 0, userData.theFileList);
						csize = LLastClick(userData.theFileList);
						mDirPathName[0] = EOS;
						if (LGetSelect(false, &csize, userData.theFileList)) {
							get_a_file(csize.v+1, mDirPathName, &isDir, FileList);
							if (mDirPathName[0] != EOS) {
								if (dbClick) {
									FileList = remove_a_file(csize.v+1, FileList, &F_numfiles);
									LDelRow(1, csize.v, userData.theFileList);
									displayFDinList();
//									displayFilesList();
									mDirPathName[0] = EOS;
									SetDialogItemUTF8(mDirPathName, userData.window, 14, TRUE);
								} else {
									SetDialogItemUTF8(mDirPathName, userData.window, 14, TRUE);
								}
							}
						}
						csize.v = csize.h = 0;
						if (LGetSelect(true, &csize, fd.theDirList)) {
							get_a_file(csize.v + 1, mDirPathName, &isDir, fd.files);
							if (mDirPathName[0] != EOS) {
								itemCtrl = GetWindowItemAsControl(userData.window, 1);
								if (isDir) {
									SetControlTitle(itemCtrl,"\pOpen");
								} else {
									SetControlTitle(itemCtrl,"\pAdd ->");
								}
							}
						} else {
							itemCtrl = GetWindowItemAsControl(userData.window, 1);
							SetControlTitle(itemCtrl,"\pAdd ->");
						}
						DrawControls(userData.window);
					} else
 */
					if (outID.id == 15) {
						short item;
						Rect  tempRect;

						itemCtrl = GetWindowItemAsControl(userData.window, 15);
						GetControlBounds(itemCtrl, &tempRect);
						theMouse.h = tempRect.left;
						theMouse.v = tempRect.top;
						LocalToGlobal(&theMouse);
						item = LoWord(PopUpMenuSelect(fd.mref, theMouse.v, theMouse.h, 1));
						if (item > 1) {
							i = CountMenuItems(fd.mref);
							if (item == i) {
								strcpy(fd.dirPathName, "/");
							} else {
								fd.dirPathName[0] = EOS;
								for (i--; i >= item; i--) {
									CopyMenuItemTextAsCFString(fd.mref, i, &cString);
									my_CFStringGetBytes(cString, mDirPathName, 256);
									CFRelease(cString);
									addFilename2Path(fd.dirPathName, mDirPathName);
								}
							}
							createPopupMenu(fd.ctrlH,fd.mref);
							displayFDinList();
						}
						itemCtrl = GetWindowItemAsControl(userData.window, 1);
						SetControlTitle(itemCtrl,"\pAdd ->");
						DrawControls(userData.window);
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return eventNotHandledErr;
}

static pascal OSStatus DataBrowserDataCallback(ControlRef browser, DataBrowserItemID itemID,
											   DataBrowserPropertyID property, DataBrowserItemDataRef itemData,
											   Boolean changeValue) {
	OSStatus err = noErr;
	FNType sTemp[FNSize+1], *fname_where;
	CFStringRef text;
	Boolean isDir;

	switch ( property ) {
		case 'DIRL':
			/* in interface builder we defined one column with the property
			 id value of 'item'.  Here we are being asked to either retrieve the
			 string to be displayed for the itemID in the column identified by
			 this property id. */
			if ( changeValue ) {
				/* this would involve renaming the file. we don't want to do 
				 that here. */
				err = errDataBrowserPropertyNotSupported;
			} else {
				get_a_file(itemID, sTemp, &isDir, fd.files);
				fname_where = strrchr(sTemp,PATHDELIMCHR);
				if (fname_where == NULL) {
					fname_where	= sTemp;
				} else
					fname_where++;
				if (isDir)
					strcat(fname_where, PATHDELIMSTR);
				text = my_CFStringCreateWithBytes(fname_where);
				err = SetDataBrowserItemDataText(itemData, text);
				if (text != NULL) {
					CFRelease(text);
				}
			}
			break;
			/* the remainder of these are essentially for
			 handling generic data browser inquiries and they're not
			 essential for understanding the sample.  comments included
			 for the interested... */
		case kDataBrowserItemIsActiveProperty:
			if ( ! changeValue ) /* is it active? yes */
				err = SetDataBrowserItemDataBooleanValue( itemData, true);
			break;
		case kDataBrowserItemIsSelectableProperty:
			if ( ! changeValue ) /* can we select it? yes */
				err = SetDataBrowserItemDataBooleanValue( itemData, true);
			break;
		case kDataBrowserContainerIsSortableProperty:
		case kDataBrowserItemIsEditableProperty:
		case kDataBrowserItemIsContainerProperty:
			if ( ! changeValue ) /* can we edit it, sort it, or put things in it? no */
				err = SetDataBrowserItemDataBooleanValue( itemData, false);
			break;
		default: /* unrecognized property */
			err = errDataBrowserPropertyNotSupported;
			break;
	}
	/* send result */
	return err;
}

static pascal void DataBrowserNotificationCallback(
												   ControlRef browser,
												   DataBrowserItemID item,
												   DataBrowserItemNotification message) {
	FNType mDirPathName[FNSize];
	Boolean isDir;
	ControlRef	itemCtrl;

	if ( message == kDataBrowserItemDoubleClicked ) {
		get_a_file(item, mDirPathName, &isDir, fd.files);
		if (mDirPathName[0] != EOS) {
			if (isDir) {
				strcpy(fd.dirPathName, mDirPathName);
				createPopupMenu(fd.ctrlH, fd.mref);
				displayFDinList();
			} else {
				if (!isDuplicate_File(mDirPathName, FileList)) {
					FileList = add_a_file(mDirPathName, 0, FileList, &F_numfiles);
					fd.files = remove_a_file(item, fd.files, &fd.numfiles);
					displayFDinList();
					displayFilesList();
				}
			}
		}
		itemCtrl = GetWindowItemAsControl(userData.window, 1);
		SetControlTitle(itemCtrl,"\pAdd ->");
	}
}

void myget(void) {
	int			i;
	GrafPtr		oldPort;
	WindowPtr	myDlg;
	Rect		tempRect;
	ControlRef	itemCtrl;
	IBNibRef	mNibs;
	MenuRef		outMenuRef;
	OSErr		err;
	ControlID controlID = { 'LIST', 5 };

	GetPort(&oldPort);
	fd_icons[0] = GetResource('SICN', 257);
	fd_icons[1] = GetResource('SICN', 258);
	fd_icons[2] = GetResource('SICN', 259);
	fd_icons[3] = GetResource('SICN', 260);
	fd_icons[4] = GetResource('SICN', 261);

	for (i=0; i < 1002; i++) {
		file_iconRef[i] = nil;
	}

	myDlg = getNibWindow(CFSTR("Input File SF"));
	CenterWindow(myDlg, -1, -1);
	ShowWindow(myDlg);
	BringToFront(myDlg);
	SetPortWindowPort(myDlg);
	err = GetControlByID(myDlg, &controlID, &fd.theDirList);
	if ( noErr != err )
		return;
	DataBrowserCallbacks dataBrowserHooks;		
	/* initialize the callback structure to the default values.  */
	dataBrowserHooks.version = kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&dataBrowserHooks);
	/* the first hook is for providing strings displayed in the list. Essentially
	 it translates indexes referencing items in the list into the list of items
	 returned by our scripts. */
	dataBrowserHooks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(DataBrowserDataCallback);
	/* the second is a notification routine we use for detecting double
	 clicks. we use double clicks for opening finder comment editing windows
	 for items in the list when they are double clicked on. */
	dataBrowserHooks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(DataBrowserNotificationCallback);
	/* install the callbacks structure in the list control */
	err = SetDataBrowserCallbacks(fd.theDirList, &dataBrowserHooks);

	if (isAllFile) {
    	SetDialogItemValue(myDlg, 8, 0);
	} else {
    	SetDialogItemValue(myDlg, 8, 1);
	}
	AdvanceKeyboardFocus(myDlg);
	EventTypeSpec eventTypeCP[] = {
									{kEventClassCommand, kEventCommandProcess},
									{kEventClassControl, kEventControlClick}
								  } ;
	InstallEventHandler(GetWindowEventTarget(myDlg), DoFileinEvents, 2, eventTypeCP, NULL, NULL);
	userData.window = myDlg;
	fd.mref = NULL;
	err = CreateNibReference(CFSTR("CLAN"), &mNibs);
	if (err == noErr) {
		err = CreateMenuFromNib(mNibs, CFSTR("pop_up"), &outMenuRef);
		DisposeNibReference(mNibs);
		if (err == noErr)
			fd.mref = outMenuRef;
		else
			return;
	} else
		return;
//	fd.mref = GetMenu(128);
	userData.theFileList = NULL;
	InsertMenuItemText(fd.mref, "\p/", 0);
	SetItemIcon(fd.mref, 1, 0) ; // 0 = 4
	InsertMenu(fd.mref, -1);

	itemCtrl = GetWindowItemAsControl(userData.window, 15);
	GetControlBounds(itemCtrl, &tempRect);

	fd.ctrlH = NewControl(userData.window,&tempRect,"\p",true,0,128,0,1008,0);
	SetControlPopupMenuHandle(fd.ctrlH, fd.mref);
	if (fd.dirPathName[0] == EOS)
		strcpy(fd.dirPathName, wd_dir);
	createPopupMenu(fd.ctrlH, fd.mref);
	displayFDinList();
	displayFilesList();
	DrawControls(userData.window);
	
	RunAppModalLoopForWindow(myDlg);
/*
	if (GetWindowBoolValue(myDlg, 8) > 0)
		isAllFile = FALSE;
	else
		isAllFile = TRUE;
*/
	if (fd.numfiles) { 
		fd.files = CleanupFileList(fd.files);
		fd.numfiles = 0;
	}
	for (i=0; i < 1002; i++) {
		if (file_iconRef[i] != nil)
			ReleaseIconRef(file_iconRef[i]);
	}
	if (userData.theFileList) {
		LDispose(userData.theFileList);
	}
	if (fd.ctrlH != nil) {
		DisposeControl(fd.ctrlH);
	}
	if ((i=CountMenuItems(fd.mref)) > 0) {
		for (; i > 0; i--)
			DeleteMenuItem(fd.mref, i-1);
	}
	DeleteMenu(GetMenuID(fd.mref));
	DisposeMenu(fd.mref);
	DisposeWindow(myDlg);
	SetPort(oldPort);
	WriteCedPreference();
}
