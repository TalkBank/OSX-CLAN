#include "ced.h"
#include "cu.h"

#define kCreatorCode  'MCED'
#define kPlugType     'pwpc'
#define kPlugTypeD    'pwpD'
#define k68KDocType   'm68K'

extern "C"
{
	extern char *PtoCstr(unsigned char *text);
	extern char *CtoPstr(char *text);
}
enum {
	kInResource,
	kInDataFork
};

short toolFolderVol	= 0;
long  toolFolderDirID = 0;
char isDebugVersion;
PlugInFileInfo *rootPlugIn;
ClanProgInfo PlugInProg;

static OSErr GetFolderDirID (short volID, long parentDirID, StringPtr folderName, long* subFolderDirID) { 
	CInfoPBRec	pb2;
	OSErr		err;
	
	pb2.hFileInfo.ioNamePtr 	= folderName;
	pb2.hFileInfo.ioVRefNum 	= volID;
	pb2.hFileInfo.ioFDirIndex	= 0;
	pb2.hFileInfo.ioDirID		= parentDirID;
	pb2.hFileInfo.ioFVersNum	= 0;	
	err= PBGetCatInfoSync(&pb2);
	if (err == noErr) {
		if ((pb2.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0) {
			*subFolderDirID = pb2.hFileInfo.ioDirID;
		} else
			err = dirNFErr;
	}
	return err;
}

void InitPlugInLoader(void) {
	OSErr				err;
	StringPtr			toolFolderName;
	long				appDirID;
	ProcessInfoRec		info;
	ProcessSerialNumber PSN;

	rootPlugIn = NULL;
	err = HGetVol(NULL, &toolFolderVol, &appDirID);
	if (err) {
		toolFolderVol	= 0;
		toolFolderDirID = 0;
		return;
	}
	toolFolderName = "\p:plug-ins:";
	err = GetFolderDirID (toolFolderVol, appDirID, toolFolderName, &toolFolderDirID);

	isDebugVersion = FALSE;
	if (GetCurrentProcess(&PSN) == noErr) {
		info.processInfoLength = sizeof(info);
		info.processName = (StringPtr)templine;
		info.processAppSpec = nil;
		if (GetProcessInformation(&PSN, &info) == noErr) {
			PtoCstr(info.processName);
			if (info.processName[strlen((char *)info.processName)-1] == 'D')
				isDebugVersion = TRUE;
		}
	}

	FindPlugIns();
}

void freePlugIns(void) {
	PlugInFileInfo *t;

	while (rootPlugIn != NULL) {
		t = rootPlugIn;
		rootPlugIn = rootPlugIn->nextFile;
		free(t);
	}
}

static char AddPlugInToList(PlugInFileInfo *oneFile) {
	char *tc;
	PlugInFileInfo *tF, *t, *tt;

    PtoCstr(oneFile->BaseName);
    if (isDebugVersion) {
    	tc = (char *)oneFile->BaseName + strlen((char *)oneFile->BaseName) - 1;
    	if (*tc == 'D')
    		*tc = EOS;
    }

	tF = NEW(PlugInFileInfo);
	if (tF == NULL) {
		do_warning("Out of memory: Can't create plug-ins list", 0);
		return(FALSE);
	}
	if (rootPlugIn == NULL) {
		rootPlugIn = tF;
		tF->nextFile = NULL;
	} else if (strcmp((char *)rootPlugIn->BaseName, (char *)oneFile->BaseName) > 0) {
		tF->nextFile = rootPlugIn;
		rootPlugIn = tF;
	} else {
		t = rootPlugIn;
		tt = rootPlugIn->nextFile;
		while (tt != NULL) {
			if (strcmp((char *)tt->BaseName, (char *)oneFile->BaseName) > 0)
				break; 
			t = tt;
			tt = tt->nextFile;
		}
		if (tt == NULL) {
			t->nextFile = tF;
			tF->nextFile = NULL;
		} else {
			tF->nextFile = tt;
			t->nextFile = tF;
		}
    }
    strcpy((char *)tF->BaseName, (char *)oneFile->BaseName);
	tF->StorageType = oneFile->StorageType;
	tF->fileFSS.vRefNum = oneFile->fileFSS.vRefNum;
	tF->fileFSS.parID = oneFile->fileFSS.parID;
	Pstrcpy(tF->fileFSS.name, oneFile->fileFSS.name, sizeof(Str63));
	tF->containerAddr = NULL;
	return(TRUE);
}

void FindPlugIns(void) {
	CInfoPBRec		pb2;           /* for the PBGetCatInfo call */
	Str31			fileName;
	short			index;
	OSErr			err;
	PlugInFileInfo	oneFile;
	OSType			docType;
	
	if ((toolFolderVol == 0) || (toolFolderDirID == 0)) {
		return;
	}

	freePlugIns();

	*fileName = '\0';
	pb2.hFileInfo.ioNamePtr = (StringPtr)&fileName;
	pb2.hFileInfo.ioVRefNum = toolFolderVol;
	index = 1;
	err = noErr;
	do {
		pb2.hFileInfo.ioFDirIndex= index;
		pb2.hFileInfo.ioDirID= toolFolderDirID;
		pb2.hFileInfo.ioACUser= 0;
		err = PBGetCatInfoSync(&pb2);
		if (err == noErr) {
			if (((pb2.hFileInfo.ioFlAttrib & kioFlAttribDirMask) == 0) && (pb2.hFileInfo.ioFlFndrInfo.fdCreator == kCreatorCode)) {
				docType = pb2.hFileInfo.ioFlFndrInfo.fdType;
				FSMakeFSSpec(toolFolderVol, toolFolderDirID, pb2.hFileInfo.ioNamePtr, &oneFile.fileFSS);
				if ((isDebugVersion && docType != kPlugTypeD) || (!isDebugVersion && docType != kPlugType))
					;
				else if (docType == kPlugType || docType == kPlugTypeD) {
					oneFile.StorageType = kInDataFork;
					Pstrcpy(oneFile.BaseName, oneFile.fileFSS.name, sizeof(Str31));
					if (!AddPlugInToList(&oneFile)) {
						freePlugIns();
						return;
					}
				} else if (docType == k68KDocType) {
					short	toolResID, refNum;
					ResType	toolResType;
					Handle	toolH;
					
					oneFile.StorageType = kInResource;
					refNum = FSpOpenResFile(&oneFile.fileFSS, fsRdPerm);
					SetResLoad(false);
					toolH = Get1Resource('tool', 1);
					SetResLoad(true);
					if (toolH /* != NULL */)
						GetResInfo(toolH, &toolResID, &toolResType, oneFile.BaseName);
					CloseResFile(refNum);
					if (toolH != NULL) {
						if (!AddPlugInToList(&oneFile)) {
							freePlugIns();
							return;
						}
					}
				}
			}
		}
		index++;
	} while (err == noErr);
}

static void AlertUser(short err) {
	short		itemHit;
	Str255		message1;
	Str255		message2;

	if (err != noErr) {
		if (err == -43)
			err = 11;
		else if (err == -2804)
			err = 12;
		if (err > 0) {
			switch (err) {
				case  1: Pstrcpy(message1, "\pAn internal error has occurred", 255); break;
				case  2: Pstrcpy(message1, "\pCan't load plug-in; OS error number", 255); break;
				case  3: Pstrcpy(message1, "\pCreate a 'Plug-ins' sub-folder and try again.", 255); break;
				case  4: Pstrcpy(message1, "\pThis plug-in may only be loaded once", 255); break;
				case  5: Pstrcpy(message1, "\pThe selected file doesn't contain any plug-in code", 255); break;
				case  6: Pstrcpy(message1, "\pCan’t run a PowerPC plug-in on a 68K machine", 255); break;
				case  7: Pstrcpy(message1, "\pOperation cancelled by user", 255); break;
				case  8: Pstrcpy(message1, "\pCircular reference in a linked list", 255); break;
				case  9: Pstrcpy(message1, "\pThe plug-in was not loaded", 255); break;
				case 10: Pstrcpy(message1, "\pNot enough memory to complete an operation", 255); break;
				case 11: Pstrcpy(message1, "\pGiven plug-in is not found or Folder  is not found", 255); break;
				case 12: Pstrcpy(message1, "\pCan't load it - Possibly incompatible version of plug-in", 255); break;
			}
			GetIndString(message1, 1000, err);
			*message2 = '\0';
		} else {
			Pstrcpy(message1, "\pCan't load plug-in; OS error number", 255);
			sprintf((char *)message2, "%d", err);
			CtoPstr((char *)message2);
		}
		
		ParamText(message1, message2, (StringPtr)"\p", (StringPtr)"\p");
		itemHit = Alert(1000, nil);
	}
}

char LoadPlugIn(PlugInFileInfo *prog) {
	CFragConnectionID	conID;
	Str255				eName;
	Str31				tName;
	short				refNum = -1;
	Handle				toolH;
	OSErr				err;
	ClanProgInfo		*PlugInPtr;

	PlugInProg.conID = NULL;
	switch (prog->StorageType) {
		case kInResource:
			if (prog->containerAddr == NULL) {
				refNum = FSpOpenResFile(&prog->fileFSS, fsRdPerm);
				if (refNum == -1) {
					return(FALSE);
				}
				// The file is open, so get the tool
				toolH = Get1IndResource('tool', 1);
				if (toolH == NULL) {
					return(FALSE);
				}
				DetachResource(toolH);
				HNoPurge(toolH);
				HLock(toolH);	// Yes, this fragments the heap. Should we build a seperate heap for tools?							
				prog->containerAddr = *toolH;
				if (refNum != -1)
					CloseResFile(refNum);	// The tool has been loaded, so close the file
			}
			Pstrcpy(tName, prog->fileFSS.name, sizeof(Str31));
			err = GetMemFragment((Ptr)prog->containerAddr,0,tName,kPrivateCFragCopy,&conID,(Ptr*)&PlugInPtr,eName);
			if (err) {
				AlertUser(err);
				return(FALSE);
			}
			PlugInProg.clan_main = PlugInPtr->clan_main;
			PlugInProg.clan_usage = PlugInPtr->clan_usage;
			PlugInProg.clan_getflag = PlugInPtr->clan_getflag;
			PlugInProg.clan_init = PlugInPtr->clan_init;
			PlugInProg.clan_call = PlugInPtr->clan_call;
			PlugInProg.conID = conID;
			break;

		case kInDataFork:
			Pstrcpy(tName, prog->fileFSS.name, sizeof(Str31));
			err = GetDiskFragment(&prog->fileFSS,0,kCFragGoesToEOF,tName,kPrivateCFragCopy,&conID,(Ptr*)&PlugInPtr,eName);
			if (err) {
				AlertUser(err);
				return(FALSE);
			}
			PlugInProg.clan_main = PlugInPtr->clan_main;
			PlugInProg.clan_usage = PlugInPtr->clan_usage;
			PlugInProg.clan_getflag = PlugInPtr->clan_getflag;
			PlugInProg.clan_init = PlugInPtr->clan_init;
			PlugInProg.clan_call = PlugInPtr->clan_call;
			PlugInProg.conID = conID;
			break;
	}
	return(TRUE);
}

