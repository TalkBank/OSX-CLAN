#include "ced.h"
#include "mac_dial.h"

#define USERNAMELEN 256

extern char isCloseProgressDialog;

static char *ulrBuf = NULL, urlUserName[USERNAMELEN+1], urlPassword[USERNAMELEN+1];
static char isGlobalRunInstaller = FALSE, isDoneDownload = 1;
static char tatalCheckBuf[6000+2]; 
static int  urlBufLen = 0, totalCheck = 0;
static unsigned long totalBytesRead = 0L, urlFileMax = 0L;
static FNType urlFname[FNSize];
static FILE *download_fp = NULL;

void initURLDownload(void) {
	totalCheck = 0;
	download_fp = NULL;
	totalBytesRead = 0L;
	urlFileMax = 0L;
	ulrBuf = NULL;
	urlBufLen = 0;
	isGlobalRunInstaller = FALSE;
	urlUserName[0] = EOS;
	urlPassword[0] = EOS;
	tatalCheckBuf[0] = EOS;
}

#define usrData struct userWindowData
struct userWindowData {
	WindowPtr window;
	short res;
};

static pascal OSStatus PasswordDialogEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	HICommand aCommand;
	usrData *userData = (usrData *)inUserData;
	OSStatus status = eventNotHandledErr;

	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandCancel:
					// we got a valid click on the OK button so let's quit our local run loop
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myCANCEL;
					break;
				case kHICommandOK:
					QuitAppModalLoopForWindow(userData->window);
					userData->res = myOK;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return status;
}

static char PasswordDialog(char *username, char *password) {
	wchar_t		usernameW[USERNAMELEN+1], passwordW[USERNAMELEN+1];
	ControlRef	userCtrl, passCtrl;
	WindowPtr	myDlg;
	usrData	userData;
	
	myDlg = getNibWindow(CFSTR("Password_Dialog"));
	CenterWindow(myDlg, -1, -1);
	userData.window = myDlg;
	userData.res = 0;
	userCtrl = GetWindowItemAsControl(myDlg, 3);
	u_strcpy(usernameW, username, USERNAMELEN);
	SetWindowUnicodeTextValue(userCtrl, TRUE, usernameW, 0);
	if (*username)
		SelectWindowItemText(myDlg, 3, 0, HUGE_INTEGER);
	passCtrl = GetWindowItemAsControl(myDlg, 4);
	AdvanceKeyboardFocus(myDlg);
	EventTypeSpec eventTypeCP = {kEventClassCommand, kEventCommandProcess};
	InstallEventHandler(GetWindowEventTarget(myDlg), PasswordDialogEvents, 1, &eventTypeCP, &userData, NULL);
	showWindow(myDlg);
	RunAppModalLoopForWindow(myDlg);
	if (userData.res == myOK) {
		GetWindowUnicodeTextValue(userCtrl, usernameW, USERNAMELEN);
		GetWindowUnicodeTextValue(passCtrl, passwordW, USERNAMELEN);
		u_strcpy(username, usernameW, USERNAMELEN);
		u_strcpy(password, passwordW, USERNAMELEN);
	}
	DisposeWindow(myDlg);
	if (userData.res == myOK)
		return(TRUE);
	else
		return(FALSE);
}

static void TerminateDownload(CFReadStreamRef stream, char isRunIt) {	
	int i;
	//***	ALWAYS set the stream client (notifier) to NULL if you are releaseing it
	//	otherwise your notifier may be called after you released the stream leaving you with a 
	//	bogus stream within your notifier.
	CFReadStreamSetClient(stream, kCFStreamEventNone, NULL, NULL);
	CFReadStreamUnscheduleFromRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
	CFReadStreamClose(stream);
	CFRelease(stream);
	if (download_fp != NULL) {
		fclose(download_fp);
		download_fp = NULL;
	}
	if (tatalCheckBuf[0] != EOS) {
		for (i=0; i < totalCheck; i++) {
			if (uS.mStrnicmp(tatalCheckBuf+i, "<TITLE>", 7) == 0) {
				for (i=i+7; isSpace(tatalCheckBuf[i]); i++) ;
				if (uS.mStrnicmp(tatalCheckBuf+i, "401 Authorization", 17) == 0) {
					isDoneDownload = 4;
					break;
				}
			}
		}
	}
	if (isDoneDownload == 4) {
		if (urlFileMax > 0L)
			CloseProgressDialog();
	} else if (isGlobalRunInstaller == FALSE) {
		if (urlFileMax > 0L)
			CloseProgressDialog();
		initURLDownload();
	} else if (isRunIt) {
		strcpy(templineC, "open -W -g ");
		strcat(templineC, urlFname);
		strcat(templineC, "; open /Volumes/CLAN/Install.mpkg");
		system(templineC);
		ExitToShell();
	} else {
		initURLDownload();
		isCloseProgressDialog = TRUE;
	}
}

