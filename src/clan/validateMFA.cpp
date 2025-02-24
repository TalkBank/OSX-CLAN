/**********************************************************************
	"Copyright 1990-2025 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
 */

#define CHAT_MODE 3

#include "cu.h"

#if !defined(UNX)
#define _main validatemfa_main
#define call validatemfa_call
#define getflag validatemfa_getflag
#define init validatemfa_init
#define usage validatemfa_usage
#endif

#define IS_WIN_MODE FALSE
#include "mul.h" 

#define BULLETLESSWORMAX 5

struct rtiers {
	char *sp;	/* code descriptor field of the turn	 */
	char *line;		/* text field of the turn		 */
	char *lineOrg;	/* text field of the turn		 */
	char matched;
	long lineno;
	struct rtiers *nexttier;	/* pointer to the next utterance, if	 */
} ;				/* there is only 1 utterance, i.e. no	 */

struct files {
	int  res;
	long lineno;
	long tlineno;
	FNType fname[FNSize];
	char currentchar;
	AttTYPE currentatt;
	FILE *fpin;
	struct rtiers *tiers;
} ;

extern struct tier *defheadtier;
extern char OverWriteFile;
extern char isExpendX;

static char isFTime, isAnyBulletFound, *isCompFiles, isCompSegFiles;
static char ignoreLetCase, ignorePause, ignoreFewNoBullet, ignoreCompBullets;
static char bulletInsideCode, bulletAfterCode;
static struct files ffile, sfile;

static void errorMessage(char *main, int m, char *wor, int w) {
	fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno-1);
	fprintf(stderr, "Difference found:\n");
	fprintf(stderr, "m0: %s\n", main+m);
	fprintf(stderr, "w0: %s\n", wor+w);
	fprintf(stderr, "main: %s\n", main);
	fprintf(stderr, "wor: %s\n", wor);
}

