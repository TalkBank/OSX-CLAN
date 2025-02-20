/**********************************************************************
	"Copyright 1990-2025 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#include "cu.h"
#include "check.h"
#ifdef _WIN32 
	#include "stdafx.h"
#endif

#if !defined(UNX)
#define _main chat2srt_main
#define call chat2srt_call
#define getflag chat2srt_getflag
#define init chat2srt_init
#define usage chat2srt_usage
#endif

#include "mul.h" 

#define IS_WIN_MODE FALSE
#define CHAT_MODE 3

extern struct tier *defheadtier;
extern char OverWriteFile;
extern char GExt[];

static char chat2srt_FTime;
static char isUseDepTier;
static char isClean;
static char isVTT;


void init(char f) {
	if (f) {
		chat2srt_FTime = TRUE;
		isUseDepTier = FALSE;
		OverWriteFile = TRUE;
		stout = FALSE;
		onlydata = 1;
		isClean = FALSE;
		isVTT = FALSE;
		AddCEXExtension = "";
		if (defheadtier->nexttier != NULL)
			free(defheadtier->nexttier);
		free(defheadtier);
		defheadtier = NULL;
// defined in "mmaininit" and "globinit" - nomap = TRUE;
	} else {
		if (chat2srt_FTime) {
			chat2srt_FTime = FALSE;
			if (isClean) {
				addword('\0','\0',"+</?>");
				addword('\0','\0',"+</->");
				addword('\0','\0',"+<///>");
				addword('\0','\0',"+<//>");
				addword('\0','\0',"+</>");  // list R6 in mmaininit funtion in cutt.c
				addword('\0','\0',"+[- *]");
				addword('\0','\0',"+#*");
				addword('\0','\0',"+xxx");
				addword('\0','\0',"+xx");
				addword('\0','\0',"+yyy");
				addword('\0','\0',"+yy");
				addword('\0','\0',"+www");
				maininitwords();
			} else
				FilterTier = 0;
			if (isVTT) {
				strcpy(GExt, ".vtt");
			}
		}
	}
}

void usage() {
	printf("Convert CHAT notation to SRT specific notation\n");
	printf("Usage: chat2srt [%s] filename(s)\n", mainflgs());
	puts("+d : create clean output without codes and replacements (default: output everything)");
	puts("+v : create WEBVTT formatted file (default: create SRT formatted file)");
	mainusage(TRUE);
}


CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = CHAT2SRT;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
	bmain(argc,argv,NULL);
}
		
void getflag(char *f, char *f1, int *i) {

	f++;
	switch(*f++) {
		case 'd':
			isClean = TRUE;
			break;
		case 't':
			if (*f == '%') {
				isUseDepTier = TRUE;
			}
			maingetflag(f-2,f1,i);
			break;
		case 'v':
			isVTT = TRUE;
			no_arg_option(f);
			break;
		default:
			maingetflag(f-2,f1,i);
			break;
	}
}

static void filterText(char *st) {
	int i, j;

	for (i=0; st[i] != EOS; i++) {
		if (st[i] == '[') {
			if (st[i+1] == ':' && st[i+2] == ' ') {
				if (st[i+3] == 'x' && st[i+4] == '@' && st[i+5] == 'n' && st[i+6] == ']') {
					st[i] = ' ';
					st[i+1] = ' ';
					st[i+2] = ' ';
					st[i+3] = ' ';
					st[i+4] = ' ';
					st[i+5] = ' ';
					st[i+6] = ' ';
				}
			} else if ((st[i+1] == '*' || st[i+1] == '+') && st[i+2] == ' ') {
				for (j=i; st[j] != ']' && st[j] != EOS; j++) {
					st[j] = ' ';
				}
				if (st[j] == ']')
					st[j] = ' ';
			}
		} else if (st[i] == '&' && st[i+1] == '=') {
			for (j=i; !uS.isskip(st,j,&dFnt,MBF) && st[j] != EOS; j++) {
				st[j] = ' ';
			}
		}
	}
	removeExtraSpace(templineC1);
	uS.remFrontAndBackBlanks(templineC1);
}

//	Chat2Str_TextToXML(from, to);
static void Chat2Str_TextToXML(const char *from, char *to) {
	long i;

	i = 0L;
	for (; *from != EOS; from++) {
/*
		if (*from == '&') {
			strcpy(to+i, "&amp;");
			i = strlen(to);
		} else if (*from == '"') {
			strcpy(to+i, "&quot;");
			i = strlen(to);
		} else if (*from == '\'') {
			 strcpy(to+i, "&apos;");
			 i = strlen(to);
		} else
*/
		if (*from == '<') {
			strcpy(to+i, "&lt;");
			i = strlen(to);
		} else if (*from == '>') {
			strcpy(to+i, "&gt;");
			i = strlen(to);
		} else if (*from >= 0 && *from < 32) {
			sprintf(to+i,"{0x%x}", *from);
			i = strlen(to);
		} else
			to[i++] = *from;
	}
	to[i] = EOS;
}

static char isEmpty(char *st) {
	for (; *st != EOS; st++) {
		if (isalnum(*st))
			return(FALSE);
	}
	return (TRUE);
}