static void	ReadStreamClientCallBack(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo) {
	#pragma unused (clientCallBackInfo)
	int			totalLeft;
	unsigned long perc;
	UInt8		buffer[16 * 1024]; // Create a 16K buffer
	CFIndex		bytesRead;

	switch (type) {
		case kCFStreamEventHasBytesAvailable:
			bytesRead = CFReadStreamRead(stream, buffer, sizeof(buffer));
			CFHTTPMessageRef myResponse = (CFHTTPMessageRef)CFReadStreamCopyProperty(stream, kCFStreamPropertyHTTPResponseHeader);
			UInt32 urlRes = CFHTTPMessageGetResponseStatusCode(myResponse);
			if (urlRes == 401 || urlRes == 407) {
				CFHTTPAuthenticationRef	auRef = CFHTTPAuthenticationCreateFromResponse(NULL, myResponse);
				if (auRef != NULL) {
					if (CFHTTPMessageApplyCredentials(messageRef, auRef, CFSTR("broca"), CFSTR("wernicke"), NULL))
						;
					CFRelease(myResponse);
					bytesRead = CFReadStreamRead(stream, buffer, sizeof(buffer));
					CFRelease(auRef);
				}
			}
			if (totalCheck < 6000) {
				if (totalCheck+bytesRead >= 6000)
					totalLeft = 6000 - totalCheck;
				else
					totalLeft = bytesRead;
				strncpy(tatalCheckBuf+totalCheck, (char *)buffer, totalLeft);
				totalCheck += totalLeft;
				tatalCheckBuf[totalCheck] = EOS;
			} else
				totalCheck = 6000;
			if (bytesRead > 0) { // If zero bytes were read, wait for the EOF to come.
				if (ulrBuf != NULL) {
					if (bytesRead > urlBufLen)
						bytesRead = urlBufLen;
					strncpy(ulrBuf, (char *)buffer, bytesRead);
					ulrBuf[urlBufLen] = EOS;
					if (urlFileMax > 0L) {
						NonModal = 0;
						UpdateProgressDialog(100);
						NonModal = 145;
					}
					isDoneDownload = 1;
					TerminateDownload(stream, TRUE);
				} else if (download_fp == NULL || fwrite((char *)buffer, 1, bytesRead, download_fp) != bytesRead) {
					if (urlFileMax > 0L) {
						NonModal = 0;
						UpdateProgressDialog(100);
						NonModal = 145;
					}
					isDoneDownload = 2;
					TerminateDownload(stream, FALSE); // an error occured
				} else if (urlFileMax > 0L) {
					NonModal = 0;
					totalBytesRead += bytesRead;
					perc = (100L * totalBytesRead) / urlFileMax; // 3072000L
					if (!UpdateProgressDialog((short)perc)) {
						isDoneDownload = 3;
						TerminateDownload(stream, FALSE); // User pressed cancel
					}
					NonModal = 145;
				}
			} else if (bytesRead < 0)	{ // Less than zero is an error
				isDoneDownload = 2;
				TerminateDownload(stream, FALSE);
			} else { // 0 assume we are done with the stream
				if (urlFileMax > 0L) {
					NonModal = 0;
					UpdateProgressDialog(100);
					NonModal = 145;
				}
				isDoneDownload = 1;
				TerminateDownload(stream, TRUE);
			}
			break;
		case kCFStreamEventEndEncountered:
			if (urlFileMax > 0L) {
				NonModal = 0;
				UpdateProgressDialog(100);
				NonModal = 145;
			}
			isDoneDownload = 1;
			TerminateDownload(stream, TRUE);
			break;
		case kCFStreamEventErrorOccurred:
			isDoneDownload = 2;
			TerminateDownload(stream, FALSE);
			break;
		default:
			break;
	}
}