static int compareMainWorTiers(char *main, char *wor, int missingBulletsCnt) {
	char isBulletFound, isWordFound;
	int m, w, lastSq;
	char isDone;

	for (m=0; main[m] != EOS; m++) {
		if (main[m] == '\n')
			main[m] = ' ';
	}
	for (w=0; wor[w] != EOS; w++) {
		if (wor[w] == '\n')
			wor[w] = ' ';
	}
	isBulletFound = FALSE;
	isWordFound = FALSE;
	isDone = FALSE;
	lastSq = -1;
	if (lineno > 1139)
		m = 0;
	m = 0;
	w = 0;
	while (main[m] != EOS && wor[w] != EOS) {
		while (isSpace(main[m]) || main[m] == HIDEN_C) {
			for (; isSpace(main[m]); m++) ;
			if (main[m] == HIDEN_C) {
				for (m++; main[m] != HIDEN_C && main[m] != EOS; m++) ;
				if (main[m] == HIDEN_C)
					m++;
			}
		}
		while (isSpace(wor[w]) || wor[w] == HIDEN_C) {
			for (; isSpace(wor[w]); w++) ;
			if (wor[w] == HIDEN_C) {
				isBulletFound = TRUE;
				isAnyBulletFound = TRUE;
				for (w++; wor[w] != HIDEN_C && wor[w] != EOS; w++) ;
				if (wor[w] == HIDEN_C)
					w++;
			}
		}
		if (main[m] == '[') {
			lastSq = w;
			for (; main[m] != ']' && main[m] != EOS; m++, w++) {
				if (main[m] != wor[w]) {
					errorMessage(main, m, wor, w);
					isDone = TRUE;;
					break;
				}
				if (wor[w] == HIDEN_C) {
					if (bulletInsideCode == TRUE) {
						errorMessage(main, m, wor, lastSq);
						isDone = TRUE;;
						break;
					}
				}
			}
			if (main[m] == ']' && wor[w] == ']') {
				m++;
				w++;
			} else if (!isDone && main[m] != wor[w]) {
				errorMessage(main, m, wor, w);
				isDone = TRUE;;
				break;
			}
		} else {
			lastSq = -1;
			if (main[m] == '<' && wor[w] == '<') {
				for (; main[m] == '<' && wor[w] == '<'; m++, w++) ;
				for (; isSpace(main[m]); m++) ;
				for (; isSpace(wor[w]); w++) ;
			}
			if (wor[w] == '+' || wor[w] == '0'|| wor[w] == '>' || wor[w] == EOS || uS.IsUtteranceDel(wor, w) ||
				(wor[w] == 'x' && wor[w+1] == 'x') || 
				(wor[w] == '&' && wor[w+1] != '+' && wor[w+1] != '-' )) {
			} else
				isWordFound = TRUE;
			for (; !isSpace(main[m]) && main[m] != '>' && main[m] != EOS; m++, w++) {
				if ((ignoreLetCase==FALSE && main[m] != wor[w]) ||
					(ignoreLetCase==TRUE && toupper((unsigned char)main[m]) != toupper((unsigned char)wor[w]))) {
					if (main[m] == '>' && isSpace(wor[w])) {
						break;
					} else if (wor[w]==HIDEN_C && ignoreCompBullets && m > 0 && main[m-1]=='+' && w>0 && wor[w-1]=='+') {
						while (isSpace(wor[w]) || wor[w] == HIDEN_C) {
							while (isSpace(wor[w]))
								w++;
							if (wor[w] == HIDEN_C) {
								for (w++; wor[w] != HIDEN_C && wor[w] != EOS; w++) ;
								if (wor[w] == HIDEN_C)
									w++;
							}
						}
					} else {
						errorMessage(main, m, wor, w);
						isDone = TRUE;;
						break;
					}
				}
			}
			if (isDone == FALSE) {
				for (; main[m] == '>' && wor[w] == '>'; m++, w++) ;
				if (!isSpace(wor[w]) && wor[w] != '<' && wor[w] != '>') {
					errorMessage(main, m, wor, w);
					isDone = TRUE;;
				}
			}
		}

		if (isDone)
			return(missingBulletsCnt);

		while (isSpace(main[m]) || main[m] == HIDEN_C) {
			for (; isSpace(main[m]); m++) ;
			if (main[m] == HIDEN_C) {
				for (m++; main[m] != HIDEN_C && main[m] != EOS; m++) ;
				if (main[m] == HIDEN_C)
					m++;
			}
		}
		while (isSpace(wor[w]) || wor[w] == HIDEN_C) {
			for (; isSpace(wor[w]); w++) ;
			if (wor[w] == HIDEN_C) {
				isBulletFound = TRUE;
				isAnyBulletFound = TRUE;
				if (bulletAfterCode == TRUE && lastSq != -1) {
					errorMessage(main, m, wor, lastSq);
					isDone = TRUE;;
					break;
				}
				for (w++; wor[w] != HIDEN_C && wor[w] != EOS; w++) ;
				if (wor[w] == HIDEN_C)
					w++;
			}
		}

		if (isDone)
			return(missingBulletsCnt);

		if (ignorePause==TRUE && main[m]=='(' && main[m+1]=='.' && (wor[w]!='(' || wor[w+1]!='.')) {
			for (m++; main[m] != ')' && main[m] != EOS; m++) ;
			if (main[m] == ')')
				m++;
		}
		if (ignorePause == TRUE && (main[m] != '(' || main[m+1] != '.') && wor[w] == '(' && wor[w+1] == '.') {
			for (w++; wor[w] != ')' && wor[w] != EOS; w++) ;
			if (wor[w] == ')')
				w++;

		}
	}
	if (!isDone && main[m] != wor[w]) {
		errorMessage(main, m, wor, w);
	}
	if (!ignoreFewNoBullet && isBulletFound == FALSE && isWordFound && missingBulletsCnt <= BULLETLESSWORMAX) {
		missingBulletsCnt++;
		if (missingBulletsCnt > BULLETLESSWORMAX) {
			fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno - 1);
			fprintf(stderr, "More than %d %%wor tiers found without bullets\n\n", BULLETLESSWORMAX);
		} else {
			fprintf(fpout,"\n*** File \"%s\": line %ld.\n", oldfname, lineno - 1);
			fprintf(stderr, "No bullets found on %%wor tiers\n\n");
		}
	}
	return(missingBulletsCnt);
}

