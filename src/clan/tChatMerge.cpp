/**********************************************************************
 "Copyright 1990-2014 Brian MacWhinney. Use is subject to Gnu Public License
 as stated in the attached "gpl.txt" file."
 */

// merges CHAT files into one and update bullets inforation for each
// added file based on info gotten from corresponding media files
#define CHAT_MODE 1
#include "cu.h"
#include "ced.h"
#include <sys/stat.h>
#include <dirent.h>
#include "MMedia.h"
#include "mp3.h"

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
extern char isRecursive;

struct dir_FileList {
	FNType fname[FNSize];
	struct dir_FileList *next_file;
} ;
typedef struct dir_FileList dir_FileList;

struct dir_opts {
	FNType *path;
	FNType *targ;
	dir_FileList *root_file;
} ;
typedef struct dir_opts dir_opts;

#define MUTT struct merge_utterance
MUTT {
    char *speaker;
    char *oline;
    char *sline;
    MUTT *nextutt;
} ;

static long bOff, bOffNext;
static MUTT *root_utt;
static AttTYPE atts[UTTLINELEN+1];

void usage() {
	printf("Usage: temp [%s] filename(s)\n",mainflgs());
	mainusage(TRUE);
}

void init(char f) {
	if (f) {
		root_utt = NULL;
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
	}
}

static void free_FileList(dir_FileList *p) {
	dir_FileList *t;

	while (p != NULL) {
		t = p;
		p = p->next_file;
		free(t);
	}
}

static char dir_addToFileList(dir_opts *args, FNType *fname) {
	dir_FileList *tF, *t, *tt;

	tF = NEW(dir_FileList);
	if (tF == NULL) {
		fprintf(stderr, "Out of memory");
		return(FALSE);
	}
	if (args->root_file == NULL) {
		args->root_file = tF;
		tF->next_file = NULL;
	} else if (strcmp(args->root_file->fname, fname) > 0) {
		tF->next_file = args->root_file;
		args->root_file = tF;
	} else {
		t = args->root_file;
		tt = args->root_file->next_file;
		while (tt != NULL) {
			if (strcmp(tt->fname, fname) > 0) break;
			t = tt;
			tt = tt->next_file;
		}
		if (tt == NULL) {
			t->next_file = tF;
			tF->next_file = NULL;
		} else {
			tF->next_file = tt;
			t->next_file = tF;
		}
    }
	strcpy(tF->fname, fname);
	return(TRUE);
}

static MUTT *freeUtts(MUTT *p) {
	MUTT *t;
	
	while (p != NULL) {
		t = p;
		p = p->nextutt;
		if (t->speaker != NULL)
			free(t->speaker);
		if (t->oline != NULL)
			free(t->oline);
		if (t->sline != NULL)
			free(t->sline);
		free(t);
	}
	return(NULL);
}

static void remSpaces(char *s) {
	while (*s != EOS) {
		if (*s == '\t' || *s == ' ' || *s == '\n')
			strcpy(s, s+1);
		else
			s++;
	}
}

static char OpenSoundFile(FNType *rFileName, sndInfo *tSound) {
	int len;
	
	len = strlen(rFileName);
	uS.str2FNType(rFileName, strlen(rFileName), ".aiff");
	if ((tSound->SoundFPtr=fopen(rFileName, "rb")) == NULL) {
		rFileName[len] = EOS;
		uS.str2FNType(rFileName, strlen(rFileName), ".aif");
		if ((tSound->SoundFPtr=fopen(rFileName, "rb")) == NULL) {
			rFileName[len] = EOS;
			uS.str2FNType(rFileName, strlen(rFileName), ".wav");
			if ((tSound->SoundFPtr=fopen(rFileName, "rb")) == NULL) {
				rFileName[len] = EOS;
				uS.str2FNType(rFileName, strlen(rFileName), ".mp3");
				if (GetMovieMedia(rFileName,&tSound->mp3.theSoundMedia,&tSound->mp3.hSys7SoundData) != noErr) {
					return(FALSE);
				} else {
					tSound->isMP3 = TRUE;
				}
			}
		}
	}
	strcpy(tSound->rSoundFile,rFileName);
	return(TRUE);
}

