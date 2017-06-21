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
	ControlRef theFileList;
} userData;

static int  lastfn;
static int  firstSelectFile;
static struct f_list *FileList = NULL;
//static Handle fd_icons[5];
//static IconRef file_iconRef[1002];
static DataBrowserItemID DItemSelected;
static DataBrowserItemID FItemSelected;

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

static void displayFDinList(char isSelect) {
	int i, len;
	FNType sTemp[FNSize+1], mFileName[FNSize];
	FSRef		tRef;
	Boolean		isDir;
	ItemCount	numItems;
	ControlRef	itemCtrl;

	if (fd.numfiles) { 
		fd.files = CleanupFileList(fd.files);
		fd.numfiles = 0;
	}
	if (fd.theDirList)
		RemoveDataBrowserItems(fd.theDirList, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty);
/*
	for (i=0; i < 1002; i++) {
		if (file_iconRef[i] != nil)
			ReleaseIconRef(file_iconRef[i]);
		file_iconRef[i] = nil;
	}
*/
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
	itemCtrl = GetWindowItemAsControl(userData.window, 1);
	if (fd.numfiles && fd.theDirList != NULL) {
		AddDataBrowserItems(fd.theDirList, kDataBrowserNoItem, fd.numfiles, NULL, kDataBrowserItemNoProperty);
		if (isSelect) {
			DItemSelected = 1;
			GetDataBrowserItemCount(fd.theDirList,kDataBrowserNoItem,false,kDataBrowserItemAnyState,&numItems);
			if (DItemSelected > numItems && numItems > 0)
				DItemSelected--;
			if (DItemSelected > 0) {
				SetDataBrowserSelectedItems(fd.theDirList,1,&DItemSelected,kDataBrowserItemsAdd);
				RevealDataBrowserItem(fd.theDirList, DItemSelected, kDataBrowserNoItem, kDataBrowserRevealOnly);
				get_a_file(DItemSelected, sTemp, &isDir, fd.files);
				if (sTemp[0] != EOS) {
					if (isDir) {
						SetControlTitle(itemCtrl,"\pOpen");
					} else {
						SetControlTitle(itemCtrl,"\pAdd ->");
					}
				} else
					SetControlTitle(itemCtrl,"\pAdd ->");
			} else
				SetControlTitle(itemCtrl,"\pAdd ->");
		} else {
			DItemSelected = 0;
			SetControlTitle(itemCtrl,"\pAdd ->");
		}
	} else {
		DItemSelected = 0;
		SetControlTitle(itemCtrl,"\pAdd ->");
	}
}

static void displayFilesList(void) {
	if (userData.theFileList) {
		RemoveDataBrowserItems(userData.theFileList, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty);
		AddDataBrowserItems(userData.theFileList, kDataBrowserNoItem, F_numfiles, NULL, kDataBrowserItemNoProperty);
		FItemSelected = 0;
	}
}

static pascal OSStatus DoFileinEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	int			i;
	char		mDirPathName[FNSize];
	Boolean		isDir;
	HICommand	aCommand;
	ControlRef	itemCtrl;
	FSRef		tRef;
	CFStringRef	cString;
	ItemCount	numItems;
	DataBrowserItemID item;

	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandOK:
					if (DItemSelected != 0) {
						get_a_file(DItemSelected, mDirPathName, &isDir, fd.files);
						if (mDirPathName[0] != EOS) {
							if (isDir) {
								strcpy(fd.dirPathName, mDirPathName);
								createPopupMenu(fd.ctrlH, fd.mref);
								displayFDinList(TRUE);
							} else {
								if (!isDuplicate_File(mDirPathName, FileList)) {
									FileList = add_a_file(mDirPathName, 0, FileList, &F_numfiles);
									fd.files = remove_a_file(DItemSelected, fd.files, &fd.numfiles);
									item = DItemSelected;
									displayFDinList(FALSE);
									displayFilesList();
									if (fd.theDirList) {
//										SetDataBrowserSelectedItems(fd.theDirList,1,&DItemSelected,kDataBrowserItemsRemove);
										GetDataBrowserItemCount(fd.theDirList,kDataBrowserNoItem,false,kDataBrowserItemAnyState,&numItems);
										if (item > numItems && numItems > 0)
											item--;
										SetDataBrowserSelectedItems(fd.theDirList,1,&item,kDataBrowserItemsAdd);
										DItemSelected = item;
										RevealDataBrowserItem(fd.theDirList, DItemSelected, kDataBrowserNoItem, kDataBrowserRevealOnly);
										get_a_file(item, mDirPathName, &isDir, fd.files);
										if (mDirPathName[0] != EOS) {
											itemCtrl = GetWindowItemAsControl(userData.window, 1);
											if (isDir) {
												SetControlTitle(itemCtrl,"\pOpen");
											} else {
												SetControlTitle(itemCtrl,"\pAdd ->");
											}
										}
									}
								}
							}
						}
//						DrawControls(userData.window);
					}
					break;
				case 'ID04':
					if (FSFindFolder(kOnAppropriateDisk,kDesktopFolderType,kDontCreateFolder,&tRef) == noErr) {
						my_FSRefMakePath(&tRef, mDirPathName, FNSize);
						strcpy(fd.dirPathName, mDirPathName);
						createPopupMenu(fd.ctrlH, fd.mref);
						displayFDinList(TRUE);
//						DrawControls(userData.window);
					}
					break;
				case 'ID06':
					for (i=1; i <= fd.numfiles; i++) {
						get_a_file(i, mDirPathName, &isDir, fd.files);
						if (!isDir && !isDuplicate_File(mDirPathName, FileList))
							FileList = add_a_file(mDirPathName, 0, FileList, &F_numfiles);
				 	}			
					displayFDinList(TRUE);
					displayFilesList();