static void compareMainToWor(void) {
	int  missingBulletsCnt;
	char isWORFound;

	isAnyBulletFound = FALSE;
	missingBulletsCnt = 0;
	isWORFound = FALSE;
	spareTier1[0] = EOS;
	spareTier2[0] = EOS;
	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	while (getwholeutter()) {
		if (utterance->speaker[0] == '*') {
			if (spareTier2[0] != EOS) {
				missingBulletsCnt = compareMainWorTiers(spareTier1, spareTier2, missingBulletsCnt);
			}
			strcpy(spareTier1, utterance->line);
			spareTier2[0] = EOS;
		} else if (uS.partcmp(utterance->speaker,"%wor:",FALSE,FALSE)) {
			strcpy(spareTier2, utterance->line);
			isWORFound = TRUE;
		} else if (uS.partcmp(utterance->speaker,"%xwor:",FALSE,FALSE)) {
			isWORFound = TRUE;
			isAnyBulletFound = TRUE;
			fprintf(stderr, "\n%%xwor tiers are not allowed\n\n");
			break;
		}
	}
	if (spareTier2[0] != EOS) {
		missingBulletsCnt = compareMainWorTiers(spareTier1, spareTier2, missingBulletsCnt);
	}
	if (isWORFound == FALSE) {
		fprintf(stderr, "\nNo %%wor or %%xwor tier found\n\n");
	}
	if (isAnyBulletFound == FALSE) {
		fprintf(stderr, "\nNo bullets found on any %%wor tier\n\n");
	}
}

static struct rtiers *FreeUpTiers(struct rtiers *p) {
	struct rtiers *t;
	
	while (p != NULL) {
		t = p;
		p = p->nexttier;
		if (t->sp != NULL)
			free(t->sp);
		if (t->line != NULL)
			free(t->line);
		if (t->lineOrg != NULL)
			free(t->lineOrg);
		free(t);
	}
	return(NULL);
}
// compare files BEG
static struct rtiers *AddTiers(struct rtiers *p, char *sp, char *line, long lineno) {
	int i, j;
	
	if (isCompSegFiles == FALSE && uS.partcmp(sp,"%wor:",FALSE,FALSE))
		return(p);
	if (uS.partcmp(sp,"@Options:",FALSE,FALSE) || uS.partcmp(sp,"@Window:",FALSE,FALSE))
		return(p);
	if (!checktier(sp))
		return(p);
	if (p == NULL) {
		if ((p=NEW(struct rtiers)) == NULL)
			out_of_mem();
		if ((p->sp=(char *)malloc(strlen(sp)+1)) == NULL)
			out_of_mem();
		strcpy(p->sp, sp);
		if ((p->lineOrg=(char *)malloc(strlen(line)+1)) == NULL)
			out_of_mem();
		strcpy(p->lineOrg, line);
		i = 0;
		while (line[i] != EOS) {
			if (line[i] == '\n')
				line[i] = ' ';
			if (line[i] == HIDEN_C) {
				for (j=i+1; line[j] != HIDEN_C && line[j] != EOS; j++) ;
				if (line[j] == HIDEN_C) {
					strcpy(line+i, line+j+1);
				} else
					i++;
			} else
				i++;
		}
		removeExtraSpace(line);
		if ((p->line=(char *)malloc(strlen(line)+1)) == NULL)
			out_of_mem();
		strcpy(p->line, line);
		p->matched = FALSE;
		p->lineno = lineno;
		p->nexttier = NULL;
	} else
		p->nexttier = AddTiers(p->nexttier, sp, line, lineno);
	return(p);
}