static char getBulletInfo(char *line, long *Beg, long *End) {
	int i;
	long beg = 0L, end = 0L;
	char s[512+2];
	
	if (*line == HIDEN_C)
		line++;
	
	while (*line && (isSpace(*line) || *line == '_'))
		line++;
	if (*line == EOS)
		return(FALSE);
	
	while (*line && !isdigit(*line) && *line != HIDEN_C)
		line++;
	if (!isdigit(*line))
		return(FALSE);
	for (i=0; *line && isdigit(*line) && i < 512; line++)
		s[i++] = *line;
	s[i] = EOS;
	beg = atol(s);
	while (*line && !isdigit(*line) && *line != HIDEN_C)
		line++;
	if (!isdigit(*line))
		return(FALSE);
	for (i=0; *line && isdigit(*line) && i < 512; line++)
		s[i++] = *line;
	s[i] = EOS;
	end = atol(s);
	*Beg = beg;
	*End = end;
	return(TRUE);
}

static void offsetBullets(char *word, AttTYPE *atts) {
	long i, len, lenNew;
	long cur_SNDBeg = 0L;
	long cur_SNDEnd = 0L;
	AttTYPE att;

	if (word[0] == HIDEN_C && isdigit(word[1])) {
		if (getBulletInfo(word, &cur_SNDBeg, &cur_SNDEnd)) {
			len = strlen(word);
			cur_SNDBeg = cur_SNDBeg + bOff;
			if (cur_SNDBeg < 0L)
				cur_SNDBeg = 0L;
			cur_SNDEnd = cur_SNDEnd + bOff;
			if (cur_SNDEnd < 0L)
				cur_SNDEnd = 0L;
			sprintf(word, "%c%ld_%ld%c", HIDEN_C, cur_SNDBeg, cur_SNDEnd, HIDEN_C);
			lenNew = strlen(word);
			if (len < lenNew) {
				att = atts[len-1];
				for (i=len; i < lenNew; i++)
					atts[i] = att;
			}
		}
	}
}

