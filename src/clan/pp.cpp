
/**********************************************************************
	"Copyright 1990-2025 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/

#include "cu.h"
#ifdef _WIN32 
	#include "stdafx.h"
#endif

#if !defined(UNX)
#define _main pp_main
#define call pp_call
#define getflag pp_getflag
#define init pp_init
#define usage pp_usage
#endif

#include "mul.h" 

#define IS_WIN_MODE FALSE
#define CHAT_MODE 0

extern char OverWriteFile;

static char isCompact;

void init(char f) {
	char *s;

	if (f) {
		OverWriteFile = TRUE;
		AddCEXExtension = ".xml";
		onlydata = 1;
		stout = FALSE;
		isCompact = FALSE;
	} else {
		if ((s=strrchr(oldfname, '.')) != NULL) {
			AddCEXExtension = s;
		}
	}
}

void usage() {
	printf("Usage: pp [c] filename(s)\n",mainflgs());
	puts("+c : create compact version");
	mainusage(TRUE);
}


CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = PP;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
	bmain(argc,argv,NULL);
}

void getflag(char *f, char *f1, int *i) {

	f++;
	switch(*f++) {
		case 'c':
			isCompact = TRUE;
			no_arg_option(f);
			break;
		default:
			maingetflag(f-2,f1,i);
			break;
	}
}

static void changeGroupIndent(char *groupIndent, int d) {
	int  i, offset;
	char sps[10];

	if (isCompact == TRUE) {
		strcpy(sps, "  ");
		offset = 2;
	} else {
		strcpy(sps, "    ");
		offset = 4;
	}
	if (d > 0)
		strcat(groupIndent, sps);
	else {
		i = strlen(groupIndent) - offset;
		if (i < 0)
			i = 0;
		groupIndent[i] = EOS;
	}
}

void call() {
	int inp;
	char groupIndent[1024];
	char c, lastC;
	char isCR, isLastSpace, isIndent, ftime;

	ftime = true;
	lastC = 0;
	isCR = true;
	isIndent = TRUE;
	groupIndent[0] = EOS;
	if (fgets_cr(templineC, UTTLINELEN, fpin) == NULL)
		return;
	inp = 0;
	while (true) {
		if (templineC[inp] == EOS) {
			if (fgets_cr(templineC, UTTLINELEN, fpin) == NULL) {
					if (!isCR) {
						putc('\n', fpout);
						isLastSpace = FALSE;
					}
					break;
				}
			inp = 0;
		}
		c = templineC[inp++];
		if (isCompact && c == '\t')
			c = ' ';
		if (c == '<') {
			if (!ftime && isIndent == TRUE) {
				putc('\n', fpout);
				isLastSpace = FALSE;
			}
			lastC = c;
			if (templineC[inp] == EOS) {
				if (fgets_cr(templineC, UTTLINELEN, fpin) == NULL) {
					if (!isCR) {
						putc('\n', fpout);
						isLastSpace = FALSE;
					}
					break;
				}
				inp = 0;
			}
			c = templineC[inp++];
				if (c != '/') {
					if (isIndent == TRUE)
						fputs(groupIndent, fpout);
					changeGroupIndent(groupIndent, 1);
				} else {
					changeGroupIndent(groupIndent, -1);
					if (isIndent == TRUE)
						fputs(groupIndent, fpout);
				}
			isIndent = TRUE;
			if (isCompact == TRUE) {
				if (uS.mStrnicmp(templineC+inp-1, "comment ", 8) == 0 ||
					uS.mStrnicmp(templineC+inp-1, "stem>", 5) == 0 ||
					uS.mStrnicmp(templineC+inp-1, "mk ", 3) == 0 ||
					uS.mStrnicmp(templineC+inp-1, "c>", 2) == 0 ||
					uS.mStrnicmp(templineC+inp-1, "s>", 2) == 0
//					uS.mStrnicmp(templineC+inp-1, "w>", 2) == 0
					)
					isIndent = FALSE;
			} 
			putc(lastC, fpout);
			isLastSpace = FALSE;
			if (c != '\n') {
				if (c == ' ' && isLastSpace == TRUE && isCompact == TRUE) {
				} else
					putc(c, fpout);
			}
			if (c == ' ')
				isLastSpace = TRUE;
			else
				isLastSpace = FALSE;
			isCR = true;
		} else if (c == '>') {
			putc(c, fpout);
			isLastSpace = FALSE;
			if (lastC == '/' || lastC == '?')
				changeGroupIndent(groupIndent, -1);
			isCR = false;
		} else if (c != '\n' && c != 0x0d && c != 0x0a) {
			if (!isspace(c) && isIndent == TRUE) {
				if (!isCR) {
					putc('\n', fpout);
					isLastSpace = FALSE;
					fputs(groupIndent, fpout);
				}
				isCR = true;
			}
			if (c == ' ' && isLastSpace == TRUE && isCompact == TRUE) {
			} else
				putc(c, fpout);
			if (c == ' ')
				isLastSpace = TRUE;
			else
				isLastSpace = FALSE;
		}
		lastC = c;
		ftime = false;
	}
}
