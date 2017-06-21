#include "cu.h"
#include "ced.h"
#include "AIFF.h"
#include "mp3.h"
#include "c_clan.h"
#include "mac_MUPControl.h"
#include "MMedia.h"
//#include <FixMath.h>
#include <fp.h>
#include <ctype.h>

#define SCALE_TIME_LIMIT 100000
#define SCROLLPERCENT 10

#if (__MACH__)
	extern "C"
	{

		EXTERN_API_C( double ) x80tod(const extended80 * x80);

	}
#endif

char leftChan = 1;
char rightChan = 1;
/* sound variables begin */

	#define DOUBLEBUFFERSIZE	16384L // 10240L // 8192L
	#define MACBUFSIZE			32768L // 15360L // 10240L // MUST be multiple of 256-2
	static Size kDoubleBufferSize = DOUBLEBUFFERSIZE;
	static Size MacBufSize =		MACBUFSIZE;
	struct LocalVars { /* variables used by doubleback proc */
	    char ISB1;
	    long bytesTotal;      /* total number of samples */
	    long bytesCopied;     /* number of samples copied to buffers */
	    Ptr  dataPtr;         /* pointer to sample to copy */
	};

	typedef struct LocalVars LocalVars;
	typedef LocalVars *LocalVarsPtr;

	static Boolean gBufferDone;
	static Ptr SB1, SB2;
	static char PF, RF, DF;
	static long	bytesCount;
	static long CurF, CurF2, CurFP;

/* sound variables end */
static	char	NewMediaFileTypes;

long conv_to_msec_MP3_pure(sndInfo *SnTr, long num) {
	double res, num2;

	res = (double)num;
	num2 = SnTr->mp3.ts;
	res = res / num2;
	res = res * 1000.0000;
	return(roundUp(res));
}

long conv_from_msec_rep_pure(sndInfo *SnTr, long num) {
	double res, num2;

	res = (double)num;
	num2 = SnTr->SNDrate / (double)1000.0000;
	res = res * num2;
	num2 = (double)SnTr->SNDsample;
	res = res * num2;
	num2 = (double)SnTr->SNDchan;
	res = res * num2;
	return(roundUp(res));
}

long conv_to_msec_rep_pure(sndInfo *SnTr, long num) {
	double res, num2;

	res = (double)num;
	num2 = SnTr->SNDrate / (double)1000.0000;
	res = res / num2;
	num2 = (double)SnTr->SNDsample;
	res = res / num2;
	num2 = (double)SnTr->SNDchan;
	res = res / num2;
	return(roundUp(res));
}

long conv_to_msec_MP3(long num) {
	double res, num2;
	
	res = (double)num;
	num2 = global_df->SnTr.mp3.ts;
	res = res / num2;
	res = res * 1000.0000;
	return(roundUp(res));
}

long conv_from_msec_MP3(long num) {
	double res, num2;
	
	res = (double)num;
	res = res / 1000.0000;
	num2 = global_df->SnTr.mp3.ts;
	res = res * num2;
	return(roundUp(res));
}

long conv_to_msec_rep(long num) {
	double res, num2;
	
	/*
	 if (global_df->SnTr.isMP3 == TRUE) {
	 res = (double)num;
	 num2 = global_df->SnTr.mp3.ts;
	 res /= num2;
	 res *= 1000.0000;
	 } else {
	 */
	res = (double)num;
	num2 = global_df->SnTr.SNDrate / (double)1000.0000;
	res = res / num2;
	num2 = (double)global_df->SnTr.SNDsample;
	res = res / num2;
	num2 = (double)global_df->SnTr.SNDchan;
	res = res / num2;
	
	//	}
	return(roundUp(res));
}

long conv_from_msec_rep(long num) {
	double res, num2;
	
	/*
	 if (global_df->SnTr.isMP3 == TRUE) {
	 res = (double)num;
	 num2 = global_df->SnTr.mp3.ts;
	 res /= 1000.0000;
	 res *= num2;
	 } else {
	 */
	res = (double)num;
	num2 = global_df->SnTr.SNDrate / (double)1000.0000;
	res = res * num2;
	num2 = (double)global_df->SnTr.SNDsample;
	res = res * num2;
	num2 = (double)global_df->SnTr.SNDchan;
	res = res * num2;
	
	//	}
	return(roundUp(res));
}

#define WAVE_FORMAT_PCM	  1
#define WAVE_FORMAT_ALAW  6
#define WAVE_FORMAT_MULAW 7

#define Samples_ID 1000
#define Channels_ID 1002
#define Sample_Rate_ID 1001

static void flipLong(long *num) {
	char *s, t0, t1, t2, t3;

	s = (char *)num;
	t0 = s[0];
	t1 = s[1];
	t2 = s[2];
	t3 = s[3];
	s[3] = t0;
	s[2] = t1;
	s[1] = t2;
	s[0] = t3;
}

static void flipShort(short *num) {
	char *s, t0, t1;

	s = (char *)num;
	t0 = s[0];
	t1 = s[1];
	s[1] = t0;
	s[0] = t1;
}
/* 2009-10-13
static double myatof(char *st) {
	int i, dot;
	long divider;
	double num;

	for (i=0; st[i] != '.' && st[i] != EOS; i++) ;
	if (st[i] == EOS) num = (double)atol(st);
	else {
		dot = i;
		for (divider=1, i++; st[i] != EOS; i++) divider *= 10;
		for (i--; i > dot && st[i] == '0'; i--) divider /= 10;
		st[i+1] = EOS;
		strcpy(st+dot, st+dot+1);
		num = (double)atol(st);
		num /= divider;
	}
	return(num);
}
*/
#pragma options align=mac68k 
struct myShortRep {
	char b16:1,b15:1,b14:1,b13:1,b12:1,b11:1,b10:1,b9:1,b8:1,b7:1,b6:1,b5:1,b4:1,b3:1,b2:1,b1:1;
} ;
struct myX80Rep {
	unsigned short exp;
	char b16:1,b15:1,b14:1,b13:1,b12:1,b11:1,b10:1,b9:1,b8:1,b7:1,b6:1,b5:1,b4:1,b3:1,b2:1,b1:1;
	char c16:1,c15:1,c14:1,c13:1,c12:1,c11:1,c10:1,c9:1,c8:1,c7:1,c6:1,c5:1,c4:1,c3:1,c2:1,c1:1;
	char d16:1,d15:1,d14:1,d13:1,d12:1,d11:1,d10:1,d9:1,d8:1,d7:1,d6:1,d5:1,d4:1,d3:1,d2:1,d1:1;
	char e16:1,e15:1,e14:1,e13:1,e12:1,e11:1,e10:1,e9:1,e8:1,e7:1,e6:1,e5:1,e4:1,e3:1,e2:1,e1:1;
} ;
struct myDoubleRep {
	char d16:1,d15:1,d14:1,d13:1,d12:1,d11:1,d10:1,d9:1,d8:1,d7:1,d6:1,d5:1,d4:1,d3:1,d2:1,d1:1;
	char c16:1,c15:1,c14:1,c13:1,c12:1,c11:1,c10:1,c9:1,c8:1,c7:1,c6:1,c5:1,c4:1,c3:1,c2:1,c1:1;
	char b16:1,b15:1,b14:1,b13:1,b12:1,b11:1,b10:1,b9:1,b8:1,b7:1,b6:1,b5:1,b4:1,b3:1,b2:1,b1:1;
	unsigned short exp;
} ;
static double x80toDouble(struct myX80Rep *x80) {
	double num;
	unsigned short exp;
	struct myDoubleRep *myDouble;
	struct myShortRep  *myShort;
	
	exp = x80->exp - 16383;
	exp = exp + 1023;
	exp = exp << 4;
	myShort = (struct myShortRep *)&exp;
	myShort->b1 = 0;
//	myShort->b12 = x80->b1;
	myShort->b13 = x80->b2;
	myShort->b14 = x80->b3;
	myShort->b15 = x80->b4;
	myShort->b16 = x80->b5;
	
	myDouble = (struct myDoubleRep *)&num;
	myDouble->exp = exp;
	myDouble->b1  = x80->b6;
	myDouble->b2  = x80->b7;
	myDouble->b3  = x80->b8;
	myDouble->b4  = x80->b9;
	myDouble->b5  = x80->b10;
	myDouble->b6  = x80->b11;
	myDouble->b7  = x80->b12;
	myDouble->b8  = x80->b13;
	myDouble->b9  = x80->b14;
	myDouble->b10 = x80->b15;
	myDouble->b11 = x80->b16;
	myDouble->b12 = x80->c1;
	myDouble->b13 = x80->c2;
	myDouble->b14 = x80->c3;
	myDouble->b15 = x80->c4;
	myDouble->b16 = x80->c5;
	myDouble->c1  = x80->c6;
	myDouble->c2  = x80->c7;
	myDouble->c3  = x80->c8;
	myDouble->c4  = x80->c9;
	myDouble->c5  = x80->c10;
	myDouble->c6  = x80->c11;
	myDouble->c7  = x80->c12;
	myDouble->c8  = x80->c13;
	myDouble->c9  = x80->c14;
	myDouble->c10 = x80->c15;
	myDouble->c11 = x80->c16;
	myDouble->c12 = x80->d1;
	myDouble->c13 = x80->d2;
	myDouble->c14 = x80->d3;
	myDouble->c15 = x80->d4;
	myDouble->c16 = x80->d5;
	myDouble->d1  = x80->d6;
	myDouble->d2  = x80->d7;
	myDouble->d3  = x80->d8;
	myDouble->d4  = x80->d9;
	myDouble->d5  = x80->d10;
	myDouble->d6  = x80->d11;
	myDouble->d7  = x80->d12;
	myDouble->d8  = x80->d13;
	myDouble->d9  = x80->d14;
	myDouble->d10 = x80->d15;
	myDouble->d11 = x80->d16;
	myDouble->d12 = x80->e1;
	myDouble->d13 = x80->e2;
	myDouble->d14 = x80->e3;
	myDouble->d15 = x80->e4;
	myDouble->d16 = x80->e5;
	return(num);
}

static void Doubletox80(double *num, struct myX80Rep *x80) {
	unsigned short exp;
	struct myDoubleRep *myDouble;
	struct myShortRep  *myShort;
	
	myDouble = (struct myDoubleRep *)num;
	exp = myDouble->exp;
	x80->b6  = myDouble->b1;
	x80->b7  = myDouble->b2;
	x80->b8  = myDouble->b3;
	x80->b9  = myDouble->b4;
	x80->b10 = myDouble->b5;
	x80->b11 = myDouble->b6;
	x80->b12 = myDouble->b7;
	x80->b13 = myDouble->b8;
	x80->b14 = myDouble->b9;
	x80->b15 = myDouble->b10;
	x80->b16 = myDouble->b11;
	x80->c1  = myDouble->b12;
	x80->c2  = myDouble->b13;
	x80->c3  = myDouble->b14;
	x80->c4  = myDouble->b15;
	x80->c5  = myDouble->b16;
	x80->c6  = myDouble->c1;
	x80->c7  = myDouble->c2;
	x80->c8  = myDouble->c3;
	x80->c9  = myDouble->c4;
	x80->c10 = myDouble->c5;
	x80->c11 = myDouble->c6;
	x80->c12 = myDouble->c7;
	x80->c13 = myDouble->c8;
	x80->c14 = myDouble->c9;
	x80->c15 = myDouble->c10;
	x80->c16 = myDouble->c11;
	x80->d1  = myDouble->c12;
	x80->d2  = myDouble->c13;
	x80->d3  = myDouble->c14;
	x80->d4  = myDouble->c15;
	x80->d5  = myDouble->c16;
	x80->d6  = myDouble->d1;
	x80->d7  = myDouble->d2;
	x80->d8  = myDouble->d3;
	x80->d9  = myDouble->d4;
	x80->d10 = myDouble->d5;
	x80->d11 = myDouble->d6;
	x80->d12 = myDouble->d7;
	x80->d13 = myDouble->d8;
	x80->d14 = myDouble->d9;
	x80->d15 = myDouble->d10;
	x80->d16 = myDouble->d11;
	x80->e1  = myDouble->d12;
	x80->e2  = myDouble->d13;
	x80->e3  = myDouble->d14;
	x80->e4  = myDouble->d15;
	x80->e5  = myDouble->d16;

	myShort = (struct myShortRep *)&exp;
	myShort->b1 = 0;
	x80->b1 = 1; // myShort->b12;
	x80->b2 = myShort->b13;
	x80->b3 = myShort->b14;
	x80->b4 = myShort->b15;
	x80->b5 = myShort->b16;
	exp = exp >> 4;
	exp = exp - 1023;
	x80->exp = exp + 16383;
}

static struct shortCommonChunk {
	short			numChannels;
	unsigned long	numSampleFrames;
	short			sampleSize;
	extended80		sampleRate;
} sCommonChunk;

static struct mContainerChunk {
	unsigned long	ckID;
	long			ckSize;
	unsigned long	formType;
} formAIFF;

static struct mCommonChunk {
	unsigned long	ckID;
	long			ckSize;
	short			numChannels;
	unsigned long	numSampleFrames;
	short			sampleSize;
	extended80		sampleRate;
} commonChunk;

static struct mSoundDataChunk {
	unsigned long	ckID;
	long			ckSize;
	unsigned long	offset;
	unsigned long	blockSize;
} soundDataChunk;

typedef struct {
	short wFormatTag;
	short nChannels;
	long  nSamplesPerSec;
	long  nAvgBytesPerSec;
	short nBlockAlign;
	short wBitsPerSample;
	char  cbSize1, cbSize2;
} WAVEFORMATEX;
#pragma options align=reset 