static char process(FNType *fname, FNType *path, int fNum) {
	int  i, j, k;
	char isMediaHeaderFound, isOpened, isMediaFileFound;
	FNType *s;
	FNType mediaFname[FILENAME_MAX];
	sndInfo SnTr;
	extern int  getc_cr_lc;
	extern char fgets_cr_lc;
	extern char Tspchanged;
	extern char contSpeaker[];
	extern char isLanguageExplicit;
	extern char cutt_isMultiFound;
	extern char cutt_isCAFound;
	extern char cutt_isBlobFound;
	extern char cutt_depWord;
	extern char *TSoldsp;

	cMediaFileName[0] = EOS;
	oldfname = fname;
	if (utterance != NULL) {
		UTTER *ttt = utterance;
		
		do {
			ttt->speaker[0]	= EOS;
			ttt->attSp[0]	= EOS;
			ttt->line[0]	= EOS;
			ttt->attLine[0]	= EOS;
			ttt = ttt->nextutt;
		} while (ttt != utterance) ;
	}
	getc_cr_lc = '\0';
	fgets_cr_lc = '\0';
	contSpeaker[0] = EOS;
	Tspchanged = FALSE;

	if (isLanguageExplicit)
		InitLanguagesTable();

	cutt_isMultiFound = FALSE;
	cutt_isCAFound = FALSE, 
	cutt_isBlobFound = FALSE;
	cutt_depWord = FALSE;
	init('\0');
	if (TSoldsp != NULL)
		*TSoldsp = EOS;
	if (!combinput)
		WUCounter = 0L;
	else if (CLAN_PROG_NUM == QUOTES || CLAN_PROG_NUM == VOCD || CLAN_PROG_NUM == EVAL || CLAN_PROG_NUM == MORTABLE ||
			 CLAN_PROG_NUM == COMBO || CLAN_PROG_NUM == SCRIPT_P || CLAN_PROG_NUM == KIDEVAL || CLAN_PROG_NUM == TIMEDUR ||
			 (CLAN_PROG_NUM == MLT && onlydata == 1) || (CLAN_PROG_NUM == MLU && onlydata == 1))
		WUCounter = 0L;
	if (lineno > -1L) {
		tlineno = 1L;
		lineno = deflineno;
	}
	fpin = fopen(fname, "r");
	if (fpin == NULL) {
		fprintf(stderr,"Can't open file %s.\n", fname);
		if (fpout != NULL)
			fclose(fpout);
		return(FALSE);
	}
	mediaFname[0] = EOS;
	FileName1[0] = EOS;
	isMediaFileFound = TRUE;
	isMediaHeaderFound = FALSE;
	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	while (getwholeutter()) {
		if (utterance->speaker[0] == '@') {
			if (uS.partcmp(utterance->speaker,"@Media:",FALSE,FALSE)) {
				isMediaHeaderFound = TRUE;
				if (fNum == 1) {
					strcpy(mediaFname, newfname);
					strcat(mediaFname, ", audio");
					printout("@Media:",mediaFname,NULL,NULL,FALSE);
				}
				strcpy(FileName1, path);
				s = strrchr(FileName1, '/');
				*s = EOS;
				s = strrchr(FileName1, '/');
				strcpy(templineC1, s+1);
				*s = EOS;
				s = strrchr(FileName1, '/');
				*s = EOS;
				strcpy(templineC, s+1);
				s = strrchr(templineC, '-');
				*s = EOS;
				strcat(templineC, "-media");
				addFilename2Path(FileName1, templineC);
				addFilename2Path(FileName1, templineC1);
				strcpy(mediaFname, utterance->line);
				s = strchr(mediaFname, ',');
				if (s != NULL)
					*s = EOS;
				s = strrchr(mediaFname, '.');
				if (s != NULL)
					*s = EOS;
				uS.remblanks(mediaFname);
				addFilename2Path(FileName1, mediaFname);
				SnTr.SSC_TOP = 0;
				SnTr.SSC_MID = 0;
				SnTr.SSC_BOT = 0;
				SnTr.SDrawCur = 1;
				SnTr.isMP3 = FALSE;
				SnTr.mp3.theSoundMedia = NULL;
				SnTr.mp3.hSys7SoundData = NULL;
				SnTr.DDsound = TRUE;
				SnTr.SNDchan = 1;
				SnTr.SNDsample = 1;
				SnTr.BegF = 0L;
				SnTr.EndF = 0L;
				SnTr.WBegF = 0L;
				SnTr.WEndF = 0L;
				SnTr.contPlayBeg = 0L;
				SnTr.contPlayEnd = 0L;
				isOpened = OpenSoundFile(FileName1, &SnTr);
				if (isOpened) {
					isMediaFileFound = TRUE;
					getFileType(FileName1, &SnTr.SFileType);
					if (SnTr.SFileType == 'MP3!' && SnTr.isMP3 != TRUE) {
						if (SnTr.SoundFPtr != NULL)
							fclose(SnTr.SoundFPtr);
						SnTr.SoundFPtr = 0;
						SnTr.isMP3 = TRUE;
						if (GetMovieMedia(FileName1,&SnTr.mp3.theSoundMedia,&SnTr.mp3.hSys7SoundData) != noErr) {
							isMediaFileFound = FALSE;
						}
					}
				} else {
					SnTr.SoundFile[0] = EOS;
					SnTr.SoundFPtr = 0;
					isMediaFileFound = FALSE;
					SnTr.isMP3 = FALSE;
				}
				if (SnTr.SoundFPtr == NULL && SnTr.isMP3 != TRUE) {
					SnTr.SoundFile[0] = EOS;
					SnTr.SoundFPtr = 0;
					isMediaFileFound = FALSE;
				}
				if (!isMediaFileFound)
					bOffNext = 0L;
				else if (!CheckRateChan(&SnTr, templineC1)) {
					fprintf(stderr,"\n%s.\n", templineC1);
					return(FALSE);
				} else
					bOffNext = conv_to_msec_rep_pure(&SnTr, SnTr.SoundFileSize);
				if (SnTr.isMP3 == TRUE) {
					if (SnTr.mp3.hSys7SoundData)
						DisposeHandle(SnTr.mp3.hSys7SoundData);
					SnTr.mp3.theSoundMedia = NULL;
					SnTr.mp3.hSys7SoundData = NULL;
					SnTr.isMP3 = FALSE;
				} else {
					if (SnTr.SoundFPtr != NULL)
						fclose(SnTr.SoundFPtr);
				}
				SnTr.SoundFile[0] = EOS;
				SnTr.SoundFPtr = 0;
			} else if (fNum == 1) {
				if (!uS.partcmp(utterance->speaker,"@End",FALSE,FALSE)) {
					printout(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine,FALSE);
				}
//				if (!add2Utts(utterance->speaker,utterance->line))
//					return(FALSE);
			} else {
				strcpy(uttline,utterance->line);
				remSpaces(utterance->speaker);
				remSpaces(uttline);
				if (uS.partcmp(utterance->speaker,"@Situation:",FALSE,FALSE))
					printout(utterance->speaker,utterance->line,utterance->attSp,utterance->attLine,FALSE);
			}
		} else {
			if (bOff > 0) {
				j = 0;
				uttline[j] = EOS;
				for (i=0L; utterance->line[i]; i++) {
					if (utterance->line[i] == HIDEN_C && (utterance->line[i+1] == '%' || isdigit(utterance->line[i+1]))) {
						k = 0L;
						do {
							spareTier2[k] = utterance->line[i];
							atts[k] = utterance->attLine[i];
							k++;
							i++;
						} while (utterance->line[i] != HIDEN_C && utterance->line[i] != EOS) ;
						spareTier2[k] = utterance->line[i];
						atts[k] = utterance->attLine[i];
						spareTier2[k+1] = EOS;
						offsetBullets(spareTier2, atts);
						att_cp(j, uttline, spareTier2, utterance->attLine, atts);
						j = strlen(uttline);
					} else
						uttline[j++] = utterance->line[i];
				}
				uttline[j] = EOS;
			} else
				strcpy(uttline, utterance->line);
			if (mediaFname[0] != EOS) {
				printout("@G:",mediaFname,NULL,NULL,FALSE);
				mediaFname[0] = EOS;
			}
			printout(utterance->speaker,uttline,utterance->attSp,utterance->attLine,FALSE);
		}
	}
	if (fpin != NULL) {
		fclose(fpin);
		fpin = NULL;
	}
	bOff = bOff + bOffNext;
	if (!isMediaHeaderFound) {
		fprintf(stderr,"Can't find @Media in file: %s.\n", fname);
	} else if (!isMediaFileFound) {
		fprintf(stderr, "Can't open sound file: %s\n", FileName1);
	}
	return(TRUE);
}