//					DrawControls(userData.window);
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
					displayFDinList(TRUE);
//					DrawControls(userData.window);
					break;
				case 'ID10':
					if (FItemSelected != 0) {
						get_a_file(FItemSelected, mDirPathName, &isDir, FileList);
						if (mDirPathName[0] != EOS) {
							FileList = remove_a_file(FItemSelected, FileList, &F_numfiles);
							displayFDinList(FALSE);
							displayFilesList();
							mDirPathName[0] = EOS;
							SetDialogItemUTF8(mDirPathName, userData.window, 14, TRUE);
							itemCtrl = GetWindowItemAsControl(userData.window, 1);
							SetControlTitle(itemCtrl,"\pAdd ->");
//							DrawControls(userData.window);
						}
					}
					break;
				case 'ID11':
					if (F_numfiles) { 
						FileList = CleanupFileList(FileList);
						F_numfiles = 0;
						displayFDinList(FALSE);
						displayFilesList();
//						DrawControls(userData.window);
					}
					break;
				case kHICommandCancel:
					QuitAppModalLoopForWindow(userData.window);
					break;
				default:
					break;
			}
			break;
		case kEventClassControl:
			switch (GetEventKind(inEvent)) {
				case kEventControlClick:
					Point theMouse;
					ControlID outID;

					GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &itemCtrl);
					if (GetControlID(itemCtrl, &outID) != noErr)
						return eventNotHandledErr;
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
							displayFDinList(TRUE);
						}
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