static void findAndEpandX(const char *sp, char *ch, AttTYPE *att) {
	int x, index;

	index = 0;
	while (ch[index] != EOS) {
		if (uS.isRightChar(ch, index, '[', &dFnt, MBF)) {
			x = index + 1;
			for (index++; ch[index] != EOS && !uS.isRightChar(ch, index, ']', &dFnt, MBF); index++) ;
			if (ch[index] != EOS)
				index++;
			if ((ch[x] == 'x' || ch[x] == 'X') && isSpace(ch[x+1])) {
				if (ch[index] != EOS)
					index = expandX(ch, att, x, index);
			}
		} else
			index++;
	}
}

static char CompareFilesTiers(void) {
	struct rtiers *fTier, *sTier;
	
	for (fTier=ffile.tiers; fTier != NULL; fTier=fTier->nexttier) {
		for (sTier=sfile.tiers; sTier != NULL; sTier=sTier->nexttier) {
			if (strcmp(fTier->sp, sTier->sp) == 0 && strcmp(fTier->line, sTier->line) == 0) {
				fTier->matched = TRUE;
				sTier->matched = TRUE;
			}
		}
	}
	for (fTier=ffile.tiers; fTier != NULL; fTier=fTier->nexttier) {
		if (fTier->matched == FALSE) {
			fprintf(fpout,"\n*** File \"%s\": line %ld.\n", sfile.fname, sfile.lineno);
			fprintf(fpout,"*** File \"%s\": line %ld.\n", ffile.fname, fTier->lineno);
			fprintf(fpout, "No match of MFA file's tier\n");
			fprintf(fpout, "%s%s\n", fTier->sp, fTier->lineOrg);
			return(TRUE);
		}
	}
	for (sTier=sfile.tiers; sTier != NULL; sTier=sTier->nexttier) {
		if (sTier->matched == FALSE) {
			fprintf(fpout,"\n*** File \"%s\": line %ld.\n", ffile.fname, ffile.lineno);
			fprintf(fpout,"*** File \"%s\": line %ld.\n", sfile.fname, sTier->lineno);
			fprintf(fpout, "No match of original file's tier\n");
			fprintf(fpout, "%s%s\n", sTier->sp, sTier->lineOrg);
			return(TRUE);
		}
	}
	return(FALSE);
}

