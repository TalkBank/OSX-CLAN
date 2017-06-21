/**********************************************************************
 "Copyright 1990-2014 Brian MacWhinney. Use is subject to Gnu Public License
 as stated in the attached "gpl.txt" file."
 */

// compair two indentical in speaker tier codes files and copy bullets from one of them to another file

#define CHAT_MODE 1
#include "cu.h"

#if !defined(UNX)
#define _main temp_main
#define call temp_call
#define getflag temp_getflag
#define init temp_init
#define usage temp_usage
#endif

#define IS_WIN_MODE FALSE
#include "mul.h" 

extern struct tier *defheadtier;
extern char OverWriteFile;

struct rtiers {
	char *speaker;	/* code descriptor field of the turn	 */
	AttTYPE *attSp;
	char *line;		/* text field of the turn		 */
	AttTYPE *attLine;
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

static FNType OutFile[FNSize];
static struct files ffile, sfile;

void usage() {
	printf("Usage: temp [%s] filename(s)\n",mainflgs());
	mainusage(TRUE);
}

void init(char f) {
	if (f) {
		stout = TRUE;
		onlydata = 1;
		OverWriteFile = TRUE;
		FilterTier = 0;
		LocalTierSelect = TRUE;
		if (defheadtier != NULL) {
			if (defheadtier->nexttier != NULL)
				free(defheadtier->nexttier);
			free(defheadtier);
			defheadtier = NULL;
		}
	} else {
	}
}

CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = TEMP;
	OnlydataLimit = 0;
	UttlineEqUtterance = TRUE;
	bmain(argc,argv,NULL);
}

void getflag(char *f, char *f1, int *i) {
	f++;
	switch(*f++) {
		default:
			maingetflag(f-2,f1,i);
			break;
	}
}

static struct rtiers *FreeUpTiers(struct rtiers *p) {
	struct rtiers *t;
	
	while (p != NULL) {
		t = p;
		p = p->nexttier;
		free(t->speaker);
		free(t->attSp);
		free(t->line);
		free(t->attLine);
		free(t);
	}
	return(NULL);
}

static struct rtiers *AddTiers(struct rtiers *p, char *sp, AttTYPE *attSp, char *line, AttTYPE *attLine, long lineno) {
	if (p == NULL) {
		if ((p=NEW(struct rtiers)) == NULL) out_of_mem();
		if ((p->speaker=(char *)malloc(strlen(sp)+1)) == NULL) out_of_mem();
		if ((p->attSp=(AttTYPE *)malloc((strlen(sp)+1)*sizeof(AttTYPE))) == NULL) out_of_mem();
		if ((p->line=(char *)malloc(strlen(line)+1)) == NULL) out_of_mem();
		if ((p->attLine=(AttTYPE *)malloc((strlen(line)+1)*sizeof(AttTYPE))) == NULL) out_of_mem();
		att_cp(0, p->speaker, sp, p->attSp, attSp);
		att_cp(0, p->line, line, p->attLine, attLine);
		p->lineno = lineno;
		p->nexttier = NULL;
	} else
		p->nexttier = AddTiers(p->nexttier, sp, attSp, line, attLine, lineno);
	return(p);
}

static void transfer(void) {
	int i, j, len;
	struct rtiers *ft, *st;

	ft = ffile.tiers;
	st = sfile.tiers;
	if (st->speaker[0] == '*') {
		att_cp(0, templineC, st->line, tempAtt, st->attLine);
		free(st->line);
		free(st->attLine);
		len = strlen(templineC);
		j = len;
		for (i=0L; ft->line[i] != EOS; ) {
			if (ft->line[i] == HIDEN_C) {
				templineC[j++] = ' ';
				do {
					templineC[j++] = ft->line[i];
					i++;
				} while (ft->line[i] != HIDEN_C && ft->line[i] != EOS) ;
				if (ft->line[i] == HIDEN_C) {
					templineC[j++] = ft->line[i];
					i++;
				}
			} else
				i++;
		}
		templineC[j] = EOS;
		for (j=len; templineC[j] != EOS; j++)
			tempAtt[j] = 0;
		if ((st->line=(char *)malloc(strlen(templineC)+1)) == NULL) out_of_mem();
		if ((st->attLine=(AttTYPE *)malloc((strlen(templineC)+1)*sizeof(AttTYPE))) == NULL) out_of_mem();
		att_cp(0, st->line, templineC, st->attLine, tempAtt);
	}
	while (st != NULL) {
		uS.remblanks(st->speaker);
		printout(st->speaker,st->line,st->attSp,st->attLine,TRUE);
		st = st->nexttier;
	}
}