static pascal OSStatus DListCallback(ControlRef browser,DataBrowserItemID itemID,DataBrowserPropertyID property,
									 DataBrowserItemDataRef itemData,Boolean changeValue) {
	OSStatus err = noErr;
	FNType sTemp[FNSize+1], *fname_where;
	CFStringRef text;
	Boolean isDir;

	switch ( property ) {
		case 'DIRL':
			if ( changeValue ) { /* this would involve renaming the file. we don't want to do that here. */
				err = errDataBrowserPropertyNotSupported;
			} else {
				get_a_file(itemID, sTemp, &isDir, fd.files);
				fname_where = strrchr(sTemp,PATHDELIMCHR);
				if (fname_where == NULL)
					fname_where	= sTemp;
				else
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
	return err;
}

static pascal void DListNotificationCallback(ControlRef browser,DataBrowserItemID item,DataBrowserItemNotification message) {
	FNType mDirPathName[FNSize];
	Boolean isDir;
	ControlRef	itemCtrl;

	if ( message == kDataBrowserItemDoubleClicked ) {
		get_a_file(item, mDirPathName, &isDir, fd.files);
		if (mDirPathName[0] != EOS) {
			if (isDir) {
				strcpy(fd.dirPathName, mDirPathName);
				createPopupMenu(fd.ctrlH, fd.mref);
				displayFDinList(TRUE);
			} else {
				if (!isDuplicate_File(mDirPathName, FileList)) {
					FileList = add_a_file(mDirPathName, 0, FileList, &F_numfiles);
					fd.files = remove_a_file(item, fd.files, &fd.numfiles);
					displayFDinList(FALSE);
					displayFilesList();
					SetDialogItemUTF8("", userData.window, 14, TRUE);					
				}
			}
		}
	} else if ( message == kDataBrowserItemSelected ) {
		DItemSelected = item;
		get_a_file(item, mDirPathName, &isDir, fd.files);
		if (mDirPathName[0] != EOS) {
			itemCtrl = GetWindowItemAsControl(userData.window, 1);
			if (isDir) {
				SetControlTitle(itemCtrl,"\pOpen");
			} else {
				SetControlTitle(itemCtrl,"\pAdd ->");
			}
		}
	}
}

static pascal OSStatus FListCallback(ControlRef browser,DataBrowserItemID itemID,DataBrowserPropertyID property,
									 DataBrowserItemDataRef itemData,Boolean changeValue) {
	OSStatus err = noErr;
	FNType sTemp[FNSize+1], *fname_where;
	CFStringRef text;
	Boolean isDir;

	switch ( property ) {
		case 'FILL':
			if ( changeValue ) { /* this would involve renaming the file. we don't want to do that here. */
				err = errDataBrowserPropertyNotSupported;
			} else {
				get_a_file(itemID, sTemp, &isDir, FileList);
				fname_where = strrchr(sTemp,PATHDELIMCHR);
				if (fname_where == NULL)
					fname_where	= sTemp;
				else
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
	return err;
}

static pascal void FListNotificationCallback(ControlRef browser,DataBrowserItemID item,DataBrowserItemNotification message) {
	FNType mDirPathName[FNSize];
	Boolean isDir;

	if ( message == kDataBrowserItemDoubleClicked ) {
		get_a_file(item, mDirPathName, &isDir, FileList);
		if (mDirPathName[0] != EOS) {
			FileList = remove_a_file(item, FileList, &F_numfiles);
			displayFDinList(FALSE);
			displayFilesList();
			mDirPathName[0] = EOS;
			SetDialogItemUTF8(mDirPathName, userData.window, 14, TRUE);
		}
	} else if ( message == kDataBrowserItemSelected ) {
		FItemSelected = item;
		get_a_file(item, mDirPathName, &isDir, FileList);
		if (mDirPathName[0] != EOS) {
			SetDialogItemUTF8(mDirPathName, userData.window, 14, TRUE);
		}
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
	ControlID	DcontrolID = { 'DLST', 5 };
	ControlID	FcontrolID = { 'FLST', 12 };
	DataBrowserCallbacks DdataBrowserHooks;		
	DataBrowserCallbacks FdataBrowserHooks;		

	GetPort(&oldPort);
/*
	fd_icons[0] = GetResource('SICN', 257);
	fd_icons[1] = GetResource('SICN', 258);
	fd_icons[2] = GetResource('SICN', 259);
	fd_icons[3] = GetResource('SICN', 260);
	fd_icons[4] = GetResource('SICN', 261);
	for (i=0; i < 1002; i++) {
		file_iconRef[i] = nil;
	}
*/
	myDlg = getNibWindow(CFSTR("Input File SF"));
	CenterWindow(myDlg, -1, -1);
	ShowWindow(myDlg);
	BringToFront(myDlg);
	SetPortWindowPort(myDlg);
	err = GetControlByID(myDlg, &DcontrolID, &fd.theDirList);
	if ( noErr != err )
		return;
	DItemSelected = 0;
	DdataBrowserHooks.version = kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&DdataBrowserHooks);
	DdataBrowserHooks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(DListCallback);
	DdataBrowserHooks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(DListNotificationCallback);
	err = SetDataBrowserCallbacks(fd.theDirList, &DdataBrowserHooks);

	err = GetControlByID(myDlg, &FcontrolID, &userData.theFileList);
	if ( noErr != err )
		return;
	FItemSelected = 0;
	FdataBrowserHooks.version = kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&FdataBrowserHooks);
	FdataBrowserHooks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(FListCallback);
	FdataBrowserHooks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(FListNotificationCallback);
	err = SetDataBrowserCallbacks(userData.theFileList, &FdataBrowserHooks);

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
	displayFDinList(TRUE);
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
/*
	for (i=0; i < 1002; i++) {
		if (file_iconRef[i] != nil)
			ReleaseIconRef(file_iconRef[i]);
	}
*/
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