static char rec_dir(dir_opts *args, int level) {
	int i, dPos, len;
	FNType fname[FILENAME_MAX], *s;
	dir_FileList *tFile;
	struct dirent *dp;
	struct stat sb;
	DIR *cDIR;

	if (level > 2)
		return(TRUE);
	dPos = strlen(args->path);
 	if (SetNewVol(args->path)) {
		fprintf(stderr,"\nCan't change to folder: %s.\n", args->path);
		return(FALSE);
	}
	if (level > 1) {
		free_FileList(args->root_file);
		args->root_file = NULL;
		i = 1;
		while ((i=Get_File(fname, i)) != 0) {
			if (uS.fIpatmat(fname, args->targ)) {
				addFilename2Path(args->path, fname);
				if (!dir_addToFileList(args, args->path)) {
					return(FALSE);
				}
				args->path[dPos] = EOS;
			}
		}
		bOff = 0L;
		bOffNext = 0L;
		fpout = NULL;
		fprintf(stderr, "Folder: %s\n", args->path);
		i = 0;
		for (tFile=args->root_file; tFile != NULL; tFile=tFile->next_file) {
			i++;
			if (i == 1) {
				strcpy(fname, args->path);
				len = strlen(fname) - 1;
				if (fname[len] == '/')
					fname[len] = EOS;
				strcat(fname, ".cha");
				fpout = fopen(fname, "w");
				if (fpout == NULL) {
					fprintf(stderr,"Can't write file %s.\n", fname);
					return(FALSE);
				}
				extractFileName(newfname, fname);
				s = strrchr(newfname, '.');
				if (s != NULL)
					*s = EOS;
			}
			if (!process(tFile->fname, args->path, i)) {
				root_utt = freeUtts(root_utt);
				return(FALSE);
			}
			if (isKillProgram)
				break;
		}
		if (fpout != NULL) {
			printout("@End","",NULL,NULL,FALSE);
			fclose(fpout);
			fpout = NULL;
		}
		root_utt = freeUtts(root_utt);
	}
 	SetNewVol(args->path);
	if ((cDIR=opendir(".")) != NULL) {
		while ((dp=readdir(cDIR)) != NULL) {
			if (stat(dp->d_name, &sb) == 0) {
				if (!S_ISDIR(sb.st_mode)) {
					continue;
				}
			} else
				continue;
			if (dp->d_name[0] == '.')
				continue;
			addFilename2Path(args->path, dp->d_name);
			uS.str2FNType(args->path, strlen(args->path), PATHDELIMSTR);
			if (!rec_dir(args, level+1)) {
				args->path[dPos] = EOS;
				closedir(cDIR);
				return(FALSE);
			}
			args->path[dPos] = EOS;
			SetNewVol(args->path);
		}
		closedir(cDIR);
	}
	return(TRUE);
}

CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	FNType path[FNSize];
	dir_opts args;

	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = TEMP;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
	strcpy(path, wd_dir);
	isRecursive = TRUE;
	mmaininit();
	InitOptions();
	init('\001');
	SetNewVol(wd_dir);
	args.root_file = NULL;
	args.path = path;
	args.targ = "*.cha";
	rec_dir(&args,1);
	free_FileList(args.root_file);
	main_cleanup();
}
		
void getflag(char *f, char *f1, int *i) {
}

void call(void) {
}