void call(void) {
	char *s, cc;
	struct rtiers *ft, *st;

	strcpy(ffile.fname, wd_dir);
	addFilename2Path(ffile.fname, oldfname);
	ffile.fpin = fpin;
	ffile.lineno = lineno;
	ffile.tlineno = tlineno;
	ffile.currentatt = 0;
	ffile.currentchar = (char)getc_cr(ffile.fpin, &ffile.currentatt);
	strcpy(FileName1, ffile.fname);
	s = strrchr(FileName1, '/');
	*s = EOS;
	s = strrchr(FileName1, '/');
	*s = EOS;
	addFilename2Path(FileName1, "Weist");
	strcpy(OutFile, FileName1);
	addFilename2Path(FileName1, oldfname);
	if ((sfile.fpin=fopen(FileName1, "r")) == NULL) {
		fprintf(stderr, "Can't read file %s\n", FileName1);
		cutt_exit(0);
	}
	s = strrchr(oldfname, '.');
	if (s != NULL) {
		*s = EOS;
		addFilename2Path(OutFile, oldfname);
		*s = '.';
		strcat(OutFile, ".tmp.cex");
	} else {
		addFilename2Path(OutFile, oldfname);
		strcat(OutFile, ".tmp.cex");
	}
	if ((fpout=fopen(OutFile, "w")) == NULL) {
		fprintf(stderr, "Can't create file \"%s\", perhaps it is opened by another application\n", OutFile);
		fclose(sfile.fpin);
		cutt_exit(0);
	}
#if defined(_MAC_CODE)
	settyp(OutFile, 'TEXT', the_file_creator.out, FALSE);
#endif
	strcpy(newfname, OutFile);
	strcpy(sfile.fname, FileName1);
	sfile.lineno = lineno;
	sfile.tlineno = tlineno;
	sfile.currentatt = 0;
	sfile.currentchar = (char)getc_cr(sfile.fpin, &sfile.currentatt);
	do {
		fpin = ffile.fpin;
		currentchar = ffile.currentchar;
		currentatt = ffile.currentatt;
		lineno  = ffile.lineno;
		tlineno = ffile.tlineno;
		cc = currentchar;
		do {
			if ((ffile.res = getwholeutter())) {
				ffile.fpin = fpin;
				ffile.currentchar = currentchar;
				ffile.currentatt = currentatt;
				ffile.lineno = lineno;
				ffile.tlineno = tlineno;
				ffile.tiers = AddTiers(ffile.tiers, utterance->speaker, utterance->attSp, uttline, utterance->attLine, lineno);
			}
		} while (ffile.res && currentchar != '*' && (currentchar != '@' || cc == '@')) ;
		fpin = sfile.fpin;
		currentchar = sfile.currentchar;
		currentatt = sfile.currentatt;
		lineno  = sfile.lineno;
		tlineno = sfile.tlineno;
		cc = currentchar;
		do {
			if ((sfile.res = getwholeutter())) {
				sfile.fpin = fpin;
				sfile.currentchar = currentchar;
				sfile.currentatt = currentatt;
				sfile.lineno = lineno;
				sfile.tlineno = tlineno;
				sfile.tiers = AddTiers(sfile.tiers, utterance->speaker, utterance->attSp, uttline, utterance->attLine, lineno);
			}
		} while (sfile.res && currentchar != '*' && (currentchar != '@' || cc == '@')) ;
		fpin = ffile.fpin;
		if (ffile.res && !sfile.res) {
			fprintf(stderr, "File \"%s\" ended before file \"%s\" did.\n",  sfile.fname, ffile.fname);
			ffile.tiers = FreeUpTiers(ffile.tiers);
			sfile.tiers = FreeUpTiers(sfile.tiers);
			fclose(sfile.fpin);
			cutt_exit(0);
		}
		if (!ffile.res && sfile.res) {
			fprintf(stderr, "File \"%s\" ended before file \"%s\" did.\n", ffile.fname, sfile.fname);
			ffile.tiers = FreeUpTiers(ffile.tiers);
			sfile.tiers = FreeUpTiers(sfile.tiers);
			fclose(sfile.fpin);
			cutt_exit(0);
		}
		if (!ffile.res && !sfile.res) {
			transfer();
			ffile.tiers = FreeUpTiers(ffile.tiers);
			sfile.tiers = FreeUpTiers(sfile.tiers);
			break;
		}
		ft = ffile.tiers;
		st = sfile.tiers;
		while (ft != NULL && st != NULL) {
			uS.remblanks(ft->speaker);
			uS.remblanks(st->speaker);
			if ((ffile.tiers->speaker[0] != '@' || sfile.tiers->speaker[0] != '@') && strcmp(ffile.tiers->speaker, sfile.tiers->speaker)) {
				fprintf(stderr, "** Tier names do not match:\n");
				fprintf(stderr, "    File \"%s\": line %ld\n", ffile.fname, ft->lineno);
				if (ffile.tiers != NULL)
					fprintf(stderr, "        %s\n", ft->speaker);
				fprintf(stderr, "    File \"%s\": line %ld\n", sfile.fname, st->lineno);
				if (sfile.tiers != NULL)
					fprintf(stderr, "        %s\n", st->speaker);
				fclose(sfile.fpin);
				cutt_exit(0);
			}
			ft = ft->nexttier;
			st = st->nexttier;
			if (TRUE) {
				while (ft != NULL && ft->speaker[0] == '%')
					ft = ft->nexttier;
				while (st != NULL && st->speaker[0] == '%') 
					st = st->nexttier;
			}
		}
		if (ft != st && FALSE) {
			if (ffile.tiers->speaker[0] == '@') {
				fprintf(stderr, "** Inconsistent number of header tiers found (perhaps some hidden tiers mismatched)\n");
				fprintf(stderr, "   Starting at header tiers:\n");
			} else
				fprintf(stderr, "** Inconsistent number of dependent tiers found at speaker tier:\n");
			if (ffile.tiers != NULL) {
				fprintf(stderr, "    File \"%s\": line %ld\n", ffile.fname, ffile.tiers->lineno);
				fprintf(stderr, "        %s\n", ffile.tiers->speaker);
			} else
				fprintf(stderr, "    File \"%s\": line %ld\n", ffile.fname, ffile.lineno);
			if (sfile.tiers != NULL) {
				fprintf(stderr, "    File \"%s\": line %ld\n", sfile.fname, sfile.tiers->lineno);
				fprintf(stderr, "        %s\n", sfile.tiers->speaker);
			} else
				fprintf(stderr, "    File \"%s\": line %ld\n", sfile.fname, sfile.lineno);
			fclose(sfile.fpin);
			cutt_exit(0);
		}
		if (ffile.currentchar == '*' || ffile.currentchar == '@' || ffile.currentchar == EOF) {
			transfer();
			ffile.tiers = FreeUpTiers(ffile.tiers);
			sfile.tiers = FreeUpTiers(sfile.tiers);
		}
		if (!ffile.res && !sfile.res) {
			break;
		}
	} while (1) ;
	if (fpout != NULL) {
		fclose(fpout);
		fpout = NULL;
	}
	fclose(sfile.fpin);
	fpin = ffile.fpin;
}