char CheckRateChan(sndInfo *snd, char *errMess) {
	long count;
#ifdef __cplusplus
	long double tLongDouble = 0.0L;
#endif

	snd->errMess = NULL;
	snd->DDsound = TRUE;
	snd->AIFFOffset = 0L;
	snd->SNDformat = 0L;
	if (snd->SFileType == 'MP3!') {
		CmpSoundHeader sndH;
		UnsignedFixed isSignBitFound;

		if (MyGetSoundDescriptionExtension(snd->mp3.theSoundMedia,NULL,&sndH) == noErr) {
			snd->SNDchan = sndH.numChannels;
			snd->SNDsample = sndH.sampleSize / 8;

			isSignBitFound = (sndH.sampleRate & 0x80000000);
			if (isSignBitFound)
				sndH.sampleRate = sndH.sampleRate & 0x7fffffff;
			snd->SNDrate = (double) Fix2X(sndH.sampleRate);
			if (isSignBitFound)
				snd->SNDrate += 32768L;

			snd->isMP3 = TRUE;
			snd->mp3.ts = (double)GetMediaTimeScale (snd->mp3.theSoundMedia);
			if (snd->SNDrate < 8000.00000 || snd->SNDrate > 49000.000000) {
				if (sndH.sampleRate == rate48khz) {
					snd->SNDrate = 48000.00000;
//					snd->mp3.ts = snd->mp3.ts + 4;
				} else if (sndH.sampleRate == rate44khz) {
					snd->SNDrate = 44100.00000;
//					snd->mp3.ts = snd->mp3.ts + 5;
				} else if (sndH.sampleRate == rate32khz) {
					snd->SNDrate = 32000.00000;
//					snd->mp3.ts = snd->mp3.ts + 5;
				} else if (sndH.sampleRate == rate22050hz) {
					snd->SNDrate = 22050.00000;
//					snd->mp3.ts = snd->mp3.ts + 3;
				} else if (sndH.sampleRate == rate22khz)
					snd->SNDrate = 22254.54545;
				else if (sndH.sampleRate == rate16khz) {
					snd->SNDrate = 16000.00000;
//					snd->mp3.ts = snd->mp3.ts + 3;
				} else if (sndH.sampleRate == rate11khz) {
					snd->SNDrate = 11127.27273;
//					snd->mp3.ts = snd->mp3.ts + 3;
				} else if (sndH.sampleRate == rate11025hz) {
					snd->SNDrate = 11025.00000;
//					snd->mp3.ts = snd->mp3.ts + 3;
				} else if (sndH.sampleRate == rate8khz)
					snd->SNDrate = 8000.00000;
				else {
					sprintf(errMess, "+In sonic only 8, 11, 16, 22, 32, 44 and 48kHz rates supported.");
					snd->errMess  = errMess+1;
					return(0);
				}
			} /* else {
				if (snd->SNDrate == 48000.00000) {
					snd->mp3.ts = snd->mp3.ts + 4;
				} else if (snd->SNDrate == 44100.00000) {
					snd->mp3.ts = snd->mp3.ts + 5;
				} else if (snd->SNDrate == 32000.00000) {
					snd->mp3.ts = snd->mp3.ts + 5;
				} else if (snd->SNDrate == 22050.00000) {
					snd->mp3.ts = snd->mp3.ts + 3;
				} else if (snd->SNDrate == 16000.00000) {
					snd->mp3.ts = snd->mp3.ts + 3;
				} else if (snd->SNDrate == 11127.27273) {
					snd->mp3.ts = snd->mp3.ts + 3;
				} else if (snd->SNDrate == 11025.00000) {
					snd->mp3.ts = snd->mp3.ts + 3;
				}
			}*/
			snd->SoundFileSize = conv_from_msec_rep_pure(snd, conv_to_msec_MP3_pure(snd, GetMediaDuration(snd->mp3.theSoundMedia)));
		} else {
			sprintf(errMess, "+Error reading sound file: %s. It is either corrupt or mislabeled with incorrect file extension", snd->rSoundFile);
			snd->errMess  = errMess+1;
			return(0);
		}
	} else if (snd->SFileType == 'AIFF' || snd->SFileType == 'AIFC') {
		ContainerChunk hHead;
		ChunkHeader  cHead;
		creator_type ct;
		char isFlipNums;
		long pos;

		isFlipNums = FALSE;
		snd->SoundFileSize = 0L;
		if (fseek(snd->SoundFPtr, 0, SEEK_SET) != 0) {
			sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
			snd->errMess  = errMess+1;
			return(0);
		}
		count = sizeof(hHead);
		if (fread((char *)&hHead, 1, count, snd->SoundFPtr) != count) {
			sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
			snd->errMess  = errMess+1;
			return(0);
		}
		if (hHead.ckID != FORMID) {
			flipLong((long*)&hHead.ckID);
			if (hHead.ckID != FORMID) {
				sprintf(errMess, "+Unknown sound file format: %s", snd->rSoundFile);
				snd->errMess  = errMess+1;
				return(0);
			}
			isFlipNums = TRUE;
		}
		if (isFlipNums) {
			flipLong(&hHead.ckSize);
			flipLong((long*)&hHead.formType);
		}
		if (hHead.formType != AIFFID) {
			ct.out = hHead.formType;
			sprintf(errMess, "+Sound file type '%c%c%c%c' is not currently supported",ct.in[0], ct.in[1], ct.in[2], ct.in[3]);
			snd->errMess  = errMess+1;
			return(0);
		}
		snd->SFileType = AIFFID;
		hHead.ckSize += 8;
		pos = count;
		while (pos < hHead.ckSize) {
			if (fseek(snd->SoundFPtr, pos, SEEK_SET) != 0) {
				sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
				snd->errMess  = errMess+1;
				return(0);
			}
			count = sizeof(cHead);
			if (fread((char *)&cHead, 1, count, snd->SoundFPtr) != count) {
				sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
				snd->errMess  = errMess+1;
				return(0);
			}
			if (isFlipNums) {
				flipLong((long*)&cHead.ckID);
				flipLong(&cHead.ckSize);
			}
			if (cHead.ckID == CommonID) {
				count = sizeof(sCommonChunk);
				if (fread((char *)&sCommonChunk, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				if (isFlipNums) {
					flipShort(&sCommonChunk.numChannels);
					flipLong((long*)&sCommonChunk.numSampleFrames);
					flipShort(&sCommonChunk.sampleSize);
					flipShort(&sCommonChunk.sampleRate.exp);
					flipShort((short*)&sCommonChunk.sampleRate.man[0]);
					flipShort((short*)&sCommonChunk.sampleRate.man[1]);
					flipShort((short*)&sCommonChunk.sampleRate.man[2]);
					flipShort((short*)&sCommonChunk.sampleRate.man[3]);
				}
				snd->SNDsample = sCommonChunk.sampleSize;
				if (snd->SNDsample == 8) snd->SNDsample = 1;
				else if (snd->SNDsample == 16) snd->SNDsample = 2;
				else snd->SNDsample = 0;
				snd->SNDchan = sCommonChunk.numChannels;
//				x80told(&sCommonChunk.sampleRate, &tLongDouble); // old, not used in Carbon
				if (byteOrder == CFByteOrderLittleEndian)
					tLongDouble = x80toDouble((struct myX80Rep *)&sCommonChunk.sampleRate);					
				else
					tLongDouble = x80tod(&sCommonChunk.sampleRate);
				snd->SNDrate = (double)tLongDouble;
//				snd->SNDrate = (double)(*(long double *)&sCommonChunk.sampleRate);
			} else if (cHead.ckID == SoundDataID) {
				count = 4;
				if (fread((char *)&snd->AIFFOffset, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				if (isFlipNums)
					flipLong((long*)&snd->AIFFOffset);
				if (snd->AIFFOffset != 0L) {
					sprintf(errMess, "+Not supported type of AIFF: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				count = 4;
				if (fread((char *)&snd->AIFFOffset, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				snd->AIFFOffset = pos + 16;
				snd->SoundFileSize = cHead.ckSize - sizeof(cHead);
			}
			pos += cHead.ckSize + sizeof(cHead);
			if (cHead.ckSize % 2L != 0L)
				pos++;
		}
	} else if (snd->SFileType == 'WAVE') {
		int   errCnt;
		long  tLong;
		WAVEFORMATEX pFormat;

		errCnt = 0;
		if (fseek(snd->SoundFPtr, 0, SEEK_SET) != 0) {
			sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
			snd->errMess  = errMess+1;
			return(0);
		}
		while (1) {
			count = 4;
			if (fread((char *)&tLong, 1, count, snd->SoundFPtr) != count) {
				sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
				snd->errMess  = errMess+1;
				return(0);
			}
			if (byteOrder == CFByteOrderLittleEndian)
				flipLong(&tLong);
			if (tLong == 'fmt ') {
				long  lcount;

				errCnt = 0;
				count = 4;
				if (fread((char *)&tLong, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				if (byteOrder == CFByteOrderBigEndian)
					flipLong(&tLong);
				lcount = tLong;
				count = sizeof(WAVEFORMATEX);
				lcount -= count;
				if (fread((char *)&pFormat, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				if (byteOrder == CFByteOrderBigEndian) {
					flipShort(&pFormat.wFormatTag);
					flipShort(&pFormat.nChannels);
					flipLong(&pFormat.nSamplesPerSec);
					flipShort(&pFormat.wBitsPerSample);
				}
				if (pFormat.wFormatTag == WAVE_FORMAT_PCM) {
					snd->SNDformat = 0L;
					if (isalpha(pFormat.cbSize1) && isalpha(pFormat.cbSize2)) {
						fseek(snd->SoundFPtr, -2L, SEEK_CUR);
						lcount += 2;
					}
				} else if (pFormat.wFormatTag == WAVE_FORMAT_ALAW) {
					snd->SNDformat = kALawCompression;
					count = 2;
					lcount -= count;
					if (fread((char *)&tLong, 1, count, snd->SoundFPtr) != count) {
						sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
						snd->errMess  = errMess+1;
						return(0);
					}
				} else if (pFormat.wFormatTag == WAVE_FORMAT_MULAW) {
					snd->SNDformat = kULawCompression;
					count = 2;
					lcount -= count;
					if (fread((char *)&tLong, 1, count, snd->SoundFPtr) != count) {
						sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
						snd->errMess  = errMess+1;
						return(0);
					}
				} else {
					sprintf(errMess, "+Unsupported WAV files format; file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				snd->SNDchan = pFormat.nChannels;
				snd->SNDrate = (double)pFormat.nSamplesPerSec;
				snd->SNDsample = pFormat.wBitsPerSample / 8;
				if ((snd->SNDsample == 1 || snd->SNDsample == 2) && 
								snd->SNDformat == 0L) {
					snd->DDsound = FALSE;
				}
				if (lcount > 0) {
					fseek(snd->SoundFPtr, lcount, SEEK_CUR);
				}
			} else if (tLong == 'data') {
				errCnt = 0;
				count = 4;
				if (fread((char *)&tLong, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				if (byteOrder == CFByteOrderBigEndian)
					flipLong(&tLong);
				snd->SoundFileSize = tLong;
				if ((tLong=ftell(snd->SoundFPtr)) <= 0L) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				snd->AIFFOffset = tLong;
				break;
			} else if (tLong == 'AIFF') {
				sprintf(errMess, "+This looks like an AIFF sound file: %s. Please change it's extension to \".aif\"", snd->rSoundFile);
				snd->errMess  = errMess+1;
				return(0);
			} else if (tLong == 'RIFF') {
				errCnt = 0;
				count = 4;
				if (fread((char *)&tLong, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
			} else if (tLong == 'LIST' || tLong == 'cue ' || tLong == 'PAD ') {
				errCnt = 0;
				count = 4;
				if (fread((char *)&tLong, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				if (byteOrder == CFByteOrderBigEndian)
					flipLong(&tLong);
				if (tLong > 0) {
					fseek(snd->SoundFPtr, tLong, SEEK_CUR);
				}
/*
			} else if (tLong == 'fact') {
				count = 4;
				if (fread((char *)&tLong, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				count = 4;
				if (fread((char *)&tLong, 1, count, snd->SoundFPtr) != count) {
					sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
*/
			} else if (tLong == 'WAVE') {
			} else {
				errCnt++;
				if (errCnt > 5000) {
					sprintf(errMess, "+Error reading sound file: %s. It is either corrupt or mislabeled with incorrect file extension", snd->rSoundFile);
					snd->errMess  = errMess+1;
					return(0);
				}
				if (fseek(snd->SoundFPtr, -3L, SEEK_CUR) != 0) {
					if (errno == EOVERFLOW || errno == EINVAL)
						break;
					else {
						sprintf(errMess, "+Error reading sound file: %s", snd->rSoundFile);
						snd->errMess  = errMess+1;
						return(0);
					}
				}
			}
		}
	} else {
		sprintf(errMess, "+Unsupported audio file format; file: %s", snd->rSoundFile);
		snd->errMess  = errMess+1;
		return(0);
	}

	if (snd->SNDsample != 1 && snd->SNDsample != 2) {
		snd->errMess = "When displaying a wave only 8 or 16 bit sample size supported.";
		sprintf(errMess, "+Error, only 8 or 16 bit sample size supported. This sound file has %d bits.", snd->SNDsample*8);
		return(0);
	}
	if (snd->SNDchan != 1 && snd->SNDchan != 2) {
		snd->errMess = "When displaying a wave only monaural and stereo sounds supported.";
	}
	if ((snd->SNDrate >= 48000.0 && snd->SNDrate < 49000.0) ||
		(snd->SNDrate >= 44000.0 && snd->SNDrate < 45000.0) ||
		(snd->SNDrate >= 32000.0 && snd->SNDrate < 33000.0) ||
		(snd->SNDrate >= 24000.0 && snd->SNDrate < 25000.0) ||
		(snd->SNDrate >= 22000.0 && snd->SNDrate < 23000.0) ||
		(snd->SNDrate >= 16000.0 && snd->SNDrate < 17000.0) ||
		(snd->SNDrate >= 11000.0 && snd->SNDrate < 12000.0) ||
		(snd->SNDrate >=  8000.0 && snd->SNDrate <  9000.0))
		;
	else {
		snd->errMess = "In sonic only 8, 11, 16, 22, 24, 32, 44 and 48kHz rates supported.";
	}
	if (snd->IsSoundOn && snd->errMess != NULL) {
		cChan = -1;
		soundwindow(0);
		snd->IsSoundOn = FALSE;
	} else if (snd->IsSoundOn && cChan != -1 && cChan != snd->SNDchan && !doMixedSTWave) {
		cChan = -1;
		soundwindow(0);
		snd->IsSoundOn = (char)(soundwindow(1) == 0);
		if (snd->IsSoundOn)
			cChan = snd->SNDchan;
	}
	return(1);
}

void getFileType(FNType *fn, OSType *type) {
	FNType *s;
	FNType fileName[FNSize];
	FSRef  ref;
	creator_type the_file_type;
	FSCatalogInfo catalogInfo;

	*type = 0L;
	my_FSPathMakeRef(fn, &ref); 
	if (FSGetCatalogInfo(&ref, kFSCatInfoFinderInfo, &catalogInfo, NULL, NULL, NULL) == noErr) {
		the_file_type.in[0] = catalogInfo.finderInfo[0];
		the_file_type.in[1] = catalogInfo.finderInfo[1];
		the_file_type.in[2] = catalogInfo.finderInfo[2];
		the_file_type.in[3] = catalogInfo.finderInfo[3];
	} else
		return;

	if (the_file_type.out == 'mp3!' || // mp3
		the_file_type.out == 'MP3!' || // mp3
		the_file_type.out == 'MPG3' || // mp3
		the_file_type.out == 'Mp3 ' || // mp3
		the_file_type.out == 'MP3 ') {   // mp3
		*type = 'MP3!';
	} else if (the_file_type.out == '.WAV') // WAVE
		*type = 'WAVE';
	else {
		strcpy(fileName, fn);
		uS.lowercasestr(fileName, &dFnt, FALSE);
		s = strrchr(fileName, '.');

		if (s != NULL) {
			if (!uS.FNTypeicmp(s, ".aiff", 0L) || !uS.FNTypeicmp(s, ".aif", 0L))
				*type = 'AIFF';
			else if (!uS.FNTypeicmp(s, ".wav", 0L)  || !uS.FNTypeicmp(s, ".wave", 0L))
				*type = 'WAVE';
			else if (!uS.FNTypeicmp(s, ".mp3", 0L)) {
				*type = 'MP3!';
			} else if (!uS.FNTypeicmp(s, ".mov", 0L))
				*type = 'MooV';
			else if (!uS.FNTypeicmp(s, ".m4v", 0L))
				*type = 'MooV';
			else if (!uS.FNTypeicmp(s, ".mp4", 0L))
				*type = 'mpg4'; // 'MooV';
			else if (!uS.FNTypeicmp(s, ".flv", 0L))
				*type = 'flv '; // 'MooV';
			else if (!uS.FNTypeicmp(s, ".dv", 0L))
				*type = 'dvc!';
			else if (!uS.FNTypeicmp(s, ".dat", 0L)) // vcd file
				*type = 'MPG ';
			else if (!uS.FNTypeicmp(s, ".txt", 0L)  || !uS.FNTypeicmp(s, ".cut", 0L) ||
					 !uS.FNTypeicmp(s, ".cha", 0L)  || !uS.FNTypeicmp(s, ".ca", 0L))
				*type = 'TEXT';
			else if (!uS.FNTypeicmp(s, ".mpeg", 0L)  || !uS.FNTypeicmp(s, ".mpg", 0L))
				*type = 'MPG ';
			else if (!uS.FNTypeicmp(s, ".avi", 0L))
				*type = 'AVI ';
			else if (!uS.FNTypeicmp(s, ".pict", 0L))
				*type = 'PICT';
			else if (!uS.FNTypeicmp(s, ".jpg", 0L)  || !uS.FNTypeicmp(s, ".jpeg", 0L))
				*type = 'JPEG';
			else if (!uS.FNTypeicmp(s, ".gif", 0L))
				*type = 'GIFf';
			else
				*type = the_file_type.out;
		} else
			*type = the_file_type.out;
	}
}

void CantFindMedia(char *err_message, unCH *fname) {
	FNType *c;
	FNType fileName[FNSize];
	FNType tFileName[FNSize];

	u_strcpy(tFileName, fname, FNSize);
	if (isRefEQZero(global_df->fileName)) {
		sprintf(global_df->err_message, "+If this is a newfile, then please save it to the disk before you try again. Locate the media file \"%s\" and move it to the folder which has this transcript file or to the \"media\" folder below it.", tFileName);
		return;
	}

	strcpy(fileName, global_df->fileName);
	c = strrchr(fileName, PATHDELIMCHR);
	if (c == NULL) {
		sprintf(global_df->err_message, "+Please locate the media file \"%s\" and move it into the folder which has your transcript or the \"media\" folder below the transcript and try again.", tFileName);
		return;
	}
	*(c+1) = EOS;

	sprintf(global_df->err_message, "+Please locate the media file \"%s\" and move it to the \"%s\" folder or the \"media\" folder inside of it and try again.", tFileName, fileName);
	if (strlen(global_df->err_message) >= 300)
		sprintf(global_df->err_message, "+Please locate the media file \"%s\" and move it into the folder which has your transcript or the \"media\" folder below the transcript and try again.", tFileName);
}

static char isRightMediaFolder(FNType *sfFile, char *err_message) {
	FNType fileName1[FNSize], fileName2[FNSize];

	if (isRefEQZero(global_df->fileName))
		return(TRUE);

	extractPath(fileName2, sfFile);	

	extractPath(fileName1, global_df->fileName);
	if (pathcmp(fileName1, fileName2) == 0)
		return(TRUE);

	addFilename2Path(fileName1, "media");
	if (pathcmp(fileName1, fileName2) == 0)
		return(TRUE);


	if (!isRefEQZero(wd_dir)) {
		strcpy(fileName1, wd_dir);
		if (pathcmp(fileName1, fileName2) == 0)
			return(TRUE);
	}

	if (!isRefEQZero(lib_dir)) {
		strcpy(fileName1, lib_dir);
		if (pathcmp(fileName1, fileName2) == 0)
			return(TRUE);
	}

	strcpy(fileName2, sfFile);
	extractPath(fileName1, global_df->fileName);
	sprintf(err_message, "+Please locate the media file \"%s\" and move it to the \"%s\" folder or the \"media\" folder inside of it and try again.", fileName2, fileName1);
	if (strlen(err_message) >= 300)
		sprintf(err_message, "+Please locate the media file \"%s\" and move it into the folder which has your transcript or the \"media\" folder below the transcript and try again.", fileName2);
	return(FALSE);
}

// select media dialog box BEGIN
struct f_list {
	FNType *fname;
	struct f_list *nextFile;
} ;

static struct FDS {
	FNType dirPathName[FNSize];
	f_list *files;
	int  numfiles;
	ControlRef ctrlH;
	ControlRef theDirList;
} fd;

static struct mediaUserWindowData {
	WindowPtr window;
	char res;
} userData;

static DataBrowserItemID DItemSelected;

static void get_a_file(int fnum, FNType *s, struct f_list *fl) {	
	struct f_list *t;
	
	*s = EOS;
	if (fnum < 1 || fl == NULL)
		return;
	
	for (t=fl; t != NULL; fnum--, t=t->nextFile) {
		if (fnum == 1) {
			strncpy(s, t->fname, FNSize-1);
			s[FNSize-1] = EOS;
			break;
		}
	}	
}

static struct f_list *add_a_file(FNType *s, struct f_list *fl, int *fn) {
	struct f_list *tw, *t;
	
	tw = NEW(struct f_list);
	if (tw == NULL)
		ProgExit("Out of memory");
	if (fl == NULL) {
		fl = tw;
	} else {
		for (t=fl; t->nextFile != NULL; t=t->nextFile) ;
		t->nextFile = tw;
	}
	tw->nextFile = NULL;
	tw->fname = (FNType *)malloc((strlen(s)+1)*sizeof(FNType));
	if (tw->fname == NULL)
		ProgExit("Out of memory");
	strcpy(tw->fname, s);
	(*fn)++;
	return(fl);
}

static pascal OSStatus DoMediaEvents(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData) {
	char		mDirPathName[FNSize];
	HICommand	aCommand;
	ControlRef	itemCtrl;

	switch (GetEventClass(inEvent)) {
		case kEventClassCommand:
			GetEventParameter(inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &aCommand);
			switch (aCommand.commandID) {
				case kHICommandOK:
					if (DItemSelected != 0) {
						get_a_file(DItemSelected, mDirPathName, fd.files);
						if (mDirPathName[0] != EOS) {
							strcpy(FileName1, mDirPathName);
							userData.res = TRUE;
							QuitAppModalLoopForWindow(userData.window);
						}
					}
					break;
				case kHICommandCancel:
					userData.res = FALSE;
					QuitAppModalLoopForWindow(userData.window);
					break;
				default:
					break;
			}
			break;
		case kEventClassControl:
			switch (GetEventKind(inEvent)) {
				case kEventControlClick:
					ControlID outID;
					GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &itemCtrl);
					if (GetControlID(itemCtrl, &outID) != noErr)
						return eventNotHandledErr;
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
	FNType sTemp[FNSize+1];
	CFStringRef text;
	
	switch ( property ) {
		case 'DIRL':
			if ( changeValue ) { /* this would involve renaming the file. we don't want to do that here. */
				err = errDataBrowserPropertyNotSupported;
			} else {
				get_a_file(itemID, sTemp, fd.files);
				text = my_CFStringCreateWithBytes(sTemp);
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
	
	if ( message == kDataBrowserItemDoubleClicked ) {
		get_a_file(item, mDirPathName, fd.files);
		if (mDirPathName[0] != EOS) {
			strcpy(FileName1, mDirPathName);
			userData.res = TRUE;
			QuitAppModalLoopForWindow(userData.window);
		}
	} else if ( message == kDataBrowserItemSelected ) {
		DItemSelected = item;
		get_a_file(item, mDirPathName, fd.files);
		if (mDirPathName[0] != EOS) {
			strcpy(FileName1, mDirPathName);
		}
	}
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

static char strictMediaFileMatch(FNType *sTemp) {
	char *s;
	
	s = strrchr(sTemp, '.');
	if (s != NULL) {
		if (!uS.FNTypeicmp(s, ".aiff", 0L) || !uS.FNTypeicmp(s, ".aif", 0L)  ||
			!uS.FNTypeicmp(s, ".wav", 0L)  || !uS.FNTypeicmp(s, ".wave", 0L) ||
			!uS.FNTypeicmp(s, ".mp3", 0L)  ||
			!uS.FNTypeicmp(s, ".mov", 0L)  || !uS.FNTypeicmp(s, ".mp4", 0L)  || 
			!uS.FNTypeicmp(s, ".m4v", 0L)  ||!uS.FNTypeicmp(s, ".flv", 0L)  ||
			!uS.FNTypeicmp(s, ".mpeg", 0L) || !uS.FNTypeicmp(s, ".mpg", 0L)  ||
			!uS.FNTypeicmp(s, ".avi", 0L)  || !uS.FNTypeicmp(s, ".dv", 0L)   || 
			!uS.FNTypeicmp(s, ".dat", 0L) 		// vcd file
			) {
			return(TRUE); /* show in box */
		}
	}
	return(FALSE); /* do not show */
}

static void createListOfMediaFiles(void) {
	int i, len;
	char isMediaFolderFound;
	FNType sTemp[FNSize+1];
	ItemCount	numItems;
	
	if (fd.files != NULL) { 
		fd.files = CleanupFileList(fd.files);
		fd.numfiles = 0;
	}
	if (fd.theDirList)
		RemoveDataBrowserItems(fd.theDirList, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty);
	i = 1;
	SetNewVol(fd.dirPathName);
	isMediaFolderFound = FALSE;
	while ((i=Get_Dir(sTemp, i)) != 0) {
		if (sTemp[0] != '.') {
			if (uS.mStricmp(sTemp, "media") == 0)
				isMediaFolderFound = TRUE;
		}
	}
	i = 1;
	while ((i=Get_File(sTemp, i)) != 0) {
		if (sTemp[0] != '.') {
			if (strictMediaFileMatch(sTemp)) {
				fd.files = add_a_file(sTemp, fd.files, &fd.numfiles);
			}
		}
	}
	len = strlen(fd.dirPathName);
	if (isMediaFolderFound) {
		addFilename2Path(fd.dirPathName, "media");
		strcpy(sTemp, "media/");
		SetNewVol(fd.dirPathName);
		i = 1;
		while ((i=Get_File(sTemp+6, i)) != 0) {
			if (sTemp[6] != '.') {
				if (strictMediaFileMatch(sTemp+6)) {
					fd.files = add_a_file(sTemp, fd.files, &fd.numfiles);
				}
			}
		}
		fd.dirPathName[len] = EOS;
	}
	if (fd.numfiles && fd.theDirList != NULL) {
		AddDataBrowserItems(fd.theDirList, kDataBrowserNoItem, fd.numfiles, NULL, kDataBrowserItemNoProperty);
		DItemSelected = 1;
		GetDataBrowserItemCount(fd.theDirList,kDataBrowserNoItem,false,kDataBrowserItemAnyState,&numItems);
		if (DItemSelected > numItems && numItems > 0)
			DItemSelected--;
		if (DItemSelected > 0) {
			SetDataBrowserSelectedItems(fd.theDirList,1,&DItemSelected,kDataBrowserItemsAdd);
			RevealDataBrowserItem(fd.theDirList, DItemSelected, kDataBrowserNoItem, kDataBrowserRevealOnly);
		}
	}
}

char LocateMediaFile(FNType *fname) {
	GrafPtr		oldPort;
	WindowPtr	myDlg;
	OSErr		err;
	ControlID	DcontrolID = { 'DLST', 5 };
	DataBrowserCallbacks DdataBrowserHooks;		

	GetPort(&oldPort);
	myDlg = getNibWindow(CFSTR("SelectMediaFile"));
	CenterWindow(myDlg, -1, -1);
	ShowWindow(myDlg);
	BringToFront(myDlg);
	SetPortWindowPort(myDlg);
	err = GetControlByID(myDlg, &DcontrolID, &fd.theDirList);
	if ( noErr != err )
		return(FALSE);
	DItemSelected = 0;
	fd.files = NULL;
	fd.numfiles = 0;
	strcpy(fd.dirPathName, FileName1);
	DdataBrowserHooks.version = kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&DdataBrowserHooks);
	DdataBrowserHooks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(DListCallback);
	DdataBrowserHooks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(DListNotificationCallback);
	err = SetDataBrowserCallbacks(fd.theDirList, &DdataBrowserHooks);
	AdvanceKeyboardFocus(myDlg);
	EventTypeSpec eventTypeCP[] = {
		{kEventClassCommand, kEventCommandProcess},
		{kEventClassControl, kEventControlClick}
	} ;
	InstallEventHandler(GetWindowEventTarget(myDlg), DoMediaEvents, 2, eventTypeCP, NULL, NULL);
	userData.window = myDlg;
	createListOfMediaFiles();
	DrawControls(myDlg);
	RunAppModalLoopForWindow(myDlg);
	if (fd.numfiles) { 
		fd.files = CleanupFileList(fd.files);
		fd.numfiles = 0;
	}
	DisposeWindow(myDlg);
	SetPort(oldPort);
	if (userData.res) {
		return(TRUE);
	}
	return(FALSE);
}
// select media dialog box END

static pascal Boolean MyMediaFileFilter(AEDesc *theItem,void *info,void *callBackUD,NavFilterModes filterMode) {
#pragma unused(callBackUD, filterMode)

	NavFileOrFolderInfo *FDinfo;

	if (theItem->descriptorType != typeFSRef)
		return true;

	FDinfo = (NavFileOrFolderInfo *)info;
	if (FDinfo->isFolder)
		return true;


	if (NewMediaFileTypes == isAudio) {
		if (FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'AIFF' || // AIFF
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'AIFC' || // AIFC
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'WAVE' || // WAVE
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'jB1 ' || // SoundEdit 16
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'Sd2f' || // SoundDesignerII
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'mp3!' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MP3!' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MPG3' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'Mp3 ' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MP3 ')   // mp3
			return true; /* show in box */
	} else if (NewMediaFileTypes == isVideo) {
		if (FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MooV' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'mpg4' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'flv ' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MPG ' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'AVI ')
			return true; /* show in box */
	} else if (NewMediaFileTypes == 3) {
		if (FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'AIFF' || // AIFF
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'AIFC' || // AIFC
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'WAVE' || // WAVE
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'jB1 ' || // SoundEdit 16
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'Sd2f' || // SoundDesignerII
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'mp3!' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MP3!' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MPG3' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'Mp3 ' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MP3 ' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MooV' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'mpg4' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'flv ' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MPG ' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'AVI ')
			return true; /* show in box */
	} else {
		if (FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'AIFF' || // AIFF
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'AIFC' || // AIFC
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'WAVE' || // WAVE
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'jB1 ' || // SoundEdit 16
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'Sd2f' || // SoundDesignerII
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'mp3!' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MP3!' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MPG3' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'Mp3 ' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MP3 ' || // mp3
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MooV' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'mpg4' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'flv ' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'MPG ' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'AVI ' ||
//			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'TEXT' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'PICT' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'JPEG' ||
			FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'GIFf')
			return true; /* show in box */
	}

	if (theItem->descriptorType == typeFSRef) {
		FSRef ref;
		FNType fileName[FNSize];
		FNType *s;

		AEGetDescData(theItem, &ref, sizeof(ref));
		my_FSRefMakePath(&ref, fileName, FNSize);
		uS.lowercasestr(fileName, &dFnt, FALSE);
		s = strrchr(fileName, '.');
		if (s != NULL) {
			if (NewMediaFileTypes == isAudio) {
				if (!uS.FNTypeicmp(s, ".aiff", 0L) || !uS.FNTypeicmp(s, ".aif", 0L)  ||
					!uS.FNTypeicmp(s, ".wav", 0L)  || !uS.FNTypeicmp(s, ".wave", 0L) ||
					!uS.FNTypeicmp(s, ".mp3", 0L) 		// vcd file
					) {
					return true; /* show in box */
				}
			} else if (NewMediaFileTypes == isVideo) {
				if (!uS.FNTypeicmp(s, ".mov", 0L)  || !uS.FNTypeicmp(s, ".mp4", 0L)  ||
					!uS.FNTypeicmp(s, ".m4v", 0L)  || !uS.FNTypeicmp(s, ".flv", 0L)  ||
					!uS.FNTypeicmp(s, ".mpeg", 0L) || !uS.FNTypeicmp(s, ".mpg", 0L)  ||
					!uS.FNTypeicmp(s, ".avi", 0L)  || !uS.FNTypeicmp(s, ".dv", 0L)   ||
					!uS.FNTypeicmp(s, ".dat", 0L) 		// vcd file
					) {
					return true; /* show in box */
				}
			} else if (NewMediaFileTypes == 3) {
				if (!uS.FNTypeicmp(s, ".aiff", 0L) || !uS.FNTypeicmp(s, ".aif", 0L)  ||
					!uS.FNTypeicmp(s, ".wav", 0L)  || !uS.FNTypeicmp(s, ".wave", 0L) ||
					!uS.FNTypeicmp(s, ".mp3", 0L)  ||
					!uS.FNTypeicmp(s, ".mov", 0L)  || !uS.FNTypeicmp(s, ".mp4", 0L)  ||
					!uS.FNTypeicmp(s, ".m4v", 0L)  || !uS.FNTypeicmp(s, ".flv", 0L)  ||
					!uS.FNTypeicmp(s, ".mpeg", 0L) || !uS.FNTypeicmp(s, ".mpg", 0L)  ||
					!uS.FNTypeicmp(s, ".avi", 0L)  || !uS.FNTypeicmp(s, ".dv", 0L)   || 
					!uS.FNTypeicmp(s, ".dat", 0L) 		// vcd file
					) {
					return true; /* show in box */
				}
			} else {
				if (!uS.FNTypeicmp(s, ".aiff", 0L) || !uS.FNTypeicmp(s, ".aif", 0L)  ||
					!uS.FNTypeicmp(s, ".wav", 0L)  || !uS.FNTypeicmp(s, ".wave", 0L) ||
					!uS.FNTypeicmp(s, ".mp3", 0L)  ||
					!uS.FNTypeicmp(s, ".mov", 0L)  || !uS.FNTypeicmp(s, ".mp4", 0L)  ||
					!uS.FNTypeicmp(s, ".m4v", 0L)  || !uS.FNTypeicmp(s, ".flv", 0L)  ||
					!uS.FNTypeicmp(s, ".mpeg", 0L) || !uS.FNTypeicmp(s, ".mpg", 0L)  ||
					!uS.FNTypeicmp(s, ".avi", 0L)  || !uS.FNTypeicmp(s, ".dv", 0L)   ||
					!uS.FNTypeicmp(s, ".txt", 0L)  || !uS.FNTypeicmp(s, ".cut", 0L)  ||
					!uS.FNTypeicmp(s, ".cha", 0L)  || !uS.FNTypeicmp(s, ".cdc", 0L)  ||
					!uS.FNTypeicmp(s, ".pict", 0L) ||
					!uS.FNTypeicmp(s, ".jpg", 0L)  || !uS.FNTypeicmp(s, ".jpeg", 0L) ||
					!uS.FNTypeicmp(s, ".gif", 0L)  || 
					!uS.FNTypeicmp(s, ".dat", 0L) 		// vcd file
					) {
					return true; /* show in box */
				}
			}
		}
		return false; /* do not show */
	} else
		return true;
}

char GetNewMediaFile(char isCheckError, char isAllMedia) {
	FNType		*c, retFileName[FNSize];
	OSType		resType;
	Boolean		good;
	char		*prompt;

	NewMediaFileTypes = isAllMedia;
	if (isAllMedia == 0) {
		prompt = "Please locate movie, sound, picture or text file";
	} else if (isAllMedia == isAudio) {
		prompt = "Please locate sound file ONLY";
	} else {
		prompt = "Please locate movie or sound file ONLY";
	}

	retFileName[0] = EOS;
	if (myNavGetFile(prompt, -1, nil, (NavObjectFilterProcPtr)MyMediaFileFilter, retFileName)) {
		good = true;
		getFileType(retFileName, &resType);
	} else
		good = false;
	if (good) {
		if (!isRightMediaFolder(retFileName, global_df->err_message)) {
			if (global_df->SnTr.SoundFile[0] != EOS) {
				global_df->SnTr.SoundFile[0] = EOS;
				if (global_df->SnTr.isMP3 == TRUE) {
					global_df->SnTr.isMP3 = FALSE;
					if (global_df->SnTr.mp3.hSys7SoundData)
						DisposeHandle(global_df->SnTr.mp3.hSys7SoundData);
					global_df->SnTr.mp3.theSoundMedia = NULL;
					global_df->SnTr.mp3.hSys7SoundData = NULL;
				} else {
					fclose(global_df->SnTr.SoundFPtr);
				}
				global_df->SnTr.SoundFPtr = 0;
				if (global_df->SoundWin)
					DisposeOfSoundWin();
			}
		} else if (resType == 'PICT' || resType == 'JPEG' || resType == 'GIFf') {
			strcpy(global_df->PcTr.pictFName,retFileName);
			return(isPict);
		} else if (resType == 'TEXT') {
			strcpy(global_df->TxTr.textFName,retFileName);
			return(isText);
		} else if (resType == 'MooV' || resType == 'mpg4' || resType == 'flv ' || resType == 'MPG ' || resType == 'AVI ' || resType == 'dvc!') {
			if (isCheckError && !strcmp(retFileName, global_df->MvTr.rMovieFile)) {
				strcpy(global_df->err_message, "+Movie marker is not found at cursor position. Please move text cursor next to the bullet.");
				return(0);
			}
			global_df->MvTr.MBeg = 0L;
			global_df->MvTr.MEnd = 0L;
			strcpy(global_df->MvTr.rMovieFile, retFileName);
			if ((c=strrchr(retFileName, PATHDELIMCHR)) != NULL)
				strcpy(retFileName, c+1);
			if ((c=strrchr(retFileName,'.')) != NULL)
				*c = EOS;
			u_strcpy(global_df->MvTr.MovieFile, retFileName, FILENAME_MAX);
			return(isVideo);
		} else if (resType == 'AIFF'  || resType == 'AIFC' || resType == 'WAVE' || resType == 'MP3!') {
			if (isCheckError && global_df->SnTr.SoundFile[0] != EOS && !strcmp(retFileName, global_df->SnTr.rSoundFile)) {
				strcpy(global_df->err_message, "+Sound marker is not found at cursor position. If you are planning to link sound to text then please use Sonic mode \"ESC-0\"");
				return(0);
			}
			if (global_df->SnTr.isMP3 == TRUE) {
				global_df->SnTr.isMP3 = FALSE;
				if (global_df->SnTr.mp3.hSys7SoundData)
					DisposeHandle(global_df->SnTr.mp3.hSys7SoundData);
				global_df->SnTr.mp3.theSoundMedia = NULL;
				global_df->SnTr.mp3.hSys7SoundData = NULL;
			} else if (global_df->SnTr.SoundFPtr != 0) {
				fclose(global_df->SnTr.SoundFPtr);
				global_df->SnTr.SoundFile[0] = EOS;
				global_df->SnTr.SoundFPtr = 0;
				if (global_df->SoundWin)
					DisposeOfSoundWin();
			}
			DrawSoundCursor(0);
			global_df->SnTr.BegF = 0L;
			global_df->SnTr.EndF = 0L;
			SetPBCglobal_df(false, 0L);
			global_df->SnTr.SFileType = resType;
			if (global_df->SnTr.SFileType == 'MP3!') {
				if (GetMovieMedia(retFileName,&global_df->SnTr.mp3.theSoundMedia,&global_df->SnTr.mp3.hSys7SoundData) != noErr) {
					sprintf(global_df->err_message, "+Can't open sound file: %s. Perhaps it is already opened by other application or in another window.", retFileName);
					global_df->SnTr.SoundFile[0] = EOS;
					global_df->SnTr.SoundFPtr = 0;
					if (global_df->SoundWin)
						DisposeOfSoundWin();
					return(0);
				}
				global_df->SnTr.isMP3 = TRUE;
			} else if ((global_df->SnTr.SoundFPtr=fopen(retFileName, "rb")) == NULL) {
				sprintf(global_df->err_message, "+Can't open sound file: %s. Perhaps it is already opened by other application or in another window.", retFileName);
				global_df->SnTr.SoundFile[0] = EOS;
				global_df->SnTr.SoundFPtr = 0;
				if (global_df->SoundWin)
					DisposeOfSoundWin();
				return(0);
			}
			strcpy(global_df->SnTr.rSoundFile,retFileName);
			if (!CheckRateChan(&global_df->SnTr, global_df->err_message)) {
				global_df->SnTr.SoundFile[0] = EOS;
				if (global_df->SnTr.isMP3 == TRUE) {
					global_df->SnTr.isMP3 = FALSE;
					if (global_df->SnTr.mp3.hSys7SoundData)
						DisposeHandle(global_df->SnTr.mp3.hSys7SoundData);
					global_df->SnTr.mp3.theSoundMedia = NULL;
					global_df->SnTr.mp3.hSys7SoundData = NULL;
				} else {
					fclose(global_df->SnTr.SoundFPtr);
					global_df->SnTr.SoundFPtr = 0;
					if (global_df->SoundWin)
						DisposeOfSoundWin();
				}
				return(0);
			}
			if ((c=strrchr(retFileName, PATHDELIMCHR)) != NULL)
				strcpy(retFileName, c+1);
			if ((c=strrchr(retFileName,'.')) != NULL)
				*c = EOS;
			u_strcpy(global_df->SnTr.SoundFile, retFileName, FILENAME_MAX);
			ResetUndos();
			global_df->SnTr.WBegFM = 0L;
			global_df->SnTr.WEndFM = 0L;
			return(isAudio);
		} else {
			if (global_df->SnTr.SoundFile[0] != EOS) {
				global_df->SnTr.SoundFile[0] = EOS;
				if (global_df->SnTr.isMP3 == TRUE) {
					global_df->SnTr.isMP3 = FALSE;
					if (global_df->SnTr.mp3.hSys7SoundData)
						DisposeHandle(global_df->SnTr.mp3.hSys7SoundData);
					global_df->SnTr.mp3.theSoundMedia = NULL;
					global_df->SnTr.mp3.hSys7SoundData = NULL;
				} else {
					fclose(global_df->SnTr.SoundFPtr);
				}
				global_df->SnTr.SoundFPtr = 0;
				if (global_df->SoundWin)
					DisposeOfSoundWin();
			}
			strcpy(global_df->err_message, DASHES);
		}
	} else {
		if (global_df->SnTr.SoundFile[0] != EOS) {
			global_df->SnTr.SoundFile[0] = EOS;
			if (global_df->SnTr.isMP3 == TRUE) {
				global_df->SnTr.isMP3 = FALSE;
				if (global_df->SnTr.mp3.hSys7SoundData)
					DisposeHandle(global_df->SnTr.mp3.hSys7SoundData);
				global_df->SnTr.mp3.theSoundMedia = NULL;
				global_df->SnTr.mp3.hSys7SoundData = NULL;
			} else {
				fclose(global_df->SnTr.SoundFPtr);
			}
			global_df->SnTr.SoundFPtr = 0;
			if (global_df->SoundWin)
				DisposeOfSoundWin();
		}
		strcpy(global_df->err_message, DASHES);
	}
	return(0);
}

static pascal Boolean MyPictureFileFilter (AEDesc *theItem,void *info,void *callBackUD,NavFilterModes filterMode) {
#pragma unused(theItem, callBackUD, filterMode)

	NavFileOrFolderInfo *FDinfo;

	if (theItem->descriptorType != typeFSRef)
		return true;

	FDinfo = (NavFileOrFolderInfo *)info;
	if (FDinfo->isFolder)
		return true;

	if (FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'PICT' ||
		FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'JPEG' ||
		FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'GIFf')   // SoundDesignerII
		return true; /* show in box */

	if (theItem->descriptorType == typeFSRef) {
		FSRef ref;
		FNType fileName[FNSize];
		FNType *s;

		AEGetDescData(theItem, &ref, sizeof(ref));
		my_FSRefMakePath(&ref, fileName, FNSize);
		uS.lowercasestr(fileName, &dFnt, FALSE);
		s = strrchr(fileName, '.');
		if (s != NULL) {
			if (!uS.FNTypeicmp(s, ".pict", 0L) ||
				!uS.FNTypeicmp(s, ".jpg", 0L)  || !uS.FNTypeicmp(s, ".jpeg", 0L) ||
				!uS.FNTypeicmp(s, ".gif", 0L)) {
				return true; /* show in box */
			}
		}

		return false; /* do not show */
	} else
		return true;
}

char GetNewPictFile(FNType *fname) {
	FNType		fileName[FNSize];
	char		prompt[FNSize+64];

	if (fname == NULL)
		strcpy(prompt, "Please locate picture file.");
	else {
		extractFileName(fileName, fname);
		sprintf(prompt, "Please locate picture file: %s", fileName);
	}
	fileName[0] = EOS;
	if (myNavGetFile(prompt, -1, nil, (NavObjectFilterProcPtr)MyPictureFileFilter, fileName)) {
		DrawSoundCursor(0);
		if (access(fileName, 0)) {
			sprintf(global_df->err_message, "+Can't open picture file: %s.", fileName);
			return(0);
		}
		strcpy(global_df->PcTr.pictFName,fileName);
	} else { 
		strcpy(global_df->err_message, DASHES);		
		return(0);
	}
	return(1);
}

static pascal Boolean MyTextFileFilter (AEDesc *theItem,void *info,void *callBackUD,NavFilterModes filterMode) {
#pragma unused(theItem, callBackUD, filterMode)

	NavFileOrFolderInfo *FDinfo;
	
	if (theItem->descriptorType != typeFSRef)
		return true;

	FDinfo = (NavFileOrFolderInfo *)info;
	if (FDinfo->isFolder)
		return true;

//	if (FDinfo->fileAndFolder.fileInfo.finderInfo.fdType == 'TEXT')
//		return true; /* show in box */

	if (theItem->descriptorType == typeFSRef) {
		FSRef ref;
		FNType fileName[FNSize];
		FNType *s;

		AEGetDescData(theItem, &ref, sizeof(ref));
		my_FSRefMakePath(&ref, fileName, FNSize);
		uS.lowercasestr(fileName, &dFnt, FALSE);
		s = strrchr(fileName, '.');
		if (s != NULL) {
			if (!uS.FNTypeicmp(s, ".txt", 0L) || !uS.FNTypeicmp(s, ".cut", 0L) ||
				!uS.FNTypeicmp(s, ".cha", 0L) || !uS.FNTypeicmp(s, ".cdc", 0L)) {
				return true; /* show in box */
			}
		}

		return false; /* do not show */
	} else
		return true;
}

char GetNewTextFile(FNType *fname) {
	FNType		fileName[FNSize];
	char		prompt[FNSize+64];

	if (fname == NULL)
		strcpy(prompt, "Please locate text file.");
	else {
		extractFileName(fileName, fname);
		sprintf(prompt, "Please locate text file: %s", fileName);
	}
	fileName[0] = EOS;

	if (myNavGetFile(prompt, -1, nil, (NavObjectFilterProcPtr)MyTextFileFilter, fileName)) {
		DrawSoundCursor(0);
		if (access(fileName, 0)) {
			sprintf(global_df->err_message, "+Can't open text file: %s.", fileName);
			return(0);
		}
		strcpy(global_df->TxTr.textFName,fileName);
	} else { 
		strcpy(global_df->err_message, DASHES);		
		return(0);
	}
	return(1);
}

static char ReadSound8bit(int col, int cm, Size cur) {
	int row, hp1, lp1, hp2, lp2, scale;
	short t;
	double d;
	Size i, count, scale_cnt;

	if (cm == 0) {
		GetSoundWinDim(&row, &cm);
		global_df->SnTr.WEndF = global_df->SnTr.WBegF + ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
	} else {
		GetSoundWinDim(&row, &hp1);
		global_df->SnTr.WEndF = global_df->SnTr.WBegF + ((Size)hp1 * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
	}
	if (cur == 0)
		cur = global_df->SnTr.WBegF;
	scale = ((0xFF / global_df->mainVScale) / row) + 1;
	global_df->SnTr.WBegFM = global_df->SnTr.WBegF;
	global_df->SnTr.WEndFM = global_df->SnTr.WEndF;
	scale_cnt = 0L;
	hp1 = 0; lp1 = hp1 - 1;
	hp2 = 0; lp2 = hp2 - 1;

	wdrawcontr(global_df->SoundWin, TRUE);
	while (cur < global_df->SnTr.WEndF && cur < global_df->SnTr.SoundFileSize && col < cm) {
		count = ((Size)SNDBUFSIZE < global_df->SnTr.WEndF-cur ? (Size)SNDBUFSIZE : global_df->SnTr.WEndF-cur);
		if (cur+count >= global_df->SnTr.SoundFileSize)
			count = global_df->SnTr.SoundFileSize - cur;
		if (fseek(global_df->SnTr.SoundFPtr, cur+global_df->SnTr.AIFFOffset, SEEK_SET) != 0)
			return(0);
		if (fread(global_df->SnTr.SWaveBuf, 1, count, global_df->SnTr.SoundFPtr) != count)
			return(0);
		if (!global_df->SnTr.DDsound) {
			for (i=0; i < count; i++)
				global_df->SnTr.SWaveBuf[i] -= 128;
		}
		for (i=0; i < count && col < cm; i++) {
			if (global_df->SnTr.SNDformat == 0L) 
				row = (int)(global_df->SnTr.SWaveBuf[i]) / scale;
			else if (global_df->SnTr.SNDformat == kULawCompression) {
				t = Mulaw2Lin((unsigned char)global_df->SnTr.SWaveBuf[i]);
				d = (double)t;
				d = d * 0.38909912109375 / 100.000000000000;
				t = (short)d;
				row = t / scale;
			} else if (global_df->SnTr.SNDformat == kALawCompression) {
				t = Alaw2Lin((unsigned char)global_df->SnTr.SWaveBuf[i]);
				d = (double)t;
				d = d * 0.38909912109375 / 100.000000000000;
				t = (short)d;
				row = t / scale;
			}	
			if (global_df->SnTr.SNDchan == 1 || doMixedSTWave) {
				if (row > hp1) hp1 = row;
				if (row < lp1) lp1 = row;
			} else {
				if ((i % 2) == 0) {
					if (row > hp1) hp1 = row;
					if (row < lp1) lp1 = row;
				} else {
					if (row > hp2) hp2 = row;
					if (row < lp2) lp2 = row;
				}
			}
			scale_cnt++;
			if (scale_cnt == global_df->scale_row) {
				if (global_df->TopP1 && global_df->BotP1) {
					global_df->TopP1[col] = hp1;
					global_df->BotP1[col] = lp1;
				}
				if (global_df->TopP2 && global_df->BotP2) {
					global_df->TopP2[col] = hp2;
					global_df->BotP2[col] = lp2;
				}
				wdrawdot(global_df->SoundWin, hp1, lp1, hp2, lp2, col);
				col++;
				scale_cnt = 0L;
				hp1 = 0; lp1 = hp1 - 1;
				hp2 = 0; lp2 = hp2 - 1;
			}
		}
		cur += i;
	}
	if (global_df->TopP1 && global_df->BotP1 && cm != 1)
		global_df->TopP1[col] = -1;
	return(1);
}

static char ReadSound16bit(int col, int cm, Size cur) {
	int row, hp1, lp1, hp2, lp2, scale;
	short *tbuf;
	Size  i, count, scale_cnt;

	if (cm == 0) {
		GetSoundWinDim(&row, &cm);
		global_df->SnTr.WEndF = global_df->SnTr.WBegF + ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
	} else {
		GetSoundWinDim(&row, &hp1);
		global_df->SnTr.WEndF = global_df->SnTr.WBegF + ((Size)hp1 * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
	}
	if (cur == 0)
		cur = global_df->SnTr.WBegF;
	scale = (int)((0xFFFFL / (Size)global_df->mainVScale) / (Size)row) + 1;
	global_df->SnTr.WBegFM = global_df->SnTr.WBegF;
	global_df->SnTr.WEndFM = global_df->SnTr.WEndF;
	scale_cnt = 0L;
	hp1 = 0; lp1 = hp1 - 1;
	hp2 = 0; lp2 = hp2 - 1;

	wdrawcontr(global_df->SoundWin, TRUE);
	while (cur < global_df->SnTr.WEndF && cur < global_df->SnTr.SoundFileSize && col < cm) {
		count = ((Size)SNDBUFSIZE < global_df->SnTr.WEndF-cur ? (Size)SNDBUFSIZE : global_df->SnTr.WEndF-cur);
		if (cur+count >= global_df->SnTr.SoundFileSize)
			count = global_df->SnTr.SoundFileSize - cur;
		if (fseek(global_df->SnTr.SoundFPtr, cur+global_df->SnTr.AIFFOffset, SEEK_SET) != 0)
			return(0);
		if (fread(global_df->SnTr.SWaveBuf, 1, count, global_df->SnTr.SoundFPtr) != count)
			return(0);
		tbuf = (short *)global_df->SnTr.SWaveBuf;
		count /= 2;
		if (!global_df->SnTr.DDsound && byteOrder == CFByteOrderBigEndian) {
			for (i=0; i < count; i++) {
				tbuf[i] -= 32768;
			}
		}
		if (global_df->SnTr.SFileType == 'WAVE' && byteOrder == CFByteOrderBigEndian) {
			for (i=0; i < count; i++) {
				flipShort(&tbuf[i]);
			}
		}
		if (global_df->SnTr.SFileType == 'AIFF' && byteOrder == CFByteOrderLittleEndian) {
			for (i=0; i < count; i++) {
				flipShort(&tbuf[i]);
			}
		}
		for (i=0; i < count && col < cm; i++) {
			row = (int)(tbuf[i]) / scale;
			if (global_df->SnTr.SNDchan == 1 || doMixedSTWave) {
				if (row > hp1) hp1 = row;
				if (row < lp1) lp1 = row;
			} else {
				if ((i % 2) == 0) {
					if (row > hp1) hp1 = row;
					if (row < lp1) lp1 = row;
				} else {
					if (row > hp2) hp2 = row;
					if (row < lp2) lp2 = row;
				}
			}
			scale_cnt++;
			if (scale_cnt == global_df->scale_row) {
				if (global_df->TopP1 && global_df->BotP1) {
					global_df->TopP1[col] = hp1;
					global_df->BotP1[col] = lp1;
				}
				if (global_df->TopP2 && global_df->BotP2) {
					global_df->TopP2[col] = hp2;
					global_df->BotP2[col] = lp2;
				}
				wdrawdot(global_df->SoundWin, hp1, lp1, hp2, lp2, col);
				col++;
				scale_cnt = 0L;
 				hp1 = 0; lp1 = hp1 - 1;
 				hp2 = 0; lp2 = hp2 - 1;
			}
		}
		cur += ((Size)i * 2L);
	}
	if (global_df->TopP1 && global_df->BotP1 && cm != 1)
		global_df->TopP1[col] = -1;
	return(1);
}

void PrintSoundWin(int col, int cm, Size cur) {
	char err_mess[512];
	int rm;
	short winHeight;
	short offset;

	if (global_df->SoundWin == NULL)
		return;
	if (global_df->SnTr.SoundFPtr == NULL && global_df->SnTr.rSoundFile[0] != EOS)
		global_df->SnTr.SoundFPtr = fopen(global_df->SnTr.rSoundFile, "rb");
	if (global_df->SnTr.SoundFPtr == NULL) {
		if (global_df->SoundWin)
			DisposeOfSoundWin();
		sprintf(err_mess, "Internal error occured.");
		do_warning(err_mess, -1);
		return;
	}
	offset = ComputeHeight(global_df->SoundWin,0,1);
	winHeight = ComputeHeight(global_df->SoundWin,1,global_df->SoundWin->num_rows);
	if (global_df->SnTr.SNDchan == 1 || doMixedSTWave) {
		global_df->SnTr.SNDWHalfRow = (winHeight / 2) + 1;
		global_df->SnTr.SNDWprint_row1 = global_df->SoundWin->LT_row + offset + winHeight;
	} else {
		global_df->SnTr.SNDWHalfRow = (winHeight / 4) + 1;
		global_df->SnTr.SNDWprint_row1 = global_df->SoundWin->LT_row + offset + (winHeight / 2);
		global_df->SnTr.SNDWprint_row2 = global_df->SoundWin->LT_row + offset + winHeight;
	}
	if (global_df->TopP1 && global_df->BotP1 && global_df->SnTr.WBegFM == global_df->SnTr.WBegF && global_df->SnTr.WEndFM == global_df->SnTr.WEndF) {
		if (global_df->TopP1[0] == -1) {
			if (global_df->SnTr.isMP3 == TRUE)
				ReadMP3Sound(col, cm, cur);
			else if (global_df->SnTr.SNDsample == 1)
				ReadSound8bit(col, cm, cur);
			else if (global_df->SnTr.SNDsample == 2)
				ReadSound16bit(col, cm, cur);
		} else {
			wdrawcontr(global_df->SoundWin, TRUE);
			GetSoundWinDim(&rm, &cm);
			for (rm=0; global_df->TopP1[rm] != -1 && rm < cm; rm++) {
				if (global_df->TopP2 == NULL || global_df->BotP2 == NULL)
					wdrawdot(global_df->SoundWin, global_df->TopP1[rm], global_df->BotP1[rm], 0, 0, rm);
				else
					wdrawdot(global_df->SoundWin, global_df->TopP1[rm], global_df->BotP1[rm], global_df->TopP2[rm], global_df->BotP2[rm], rm);
			}
			PutSoundStats(global_df->SnTr.WEndF - global_df->SnTr.WBegF);
		}
	} else {
		if (global_df->SnTr.isMP3 == TRUE)
			ReadMP3Sound(col, cm, cur);
		else if (global_df->SnTr.SNDsample == 1)
			ReadSound8bit(col, cm, cur);
		else if (global_df->SnTr.SNDsample == 2)
			ReadSound16bit(col, cm, cur);

		PutSoundStats(global_df->SnTr.WEndF - global_df->SnTr.WBegF);
	}
	wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
	wrefresh(global_df->w1);
}

long ComputeOffset(long prc) {
	long Offset;

	Offset = (global_df->SnTr.WEndF-global_df->SnTr.WBegF) / 100L;
	if (Offset == 0) Offset = 1;
	Offset *= prc;
	return(Offset);
}

void DisplayEndF(char all) {
	if ((global_df->SnTr.EndF < global_df->SnTr.WBegF || global_df->SnTr.EndF > global_df->SnTr.WEndF) && global_df->SnTr.EndF != 0 && global_df->SnTr.EndF != global_df->SnTr.BegF) {
		global_df->SnTr.WBegF = AlignMultibyteMediaStream(global_df->SnTr.EndF-ComputeOffset(90L),'-');
		global_df->WinChange = FALSE;
		touchwin(global_df->SoundWin); wrefresh(global_df->SoundWin);
		global_df->WinChange = TRUE;
	} else if ((global_df->SnTr.EndF == 0 || global_df->SnTr.BegF == global_df->SnTr.EndF) && (global_df->SnTr.BegF < global_df->SnTr.WBegF || global_df->SnTr.BegF > global_df->SnTr.WEndF)) {
		global_df->SnTr.WBegF = AlignMultibyteMediaStream(global_df->SnTr.BegF-ComputeOffset(15L), '-');
		global_df->WinChange = FALSE;
		touchwin(global_df->SoundWin); wrefresh(global_df->SoundWin);
		global_df->WinChange = TRUE;
	} else if (all) {
		global_df->WinChange = FALSE;
		touchwin(global_df->SoundWin); wrefresh(global_df->SoundWin);
		global_df->WinChange = TRUE;
	}
}

static void CheckNewF(long NewF, char *dir) {
	long t1, HalfF;

	if (global_df->SnTr.EndF < global_df->SnTr.BegF && global_df->SnTr.EndF != 0L) {
		t1 = global_df->SnTr.EndF; global_df->SnTr.EndF = global_df->SnTr.BegF; global_df->SnTr.BegF = t1;
	}

	if (global_df->SnTr.EndF == 0L || global_df->SnTr.BegF == global_df->SnTr.EndF) {
		*dir = 0;
		if (NewF != global_df->SnTr.BegF) DrawSoundCursor(0);
		if (global_df->SnTr.BegF > NewF) {
			global_df->SnTr.EndF = global_df->SnTr.BegF;
			global_df->SnTr.BegF = NewF;
		} else global_df->SnTr.EndF = NewF;
		DrawSoundCursor(2);
	} else if (NewF > global_df->SnTr.EndF) {
		if (*dir == -1) {
			DrawSoundCursor(2);
			t1 = global_df->SnTr.EndF;
		} else 		t1 = global_df->SnTr.BegF;
		global_df->SnTr.BegF = global_df->SnTr.EndF; global_df->SnTr.EndF = NewF;
		DrawSoundCursor(2);
		global_df->SnTr.BegF = t1;
		*dir = 1;
	} else if (NewF < global_df->SnTr.BegF) {
		if (*dir == 1) {
			DrawSoundCursor(2);
			t1 = global_df->SnTr.BegF;
		} else t1 = global_df->SnTr.EndF;
		global_df->SnTr.EndF = global_df->SnTr.BegF; global_df->SnTr.BegF = NewF; 
		DrawSoundCursor(2);
		global_df->SnTr.EndF = t1;
		*dir = -1;
	} else if (NewF > global_df->SnTr.BegF && NewF < global_df->SnTr.EndF && *dir == 1) {
		t1 = global_df->SnTr.BegF; global_df->SnTr.BegF = NewF;
		DrawSoundCursor(2);
		global_df->SnTr.BegF = t1; global_df->SnTr.EndF = NewF;
	} else if (NewF > global_df->SnTr.BegF && NewF < global_df->SnTr.EndF && *dir == -1) {
		t1 = global_df->SnTr.EndF; global_df->SnTr.EndF = NewF;
		DrawSoundCursor(2);
		global_df->SnTr.BegF = NewF; global_df->SnTr.EndF = t1;
	} else {
		HalfF = (global_df->SnTr.EndF - global_df->SnTr.BegF) / 2;
		if (NewF > global_df->SnTr.BegF && NewF < global_df->SnTr.BegF + HalfF) {
			t1 = global_df->SnTr.EndF; global_df->SnTr.EndF = NewF;
			DrawSoundCursor(2);
			global_df->SnTr.BegF = NewF; global_df->SnTr.EndF = t1;
			*dir = -1;
		} else if (NewF > global_df->SnTr.BegF+HalfF && NewF < global_df->SnTr.EndF) {
			t1 = global_df->SnTr.BegF; global_df->SnTr.BegF = NewF;
			DrawSoundCursor(2);
			global_df->SnTr.BegF = t1; global_df->SnTr.EndF = NewF;
			*dir = 1;
		}
	}
}

void delay_mach(Size num) {
	Size t;
	
	t = TickCount() + num;
	do {  
		if (TickCount() > t) 
			break;
	} while (Button() == TRUE) ;	
}

char SetWaveTimeValue(WindowProcRec *windProc, long timeBeg, long timeEnd) {
	long t;

	if (windProc != NULL && windProc->id == 500) {
		if ((theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
			global_df = theMovie->df;
			if (syncAudioAndVideo(global_df->MvTr.MovieFile)) {
				t =  AlignMultibyteMediaStream(conv_from_msec_rep(timeBeg), '+');
				if (t != global_df->SnTr.BegF)
					global_df->SnTr.BegF = t;
				if (timeEnd == 0L)
					global_df->SnTr.EndF = 0L;
				else {
					t = AlignMultibyteMediaStream(conv_from_msec_rep(timeEnd), '+');
					if (t == global_df->SnTr.BegF)
						global_df->SnTr.EndF = 0L;
					else if (t > global_df->SnTr.BegF && t != global_df->SnTr.EndF)
						global_df->SnTr.EndF = t;
				}
				DisplayEndF(FALSE);
			}
			DrawFakeHilight(1);
			global_df = NULL;
		}
	}
	return(TRUE);
}

char SetCurrSoundTier(WindowProcRec *windProc) {
	WindowPtr windPtr;

	if (windProc != NULL && windProc->id == 500) {
		windPtr = FindAWindowProc(windProc);
		if (windPtr == NULL || theMovie == NULL)
			return(FALSE);

		if ((theMovie->df=WindowFromGlobal_df(theMovie->df)) != NULL) {
			global_df = theMovie->df;
			DrawFakeHilight(0);
			GetWindowMLTEText(windPtr, 3, UTTLINELEN, templineC3);
			theMovie->MBeg = atol(templineC3);
			GetWindowMLTEText(windPtr, 4, UTTLINELEN, templineC3);
			theMovie->MEnd = atol(templineC3);
			addBulletsToText(SOUNDTIER, theMovie->fName, theMovie->MBeg, theMovie->MEnd);
			if (syncAudioAndVideo(global_df->MvTr.MovieFile)) {
				global_df->MvTr.MBeg = theMovie->MBeg;
				global_df->MvTr.MEnd = theMovie->MEnd;
				global_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(theMovie->MBeg), '+');
				global_df->SnTr.EndF = AlignMultibyteMediaStream(conv_from_msec_rep(theMovie->MEnd), '+');
				DisplayEndF(FALSE);
			}
			DrawFakeHilight(1);
			global_df = NULL;
		}
	} else if (global_df != NULL && global_df->SoundWin) {
		if (global_df->SnTr.BegF != global_df->SnTr.EndF && global_df->SnTr.EndF != 0L) {
			DrawSoundCursor(0);
			DrawCursor(0);
			addBulletsToText(SOUNDTIER, global_df->SnTr.SoundFile, conv_to_msec_rep(global_df->SnTr.BegF), conv_to_msec_rep(global_df->SnTr.EndF));
			return(TRUE);
		}
	} else {
		char ret;
		FNType mFileName[FILENAME_MAX];
		wchar_t wFileName[FILENAME_MAX];
		FNType *c;
		if ((ret=GetNewMediaFile(TRUE, 0))) {
			if (ret == isAudio) { // sound
			} else if (ret == isVideo) { // movie
			} else if (ret == isPict) { // picture
				DrawCursor(0);
				extractFileName(mFileName, global_df->PcTr.pictFName);
				if ((c=strrchr(mFileName,'.')) != NULL)
					*c = EOS;
				u_strcpy(wFileName, mFileName, FILENAME_MAX);
				addBulletsToText(PICTTIER, wFileName, 0L, 0L);
				return(TRUE);
			} else if (ret == isText) { // text
				DrawCursor(0);
				extractFileName(mFileName, global_df->TxTr.textFName);
				if ((c=strrchr(mFileName,'.')) != NULL)
					*c = EOS;
				u_strcpy(wFileName, mFileName, FILENAME_MAX);
				addBulletsToText(TEXTTIER, wFileName, 0L, 0L);
				return(TRUE);
			} 
		}
	}
//	strcpy(global_df->err_message, "+Can't find any media to insert bullets from.");
	return(FALSE);
}

char AdjustSound(int row, int col, int ext, Size right_lim) {
	int rowExtender;

	if (doMixedSTWave)
		rowExtender = 1;
	else
		rowExtender = ((global_df->SnTr.SNDchan - 1) * 5) + 1;
	if (ext == 2) {
		if (global_df->SnTr.BegF != global_df->SnTr.EndF && global_df->SnTr.EndF != 0L)
			isPlayS = -3;
		else
			isPlayS = -2;
		return(0);
	} else if (col < global_df->SnTr.SNDWccol) {
		if (row == 2+rowExtender) {
			if (SetCurrSoundTier(NULL))
				return(0);
		} else if ((row == 0+rowExtender) || (row == 4+rowExtender)) {
			Size ws;
			int  rm, cm;

			GetSoundWinDim(&rm, &cm);
			if (row == 4+rowExtender) {
				ws = ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
				if (ws < global_df->SnTr.SoundFileSize) {
					ws = global_df->scale_row;
					global_df->scale_row += global_df->scale_row;
					if (conv_to_msec_rep(((Size)cm*(Size)global_df->scale_row*(Size)global_df->SnTr.SNDsample)) > 
							SCALE_TIME_LIMIT) {
						ws = conv_from_msec_rep(SCALE_TIME_LIMIT);
						global_df->scale_row = ws / cm / (Size)global_df->SnTr.SNDsample;
					}
					ws = ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
					if (global_df->SnTr.WBegF+ws >= global_df->SnTr.SoundFileSize) {
						global_df->SnTr.WBegF = AlignMultibyteMediaStream(global_df->SnTr.SoundFileSize-ws, '+');
					}
					global_df->SnTr.WEndF = global_df->SnTr.WBegF + ws;
				}
			} else if (row == 0+rowExtender) {
				global_df->scale_row -= (global_df->scale_row / 2);
				global_df->SnTr.WEndF = global_df->SnTr.WBegF + ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
			}
			if (StillDown() == TRUE) {
				ws = ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
				PutSoundStats(ws);
			}
			delay_mach(20);
		}
	} else if (col > right_lim) {
		if ((row == 1+rowExtender) || (row == 3+rowExtender)) {
			if (row == 1+rowExtender) leftChan = ((leftChan == 1) ? 0 : 1);
			else rightChan = ((rightChan == 1) ? 0 : 1);
			wdrawcontr(global_df->SoundWin, TRUE);
		} else if ((row == 0+rowExtender) || (row == 4+rowExtender)) {
			if (row == 0+rowExtender)
				global_df->mainVScale *= 2;
			else
				global_df->mainVScale /= 2;
			if (global_df->mainVScale < 1)
				global_df->mainVScale = 1;
			global_df->SnTr.WEndFM = !global_df->SnTr.WEndF;
		}
	} else {
		long NewF;
		long MaxTick;
		int  cm, left_lim, ScollValue;
		char dir = 0;
		Point LocalPtr;

		GetSoundWinDim(&left_lim, &cm);
		left_lim = global_df->SnTr.SNDWccol;
		if (col < left_lim || col > right_lim) 
			return(0);
		NewF = global_df->SnTr.WBegF + ((col-left_lim) * global_df->scale_row * global_df->SnTr.SNDsample);
		NewF = AlignMultibyteMediaStream(NewF, '+');
		if (ext != 1) {
			DrawSoundCursor(0);
			global_df->SnTr.BegF = NewF;
			global_df->SnTr.EndF = 0L;
		} else
			CheckNewF(NewF, &dir);
		PutSoundStats(global_df->SnTr.WEndF - global_df->SnTr.WBegF);

		ScollValue = (cm * SCROLLPERCENT / 100);
		if (ScollValue < 1)
			ScollValue = 1;
		while (StillDown() == TRUE) {
			GetMouse(&LocalPtr);
			col = LocalPtr.h;
			if (col < left_lim || col > right_lim) {
				long t;
				NewF = ((Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample) * ScollValue;
				if (col < left_lim) {
					col = left_lim;
					if (NewF >= global_df->SnTr.WBegF) {
						if (global_df->SnTr.WBegF == 0L)
							continue;
						NewF = global_df->SnTr.WBegF;
						ScollValue = NewF / ((Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
						if (ScollValue < 1) ScollValue = 1;
						global_df->SnTr.WEndF = global_df->SnTr.WEndF - global_df->SnTr.WBegF;
						global_df->SnTr.WBegF = 0L;
					} else {
						global_df->SnTr.WBegF = AlignMultibyteMediaStream(global_df->SnTr.WBegF-NewF, '+');
						global_df->SnTr.WEndF = AlignMultibyteMediaStream(global_df->SnTr.WEndF-NewF, '+');
					}
					MoveSoundWave(global_df->SoundWin, ScollValue, left_lim, right_lim);
					if (global_df->SnTr.WBegF >= global_df->SnTr.BegF && global_df->SnTr.WBegF <= global_df->SnTr.EndF) {
						t = global_df->SnTr.EndF; global_df->SnTr.EndF = global_df->SnTr.WBegF + NewF;
						DrawSoundCursor(2);
						global_df->SnTr.EndF = t;
					}
					if (global_df->TopP1 && global_df->BotP1) {
						for (t=0; global_df->TopP1[t] != -1; t++) ;
						for (t--; t >= ScollValue; t--) {
							global_df->TopP1[t] = global_df->TopP1[t-ScollValue];
							global_df->BotP1[t] = global_df->BotP1[t-ScollValue];
						}
					}
					if (global_df->TopP2 && global_df->BotP2) {
						for (t=0; global_df->TopP1[t] != -1; t++) ;
						for (t--; t >= ScollValue; t--) {
							global_df->TopP2[t] = global_df->TopP2[t-ScollValue];
							global_df->BotP2[t] = global_df->BotP2[t-ScollValue];
						}
					}
					PrintSoundWin(0, ScollValue, global_df->SnTr.WBegF);
				} else if (col > right_lim) {
					col = right_lim;
					if (NewF+global_df->SnTr.WEndF < global_df->SnTr.SoundFileSize) {
						global_df->SnTr.WBegF += NewF;
						global_df->SnTr.WEndF += NewF;
					} else {
						if (global_df->SnTr.WBegF == global_df->SnTr.SoundFileSize - (global_df->SnTr.WEndF-global_df->SnTr.WBegF))
							continue;
						NewF = global_df->SnTr.SoundFileSize - global_df->SnTr.WEndF;
						ScollValue = NewF / ((Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
						if (ScollValue < 1)
							ScollValue = 1;
						global_df->SnTr.WBegF = global_df->SnTr.SoundFileSize - (global_df->SnTr.WEndF-global_df->SnTr.WBegF);
						global_df->SnTr.WEndF = global_df->SnTr.SoundFileSize;
					}
					global_df->SnTr.WBegF = AlignMultibyteMediaStream(global_df->SnTr.WBegF, '-');
					global_df->SnTr.WEndF = AlignMultibyteMediaStream(global_df->SnTr.WEndF, '-');
					MoveSoundWave(global_df->SoundWin, -ScollValue, left_lim, right_lim);
					if (global_df->SnTr.WEndF >= global_df->SnTr.BegF && global_df->SnTr.WEndF <= global_df->SnTr.EndF) {
						t = global_df->SnTr.BegF; global_df->SnTr.BegF = global_df->SnTr.WEndF - NewF;
						DrawSoundCursor(2);
						global_df->SnTr.BegF = t;
					}
					if (global_df->TopP1 && global_df->BotP1) {
						for (t=0; global_df->TopP1[t+ScollValue] != -1; t++) {
							global_df->TopP1[t] = global_df->TopP1[t+ScollValue];
							global_df->BotP1[t] = global_df->BotP1[t+ScollValue];
						}
					}
					if (global_df->TopP2 && global_df->BotP2) {
						for (t=0; global_df->TopP1[t+ScollValue] != -1; t++) {
							global_df->TopP2[t] = global_df->TopP2[t+ScollValue];
							global_df->BotP2[t] = global_df->BotP2[t+ScollValue];
						}
					}
					PrintSoundWin(cm-ScollValue-1, cm, global_df->SnTr.WEndF-NewF);
				}
				NewF = global_df->SnTr.WBegF + ((col-left_lim) * global_df->scale_row * global_df->SnTr.SNDsample);
				NewF = AlignMultibyteMediaStream(NewF, '-');
				CheckNewF(NewF, &dir);
				if (StillDown() == TRUE) {
					MaxTick = TickCount() + 6;
					do {  
						if (TickCount() > MaxTick)
							break;		 
					} while ( StillDown() == TRUE) ;
				}
			} else {
				NewF = global_df->SnTr.WBegF + ((col-left_lim) * global_df->scale_row * global_df->SnTr.SNDsample);
				NewF = AlignMultibyteMediaStream(NewF, '-');
				CheckNewF(NewF, &dir);
				PutSoundStats(global_df->SnTr.WEndF - global_df->SnTr.WBegF);
			}
		}
		if (global_df->SnTr.BegF != global_df->SnTr.EndF && global_df->SnTr.EndF != 0L) {
			SaveUndoState(FALSE);
			isPlayS = -3;
		} else if (syncAudioAndVideo(global_df->MvTr.MovieFile)) {
			setMovieCursorToValue(conv_to_msec_rep(global_df->SnTr.BegF), global_df);
		}
		SetPBCglobal_df(false, 0L);
		return(0);
	}
	SetPBCglobal_df(false, 0L);
	return(1);
}

int soundwindow(int TurnOn) {
	int cm, rm, rows;
	Size ws;
	GrafPtr oldPort;
	char fontName[50];
	FONTINFO tf;
lxs
	if (TurnOn == 0) {
		DisposeOfSoundWin();
		return(0);
	}

	if (doMixedSTWave)
		rows = 5;
	else
		rows = global_df->SnTr.SNDchan * 5;
	rows += 1;
	cm = rows - COM_WIN_SIZE;
	if (cm > 0) {
		char t;
		global_df->EdWinSize = 0;
		global_df->CodeWinStart = 0;
		global_df->total_num_rows = rows;
		t = global_df->NoCodes;
		global_df->NoCodes = FALSE;
		if (!init_windows(true, 1, false))
			mem_err(TRUE, global_df);
		global_df->NoCodes = t;
		if (COM_WIN_SIZE < rows) {
			RemoveLastUndo();
			strcpy(global_df->err_message, "+Window is not large enough");
			return(1);
		} 
	}

	if ((cm=WindowPageWidth()) < 80)
		cm = 80;
	if ((global_df->SoundWin=newwin(rows, cm, global_df->CodeWinStart, 0, 0)) == NULL) {
		strcpy(global_df->err_message, "+Can't create sound window");
		DisposeOfSoundWin();
		return(1);
	}
	
	tf.FName = DEFAULT_ID; // 4;
	tf.FSize = DEFAULT_SIZE; // 9;
	tf.FHeight = GetFontHeight(&tf, NULL, global_df->wind); // 12;
	tf.CharSet = 0;
	tf.Encod = 0;
	global_df->SoundWin->isUTF = FALSE; //  lxs2
	for (rows--; rows >= 0; rows--) {
		global_df->SoundWin->RowFInfo[rows]->FName = tf.FName;
		global_df->SoundWin->RowFInfo[rows]->FSize = tf.FSize;
		global_df->SoundWin->RowFInfo[rows]->CharSet = tf.CharSet;
		global_df->SoundWin->RowFInfo[rows]->FHeight = tf.FHeight;
	}
	
	
	strcpy(fontName, "CAfont");
	if (!GetFontNumber(fontName, &fID)) {
		do_warning("Missing \"CAfont\" font.", 0);
		PrepareWindA4(wind, &saveRec);
		mCloseWindow(wind);
		RestoreWindA4(&saveRec);
		return;
	}
	uFont.FName = fID;
	uFont.FSize = 15;
	/*
	 uFont.FName = dFnt.fontId;
	 uFont.FSize = dFnt.fontSize;
	 */
	
	uFont.CharSet = my_FontToScript(uFont.FName, 0);
	uFont.Encod = uFont.CharSet;
	FHeight = GetFontHeight(&uFont, NULL, wind);
	uFont.FHeight = FHeight;
	
	
	
	SetTextWinMenus(TRUE);
	GetPort(&oldPort);
	SetPortWindowPort(global_df->wind);
	global_df->SnTr.SNDWccol =  FontTxtWidth(global_df->SoundWin,0,cl_T("WW"),0,2)+1;
	SetPort(oldPort);
	GetSoundWinDim(&rm, &cm);
	global_df->TopP1 = (int *)malloc((cm+1) * sizeof(int));
	global_df->BotP1 = (int *)malloc((cm+1) * sizeof(int));
	if (global_df->SnTr.SNDchan == 2 && !doMixedSTWave) {
		global_df->TopP2 = (int *)malloc((cm+1) * sizeof(int));
		global_df->BotP2 = (int *)malloc((cm+1) * sizeof(int));
	} else {
		global_df->TopP2 = NULL;
		global_df->BotP2 = NULL;
	}
	global_df->SnTr.WBegFM = 0L;
	global_df->SnTr.WEndFM = 0L;
	ws = ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
	if (global_df->scale_row <= 0L || ws >= global_df->SnTr.SoundFileSize)
		global_df->scale_row = 128L * global_df->SnTr.SNDsample * global_df->SnTr.SNDchan;
	if (global_df->mainVScale < 1)
		global_df->mainVScale = 1;
	global_df->SnTr.WBegF = 0L;
	global_df->SnTr.WEndF = global_df->SnTr.WBegF + ((Size)cm * (Size)global_df->scale_row * (Size)global_df->SnTr.SNDsample);
	ResetUndos();
	wstandend(global_df->SoundWin);
	DisplayEndF(TRUE);
	DrawSoundCursor(1);
	return(0);
}

int ContPlayPause(void) {
	if (PlayingContSound == '\002') {
		global_df->WinChange = FALSE;
		strcpy(global_df->err_message, "-Pausing ...");
		draw_mid_wm();
		global_df->WinChange = TRUE;
		while (StillDown() == TRUE) ;
	} else if (PlayingContSound == '\003') {
		PlayingContSound = FALSE;
//		DrawCursor(2);
//		ResetSelectFlag(0);
		strcpy(global_df->err_message, "-Aborted.");
		return(TRUE);
	}
	return(FALSE);
}

int play_sound(long begin, long end, int cont) {
	if (!PlayBuffer(cont)) {
		PlayingSound = FALSE;
		PlayingContSound = FALSE;
		if (!strncmp(global_df->err_message, "-Press any KEY", 14) || !strncmp(global_df->err_message, "-Playing, press any", 19))
			strcpy(global_df->err_message, "-Error during playback");
		DrawCursor(0);
		global_df->SnTr.SoundFile[0] = EOS;
		if (global_df->SnTr.isMP3 == TRUE) {
			global_df->SnTr.isMP3 = FALSE;
			if (global_df->SnTr.mp3.hSys7SoundData)
				DisposeHandle(global_df->SnTr.mp3.hSys7SoundData);
			global_df->SnTr.mp3.theSoundMedia = NULL;
			global_df->SnTr.mp3.hSys7SoundData = NULL;
		} else {
			fclose(global_df->SnTr.SoundFPtr);
		}
		global_df->SnTr.SoundFPtr = 0;
		if (global_df->SoundWin)
			DisposeOfSoundWin();
		return(1);
	}
	DrawCursor(0);
	return(0);
}

/* play sound begin */
static char ReadSound(Ptr buf, long max) {
	register long i;
	register char t0, t1, t2;

	if (global_df->SnTr.EndF == 0)
		bytesCount = max;
	else
		bytesCount = MIN(max, global_df->SnTr.EndF-CurF);
	if (CurF < global_df->SnTr.SoundFileSize) {
		if (CurF+bytesCount >= global_df->SnTr.SoundFileSize)
			bytesCount = global_df->SnTr.SoundFileSize - CurF;
		if (fseek(global_df->SnTr.SoundFPtr, CurF+global_df->SnTr.AIFFOffset, SEEK_SET) != 0) {
			strcpy(global_df->err_message, "+Error reading sound file.");
			return(0);
		}
		if (fread(buf, 1, bytesCount, global_df->SnTr.SoundFPtr) != bytesCount) {
			strcpy(global_df->err_message, "+Error reading sound file.");
			return(0);
		}
		if (global_df->SnTr.DDsound && global_df->SnTr.SFileType != 'WAVE') {
			if (global_df->SnTr.SNDsample == 1) {
				for (i=0; i < bytesCount; i++)
					buf[i] += 128;
			}
		}
		if (global_df->SnTr.SFileType == 'WAVE' && byteOrder == CFByteOrderBigEndian) {
			if (global_df->SnTr.SNDsample == 2) {
				for (i=0; i < bytesCount; i+=2) {
					t0 = buf[i];
					t1 = buf[i+1];
					buf[i+1] = t0;
					buf[i] = t1;
				}
			} else if (global_df->SnTr.SNDsample == 3) {
				for (i=0; i < bytesCount; i+=3) {
					t0 = buf[i];
					t1 = buf[i+1];
					t2 = buf[i+2];
					buf[i+2] = t0;
					buf[i+1] = t1;
					buf[i] = t2;
				}
			}
		}
		if (global_df->SnTr.SFileType == 'AIFF' && byteOrder == CFByteOrderLittleEndian) {
			if (global_df->SnTr.SNDsample != 1) {
				if (global_df->SnTr.DDsound) {
					for (i=0; i < bytesCount; i+=2) {
						t0 = buf[i];
						t1 = buf[i+1];
						buf[i+1] = t0;
						buf[i] = t1;
					}
				}
			}
		}
	} else
		bytesCount = 0;
	if (bytesCount == 0)
		global_df->SnTr.EndF = CurF;
	CurF += bytesCount;
    return(1);
}

static pascal void MySoundCallBackFunction(SndChannelPtr theChannel, SndCommand *theCmd)
{
#pragma unused(theChannel)
	#ifndef TARGET_API_MAC_CARBON
		#if !GENERATINGCFM
			long oldA5;
			oldA5 = SetA5(theCmd->param2);
		#else
			#pragma unused(theCmd)
		#endif
	#else
		#pragma unused(theCmd)
	#endif // TARGET_API_MAC_CARBON

	gBufferDone = true;
	#ifndef TARGET_API_MAC_CARBON
		#if !GENERATINGCFM
			oldA5 = SetA5(oldA5);
		#endif
	#endif // TARGET_API_MAC_CARBON
}

static char PlayPCMSound(int com) {
	SndChannelPtr	chan;
	SCStatus		status;
	LocalVars		myVars;
	SndCommand		thePlayCmd0,
					thePlayCmd1,
					theCallBackCmd,
					mySndCmd;
	SndCommand	 	*pPlayCmd;
	SndCommand	 	theVolumeCmd;
	int 			key, LoopCnt;
	char			isAborted, ret;
	char			res;
    OSErr			myErr;
	CmpSoundHeader	mySndHeader0,
					mySndHeader1;
	CmpSoundHeader	*pSndHeader = NULL;
	eBufferNumber   whichBuffer;
	Boolean			isSoundDone;
	SndCallBackUPP 	theSoundCallBackUPP;
	double			timeOffset;
	unsigned long	lTime_mark, curBegTime, lastBufSize;
	unsigned long	*pBufSize, pBuf0Size, pBuf1Size;
	extern char		leftChan;
	extern char		rightChan;

	LoopCnt = 1;
repeat_playback:
	ret = 1;
	PF = 1;
	DF = 1;
	RF = 0;
	
	kDoubleBufferSize = DOUBLEBUFFERSIZE;
	MacBufSize =		MACBUFSIZE;
	if (isPBCWindowOpen())
		kDoubleBufferSize *= 4;
	kDoubleBufferSize *= global_df->SnTr.SNDchan;
	if (global_df->SnTr.SNDrate > 30000)
		kDoubleBufferSize *= 2;
	if (PlayingContSound != '\004')
		kDoubleBufferSize *= 2;

	if (MacBufSize < kDoubleBufferSize) {
		if (kDoubleBufferSize < 500000L)
			MacBufSize = kDoubleBufferSize * 2;
		else
			MacBufSize = kDoubleBufferSize + (kDoubleBufferSize / 2); // just to be safe
	}

	CurF2 = CurF = global_df->SnTr.BegF;
	checkContinousSndPlayPossition(global_df->SnTr.BegF);
	if (com == (int)'p')
		global_df->SnTr.EndF = 0;
	strcpy(global_df->err_message, DASHES);
	if (global_df->SnTr.BegF >= global_df->SnTr.SoundFileSize) {
		strcpy(global_df->err_message, "+Starting point is beyond end of file.");
		return(0);
	}

	if (!leftChan && !rightChan) {
		strcpy(global_df->err_message, "+No channel was selected. Please goto \"sonic mode\" and click on 'L' and/or 'R'.");
		return(2);
	}

	if ((SB1=NewPtr(MacBufSize)) == NULL) {
		strcpy(global_df->err_message, "+out of mem! (SB1)");
		return(0);
	}	
	if ((SB2=NewPtr(MacBufSize)) == NULL) {
		strcpy(global_df->err_message, "+out of mem! (SB2)");
		DisposePtr((Ptr)SB1);
		return(0);
    }
	if (!ReadSound(SB1, MacBufSize)) {
		DisposePtr((Ptr)SB1);
		DisposePtr((Ptr)SB2);
		return(0);
    }

	myVars.dataPtr = SB2;
	myVars.ISB1 = FALSE;

// allocate a sound channel
	unsigned short theLeftVol, theRightVol;
	theLeftVol = 256;
	theRightVol = 256;
	theVolumeCmd.cmd = volumeCmd;
	theVolumeCmd.param1 = 0;
	theVolumeCmd.param2 = (long)((theRightVol << 16) | theLeftVol);
	chan = 0L;
	theSoundCallBackUPP = NewSndCallBackUPP(MySoundCallBackFunction);
	if (global_df->SnTr.SNDchan == 1)
		myErr = SndNewChannel(&chan, sampledSynth, initMono, theSoundCallBackUPP);
	else if (!leftChan && rightChan)
		myErr = SndNewChannel(&chan, sampledSynth, initChanRight, theSoundCallBackUPP);
	else if (leftChan && !rightChan)
		myErr = SndNewChannel(&chan, sampledSynth, initChanLeft, theSoundCallBackUPP);
	else
		myErr = SndNewChannel(&chan, sampledSynth, initStereo, theSoundCallBackUPP);
	if (myErr != noErr) {
		DisposePtr((Ptr)SB1);
		DisposePtr((Ptr)SB2);
		sprintf(global_df->err_message, "+Can't allocate sound channel. (%d)", myErr);
		return(0);
	}
	SndDoCommand(chan, &theVolumeCmd, true);
	
    /* set up SndDoubleBuffer */
	mySndHeader0.samplePtr = NULL;
	mySndHeader0.sampleSize = global_df->SnTr.SNDsample * 8;       /* 8-bit samples  */
	mySndHeader0.numChannels = global_df->SnTr.SNDchan;
	if ((global_df->SnTr.SNDrate >= 48000.00 && global_df->SnTr.SNDrate < 49000.00))
		mySndHeader0.sampleRate = rate48khz;
	else if ((global_df->SnTr.SNDrate >= 44000.00 && global_df->SnTr.SNDrate < 45000.00))
		mySndHeader0.sampleRate = rate44khz;
	else if ((global_df->SnTr.SNDrate >= 32000.00 && global_df->SnTr.SNDrate < 33000.00))
		mySndHeader0.sampleRate = rate32khz;
	else if ((global_df->SnTr.SNDrate >= 22000.00 && global_df->SnTr.SNDrate < 22100.00))
		mySndHeader0.sampleRate = rate22050hz;
	else if ((global_df->SnTr.SNDrate >= 22100.00 && global_df->SnTr.SNDrate < 23000.00))
		mySndHeader0.sampleRate = rate22khz;
	else if ((global_df->SnTr.SNDrate >= 16000.00 && global_df->SnTr.SNDrate < 17000.00))
		mySndHeader0.sampleRate = rate16khz;
	else if ((global_df->SnTr.SNDrate >= 11100.00 && global_df->SnTr.SNDrate < 12000.00))
		mySndHeader0.sampleRate = rate11khz;
	else if ((global_df->SnTr.SNDrate >= 11000.00 && global_df->SnTr.SNDrate < 11100.00))
		mySndHeader0.sampleRate = rate11025hz;
	else if ((global_df->SnTr.SNDrate >= 8000.00 && global_df->SnTr.SNDrate < 9000.00))
		mySndHeader0.sampleRate = rate8khz;
	else {
		sprintf(global_df->err_message, "+Sample rate of %.4lf is not supported (only 8, 11, 16, 22, 32, 44 and 48kHz).", global_df->SnTr.SNDrate);
		if (chan != 0L)
			myErr = SndDisposeChannel(chan, FALSE);
		return(0);
	}
	if (global_df->SnTr.SNDformat == 0L) {
		mySndHeader0.format = kSoundNotCompressed;
		mySndHeader0.compressionID = 0;	
	} else {
		mySndHeader0.format = global_df->SnTr.SNDformat;
		mySndHeader0.compressionID = fixedCompression;	
	}
	mySndHeader0.loopStart = 0;
	mySndHeader0.loopEnd = 0;
	mySndHeader0.encode = cmpSH;			// compressed sound header encode value
	mySndHeader0.baseFrequency = kMiddleC;
	// mySndHeader0.AIFFSampleRate;			// this is not used
	mySndHeader0.markerChunk = NULL;
	mySndHeader0.futureUse2 = 0;
	mySndHeader0.stateVars = NULL;
	mySndHeader0.leftOverSamples = NULL;
	mySndHeader0.packetSize = 0;			// the Sound Manager will figure this out for us
	mySndHeader0.snthID = 0;
	mySndHeader0.sampleArea[0] = 0;			// no samples here because we use samplePtr to point to our buffer instead
	if (PBC.speed != 100) {
		double t1;
 		t1 = (double)mySndHeader0.sampleRate;
 		t1 *= (double)PBC.speed;
 		t1 = t1 / 100.0000;
 		mySndHeader0.sampleRate = (unsigned long)t1;
 	}
	// setup second header, only the buffer ptr is different
	BlockMoveData(&mySndHeader0, &mySndHeader1, sizeof(mySndHeader0));

	mySndHeader0.samplePtr = NewPtr(kDoubleBufferSize);
	if (mySndHeader0.samplePtr == NULL) {
		DisposePtr((Ptr)SB1);
		DisposePtr((Ptr)SB2);
		if (chan != 0L)
			myErr = SndDisposeChannel(chan, FALSE);
		strcpy(global_df->err_message, "+Can't allocate double buffer.");
		return(0);
	}
	mySndHeader1.samplePtr = NewPtr(kDoubleBufferSize);
	if (mySndHeader1.samplePtr == NULL) {
		DisposePtr((Ptr)SB1);
		DisposePtr((Ptr)SB2);
		DisposePtr((Ptr)mySndHeader0.samplePtr);
		if (chan != 0L)
			myErr = SndDisposeChannel(chan, FALSE);
		strcpy(global_df->err_message, "+Can't allocate double buffer.");
		return(0);
	}

	thePlayCmd0.cmd = bufferCmd;
	thePlayCmd0.param1 = 0;						// not used, but clear it out anyway just to be safe
	thePlayCmd0.param2 = (long)&mySndHeader0;

	thePlayCmd1.cmd = bufferCmd;
	thePlayCmd1.param1 = 0;						// not used, but clear it out anyway just to be safe
	thePlayCmd1.param2 = (long)&mySndHeader1;

	whichBuffer = kFirstBuffer;					// buffer 1 will be free when callback runs
 	theCallBackCmd.cmd = callBackCmd;
	theCallBackCmd.param2 = SetCurrentA5();

	isSoundDone = false;
	gBufferDone = true;
	if (PlayingContSound)
		isAborted = 3;
	else
		isAborted = 0;
	if (myErr == noErr) {
		PlayingSound = TRUE;
		lTime_mark = TickCount();
		curBegTime = lastBufSize = pBuf0Size = pBuf1Size = CurF2;
	/* wait for the sound to complete by watching the channel status */
		do {
			timeOffset = (double)(TickCount() - lTime_mark);
			timeOffset *= 16.666666666666667;
			CurFP = conv_from_msec_rep((long)timeOffset) + curBegTime;
			checkContinousSndPlayPossition(CurFP);
			if (!PlayingContSound && PBC.enable && PBC.isPC) {
				strcpy(global_df->err_message, "-Playing, press any F5-F6 key to stop");
				draw_mid_wm();
				PrepWindow(0);
				NonModal = 0;
			}
			if ((key=ced_getc()) != -1) {
				if (!PlayingSound || global_df == NULL) {
					mySndCmd.cmd = quietCmd;
					myErr = SndDoImmediate(chan, &mySndCmd);
					isAborted = 1;
					break;
				}

				if (PlayingContSound == '\002') {
					mySndCmd.cmd = pauseCmd;
					myErr = SndDoImmediate(chan, &mySndCmd);
					global_df->WinChange = FALSE;
					strcpy(global_df->err_message, "-Pausing ...");
					draw_mid_wm();
					while (StillDown() == TRUE) ;
					strcpy(global_df->err_message, "-Click mouse twice to stop, once to pause");
					draw_mid_wm();
					global_df->WinChange = TRUE;
					PlayingContSound = TRUE;
					mySndCmd.cmd = resumeCmd;
					myErr = SndDoImmediate(chan, &mySndCmd);
				} else if (PlayingContSound != '\004') {
					if (!PlayingContSound) {
						if (isKeyEqualCommand(key, 91)) {
							isPlayS = 91;
							isAborted = 1;
						}
						if ((key > 0 && key < 256 || isPlayS != 0) && PBC.enable) {
							if (key != 1 || isPlayS != 0) {
								if (proccessKeys((unsigned int)key) == 1) {
									mySndCmd.cmd = quietCmd;
									myErr = SndDoImmediate(chan, &mySndCmd);
									isAborted = 1;
									strcpy(global_df->err_message, DASHES);
									break;
								}
							}
						} else {
							mySndCmd.cmd = quietCmd;
							myErr = SndDoImmediate(chan, &mySndCmd);
							if (key == 0x6200) {
								isAborted = 4; // F7
							} else if (key == 0x6500) {
								isAborted = 5; // F9
							} else
								isAborted = 1;
							strcpy(global_df->err_message, DASHES);
							break;
						}
					} else {
						mySndCmd.cmd = quietCmd;
						myErr = SndDoImmediate(chan, &mySndCmd);
						PlayingContSound = '\003';
						isAborted = 2;
						break;
					}
				}
			} else {
				if (!PlayingSound || global_df == NULL) {
					mySndCmd.cmd = quietCmd;
					myErr = SndDoImmediate(chan, &mySndCmd);
					isAborted = 1;
					break;
				}
			}
			if (PlayingContSound == '\004') {
				global_df->LeaveHighliteOn = TRUE;
				if (key == 'i' || key == 'I' || key == ' ') {
					DrawCursor(0);
					DrawSoundCursor(2);
					ResetSelectFlag(0);
					if (global_df->SnTr.BegF != CurFP && CurFP != 0L) {
						global_df->row_win2 = 0L;
						global_df->col_win2 = -2L;
						global_df->col_chr2 = -2L;
						if ((res=findStartMediaTag(TRUE, F5Option == EVERY_LINE)) != TRUE)
							findEndOfSpeakerTier(res, F5Option == EVERY_LINE);
						SaveUndoState(FALSE);
						addBulletsToText(SOUNDTIER, global_df->SnTr.SoundFile, conv_to_msec_rep(global_df->SnTr.BegF), conv_to_msec_rep(CurFP));
						global_df->SnTr.BegF = CurFP;
						SetPBCglobal_df(false, 0L);
						if (global_df->SnTr.IsSoundOn)
							DisplayEndF(FALSE);
					}
					selectNextSpeaker();
					strcpy(global_df->err_message, "-Transcribing, click mouse to stop");
					draw_mid_wm();
					strcpy(global_df->err_message, DASHES);
					wmove(global_df->w1, global_df->row_win, global_df->col_win-global_df->LeftCol);
					wrefresh(global_df->w1);
				}
			}
			
			if (gBufferDone == true && !isSoundDone) {
				long bytesToCopy;
			  
				if (PF && DF) {
					PF = 0;
					DF = 0;
					myVars.bytesTotal = bytesCount;
					myVars.bytesCopied = 0; /* no samples copied yet */
					myVars.dataPtr = ((myVars.ISB1) ? SB2 : SB1);
					myVars.ISB1 = !myVars.ISB1;
					if (CurF < global_df->SnTr.EndF || global_df->SnTr.EndF == 0)
						RF = 1;
				}
				/* get number of bytes left to copy */
				bytesToCopy = myVars.bytesTotal - myVars.bytesCopied;

			    /* If the amount left is greater than double-buffer size, */
			    /* then limit the number of bytes to copy to the size of the buffer.*/
				if (bytesToCopy >= kDoubleBufferSize) {
					bytesToCopy = kDoubleBufferSize;
				} else {
					DF = 1;
				}
				CurF2 += bytesToCopy;

			    /* copy samples to double buffer */
				curBegTime = lastBufSize;
				if (kFirstBuffer == whichBuffer) {
					pBufSize = &pBuf0Size;
					pPlayCmd = &thePlayCmd0;
					pSndHeader = &mySndHeader0;
					whichBuffer = kSecondBuffer;
					lastBufSize = pBuf1Size;
				} else {
					pBufSize = &pBuf1Size;
					pPlayCmd = &thePlayCmd1;
					pSndHeader = &mySndHeader1;
					whichBuffer = kFirstBuffer;
					lastBufSize = pBuf0Size;
				}
				*pBufSize = CurF2;
				BlockMove(myVars.dataPtr, pSndHeader->samplePtr, bytesToCopy);
			    /* update data pointer and number of bytes copied */
				myVars.dataPtr = (Ptr)(myVars.dataPtr + bytesToCopy);
				myVars.bytesCopied = myVars.bytesCopied + bytesToCopy;

				pSndHeader->numFrames = bytesToCopy / global_df->SnTr.SNDsample / global_df->SnTr.SNDchan;

			    /* If all samples have been copied, then this is the last buffer. */
				if (myVars.bytesCopied == myVars.bytesTotal) {
					if ((RF || PF) && bytesCount > 0 && CurF2 < global_df->SnTr.SoundFileSize) {
						DF = 1;
					} else {
						isSoundDone = true;
					}
				}

				gBufferDone = false;
				if (!isSoundDone) {
					SndDoCommand(chan, &theCallBackCmd, true);	// reuse callBackCmd
				}

				SndDoCommand(chan, pPlayCmd, true);			// play the next buffer
				lTime_mark = TickCount();
			}
			if (RF) {
				if (!ReadSound(((myVars.ISB1) ? SB2 : SB1), MacBufSize)) {
					mySndCmd.cmd = quietCmd;
					myErr = SndDoImmediate(chan, &mySndCmd);
					com = EOS;
					isAborted = 2;
					ret = 0;
					break;
				} else {
					PF = 1;
					RF = 0;
				}
			}
			myErr = SndChannelStatus(chan, sizeof(status), &status);
		} while (status.scChannelBusy || (isAborted == 3 && !isSoundDone)) ;

		if (isSoundDone && !isAborted) {
			timeOffset = (double)(TickCount() - lTime_mark);
			timeOffset *= 16.666666666666667;
			CurFP = conv_from_msec_rep((long)timeOffset) + curBegTime;
		}

		if (!isSoundDone && isAborted != 1 && isAborted != 2 && isAborted != 4 && isAborted != 5) {
			if (global_df)
				strcpy(global_df->err_message, "+Error playing sound. If sound file is not on the main hard disk, perhaps copying them there will help.");
			isAborted = 2;
			ret = 0;
		}
		if (com == 'p' && global_df) {
			isAborted = 2;
			global_df->SnTr.EndF = CurFP;
			if (global_df->SoundWin) {
				DisplayEndF(FALSE);
				if (global_df->SnTr.BegF != global_df->SnTr.EndF && global_df->SnTr.EndF != 0L)
					SaveUndoState(FALSE);
			}
		}
	} else {
		if (global_df)
			strcpy(global_df->err_message, "+Error playing sound.");
		isAborted = 2;
		ret = 0;
	}

	/* dispose double-buffer memory */
	DisposePtr((Ptr)SB1);
	DisposePtr((Ptr)SB2);
	if (chan != 0L)
		myErr = SndDisposeChannel(chan, FALSE);
	if (myErr != noErr && global_df) {
		strcpy(global_df->err_message, "+Can't dispose of sound channel.");
		return(0);
	}
	if (theSoundCallBackUPP)
		DisposeSndCallBackUPP(theSoundCallBackUPP);
	/* dispose double-buffer memory */
	if (mySndHeader0.samplePtr)
		DisposePtr(mySndHeader0.samplePtr);
	if (mySndHeader1.samplePtr)
		DisposePtr(mySndHeader1.samplePtr);

	if (!isAborted && PBC.enable && PBC.isPC && PBC.LoopCnt > LoopCnt && com != (int)'p') {
		LoopCnt++;
		goto repeat_playback;
	}
	if (/*!isAborted && */PBC.walk && PBC.backspace != PBC.step_length && global_df) {
		long t;
		extern long sEF;

		LoopCnt = 1;
		DrawSoundCursor(0);
		if (isAborted) {
			if (isAborted == 4) {
				t = conv_to_msec_rep(CurFP) - PBC.step_length;
				if (t < 0L)
					t = 0L;
				global_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
				if (global_df->SnTr.BegF >= global_df->SnTr.SoundFileSize)
					goto fin;
				t = conv_to_msec_rep(global_df->SnTr.BegF) + PBC.step_length;
				global_df->SnTr.EndF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
			} else if (isAborted == 5) {
				t = conv_to_msec_rep(CurFP) + PBC.step_length;
				if (t < 0L)
					t = 0L;
				global_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
				if (global_df->SnTr.BegF >= global_df->SnTr.SoundFileSize)
					goto fin;
				t = conv_to_msec_rep(global_df->SnTr.BegF) + PBC.step_length;
				global_df->SnTr.EndF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
			} else {
				t = conv_to_msec_rep(CurFP);
				if (t < 0L)
					goto fin;
				global_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
				if (global_df->SnTr.BegF >= global_df->SnTr.SoundFileSize)
					goto fin;
				t = conv_to_msec_rep(global_df->SnTr.BegF) + PBC.step_length;
				global_df->SnTr.EndF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
			}
		} else if (PBC.backspace > PBC.step_length) {
			t = conv_to_msec_rep(global_df->SnTr.BegF) - (PBC.backspace - PBC.step_length);
			if (t < 0L)
				goto fin;
			global_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '-');
			if (PBC.walk == 2 && global_df->SnTr.BegF >= sEF)
				goto fin;
			if (global_df->SnTr.BegF >= global_df->SnTr.SoundFileSize)
				goto fin;
			t = conv_to_msec_rep(global_df->SnTr.EndF) - (PBC.backspace - PBC.step_length);
			global_df->SnTr.EndF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '-');
			if (PBC.walk == 2 && global_df->SnTr.EndF > sEF)
				global_df->SnTr.EndF = sEF;
		} else {
			t = conv_to_msec_rep(global_df->SnTr.BegF) + (PBC.step_length - PBC.backspace);
			if (t < 0L)
				goto fin;
			global_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
			if (PBC.walk == 2 && global_df->SnTr.BegF >= sEF)
				goto fin;
			if (global_df->SnTr.BegF >= global_df->SnTr.SoundFileSize)
				goto fin;
			t = conv_to_msec_rep(global_df->SnTr.EndF) + (PBC.step_length - PBC.backspace);
			global_df->SnTr.EndF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
			if (PBC.walk == 2 && global_df->SnTr.EndF > sEF)
				global_df->SnTr.EndF = sEF;
		}
		SetPBCglobal_df(false, 0L);
		DisplayEndF(TRUE);
		DrawSoundCursor(3);
		if (!isAborted && PBC.pause_len > 0L && !PlayingContSound) {
			double d;
			Size t;

			d = (double)PBC.pause_len;
			t = TickCount() + (Size)(d / (double)16.666666666);
			do {  
				if (TickCount() > t) 
					break;
				if ((key=ced_getc()) != -1) {
					if (isKeyEqualCommand(key, 91)) {
						isPlayS = 91;
						isAborted = 1;
						break;
					}
					if ((key > 0 && key < 256 || isPlayS != 0) && PBC.enable) {
						if (key != 1 || isPlayS != 0) {
							if (proccessKeys((unsigned int)key) == 1) {
								if (key == 0x6200) {
									t = conv_to_msec_rep(CurFP) - PBC.step_length;
									if (t < 0L)
										t = 0L;
									global_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
									if (global_df->SnTr.BegF >= global_df->SnTr.SoundFileSize)
										goto fin;
									t = conv_to_msec_rep(global_df->SnTr.BegF) + PBC.step_length;
									global_df->SnTr.EndF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
									isAborted = 4; // F7
								} else if (key == 0x6500) {
									t = conv_to_msec_rep(CurFP) + PBC.step_length;
									if (t < 0L)
										t = 0L;
									global_df->SnTr.BegF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
									if (global_df->SnTr.BegF >= global_df->SnTr.SoundFileSize)
										goto fin;
									t = conv_to_msec_rep(global_df->SnTr.BegF) + PBC.step_length;
									global_df->SnTr.EndF = AlignMultibyteMediaStream(conv_from_msec_rep(t), '+');
									isAborted = 5; // F9
								} else
									isAborted = 1;
								strcpy(global_df->err_message, DASHES);
								break;
							}
						}
					} else {
						isAborted = 1;
						strcpy(global_df->err_message, DASHES);
						break;
					}
				} else if (!PlayingSound || isPlayS != 0) {
					isAborted = 1;
					break;
				}
			} while (true) ;	
		}
		if (isAborted != 1)
			goto repeat_playback;
	}
fin:
	if (global_df) {
		if (isAborted == 1 && PBC.walk)
			strcpy(global_df->err_message, "-Aborted.");
		else if (!PlayingContSound && PBC.enable && PBC.isPC)
			strcpy(global_df->err_message, DASHES);
		else if (global_df->LastCommand == 49) {
			extern long gCurF;

			gCurF = conv_to_msec_rep(CurFP);
			if (gCurF >= 0L) {
				if (isSoundDone && !isAborted)
					gCurF = global_df->SnTr.BegF;
				else
					gCurF = AlignMultibyteMediaStream(conv_from_msec_rep(gCurF), '+');
			} else
				gCurF = global_df->SnTr.BegF;
		}
	}
	if (global_df)
		SetPBCglobal_df(false, 0L);
	PlayingSound = FALSE;
	return(ret);
}

char PlayBuffer(int com) {
	if (global_df->SnTr.isMP3 == TRUE)
		return(PlayMP3Sound(com));
	else {
		if (global_df->SnTr.SoundFPtr == NULL && global_df->SnTr.rSoundFile[0] != EOS)
			global_df->SnTr.SoundFPtr = fopen(global_df->SnTr.rSoundFile, "rb");
		if (global_df->SnTr.SoundFPtr == NULL)
			return(FALSE);
		return(PlayPCMSound(com));
	}
}

/* play sound end */

void show_wave(char *file, long begin, long end, int add) {
}

void WriteAIFFHeader(FILE *fpout, long numBytes, unsigned int frames, short numChannels, short sampleSize, double sampleRate) {
	formAIFF.ckID = 'FORM';
	formAIFF.ckSize = numBytes + sizeof(formAIFF)+sizeof(commonChunk)+sizeof(soundDataChunk) - 16;
	formAIFF.formType = 'AIFF';
	if (byteOrder == CFByteOrderLittleEndian) {
		flipLong((long*)&formAIFF.ckID);
		flipLong(&formAIFF.ckSize);
		flipLong((long*)&formAIFF.formType);
	}

	commonChunk.ckID = 'COMM';
	commonChunk.ckSize = 18;
	commonChunk.numChannels = numChannels;
	commonChunk.numSampleFrames = frames;
	commonChunk.sampleSize = sampleSize;
/*
	commonChunk.sampleRate.exp = 0;
	commonChunk.sampleRate.man[0] = 0;
	commonChunk.sampleRate.man[1] = 0;
	commonChunk.sampleRate.man[2] = 0;
	commonChunk.sampleRate.man[3] = 0;
*/
	if (byteOrder == CFByteOrderLittleEndian)
		Doubletox80(&sampleRate, (struct myX80Rep *)&commonChunk.sampleRate);					
	else
		dtox80(&sampleRate, &commonChunk.sampleRate);
	if (byteOrder == CFByteOrderLittleEndian) {
		flipLong((long*)&commonChunk.ckID);
		flipLong(&commonChunk.ckSize);
		flipShort(&commonChunk.numChannels);
		flipLong((long*)&commonChunk.numSampleFrames);
		flipShort(&commonChunk.sampleSize);
		flipShort(&commonChunk.sampleRate.exp);
		flipShort((short*)&commonChunk.sampleRate.man[0]);
		flipShort((short*)&commonChunk.sampleRate.man[1]);
		flipShort((short*)&commonChunk.sampleRate.man[2]);
		flipShort((short*)&commonChunk.sampleRate.man[3]);
	}

	soundDataChunk.ckID = 'SSND';
	soundDataChunk.ckSize = numBytes;// 2 * frames * numChannels + 8;
	soundDataChunk.offset = 0;
	soundDataChunk.blockSize = 0;
	if (byteOrder == CFByteOrderLittleEndian) {
		flipLong((long*)&soundDataChunk.ckID);
		flipLong(&soundDataChunk.ckSize);
		flipLong((long*)&soundDataChunk.offset);
		flipLong((long*)&soundDataChunk.blockSize);
	}

	fseek(fpout, 0, SEEK_SET);
	// Write the AIFF header, previously initilized
	if (fwrite((char *)&formAIFF, 1, sizeof(formAIFF), fpout) != sizeof(formAIFF)) {
		strcpy(global_df->err_message, "+Error writting sound file.");
		return;
	}
	if (fwrite((char *)&commonChunk, 1, sizeof(commonChunk), fpout) != sizeof(commonChunk)) {
		strcpy(global_df->err_message, "+Error writting sound file.");
		return;
	}
	if (fwrite((char *)&soundDataChunk, 1, sizeof(soundDataChunk), fpout) != sizeof(soundDataChunk)) {
		strcpy(global_df->err_message, "+Error writting sound file.");
		return;
	}
}

#define	MIN(x,y)		((x) < (y) ? (x) : (y))

void SaveSoundClip(sndInfo *SnTr, char isGetNewname) {
	register long		i;
	register long		j;
	register char 		t0, t1;
	short				t, *t2;
	short				sampleSize;
	long				bytesCountW;
	long				bytesCountR;
	double				framesD1;
	unsigned int		frames;
	//	OSErr				myErr = noErr;
	Boolean				isReplace;
	FNType				*p;
	FNType				fileName[FNSize];
	FILE				*fpout = NULL;;

	if (global_df->SnTr.isMP3 == TRUE) {
		strcpy(global_df->err_message, "+This command doesn't work on MP3 files.");
		return;
	}
	UnicodeToUTF8(global_df->SnTr.SoundFile, strlen(global_df->SnTr.SoundFile), (unsigned char *)fileName, NULL, FNSize);
	p = strrchr(fileName, '.');
	if (p != NULL && p != fileName)
		*p = EOS;
	if (isGetNewname) {
		uS.sprintf(fileName+strlen(fileName), "_%d_%d.aif", conv_to_msec_rep(global_df->SnTr.BegF), conv_to_msec_rep(global_df->SnTr.EndF));
	  	if (!myNavPutFile(fileName, "Give Sound File Name", 'TEXT', &isReplace))
			return;
	} else {
		isReplace = FALSE;
		uS.str2FNType(fileName, strlen(fileName), ".aif");
		unlink(fileName);
	}
	if (isReplace)
		unlink(fileName);

	// create and open the output file
	if ((fpout=fopen(fileName, "wb")) == NULL) {
		strcpy(global_df->err_message, "+Error creating file.");
		return;
	}
	settyp(fileName, 'AIFF', 'TVOD', FALSE);
	if (global_df->SnTr.SNDformat == kULawCompression || global_df->SnTr.SNDformat == kALawCompression)
		sampleSize = 16;
	else
		sampleSize = global_df->SnTr.SNDsample * 8;
	WriteAIFFHeader(fpout, 0L, 0, global_df->SnTr.SNDchan, sampleSize, global_df->SnTr.SNDrate);
	CurF = global_df->SnTr.BegF;
	if (fseek(global_df->SnTr.SoundFPtr, CurF+global_df->SnTr.AIFFOffset, SEEK_SET) != 0) {
		strcpy(global_df->err_message, "+Error reading sound file.");
		goto bail;
	}
	while (CurF < global_df->SnTr.EndF) {
		if (global_df->SnTr.SNDformat == kULawCompression ||
			global_df->SnTr.SNDformat == kALawCompression) {
			bytesCountR = MIN(UTTLINELEN/2, global_df->SnTr.EndF-CurF);
			if (fread(templineC2, 1, bytesCountR, global_df->SnTr.SoundFPtr) != bytesCountR) {
				strcpy(global_df->err_message, "+Error reading sound file.");
				goto bail;
			}
			for (j=0L, i=0L; j < bytesCountR; j++, i+=2) {
				if (global_df->SnTr.SNDformat == kALawCompression)
					t = Alaw2Lin((unsigned char)templineC2[j]);
				else
					t = Mulaw2Lin((unsigned char)templineC2[j]);
				t2 = (short *)(templineC1+i);
				*t2 = t;
			}
			bytesCountW = bytesCountR * 2L;
		} else {
			bytesCountR = MIN(UTTLINELEN, global_df->SnTr.EndF-CurF);
			if (fread(templineC1, 1, bytesCountR, global_df->SnTr.SoundFPtr) != bytesCountR) {
				strcpy(global_df->err_message, "+Error reading sound file.");
				goto bail;
			}
			bytesCountW = bytesCountR;
		}
		if (global_df->SnTr.SFileType == 'WAVE' && global_df->SnTr.SNDsample == 2) {
			for (i=0; i < bytesCountW; i+=2) {
				t0 = templineC1[i];
				t1 = templineC1[i+1];
				templineC1[i+1] = t0;
				templineC1[i] = t1;
			}
		}
		if (!global_df->SnTr.DDsound && global_df->SnTr.SNDsample == 1) {
			for (i=0; i < bytesCountW; i++) {
				templineC1[i] -= 128;
			}
		}

		if (fwrite(templineC1, 1, bytesCountW, fpout) != bytesCountW) {
			strcpy(global_df->err_message, "+Error writting sound file.");
			goto bail;
		}
		
		CurF += bytesCountR;
	}
	bytesCountW = global_df->SnTr.EndF - global_df->SnTr.BegF;
	framesD1 = (double)conv_to_msec_rep(bytesCountW) / 1000.0000;
	framesD1 = framesD1 * (double)global_df->SnTr.SNDrate;
	frames = (unsigned int)(framesD1);
	if (fseek(fpout, 0, SEEK_SET) != 0) {
		strcpy(global_df->err_message, "+Error setting file position.");
		goto bail;
	}
	WriteAIFFHeader(fpout, bytesCountW, frames, global_df->SnTr.SNDchan, sampleSize, global_df->SnTr.SNDrate);
bail:
	if (fpout != NULL)
		fclose(fpout);
}
