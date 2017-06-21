#include "ced.h"
#include "mac_dial.h"

#define USERNAMELEN 256

extern char isCloseProgressDialog;

static int  tryCnt = 0;
static int  urlBufLen = 0, urlBufI = 0;
static char *ulrBuf = NULL, urlUserName[USERNAMELEN+1], urlPassword[USERNAMELEN+1];
static char isGlobalRunInstaller = FALSE, isDoneDownload = 1, haveExaminedHeaders = FALSE;
static unsigned long totalBytesRead = 0L, urlFileMax = 0L;
static CFMutableArrayRef authArray = NULL;
static CFHTTPMessageRef	request = NULL;
static CFMutableDictionaryRef credentialsDict= NULL;
static CFReadStreamRef readStreamRef = NULL;
static CFOptionFlags kNetworkEvents = kCFStreamEventOpenCompleted | kCFStreamEventHasBytesAvailable | kCFStreamEventEndEncountered | kCFStreamEventErrorOccurred;
static FNType urlFname[FNSize];
static FILE *download_fp = NULL;

void initURLDownload(char isAll) {
	totalBytesRead = 0L;
	urlFileMax = 0L;
	ulrBuf = NULL;
	urlBufLen = 0;
	urlBufI = 0;
	isGlobalRunInstaller = FALSE;
	haveExaminedHeaders = FALSE;
	authArray = NULL;
	request = NULL;
	credentialsDict = NULL;
	tryCnt = 0;
	if (isAll) {
		download_fp = NULL;
		readStreamRef = NULL;
		urlUserName[0] = EOS;
		urlPassword[0] = EOS;
	}
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

static void killStream(void) {
	if (readStreamRef) {
		CFReadStreamSetClient(readStreamRef, kCFStreamEventNone, NULL, NULL);
		CFReadStreamUnscheduleFromRunLoop(readStreamRef, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
		CFReadStreamClose(readStreamRef);
		CFRelease(readStreamRef);
		readStreamRef = NULL;
	}
	if (download_fp != NULL) {
		fclose(download_fp);
		download_fp = NULL;
	}
	haveExaminedHeaders = FALSE;
}

static void TerminateDownload(char isRunIt) {
	killStream();
	if (isGlobalRunInstaller == FALSE) {
		if (urlFileMax > 0L)
			CloseProgressDialog();
		initURLDownload(FALSE);
	} else if (isRunIt) {
		CloseProgressDialog();
		initURLDownload(FALSE);
		strcpy(templineC, "open -W -g ");
		strcat(templineC, urlFname);
		strcat(templineC, "; open /Volumes/CLAN/Install_Clan.pkg");
		system(templineC);
		RemoveAEProcs();
		SelectedFiles = freeSelectedFiles(SelectedFiles);
		ExitToShell();
	} else {
		initURLDownload(FALSE);
		isCloseProgressDialog = TRUE;
	}
}

static char isAuthorizationFailure(void) {
	CFHTTPMessageRef myResponse = (CFHTTPMessageRef)CFReadStreamCopyProperty(readStreamRef, kCFStreamPropertyHTTPResponseHeader);
	if (myResponse) {
		UInt32 urlRes = CFHTTPMessageGetResponseStatusCode(myResponse);
		CFRelease(myResponse);
		if (urlRes == 401 || urlRes == 407)
			return(TRUE);
	}
	return(FALSE);
}

static CFHTTPAuthenticationRef findAuthenticationForRequest(void) {
	int i, c;
	CFHTTPAuthenticationRef auth;
	
	c = CFArrayGetCount(authArray);
    for (i=0; i < c; i ++) {
        auth = (CFHTTPAuthenticationRef)CFArrayGetValueAtIndex(authArray, i);
        if (CFHTTPAuthenticationAppliesToRequest(auth, request)) {
            return(auth);
        }
    }
	return(NULL);
}

static void	ReadStreamClientCallBack(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo);

static void loadRequest(void) {
    killStream();
	if (ulrBuf != NULL) {
		urlBufI = 0;
		ulrBuf[0] = EOS;
		urlFname[0] = EOS;
		download_fp = NULL;
	} else {
		download_fp = fopen(urlFname, "wb");
	}
	readStreamRef = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, request);
	CFReadStreamSetProperty(readStreamRef, kCFStreamPropertyHTTPShouldAutoredirect, kCFBooleanTrue);
	CFStreamClientContext ctxt = { 0, (void*)NULL, NULL, NULL, NULL };
	CFReadStreamSetClient(readStreamRef, kNetworkEvents, ReadStreamClientCallBack, &ctxt);
	CFReadStreamScheduleWithRunLoop(readStreamRef, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
	CFReadStreamOpen(readStreamRef);
}

static char resumeWithCredentials() {
    CFHTTPAuthenticationRef authentication;

	authentication = findAuthenticationForRequest();	
    CFMutableDictionaryRef credentials = (CFMutableDictionaryRef)CFDictionaryGetValue(credentialsDict, authentication);
    if (!credentials) {
        credentials = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if (CFHTTPAuthenticationRequiresUserNameAndPassword(authentication)) {
			tryCnt++;
			if (urlUserName[0] == EOS || urlPassword[0] == EOS || tryCnt >= 2) {
				if (!PasswordDialog(urlUserName, urlPassword)) {
					isDoneDownload = 3;
					return(FALSE);
				}
				tryCnt = 0;
			}
            CFStringRef user;
            CFStringRef pass;
			user = CFStringCreateWithBytes(NULL, (UInt8 *)urlUserName, strlen(urlUserName), kCFStringEncodingUTF8, false);
			if (user != NULL) {
				CFDictionarySetValue(credentials, kCFHTTPAuthenticationUsername, user);
				CFRelease(user);
			} else {
				user = CFSTR("");
				CFDictionarySetValue(credentials, kCFHTTPAuthenticationUsername, user);
			}
			pass = CFStringCreateWithBytes(NULL, (UInt8 *)urlPassword, strlen(urlPassword), kCFStringEncodingUTF8, false);
			if (pass != NULL) {
				CFDictionarySetValue(credentials, kCFHTTPAuthenticationPassword, pass);
				CFRelease(pass);
			} else {
				pass = CFSTR("");
				CFDictionarySetValue(credentials, kCFHTTPAuthenticationPassword, pass);
			}
			if (CFHTTPAuthenticationRequiresAccountDomain(authentication)) {
				CFStringRef domain = NULL; // (CFStringRef)[imageClient accountDomain];
				if (!domain)
					domain = CFSTR("");
				CFDictionarySetValue(credentials, kCFHTTPAuthenticationAccountDomain, domain);
			}
        }
        CFDictionarySetValue(credentialsDict, authentication, credentials);
        CFRelease(credentials);
    }
	if (!CFHTTPMessageApplyCredentialDictionary(request, authentication, credentials, NULL)) {
// Some error occured
		isDoneDownload = 2;
		return(FALSE);
    } else {
        loadRequest();
    }
	return(TRUE);
}

static void retryAfterAuthorizationFailure(void) {	
    CFHTTPAuthenticationRef authentication;

	authentication = findAuthenticationForRequest();
    if (authentication == NULL) {
        CFHTTPMessageRef responseHeader = (CFHTTPMessageRef)CFReadStreamCopyProperty(readStreamRef, kCFStreamPropertyHTTPResponseHeader);
		authentication = CFHTTPAuthenticationCreateFromResponse(NULL, responseHeader);
        CFRelease(responseHeader);
        if (authentication) {
            CFArrayAppendValue(authArray, authentication);
            CFRelease(authentication);
        }
    }
	CFStreamError err;
    if (authentication == NULL || !CFHTTPAuthenticationIsValid(authentication, &err)) {
        if (authentication) {
            CFDictionaryRemoveValue(credentialsDict, authentication);
			CFIndex authIndex = CFArrayGetFirstIndexOfValue(authArray, CFRangeMake(0, CFArrayGetCount(authArray)), authentication);
            if (authIndex != kCFNotFound) {
                CFArrayRemoveValueAtIndex(authArray, authIndex);
            }
			if (err.domain == kCFStreamErrorDomainHTTP && (err.error == kCFStreamErrorHTTPAuthenticationBadUserName || err.error == kCFStreamErrorHTTPAuthenticationBadPassword)) {
				retryAfterAuthorizationFailure();
                return;
            } else {              
// Some error occured
				isDoneDownload = 2;
            }
        }  else {          
// Some error occured
			isDoneDownload = 2;
        }
		TerminateDownload(FALSE);
    } else {
		killStream();
		if (!resumeWithCredentials()) {
			TerminateDownload(FALSE);
			return;
		}
    }
}

static void	ReadStreamClientCallBack(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo) {
	#pragma unused (clientCallBackInfo)
	unsigned long perc;
	UInt8		buffer[16 * 1024]; // Create a 16K buffer
	CFIndex		bytesRead;

	switch (type) {
		case kCFStreamEventHasBytesAvailable:
			if (!haveExaminedHeaders) {
				haveExaminedHeaders = TRUE;
				if (isAuthorizationFailure()) {
                    retryAfterAuthorizationFailure();
                    break;
                }
			}
			bytesRead = CFReadStreamRead(stream, buffer, sizeof(buffer));
			if (bytesRead > 0) { // If zero bytes were read, wait for the EOF to come.
				if (urlFileMax > 0L) {
					NonModal = 0;
					totalBytesRead += bytesRead;
					perc = (100L * totalBytesRead) / urlFileMax; // 3072000L
					if (!UpdateProgressDialog((short)perc)) {
						isDoneDownload = 3;
						TerminateDownload(FALSE); // User pressed cancel
					}
					NonModal = 145;
				}
				if (ulrBuf != NULL) {
					if (bytesRead > urlBufLen-urlBufI) {
						bytesRead = urlBufLen-urlBufI;
						strncpy(ulrBuf+urlBufI, (char *)buffer, bytesRead);
						urlBufI += bytesRead;
						ulrBuf[urlBufI] = EOS;
						if (urlFileMax > 0L) {
							NonModal = 0;
							UpdateProgressDialog(100);
							NonModal = 145;
						}
						isDoneDownload = 1;
						TerminateDownload(TRUE);
					} else {
						strncpy(ulrBuf+urlBufI, (char *)buffer, bytesRead);
						urlBufI += bytesRead;
						ulrBuf[urlBufI] = EOS;
					}
				} else if (download_fp == NULL || fwrite((char *)buffer, 1, bytesRead, download_fp) != bytesRead) {
					isDoneDownload = 2;
					TerminateDownload(FALSE); // an error occured
				}
			} else if (bytesRead < 0)	{ // Less than zero is an error
				isDoneDownload = 2;
				TerminateDownload(FALSE);
			} else { // 0 assume we are done with the stream
				isDoneDownload = 1;
				TerminateDownload(TRUE);
			}
			break;
		case kCFStreamEventEndEncountered:
			if (!haveExaminedHeaders) {
				haveExaminedHeaders = TRUE;
				if (isAuthorizationFailure()) {
                    retryAfterAuthorizationFailure();
                    break;
                }
			}
			isDoneDownload = 1;
			TerminateDownload(TRUE);
			break;
		case kCFStreamEventErrorOccurred:
			isDoneDownload = 2;
			TerminateDownload(FALSE);
			break;
		default:
			break;
	}
}

char DownloadURL(char *url, unsigned long maxSize, char *memBuf, int memBufLen, char *fname, char isRunIt, char isRawUrl) {
	EventRecord					myEvent;
	CFStringRef					rawCFString;
	CFStringRef					normalizedCFString;
	CFStringRef					escapedCFString;
	CFURLRef					urlRef;
	CFStreamClientContext		ctxt = { 0, (void*)NULL, NULL, NULL, NULL };

	isDoneDownload = 0;
	isGlobalRunInstaller = isRunIt;
	ulrBuf = memBuf;
	urlBufLen = memBufLen;
	urlFileMax = maxSize;
	tryCnt = 0;
	totalBytesRead = 0L;
	if (urlFileMax > 0L) {
		if (!OpenProgressDialog("Downloading CLAN Installer...")) {
			NonModal = 0;
			return(FALSE);
		}
	}
	if (ulrBuf != NULL) {
		urlBufI = 0;
		ulrBuf[0] = EOS;
		urlFname[0] = EOS;
		download_fp = NULL;
	} else {
		strcpy(urlFname, fname);
		download_fp = fopen(urlFname, "wb");
		if (download_fp == NULL) {
			NonModal = 0;
			return(FALSE);
		}
	}
	request = NULL;
	authArray = NULL;
	readStreamRef = NULL;
	credentialsDict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	if (credentialsDict == NULL) goto Bail;
	authArray = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
	if (authArray == NULL) goto Bail;
	//	We first create a CFString in a standard URL format, for instance spaces, " ", become "%20" within the string
	//	To do this we normalize the URL first, then create the escaped equivalent
	rawCFString		= CFStringCreateWithCString(NULL, url, CFStringGetSystemEncoding());
	if (rawCFString == NULL) goto Bail;
	if (!isRawUrl) {
		normalizedCFString	= CFURLCreateStringByReplacingPercentEscapes(NULL, rawCFString, CFSTR(""));
		if (normalizedCFString == NULL) goto Bail;
		escapedCFString	= CFURLCreateStringByAddingPercentEscapes(NULL, normalizedCFString, NULL, NULL, kCFStringEncodingUTF8);
		if (escapedCFString == NULL) goto Bail;
	}
	if (isRawUrl)
		urlRef= CFURLCreateWithString(kCFAllocatorDefault, rawCFString, NULL);
	else
		urlRef= CFURLCreateWithString(kCFAllocatorDefault, escapedCFString, NULL);
	CFRelease(rawCFString);
	if (!isRawUrl) {
		CFRelease(normalizedCFString);
		CFRelease(escapedCFString);
	}
	if (urlRef == NULL) goto Bail;
	request = CFHTTPMessageCreateRequest(kCFAllocatorDefault, CFSTR("GET"), urlRef, kCFHTTPVersion1_1);
	if (request == NULL) goto Bail;
	// Create the stream for the request.
	readStreamRef	= CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, request);
	if (readStreamRef == NULL) goto Bail;
	//	There are times when a server checks the User-Agent to match a well known browser.  This is what Safari used at the time the sample was written
	//CFHTTPMessageSetHeaderFieldValue(request, CFSTR("User-Agent"), CFSTR("Mozilla/5.0 (Macintosh; U; PPC Mac OS X; en-us) AppleWebKit/125.5.5 (KHTML, like Gecko) Safari/125")); 
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
//	if (isGlobalRunInstaller == FALSE) { 2013-01-15
		while (isDoneDownload == 0) {
			WaitNextEvent(everyEvent, &myEvent, 5, 0L);
		}
//	}
    if (credentialsDict)
		CFRelease(credentialsDict);
	if (authArray)
		CFRelease(authArray);
	if (request)
		CFRelease(request);
	NonModal = 0;
	if (urlFileMax > 0L)
		CloseProgressDialog();
	initURLDownload(FALSE);
	if (isDoneDownload == 1)
		return(TRUE);
	else
		return(FALSE);
Bail:
	NonModal = 0;
	tryCnt = 0;
	totalBytesRead = 0L;
    if (credentialsDict)
		CFRelease(credentialsDict);
	if (authArray)
		CFRelease(authArray);
	if (request != NULL)
		CFRelease(request);
	if (readStreamRef != NULL) {
        CFReadStreamSetClient(readStreamRef, kCFStreamEventNone, NULL, NULL);
	    CFReadStreamUnscheduleFromRunLoop(readStreamRef, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
	    CFReadStreamClose(readStreamRef);
        CFRelease(readStreamRef);
    }
	if (urlFileMax > 0L)
		CloseProgressDialog();
	initURLDownload(FALSE);
	return(FALSE);
}