static void compareFiles(void) {
	char *s, ext[64]; //, cc;
	int  fnLen, compLen;

	strcpy(ffile.fname, oldfname);
	ffile.fpin = fpin;
	ffile.lineno = lineno;
	ffile.tlineno = tlineno;
	ffile.tiers = NULL;

	if (isCompFiles[0] == '-') {
		strcpy(FileName1, oldfname);
		s = strchr(FileName1, '.');
		if (s != NULL) {
			strcpy(ext, s);
			*s = EOS;
			fnLen = strlen(FileName1);
			compLen = strlen(isCompFiles);
			if (fnLen > compLen && uS.mStricmp(FileName1+(fnLen-compLen), isCompFiles) == 0)
				return;
			strcat(FileName1, isCompFiles);
			strcat(FileName1, ext);
		}
		strcpy(sfile.fname, FileName1);
	} else {
		strcpy(FileName1, isCompFiles);
		if (FileName1[strlen(FileName1)-1] != PATHDELIMCHR)
			strcat(FileName1, PATHDELIMSTR);
		strcat(FileName1, oldfname);
		strcpy(sfile.fname, FileName1);
	}
	if ((sfile.fpin=fopen(sfile.fname, "r")) == NULL) {
		fprintf(stderr, "Can't open file \"%s\".\n", sfile.fname);
		cutt_exit(0);
	}
	sfile.lineno = lineno;
	sfile.tlineno = tlineno;
	sfile.tiers = NULL;
	if (isCompFiles[0] != '-')
		fprintf(stderr,"From file <%s>\n", sfile.fname);
	
	ffile.currentatt = 0;
	ffile.currentchar = (char)getc_cr(ffile.fpin, &ffile.currentatt);
	sfile.currentatt = 0;
	sfile.currentchar = (char)getc_cr(sfile.fpin, &sfile.currentatt);
	do {
		fpin = ffile.fpin;
		currentchar = ffile.currentchar;
		currentatt = ffile.currentatt;
		lineno  = ffile.lineno;
		tlineno = ffile.tlineno;
//		cc = currentchar;
		do {
			if ((ffile.res = getwholeutter())) {
				ffile.fpin = fpin;
				ffile.currentchar = currentchar;
				ffile.currentatt = currentatt;
				ffile.lineno = lineno;
				ffile.tlineno = tlineno;
				ffile.tiers = AddTiers(ffile.tiers, utterance->speaker, utterance->line, lineno);
			}
		} while (ffile.res && currentchar != '*'/* && (currentchar != '@' || cc == '@')*/) ;

		fpin = sfile.fpin;
		currentchar = sfile.currentchar;
		currentatt = sfile.currentatt;
		lineno  = sfile.lineno;
		tlineno = sfile.tlineno;
//		cc = currentchar;
		do {
			if ((sfile.res = getwholeutter())) {
				sfile.fpin = fpin;
				sfile.currentchar = currentchar;
				sfile.currentatt = currentatt;
				sfile.lineno = lineno;
				sfile.tlineno = tlineno;
				if (isExpendX && utterance->speaker[0] == '*')
					findAndEpandX(utterance->speaker, utterance->line, utterance->attLine);
				sfile.tiers = AddTiers(sfile.tiers, utterance->speaker, utterance->line, lineno);
			}
		} while (sfile.res && currentchar != '*'/* && (currentchar != '@' || cc == '@')*/) ;
		
		if (!ffile.res && !sfile.res) {
			if (CompareFilesTiers())
				break;
			ffile.tiers = FreeUpTiers(ffile.tiers);
			sfile.tiers = FreeUpTiers(sfile.tiers);
			break;
		}
		if (ffile.currentchar == '*' || ffile.currentchar == '@' || ffile.currentchar == EOF) {
			if (CompareFilesTiers()) {
				break;
			}
			ffile.tiers = FreeUpTiers(ffile.tiers);
			sfile.tiers = FreeUpTiers(sfile.tiers);
		}

	} while (1) ;
	fclose(sfile.fpin);
}
// compare files END
// compare Seg files BEG
static struct rtiers *AdSegTiers(struct rtiers *p, char *sp, char *line, long lineno) {
	int i, j;
	
	if ((p=NEW(struct rtiers)) == NULL)
		out_of_mem();
	if ((p->sp=(char *)malloc(strlen(sp)+1)) == NULL)
		out_of_mem();
	strcpy(p->sp, sp);
	if ((p->lineOrg=(char *)malloc(strlen(line)+1)) == NULL)
		out_of_mem();
	strcpy(p->lineOrg, line);
	i = 0;
	while (line[i] != EOS) {
		if (line[i] == '\n')
			line[i] = ' ';
		else if (line[i] == '&' && line[i+1] == '&' && line[i+2] == '&') {
			line[i]   = ' ';
			line[i+1] = ' ';
			line[i+2] = ' ';
		}
		if (sp[0] != '%' && line[i] == HIDEN_C) {
			for (j=i+1; line[j] != HIDEN_C && line[j] != EOS; j++) ;
			if (line[j] == HIDEN_C) {
				strcpy(line+i, line+j+1);
			} else
				i++;
		} else
			i++;
	}
	removeExtraSpace(line);
	if ((p->line=(char *)malloc(strlen(line)+1)) == NULL)
		out_of_mem();
	strcpy(p->line, line);
	p->matched = FALSE;
	p->lineno = lineno;
	p->nexttier = NULL;
	return(p);
}

