#include "ced.h"
#include "mac_print.h"
#include <time.h>
#include <OSUtils.h>
//#include <Printing.h>

#define PageMargin 25

static char printPageInfo;
static short orgCount;

char printSelection;

PMDialog		myPMDialog;
PMPrintSession	printSession;
Handle			hPageFormat;
PMItemUPP		MyPMItemCallbackUPP; 
PMPrintDialogInitUPP MyPrintDialogInitCallbackUPP;

static pascal void MyPMPrintDialogInitCallback(PMPrintSettings printSettings,PMDialog *theDialog) {
	DialogRef myDlg;

	if (PMGetDialogPtr(myPMDialog,&myDlg) != noErr) {
		return;
	}
	if (PMSetItemProc(myPMDialog, MyPMItemCallbackUPP) != noErr) {
		return;
	}
}

static pascal void MyPMItemCallback(DialogRef theDialog,SInt16 item) {
	Handle	hdl;
	short	type;
	Rect	box;

	if (item - orgCount == 1) {
		GetDialogItem(theDialog, item, &type, &hdl, &box);
		printPageInfo = !printPageInfo;
		SetControlValue((ControlRef)hdl, printPageInfo);
	} else if (item - orgCount == 2 && (printSelection & 0x02) == 0) {
		GetDialogItem(theDialog, item, &type, &hdl, &box);
		printSelection = !printSelection;
		SetControlValue((ControlRef)hdl, printSelection);
	}
}

void print_init(void) {
	printPageInfo = TRUE;
	printSelection = FALSE;
	hPageFormat = nil;
	printSession = nil;
	MyPMItemCallbackUPP = NewPMItemUPP(&MyPMItemCallback);
	MyPrintDialogInitCallbackUPP = NewPMPrintDialogInitUPP(&MyPMPrintDialogInitCallback);
}

static OSStatus DoDefault(void) {
	OSStatus status = noErr;

	if (printSession == nil) {
		status = PMCreateSession(&printSession);
    }

	// create default page format if we don't have one already
	if ((status == noErr) && (hPageFormat == nil)) {
		PMPageFormat pageFormat = kPMNoPageFormat;

		status = PMCreatePageFormat(&pageFormat);
		if ((status == noErr) && (pageFormat != kPMNoPageFormat))
			status = PMSessionDefaultPageFormat(printSession, pageFormat);

		if (status == noErr)
			status = PMFlattenPageFormat(pageFormat, &hPageFormat);

		if (pageFormat != kPMNoPageFormat)
			(void)PMRelease(pageFormat);
	}

	return status;
}