char DownloadURL(char *url, unsigned long maxSize, char *memBuf, int memBufLen, char *fname, char isRunIt) {
	int							urlOffset;
	char						tUrl[2000];
	EventRecord					myEvent;
	CFStringRef					rawCFString;
	CFStringRef					normalizedCFString;
	CFStringRef					escapedCFString;
	CFURLRef					urlRef;
	CFHTTPMessageRef			messageRef		= NULL;
	CFReadStreamRef				readStreamRef	= NULL;
	CFStreamClientContext		ctxt			= { 0, (void*)NULL, NULL, NULL, NULL };
	CFOptionFlags kNetworkEvents = kCFStreamEventOpenCompleted  | kCFStreamEventHasBytesAvailable | kCFStreamEventEndEncountered | kCFStreamEventErrorOccurred;

	if (uS.mStrnicmp(url, "http://", 7) == 0) {
		urlOffset = 7;
	} else if (uS.mStrnicmp(url, "https://", 8) == 0) {
		urlOffset = 8;
	} else {
		return(FALSE);
	}
repeat_download:
	auRef = NULL;
	isDoneDownload = 0;
	isGlobalRunInstaller = isRunIt;
	ulrBuf = memBuf;
	urlBufLen = memBufLen;
	urlFileMax = maxSize;
	totalBytesRead = 0L;
	totalCheck = 0;
	if (urlFileMax > 0L) {
		if (!OpenProgressDialog("Downloading CLAN Installer...")) {
			return(FALSE);
		}
	}
	if (fname == NULL) {
		urlFname[0] = EOS;
		download_fp = NULL;
	} else {
		strcpy(urlFname, fname);
		download_fp = fopen(urlFname, "wb");
		if (download_fp == NULL)
			return(FALSE);
	}
	if (urlOffset == 7)
		strcpy(tUrl, "http://");
	else if (urlOffset == 8)
		strcpy(tUrl, "https://");
	if (urlUserName[0] != EOS) {
		strcat(tUrl, urlUserName);
		if (urlPassword[0] != EOS) {
			strcat(tUrl, ":");
			strcat(tUrl, urlPassword);
		}
		strcat(tUrl, "@");
	}
	strcat(tUrl, url+urlOffset);
	//	We first create a CFString in a standard URL format, for instance spaces, " ", become "%20" within the string
	//	To do this we normalize the URL first, then create the escaped equivalent
	rawCFString		= CFStringCreateWithCString(NULL, tUrl, CFStringGetSystemEncoding());
	if (rawCFString == NULL) goto Bail;
	normalizedCFString	= CFURLCreateStringByReplacingPercentEscapes(NULL, rawCFString, CFSTR(""));
	if (normalizedCFString == NULL) goto Bail;
	escapedCFString	= CFURLCreateStringByAddingPercentEscapes(NULL, normalizedCFString, NULL, NULL, kCFStringEncodingUTF8);
	if (escapedCFString == NULL) goto Bail;
	urlRef= CFURLCreateWithString(kCFAllocatorDefault, escapedCFString, NULL);
	CFRelease(rawCFString);
	CFRelease(normalizedCFString);
	CFRelease(escapedCFString);
	if (urlRef == NULL) goto Bail;
	messageRef = CFHTTPMessageCreateRequest(kCFAllocatorDefault, CFSTR("GET"), urlRef, kCFHTTPVersion1_1);
	if (messageRef == NULL) goto Bail;
	// Create the stream for the request.
	readStreamRef	= CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, messageRef);
	if (readStreamRef == NULL) goto Bail;
	//	There are times when a server checks the User-Agent to match a well known browser.  This is what Safari used at the time the sample was written
	//CFHTTPMessageSetHeaderFieldValue(messageRef, CFSTR("User-Agent"), CFSTR("Mozilla/5.0 (Macintosh; U; PPC Mac OS X; en-us) AppleWebKit/125.5.5 (KHTML, like Gecko) Safari/125")); 
    if (CFReadStreamSetProperty(readStreamRef, kCFStreamPropertyHTTPShouldAutoredirect, kCFBooleanTrue) == false)
		goto Bail;
	// Set the client notifier
	if (CFReadStreamSetClient(readStreamRef, kNetworkEvents, ReadStreamClientCallBack, &ctxt) == false)
		goto Bail;
	// Schedule the stream
	CFReadStreamScheduleWithRunLoop(readStreamRef, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
	// Start the HTTP connection
	if (CFReadStreamOpen(readStreamRef) == false)
	    goto Bail;
/*
	CFTypeRef CFReadStreamCopyProperty(readStreamRef,
									   CFStringRef propertyName
									   );
*/
	if (isGlobalRunInstaller == FALSE) {
		while (isDoneDownload == 0) {
			WaitNextEvent(everyEvent, &myEvent, 5, 0L);
		}
	}
	CFRelease(messageRef);
	if (isDoneDownload == 4) {
		if (PasswordDialog(urlUserName, urlPassword))
			goto repeat_download;
	}
	if (isDoneDownload == 1)
		return(TRUE);
	else
		return(FALSE);
Bail:
	totalBytesRead = 0L;
	if (messageRef != NULL)
		CFRelease(messageRef);
	if (readStreamRef != NULL) {
        CFReadStreamSetClient(readStreamRef, kCFStreamEventNone, NULL, NULL);
	    CFReadStreamUnscheduleFromRunLoop(readStreamRef, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
	    CFReadStreamClose(readStreamRef);
        CFRelease(readStreamRef);
    }
	return(FALSE);
}