static int getNextSegWord(char *line, int i, char *word) {
	int j;
	
	for (; isSpace(line[i]); i++) ;
	if (line[i] == EOS) {
		word[0] = EOS;
		return(0);
	}
	j = 0;
	for (; !isSpace(line[i]) && line[i] != EOS; i++) {
		word[j++] = line[i];
	}
	word[j] = EOS;
	return(i);
}

static void compareSegFiles(void) {
	int  oi, si, pass;
	char *st, ow[BUFSIZ], sw[BUFSIZ], oFTime, sFTime;
	
	strcpy(ffile.fname, oldfname);
	ffile.fpin = fpin;
	
	strcpy(FileName1, oldfname);
	st = strchr(FileName1, '.');
	if (st != NULL)
		*st = EOS;
	strcat(FileName1, ".seg.cex");
	strcpy(sfile.fname, FileName1);
	if ((sfile.fpin=fopen(sfile.fname, "r")) == NULL) {
		fprintf(stderr, "Can't open file \"%s\".\n", sfile.fname);
		cutt_exit(0);
	}
//	fprintf(stderr,"From file <%s>\n", sfile.fname);

	for (pass=0; pass < 3; pass++) {
		rewind(ffile.fpin);
		rewind(sfile.fpin);
		ffile.lineno = 0;
		ffile.tiers = NULL;
		sfile.lineno = 0;
		sfile.tiers = NULL;

		ffile.currentatt = 0;
		ffile.currentchar = (char)getc_cr(ffile.fpin, &ffile.currentatt);
		sfile.currentatt = 0;
		sfile.currentchar = (char)getc_cr(sfile.fpin, &sfile.currentatt);

//if (pass == 2)
//si = 0;
		fpin = ffile.fpin;
		currentchar = ffile.currentchar;
		currentatt = ffile.currentatt;
		lineno  = ffile.lineno;
		tlineno = ffile.tlineno;
		while ((ffile.res=getwholeutter())) {
			if (uS.partcmp(utterance->speaker,"@Options:",FALSE,FALSE) || uS.partcmp(utterance->speaker,"@Window:",FALSE,FALSE) ||
				!checktier(utterance->speaker)) {
			} else if (pass == 0 && utterance->speaker[0] != '@') {
			} else if (pass == 1 && utterance->speaker[0] != '*') {
			} else if (pass == 2 && utterance->speaker[0] != '%') {
			} else {
				ffile.fpin = fpin;
				ffile.currentchar = currentchar;
				ffile.currentatt = currentatt;
				ffile.lineno = lineno;
				ffile.tlineno = tlineno;
				ffile.tiers = AdSegTiers(ffile.tiers, utterance->speaker, utterance->line, lineno);
				break;
			}
		}
		
		fpin = sfile.fpin;
		currentchar = sfile.currentchar;
		currentatt = sfile.currentatt;
		lineno  = sfile.lineno;
		tlineno = sfile.tlineno;
		while ((sfile.res=getwholeutter())) {
			if (uS.partcmp(utterance->speaker,"@Options:",FALSE,FALSE) || uS.partcmp(utterance->speaker,"@Window:",FALSE,FALSE) ||
				!checktier(utterance->speaker)) {
			} else if (pass == 0 && utterance->speaker[0] != '@') {
			} else if (pass == 1 && utterance->speaker[0] != '*') {
			} else if (pass == 2 && utterance->speaker[0] != '%') {
			} else {
				sfile.fpin = fpin;
				sfile.currentchar = currentchar;
				sfile.currentatt = currentatt;
				sfile.lineno = lineno;
				sfile.tlineno = tlineno;
				sfile.tiers = AdSegTiers(sfile.tiers, utterance->speaker, utterance->line, lineno);
				break;
			}
		}

		oFTime = FALSE;
		sFTime = FALSE;
		oi = 0;
		si = 0;
		while (ffile.res == TRUE && sfile.res == TRUE) {
			while ((oi=getNextSegWord(ffile.tiers->line, oi, ow)) == 0 && oFTime == TRUE) {
				ffile.tiers = FreeUpTiers(ffile.tiers);
				fpin = ffile.fpin;
				currentchar = ffile.currentchar;
				currentatt = ffile.currentatt;
				lineno  = ffile.lineno;
				tlineno = ffile.tlineno;
				while ((ffile.res=getwholeutter())) {
					if (uS.partcmp(utterance->speaker,"@Options:",FALSE,FALSE) || uS.partcmp(utterance->speaker,"@Window:",FALSE,FALSE) ||
						!checktier(utterance->speaker)) {
					} else if (pass == 0 && utterance->speaker[0] != '@') {
					} else if (pass == 1 && utterance->speaker[0] != '*') {
					} else if (pass == 2 && utterance->speaker[0] != '%') {
					} else {
						ffile.fpin = fpin;
						ffile.currentchar = currentchar;
						ffile.currentatt = currentatt;
						ffile.lineno = lineno;
						ffile.tlineno = tlineno;
						ffile.tiers = AdSegTiers(ffile.tiers, utterance->speaker, utterance->line, lineno);
						oFTime = FALSE;
						break;
					}
				}
				if (ffile.res == FALSE)
					break;
			}
			oFTime = TRUE;
			if (ffile.res == FALSE)
				break;
			while ((si=getNextSegWord(sfile.tiers->line, si, sw)) == 0 && sFTime == TRUE) {
				sfile.tiers = FreeUpTiers(sfile.tiers);
				fpin = sfile.fpin;
				currentchar = sfile.currentchar;
				currentatt = sfile.currentatt;
				lineno  = sfile.lineno;
				tlineno = sfile.tlineno;
				while ((sfile.res=getwholeutter())) {
					if (uS.partcmp(utterance->speaker,"@Options:",FALSE,FALSE) || uS.partcmp(utterance->speaker,"@Window:",FALSE,FALSE) ||
						!checktier(utterance->speaker)) {
					} else if (pass == 0 && utterance->speaker[0] != '@') {
					} else if (pass == 1 && utterance->speaker[0] != '*') {
					} else if (pass == 2 && utterance->speaker[0] != '%') {
					} else {
						sfile.fpin = fpin;
						sfile.currentchar = currentchar;
						sfile.currentatt = currentatt;
						sfile.lineno = lineno;
						sfile.tlineno = tlineno;
						sfile.tiers = AdSegTiers(sfile.tiers, utterance->speaker, utterance->line, lineno);
						sFTime = FALSE;
						break;
					}
				}
				if (sfile.res == FALSE)
					break;
			}
			sFTime = TRUE;
			if (sfile.res == FALSE)
				break;
			if (uS.mStricmp(ffile.tiers->sp, sfile.tiers->sp) != 0 || uS.mStricmp(ow, sw) != 0) {
				fprintf(fpout,"\n*** File \"%s\": line %ld.\n", ffile.fname, ffile.lineno);
				fprintf(fpout,"*** File \"%s\": line %ld.\n", sfile.fname, sfile.lineno);
				fprintf(fpout, "Words mismatch:\n");
				fprintf(fpout, "orig=\"%s\"; seg=\"%s\"\n", ow, sw);
				fprintf(fpout, "on lines:\n");
				fprintf(fpout, "%s%s\n", ffile.tiers->sp, ffile.tiers->lineOrg);
				fprintf(fpout, "%s%s\n", ffile.tiers->sp, sfile.tiers->lineOrg);
				break;
			}
		}
		ffile.tiers = FreeUpTiers(ffile.tiers);
		sfile.tiers = FreeUpTiers(sfile.tiers);
	}
	fclose(sfile.fpin);
}
// compare Seg files END