static void convertMsecToStr(long t, char *st) {
	int h, m, s, ms, y;

	y = 60 * 60 * 1000;
	h = (t / y);
	m = ((t / (1000 * 60)) % 60);
	s = (t / 1000) % 60;
	ms = t - (h*y)-(m*(y/60))-(s*1000);
	st[0] = EOS;
	if (!isVTT || h > 0) {
		if (h < 10)
			strcat(st, "0");
		y = strlen(st);
		sprintf(st+y, "%d:", h);
	}
	if (m < 10)
		strcat(st, "0");
	y = strlen(st);
	sprintf(st+y, "%d:", m);
	if (s < 10)
		strcat(st, "0");
	y = strlen(st);
	sprintf(st+y, "%d", s);
	if (isVTT)
		strcat(st, ".");
	else
		strcat(st, ",");
	if (ms < 10)
		strcat(st, "00");
	else if (ms < 100)
		strcat(st, "0");
	y = strlen(st);
	sprintf(st+y, "%d", ms);
}

void call() {
	int  i, j, begI, cnt, bulletCnt;
	char isBulletFound, isSkipDepTier, begS[15], endS[15], t;
	FNType Fname[FILENAME_MAX];
	long Beg = 0L;
	long End = 0L, lastEnd;

	if (isVTT) {
		fprintf(fpout, "WEBVTT\n");
		cnt = 1;
	} else
		cnt = 0;
	isSkipDepTier = FALSE;
	lastEnd = 0L;
	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	while (getwholeutter()) {
		if (utterance->speaker[0] == '*') {
			bulletCnt = 0;
			isSkipDepTier = FALSE;
			begI = 0;
			for (i=0; uttline[i] != EOS; i++) {
				if (utterance->line[i] == HIDEN_C) {
					isBulletFound = FALSE;
					if (isdigit(utterance->line[i+1])) {
						if (cMediaFileName[0] == EOS) {
							fprintf(stderr, "\nMissing %s header tier with media file name.\n\n", MEDIAHEADER);
							cutt_exit(0);
						}
						if (getMediaTagInfo(utterance->line+i, &Beg, &End)) {
							isBulletFound = TRUE;
						}
					} else {
						if (getOLDMediaTagInfo(utterance->line+i, SOUNDTIER, Fname, &Beg, &End)) {
							isBulletFound = TRUE;
						} else if (getOLDMediaTagInfo(utterance->line+i, REMOVEMOVIETAG, Fname, &Beg, &End)) {
							isBulletFound = TRUE;
						}
					}
					if (isBulletFound) {
						t = uttline[i];
						uttline[i] = EOS;
						strcpy(templineC, uttline+begI);
						uttline[i] = t;
						for (j=0; templineC[j] != EOS; j++) {
							if (templineC[j] == '\n' || templineC[j] == '\r')
								templineC[j] = ' ';
						}
						removeExtraSpace(templineC);
						uS.remFrontAndBackBlanks(templineC);
						if (lastEnd > 0L && Beg < lastEnd)
							Beg = lastEnd;
						if (Beg < End && End > lastEnd) {
							if (isUseDepTier) {
								if (bulletCnt > 0) {
									fprintf(stderr, "*** File \"%s\": line %ld.\n", oldfname, lineno);
									fprintf(stderr, "More than one bullet found on tier.\n\n");
									fprintf(fpout, "*** line %ld.\n", lineno);
									fprintf(fpout, "More than one bullet found on tier.\n\n");
									return;
								}
								bulletCnt++;
							} else {
								convertMsecToStr(Beg, begS);
								convertMsecToStr(End, endS);
								if (!isEmpty(templineC)) {
									if (cnt > 0)
										fprintf(fpout, "\n");
									fprintf(fpout, "%d\n", cnt);
									fprintf(fpout, "%s --> %s\n", begS, endS);
									if (isVTT) {
										Chat2Str_TextToXML(templineC, templineC1);
										filterText(templineC1);
										fprintf(fpout, "%s\n", templineC1);
									} else
										fprintf(fpout, "%s\n", templineC);
									cnt++;
								}
							}
							lastEnd = End;
						} else
							isSkipDepTier = TRUE;
						for (begI=i+1; utterance->line[begI] != HIDEN_C && utterance->line[begI] != EOS; begI++) ;
						if (utterance->line[begI] == EOS)
							break;
						i = begI;
						begI++;
					}
				}
			}
		} else if (isUseDepTier && !isSkipDepTier && utterance->speaker[0] == '%') {
			strcpy(templineC, uttline);
			for (j=0; templineC[j] != EOS; j++) {
				if (templineC[j] == '\n' || templineC[j] == '\r')
					templineC[j] = ' ';
			}
			removeExtraSpace(templineC);
			uS.remFrontAndBackBlanks(templineC);
			convertMsecToStr(Beg, begS);
			convertMsecToStr(End, endS);
			if (!isEmpty(templineC)) {
				if (cnt > 0)
					fprintf(fpout, "\n");
				fprintf(fpout, "%d\n", cnt);
				fprintf(fpout, "%s --> %s\n", begS, endS);
				fprintf(fpout, "%s\n", templineC);
				cnt++;
			}
		}
	}
}