void print_file(ROWS *rt, ROWS *endt) {
	char timeS[128];  /* time string */
	char pageS[128];  /* time string */
	FNType fileS[FNSize];  /* time string */
	FNType *colon;
	int len, i, j, lineNumOffset;
	long newcol, colWin;
	long ln;
	short height, pageStart, fileNameStart, fileNameCharWidth;
    short face;
	time_t lclTime;
	struct tm *now;
	Rect pageRect;
	Point where;
	ROWS *t;
	RGBColor oldColor, theColor;
	AttTYPE oldState;
	FontInfo  fi;
	FONTINFO tFont;
	COLORTEXTLIST *tierColor = NULL;
	OSStatus status;
	PMPageFormat pageFormat = kPMNoPageFormat;
	PMPrintSettings printSettings = kPMNoPrintSettings;
	UInt32	pageNumber, iFstPage, iLstPage;

	status = DoDefault();

	// create default print settings
	if (status == noErr) {
		status = PMCreatePrintSettings(&printSettings);
		if ((status == noErr) && (printSettings != kPMNoPrintSettings))
			status = PMSessionDefaultPrintSettings(printSession, printSettings);
	} else {
		do_warning("Can't open printer. Check if any printer is selected.", 0);
		goto cleanupPrint;
	}

	if (status == noErr && (printSettings != nil) && (hPageFormat != nil)) {    
		// retrieve the page format
		status = PMUnflattenPageFormat(hPageFormat, &pageFormat);
		if ((status == noErr) && (pageFormat != kPMNoPageFormat)) {
			Boolean accepted;
			Cursor arrow;

			SetCursor(GetQDGlobalsArrow(&arrow));


			status = PMSessionPrintDialogInit(printSession,printSettings,pageFormat,&myPMDialog);

			if (status == noErr) {
				status = PMSessionPrintDialogMain(printSession,printSettings,pageFormat,&accepted, MyPrintDialogInitCallbackUPP); // OSX
			}
			if (status == noErr && !accepted)
			    status = 101;
		}
	}
		
	if (status != noErr) {
		if (status != 101)
			do_warning("Error initializing printer.", 0);
	    goto cleanupPrint;
	}


	// be sure to get the page range BEFORE calling PrValidate(), 
	// which blows it away for many drivers.
	if (status == noErr)
		status = PMGetFirstPage(printSettings, &iFstPage);
	if (status == noErr)
		status = PMGetLastPage(printSettings, &iLstPage);

	if (status == noErr)
		status = PMSessionValidatePrintSettings(printSession, printSettings, kPMDontWantBoolean);

	if (status == noErr)
		status = PMGetFirstPage(printSettings, &iFstPage);
	if (status == noErr)
		status = PMGetLastPage(printSettings, &iLstPage);
	if (status == noErr)
		status = PMSetFirstPage(printSettings, 1, true);
	if (status == noErr) {
		status = PMSetLastPage(printSettings, kPMPrintAllPages, true); // OSX
	}
	if ((status == noErr) && (printSelection & 0x01)) {
		long cnt;

		ChangeCurLineAlways(0);
		if (global_df->row_win2 < 0) {
			endt = global_df->row_txt;
			for (cnt=global_df->row_win2, rt=global_df->row_txt; cnt && !AtTopEnd(rt,global_df->head_text,FALSE); 
							cnt++, rt=ToPrevRow(rt, FALSE)) ;
		} else if (global_df->row_win2 > 0L) {
			rt = global_df->row_txt;
			for (cnt=global_df->row_win2, endt=global_df->row_txt; cnt && !AtBotEnd(endt,global_df->tail_text,FALSE);
							cnt--, endt=ToNextRow(endt, FALSE)) ;
		} else {
			rt = global_df->row_txt;
			endt = ToNextRow(rt, FALSE);
		}
	}

    if (status != noErr) {
    	if (status != 101)
			do_warning("Error initializing printer.", 0);
	    goto cleanupPrint;
	}

    status = PMSessionBeginDocument(printSession, printSettings, pageFormat);
	if (status == noErr) {
		OSStatus tempErr;
		PMRect  tempPageRect;
		GrafPtr origPort, printingPort;

		status = PMGetAdjustedPageRect(pageFormat, &tempPageRect);
		if (status == noErr) {
		    pageRect.top = tempPageRect.top;
		    pageRect.left = tempPageRect.left;
		    pageRect.bottom = tempPageRect.bottom;
		    pageRect.right = tempPageRect.right;
			/*  print each page  */
			if (printPageInfo) {
				lclTime = time(NULL);
				now = localtime(&lclTime);
				strftime(timeS, 128, "%d-%b-%y %H:%M", now);
				sprintf(pageS, "Page %d", 999);
				TextFont(DEFAULT_ID);
				TextSize(DEFAULT_SIZE);
				TextFace(bold | condense);
				pageStart = TextWidth(pageS, 0, strlen(pageS));
				len = pageRect.right - pageRect.left - PageMargin - (PageMargin / 2) - pageStart - TextWidth(timeS,0,strlen(timeS));
				strcpy(fileS, global_df->fileName);
				if (rt != global_df->head_text->next_row || endt != global_df->tail_text)
					uS.str2FNType(fileS, strlen(fileS), ".\244");
				fileNameCharWidth = strlen(fileS);
				fileNameStart = TextWidth(fileS, 0, fileNameCharWidth);
				if (fileNameStart > len-PageMargin) {
					colon = strrchr(fileS, ':');
					if (colon == NULL) {
						uS.str2FNType(fileS, fileNameCharWidth-3, "...");
						i = fileNameCharWidth - 3;
					} else {
						*(colon-3) = '.';
						*(colon-2) = '.';
						*(colon-1) = '.';
						i = colon - fileS - 3;
					}
					fileNameStart = TextWidth(fileS, 0, fileNameCharWidth);
					while (fileNameStart > len-PageMargin) {
						strcpy(fileS+i-1, fileS+i);
						i--;
						fileNameCharWidth--;
						fileNameStart = TextWidth(fileS, 0, fileNameCharWidth);
					}
				}
				fileNameStart = pageRect.right - pageStart - (PageMargin / 2) - (fileNameStart + ((len - fileNameStart) / 2));
				pageStart = pageRect.right - pageStart - (PageMargin / 2);
			}
			if (isShowLineNums) {
				ln = 1L;
				for (t=global_df->head_text->next_row; t != global_df->tail_text && t != rt; t=t->next_row) {
					if (LineNumberingType == 0 || isMainSpeaker(rt->line[0]))
						ln++;
				}
			}
			GetForeColor(&oldColor);
			ChangeCurLineAlways(0);
			pageNumber = 0;
			// Save the current QD grafport.
			GetPort(&origPort);
			SetPortWindowPort(global_df->wind);
			do {
				pageNumber++;
				sprintf(pageS, "Page %ld", pageNumber);
				TextFont(DEFAULT_ID);
				TextSize(DEFAULT_SIZE);
				TextFace(bold | condense);
				GetFontInfo(&fi);
				height = fi.ascent + fi.descent + fi.leading;

				if (pageNumber >= iFstPage && pageNumber <= iLstPage) {
					if ((status=PMSessionBeginPage(printSession, pageFormat, NULL)) != noErr)
						break;
					if ((status=PMSessionGetGraphicsContext(printSession,kPMGraphicsContextQuickdraw,(void**)&printingPort)) != noErr) {
						break;
					}
					SetPort((GrafPtr)printingPort);
					TextFont(DEFAULT_ID);
					TextSize(DEFAULT_SIZE);
					TextFace(bold | condense);
					GetFontInfo(&fi);
					height = fi.ascent + fi.descent + fi.leading;
					// Set the printing port before drawing the page.
					if (printPageInfo) {
						where.v = height;
						where.h = pageRect.left + PageMargin;
						MoveTo(where.h, where.v);
						DrawText(timeS, 0, strlen(timeS));
						where.h = fileNameStart;
						MoveTo(where.h, where.v);
						DrawText(fileS, 0, fileNameCharWidth);
						where.h = pageStart;
						MoveTo(where.h, where.v);
						DrawText(pageS, 0, strlen(pageS));
					}
				}

				where.h = pageRect.left + PageMargin;	/* leave 1/2 inch margin */
				where.v = (height * 2);
				for (; rt != endt; rt=rt->next_row) {
					tFont.FName   = rt->Font.FName;
					tFont.FSize   = rt->Font.FSize - 1;
					tFont.FHeight = rt->Font.FHeight;
					tFont.CharSet = rt->Font.CharSet;
					tFont.Encod   = rt->Font.Encod;
					
				    if (CMP_VIS_ID(rt->flag)) {
						TextFont(tFont.FName);
						TextSize(tFont.FSize);
						TextFace(DEFAULT_FACE);
						GetFontInfo(&fi);
						height = fi.ascent + fi.descent + fi.leading;
						where.v += height;
						if (where.v >= pageRect.bottom)
							break;
						else if (rt->line[0] == '\014' && rt->line[1] == NL_C) {
							rt = rt->next_row;
							break;
						}
						if (pageNumber >= iFstPage && pageNumber <= iLstPage) {
							MoveTo(where.h, where.v);
							for (len=strlen(rt->line)-1; rt->line[len] == NL_C; len--) ;
							len++;
							if (len >= UTTLINELEN)
								DrawText(rt->line, 0, len);
							else {
								j = 0;
								if (isShowLineNums) {
									if (LineNumberingType == 0 || isMainSpeaker(rt->line[0])) {
										uS.sprintf(ced_line, cl_T("%-5ld "), ln);
										ln++;
									} else
										strcpy(ced_line, "      ");
									for (; j < strlen(ced_line); j++)
										tempAtt[j] = 0;
								}
								lineNumOffset = j;
								colWin = 0L;
								for (i=0; i < len; i++) {
									if (rt->line[i] != NL_C) {
										if (rt->line[i] == '\t') {
											newcol = (((colWin / TabSize) + 1) * TabSize);
											for (; colWin < newcol; colWin++) {
												ced_line[j] = ' ';
												if (rt->att == NULL)
													tempAtt[j] = 0;
												else
													tempAtt[j] = rt->att[i];
												j++;
											}
										} else {
											colWin++;
											if (rt->line[i] == HIDEN_C) {
												if (global_df->ShowParags != '\002') {
													for (i++; rt->line[i] != HIDEN_C && rt->line[i]; i++) ;
													if (rt->line[i] == EOS)
														i--;
												} else {
													ced_line[j] = (unCH)0x2022;
													if (rt->att == NULL)
														tempAtt[j] = 0;
													else
														tempAtt[j] = rt->att[i];
													j++;
												}
											} else {
												ced_line[j] = rt->line[i];
												if (rt->att == NULL)
													tempAtt[j] = 0;
												else
													tempAtt[j] = rt->att[i];
												j++;
											}
										}
									}
								}
								len = j;

								for (i=0; i < len; i++) {
									if (is_colored(tempAtt[i]))
										tempAtt[i] = set_color_to_0(tempAtt[i]);
								}
								if (global_df->RootColorText != NULL)
									tierColor = FindColorKeywordsBounds(global_df->RootColorText, tempAtt,ced_line,lineNumOffset,len,tierColor);

								j = 0;
								oldState = tempAtt[0];
								for (i=0; i <= len; i++) {
									if (oldState != tempAtt[i] || i == len) {
										if (is_colored(tempAtt[j])) {
											if (SetKeywordsColor(global_df->RootColorText, j, &theColor))
												RGBForeColor(&theColor);
										}
										if (is_error(tempAtt[j])) {
											theColor.red = 255;
											theColor.green = 0;
											theColor.blue = 0;
											RGBForeColor(&theColor);
										}
										if (global_df->isUTF)
											DrawUTFontMac(ced_line+j, i-j, &tFont, tempAtt[j]);
										else {
											face = 0;
											if (is_underline(tempAtt[j]))
												face = face | underline;
											if (is_italic(tempAtt[j]))
												face = face | italic;
											if (is_bold(tempAtt[j]))
												face = face | bold;
											if (is_error(tempAtt[j]))
												face = face | underline;
											TextFace(face);
											DrawText(ced_line, j, i-j);
										}
										if (is_colored(tempAtt[j]) || is_error(tempAtt[j]))
											RGBForeColor(&oldColor);
										j = i;
										oldState = tempAtt[i];
									}
								}
								RGBForeColor(&oldColor);
								TextFace(0);
							}
						}
					}
				}
				if (pageNumber >= iFstPage && pageNumber <= iLstPage) {
					// if we called PMSessionBeginPage and got no error then we MUST
					// call PMSessionEndPage
					tempErr = PMSessionEndPage(printSession);
					if (status == noErr)
						status = tempErr;
					if (PMSessionError(printSession) != noErr)
						break;
				}
			} while (rt != endt);
			// Restore the QD grafport.
			SetPort(origPort);
		}
		tempErr = PMSessionEndDocument(printSession);
		if (status == noErr)
			status = tempErr;
	    if (status != noErr) {
			do_warning("Error printing.", 0);
		}
    }
cleanupPrint:

	if (printSettings != kPMNoPrintSettings)
		PMRelease(printSettings);
	if (pageFormat != kPMNoPageFormat)
		PMRelease(pageFormat);
	if (printSession) {			// DMG850 release the session when done
		PMRelease(printSession);
		printSession = nil;
	}
}

void print_setup(void) {
	OSStatus		status = noErr;
	Boolean 		accepted;
		
	status = DoDefault();
	if (status != kPMNoError) {
 		do_warning("Can't open printer. Check if any printer is selected.", 0);
	} else {
		if (hPageFormat != nil) {
			PMPageFormat pageFormat = kPMNoPageFormat;

			// retrieve the default page format
			status = PMUnflattenPageFormat(hPageFormat, &pageFormat);

			if ((status == noErr) && (pageFormat != kPMNoPageFormat)) {
				Cursor arrow;

				SetCursor(GetQDGlobalsArrow(&arrow));
				status = PMSessionPageSetupDialog(printSession, pageFormat, &accepted);
				if (!accepted)
					status = kPMCancel;
			}

			// save any changes to the page format
			if (status == noErr && accepted) {
				DisposeHandle( hPageFormat );
				hPageFormat = nil;
				status = PMFlattenPageFormat(pageFormat, &hPageFormat);
			}

			if (pageFormat != kPMNoPageFormat)
				(void)PMRelease(pageFormat);
		}
	}

	if (printSession) {
		PMRelease(printSession);
		printSession = nil;
	}
}