void call() {
	if (isCompSegFiles == TRUE)
		compareSegFiles();
	else if (isCompFiles == NULL)
		compareMainToWor();
	else
		compareFiles();
}

void init(char f) {
	if (f) {
		isFTime = TRUE;
		isCompFiles = NULL;
		ignoreLetCase = FALSE;
		bulletInsideCode = TRUE;
		bulletAfterCode = FALSE;
		ignorePause = TRUE;
		ignoreFewNoBullet = TRUE;
		ignoreCompBullets = FALSE;
		isCompSegFiles = FALSE;
		isExpendX = FALSE;
		combinput = TRUE;
		OverWriteFile = TRUE;
		FilterTier = 0;
		LocalTierSelect = TRUE;
		if (defheadtier) {
			if (defheadtier->nexttier != NULL)
				free(defheadtier->nexttier);
			free(defheadtier);
			defheadtier = NULL;
		}
	} else {
		if (isFTime) {
			isFTime = FALSE;
		}
	}
}

void usage() {
	printf("Validate MFA tier against main speaker tier and original CHAT files\n");
	printf("Usage: validatemfa [bF cN d %s] filename(s)\n",mainflgs());
	puts("+a : compare segment \"*.seg.cex\" and original \"*.cha\" files in the same folder");
	puts("+bF: compare WOR files to same filenames in F folder");
	puts("+c : ignore upper and lower case letters (default case sensitive)");
	puts("+c1: bullets inside [...] codes (default check)");
	puts("+c2: check for bullets after codes [/] instead of words (default do not check)");
	puts("+c3: do not compare pauses (default compare all pauses)");
	puts("+c4: do not ignore some missing bullets on %wor tiers (default ignore few bullets)");
	puts("+c5: ignore compound+ bullets on %wor tier (default do not ignore them)");
	puts("+d90: expand [x #] before comparing to original file");
	mainusage(FALSE);
	puts("Example:");
	puts("   if segment \"*.seg.cex\" and original \"*.cha\" files in the same folder:");
	puts("       validateMFA -a *.cha");
	puts("   if MFA files are in the working folder & original files with the same filename are	in the \"orig\" folder:");
	puts("       validateMFA -borig *.cha");
	puts("   To compare speaker tier to %wor tier:");
	puts("       validateMFA *.cha");
	puts("   Commands for expanding and validating [x #]");
	puts("       kwal -d90 +d +f +t@ +t% *.cha");
	puts("       ren -f *.kwal.cex *.cha");
	puts("       validateMFA *.cha +d90 -borig");
	puts("       validateMFA *.cha -borig");
	cutt_exit(0);
}


CLAN_MAIN_RETURN main(int argc, char *argv[]) {

	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = VALIDATEMFA;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
	bmain(argc,argv,NULL);
}

void getflag(char *f, char *f1, int *i) {
	f++;
	switch(*f++) {
		case 'a':
			isCompSegFiles = TRUE;
			no_arg_option(f);
			break;
		case 'b':
			if (*f != EOS)
				isCompFiles = f;
			else
				fprintf(stderr, "Please specify original files folder\n");
			break;
		case 'c':
			if (*f == EOS)
				ignoreLetCase = !ignoreLetCase;
			else if (*f == '1')
				bulletInsideCode = !bulletInsideCode;
			else if (*f == '2')
				bulletAfterCode = !bulletAfterCode;
			else if (*f == '3')
				ignorePause = !ignorePause;
			else if (*f == '4')
				ignoreFewNoBullet = !ignoreFewNoBullet;
			else if (*f == '5')
				ignoreCompBullets = !ignoreCompBullets;
			break;
		case 'd':
			if (*f == '9' && *(f+1) == '0') {
				isExpendX = TRUE;
			}
			break;
		default:
			maingetflag(f-2,f1,i);
			break;
	}
}
