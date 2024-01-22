/**********************************************************************
	"Copyright 1990-2024 Brian MacWhinney. Use is subject to Gnu Public License
	as stated in the attached "gpl.txt" file."
*/


#include "ced.h"
#include "cu.h"
#ifdef _MAC_CODE 
#import "AVController.h"
#elif _WIN32 
#endif

#if !defined(UNX)
#define _main silence_main
#define call silence_call
#define getflag silence_getflag
#define init silence_init
#define usage silence_usage
#endif

#include "mul.h" 

#define IS_WIN_MODE FALSE
#define CHAT_MODE 1

extern struct tier *defheadtier;
extern char OverWriteFile;

#define SILENCE struct SilenceList
struct SilenceList {
	long beg;
	long end;
	SILENCE *next;
} ;

#define silence_AVinfo struct SILAVINFO
struct SILAVINFO {
	char mediaFPath[CS_MAX_PATH+FILENAME_MAX];
	char mediaFName[CS_MAX_PATH+FILENAME_MAX];
	char isWhatType; // isAudio isVideo
//	FILE *SoundFPtr;
//	Float32 *TopP1;
//	Float32 *TopP2;
	long  BegF, EndF;
	long  SoundFileSize;
	AudioFormatID mFormatID;
	Float64 mSampleRate;
	UInt32 mBitsPerChannel;
	UInt32 mChannelsPerFrame;
	AudioFormatFlags mFormatFlags;
	UInt32 mBytesPerPacket;
	UInt32 mFramesPerPacket;
	UInt32 mBytesPerFrame;
	UInt32 mReserved;
} ;

static char SOption;
static SILENCE *TimesList;

/* *********************************************************** */

static SILENCE *free_SilenceList(SILENCE *p) {
	SILENCE *t;

	while (p != NULL) {
		t = p;
		p = p->next;
		free(t);
	}
	return(NULL);
}

static void freeAllMemory(char isQuit, silence_AVinfo *AVinfo) {
	TimesList = free_SilenceList(TimesList);
//	if (AVinfo->TopP1 != NULL)
//		free(AVinfo->TopP1);
//	if (AVinfo->TopP2 != NULL)
//		free(AVinfo->TopP2);
//	if (AVinfo->SoundFPtr != NULL)
//		fclose(AVinfo->SoundFPtr);
//	AVinfo->SoundFPtr = NULL;
	
	if (isQuit) {
		cutt_exit(0);
	}
}

void init(char f) {
	if (f) {
		OverWriteFile = TRUE;
		AddCEXExtension = "";
		stout = TRUE;
		if (defheadtier->nexttier != NULL)
			free(defheadtier->nexttier);
		free(defheadtier);
		defheadtier = NULL;
		TimesList = NULL;
		addword('\0','\0',"+xxx");
		addword('\0','\0',"+yyy");
		addword('\0','\0',"+www");
		addword('\0','\0',"+xxx@s*");
		addword('\0','\0',"+yyy@s*");
		addword('\0','\0',"+www@s*");
		addword('\0','\0',"+unk|xxx");
		addword('\0','\0',"+unk|yyy");
		addword('\0','\0',"+*|www");
		addword('\0','\0',"+.");
		addword('\0','\0',"+?");
		addword('\0','\0',"+!");
		maininitwords();
		mor_initwords();
		FilterTier = 1;
		SOption = FALSE;
	} else {
		if (!SOption) {
			fprintf(stderr, "\nPlease specify a keyword you want silenced with +s option.\n\n");
			cutt_exit(0);
		}
	}
}

void usage() {
	printf("Usage: silence [%s] filename(s)\n", mainflgs());
	mainusage(FALSE);
	puts("\nExample:");
	puts("       silence +smy *.cha");
	puts("       silence +smy +t%wor -t* *.cha");
	cutt_exit(0);
}

CLAN_MAIN_RETURN main(int argc, char *argv[]) {
	isWinMode = IS_WIN_MODE;
	chatmode = CHAT_MODE;
	CLAN_PROG_NUM = SILENCE_P;
	OnlydataLimit = 0;
	UttlineEqUtterance = FALSE;
#ifndef _CLAN_DEBUG
//	fprintf(stderr, "\nSILENCE command does not work on the Mac\n\n");
//	return;
#endif
#ifdef _WIN32 
	fprintf(stderr, "\nSILENCE command does not work on Windows PC\n\n");
#else
	bmain(argc,argv,NULL);
#endif
}
		
void getflag(char *f, char *f1, int *i) {

	f++;
	switch(*f++) {
		case 's':
			SOption = TRUE;
		default:
			maingetflag(f-2,f1,i);
			break;
	}
}

static SILENCE *AddToSilenceList(SILENCE *root, long mBeg, long mEnd, silence_AVinfo *AVinfo) {
	SILENCE *nt, *tnt;
	
	if (root == NULL) {
		root = NEW(SILENCE);
		nt = root;
		if (nt == NULL) {
			fprintf(stderr, "Out of memory.");
			freeAllMemory(TRUE, AVinfo);
		}
		nt->next = NULL;
	} else {
		tnt= root;
		nt = root;
		while (1) {
			if (nt == NULL)
				break;
			else if (nt->beg >= mBeg) {
				if (mBeg <= nt->end) {
					if (nt->end < mEnd)
						nt->end = mEnd;
					return(root);
				} else
					break;
			}
			tnt = nt;
			nt = nt->next;
		}
		if (nt == NULL) {
			tnt->next = NEW(SILENCE);
			nt = tnt->next;
			if (nt == NULL) {
				fprintf(stderr, "Out of memory.");
				freeAllMemory(TRUE, AVinfo);
			}
			nt->next = NULL;
		} else if (nt == root) {
			root = NEW(SILENCE);
			root->next = nt;
			nt = root;
			if (nt == NULL) {
				fprintf(stderr, "Out of memory.");
				freeAllMemory(TRUE, AVinfo);
			}
		} else {
			nt = NEW(SILENCE);
			if (nt == NULL) {
				fprintf(stderr, "Out of memory.");
				freeAllMemory(TRUE, AVinfo);
			}
			nt->next = tnt->next;
			tnt->next = nt;
		}
	}
	nt->beg = mBeg;
	nt->end = mEnd;
	return(root);
}

static void silence_wave(char *buf, Size beg, Size end) {
	for (; beg < end; beg++)
		buf[beg] = '\0';
}

static Size roundUpTime(Float64 num) {
	unsigned long t;

	t = (long)num;
	num = num - t;
	if (num > 0.5)
		t++;
	return(t);
}

static Float64 conv_to_msec(AudioStreamBasicDescription *asbd, Size num) {
	Float64 res, num2;

	res = (Float64)num;
	num2 = asbd->mSampleRate / (double)1000.0000;
	res = res / num2;
	num2 = (Float64)asbd->mBytesPerFrame;
	res = res / num2;

	return(res);
}

static Size conv_from_msec(AudioStreamBasicDescription *asbd, Float64 num) {
	Float64 res, num2;

	res = (Float64)num;
	num2 = asbd->mSampleRate / (double)1000.0000;
	res = res * num2;
	num2 = (Float64)asbd->mBytesPerFrame;
	res = res * num2;
	return(roundUpTime(res));
}

static char ProcessSound(silence_AVinfo *AVinfo, SILENCE *p) {
	BOOL isShowProgressBar;
	FNType *FNameOffset;
	SILENCE *t;

	AudioStreamBasicDescription asbd = {};

	asbd.mFormatID = kAudioFormatLinearPCM;
	asbd.mSampleRate = AVinfo->mSampleRate;
	asbd.mChannelsPerFrame = AVinfo->mChannelsPerFrame;
	asbd.mBitsPerChannel = 16;
	asbd.mFramesPerPacket = 1;
	asbd.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	asbd.mBytesPerFrame = asbd.mChannelsPerFrame * 2;
	asbd.mBytesPerPacket = asbd.mFramesPerPacket * asbd.mBytesPerFrame;
	asbd.mReserved = 0;
/*
	asbd.mFormatID = kAudioFormatLinearPCM;
	asbd.mSampleRate = 22050.0000;
	asbd.mChannelsPerFrame = 2;
	asbd.mBitsPerChannel = 16;
	asbd.mFramesPerPacket = 1;
	asbd.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	asbd.mBytesPerFrame = asbd.mChannelsPerFrame * 2;
	asbd.mBytesPerPacket = asbd.mFramesPerPacket * asbd.mBytesPerFrame;
	asbd.mReserved = 0;
*/

	if (AVinfo->SoundFileSize > 300000)
		isShowProgressBar = true;
	else
		isShowProgressBar = false;
//	AVinfo->TopP1 = (Float32 *)malloc((AVinfo->SoundFileSize+1) * sizeof(Float32));
//	if (AVinfo->mChannelsPerFrame == 2)
//		AVinfo->TopP2 = (Float32 *)malloc((AVinfo->SoundFileSize+1) * sizeof(Float32));

//	dispatch_async(dispatch_get_global_queue(0, 0), ^{
		FNType tMediaFileName[FILENAME_MAX+2];/* used by expend bullets */
		BOOL isRead;
		UInt32 i, pixelCnt, sampleCount, progress, oldProgress;
		Float32 val;
		NSError * error = nil;
		AVAssetReader *reader;
		CMTime timeStamp;
		Float64 BegM, EndM, totalMSecs;
		size_t sampleSize, targetSampleOffset, targetSampleSize;
		OSStatus stat;
//		SInt16 buf[10000];
		UInt8 *buf;
		int bufSize;

		buf = NULL;
		bufSize = 0;

		AVURLAsset *songAsset = [AVURLAsset URLAssetWithURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:AVinfo->mediaFPath]] options:nil];
		reader = [[AVAssetReader alloc] initWithAsset:songAsset error:&error];
		if (!error) {
			NSDictionary *setting = [[NSDictionary alloc] initWithObjectsAndKeys:
									 [NSNumber numberWithInt:asbd.mFormatID], AVFormatIDKey,
									 [NSNumber numberWithFloat:asbd.mSampleRate], AVSampleRateKey,
									 [NSNumber numberWithInt:asbd.mChannelsPerFrame], AVNumberOfChannelsKey,
									 [NSNumber numberWithInt:asbd.mBitsPerChannel], AVLinearPCMBitDepthKey,
//									 [NSNumber numberWithFloat:22050.0000], AVSampleRateKey,
//									 [NSNumber numberWithInt:2], AVNumberOfChannelsKey,
//									 [NSNumber numberWithInt:16], AVLinearPCMBitDepthKey,
									 [NSNumber numberWithBool:NO], AVLinearPCMIsFloatKey,
									 [NSNumber numberWithBool:NO], AVLinearPCMIsBigEndianKey,
									 [NSNumber numberWithBool:NO], AVLinearPCMIsNonInterleaved,
									 nil];

			AVAssetTrack *track = [songAsset.tracks objectAtIndex:0];
//			AVAssetReaderTrackOutput* readerOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:track outputSettings:setting];
			AVAssetReaderOutput* readerOutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:track outputSettings:setting];
			[reader addOutput:readerOutput];
//			[readerOutput release];
			
			extractPath(tMediaFileName, AVinfo->mediaFPath);
			if (strrchr(tMediaFileName, PATHDELIMCHR) == NULL)
				strcpy(tMediaFileName, "/Users/WORK/cur_bug/silence/");
			addFilename2Path(tMediaFileName, "test.wav");
			unlink(tMediaFileName);
			NSURL *outURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:tMediaFileName]];
			AVAssetWriter *writer = [AVAssetWriter assetWriterWithURL:outURL fileType:AVFileTypeWAVE error:&error];
			if (!error) {


					//create asbd and channel layout for output
/*
				AudioChannelLayout audioChannelLayout = {
					.mChannelLayoutTag = kAudioChannelLayoutTag_Mono,
					.mChannelBitmap = 0,
					.mNumberChannelDescriptions = 0
				};
*/
				CMAudioFormatDescriptionRef audioFormat;
				CMAudioFormatDescriptionCreate(kCFAllocatorDefault, &asbd, NULL, NULL, // &audioChannelLayout,
											   0, NULL, NULL, &audioFormat);

				AudioChannelLayout channelLayout;
				memset(&channelLayout, 0, sizeof(AudioChannelLayout));
				channelLayout.mChannelLayoutTag = kAudioChannelLayoutTag_Mono;

				AVAssetWriterInput *writerInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio outputSettings:setting];
//				AVAssetWriterInput *writerInput = [[AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio outputSettings:setting] retain];
				writerInput.expectsMediaDataInRealTime = NO;
				if ([writer canAddInput:writerInput]) {
					[writer addInput:writerInput];
					if ([writer startWriting] == YES) {
						[writer startSessionAtSourceTime:kCMTimeZero];
						
//						reader.timeRange = CMTimeRangeMake(CMTimeMake((int64_t)[self conv_to_msec_rep:AVinfo->WBegF], 1000), kCMTimePositiveInfinity);
						reader.timeRange = CMTimeRangeMake(kCMTimeZero, [songAsset duration]);
//						CMTimeRangeMake(kCMTimeZero, [songAsset duration])
						[reader startReading];
						
						oldProgress = 0;
						pixelCnt = 0;
						isRead = false;
//						while (reader.status == AVAssetReaderStatusReading) {
						CMSampleBufferRef sample = [readerOutput copyNextSampleBuffer];
						
						if (sample != NULL) {
							CMFormatDescriptionRef formatDescription = CMSampleBufferGetFormatDescription(sample);
							
							const AudioStreamBasicDescription *asbdRef = CMAudioFormatDescriptionGetStreamBasicDescription(formatDescription);
														
							
							UInt32 bytesPerSample = asbd.mBitsPerChannel * asbd.mChannelsPerFrame;
						}
						totalMSecs = 0.0;
						CMSampleBufferRef sample2;
						while ( sample != NULL ) {
//							AVAssetReaderTrackOutput * trackOutput = (AVAssetReaderTrackOutput *)[reader.outputs objectAtIndex:0];
//							CMSampleBufferRef sample = [trackOutput copyNextSampleBuffer];
//							CMSampleBufferRef sample = [readerOutput copyNextSampleBuffer];
//							if (sample) {
								sampleSize = CMSampleBufferGetTotalSampleSize(sample);
								if (bufSize < sampleSize) {
									if (buf != NULL)
										free(buf);
									buf = (UInt8 *)malloc((sampleSize+1) * sizeof(UInt8));
									if (buf == NULL) {
										fprintf(stderr, "Out of memory.");
										freeAllMemory(TRUE, AVinfo);
									}
									bufSize	= sampleSize;
									for (i=0; i < bufSize; i++)
										buf[i] = 0;
								}
								CMBlockBufferRef blockBufferRef = CMSampleBufferGetDataBuffer(sample);
								size_t length = CMBlockBufferGetDataLength(blockBufferRef);
							
							
/*
								NSMutableData *data = [NSMutableData dataWithLength:length];
								CMBlockBufferCopyDataBytes(blockBufferRef, 0, length, data.mutableBytes);
								SInt16 *samples = (SInt16 *)data.mutableBytes;
								sampleCount = length / bytesPerSample;
								for (i=0; i < sampleCount ; i++) {
									val = (Float32)*samples++;
									AVinfo->TopP1[pixelCnt] = val;
									if (AVinfo->mChannelsPerFrame==2) {
										val = (Float32)*samples++;
										AVinfo->TopP2[pixelCnt] = val;
									}
									progress = (UInt32)((double)pixelCnt/AVinfo->SoundFileSize * 100.0000);
									if (progress > oldProgress && isShowProgressBar) {
										oldProgress = progress;
										// Update the progress bar which is in the sheet:
									}
								}
*/

								timeStamp = CMSampleBufferGetPresentationTimeStamp(sample);
								BegM = CMTimeGetSeconds(timeStamp) * 1000.000000;
								EndM = conv_to_msec(&asbd, sampleSize);
								totalMSecs += EndM;
								targetSampleSize = conv_from_msec(&asbd, EndM);
fprintf(stdout, "msec=%lf; total=%lf; sampleSize=%ld; RT=%ld\n", EndM, totalMSecs, sampleSize, targetSampleSize);
								EndM = BegM + EndM;

								for (t=p; t != NULL; t=t->next) {
									if (t->end < BegM || t->beg > EndM)
										;
									else if (t->beg <= BegM && t->end >= BegM && t->beg <= EndM && t->end >= EndM) {
										targetSampleOffset = 0;
										targetSampleSize = sampleSize;
fprintf(stdout, "\t1-BegM=%lf; EndM=%lf; t->beg=%ld; t->end=%ld; Offset=%ld; off=%ld; len=%ld\n",
	BegM, EndM, t->beg, t->end, targetSampleOffset, targetSampleSize);
										stat = CMBlockBufferReplaceDataBytes(buf, blockBufferRef, targetSampleOffset, targetSampleSize);
									} else if (t->beg >= BegM && t->end <= EndM) {
										targetSampleOffset = conv_from_msec(&asbd, t->beg - BegM);
										targetSampleSize = conv_from_msec(&asbd, t->end - BegM);
fprintf(stdout, "\t2-BegM=%lf; EndM=%lf; t->beg=%ld; t->end=%ld; Offset=%ld; off=%ld; len=%ld\n",
	BegM, EndM, t->beg, t->end, targetSampleOffset, targetSampleSize);
										stat = CMBlockBufferReplaceDataBytes(buf, blockBufferRef, targetSampleOffset, targetSampleSize);
									} else if (t->beg <= BegM && t->end >= BegM && t->end <= EndM) {
										targetSampleOffset = 0;
										targetSampleSize = conv_from_msec(&asbd, t->end - BegM);
fprintf(stdout, "\t3-BegM=%lf; EndM=%lf; t->beg=%ld; t->end=%ld; Offset=%ld; off=%ld; len=%ld\n",
	BegM, EndM, t->beg, t->end, targetSampleOffset, targetSampleSize);
										stat = CMBlockBufferReplaceDataBytes(buf, blockBufferRef, targetSampleOffset, targetSampleSize);
									} else if (t->beg >= BegM && t->beg <= EndM && t->end >= EndM) {
										targetSampleOffset = conv_from_msec(&asbd, t->beg - BegM);
										targetSampleSize = sampleSize - targetSampleOffset;
fprintf(stdout, "\t4-BegM=%lf; EndM=%lf; t->beg=%ld; t->end=%ld; Offset=%ld; off=%ld; len=%ld\n",
	BegM, EndM, t->beg, t->end, targetSampleOffset, targetSampleSize);
										stat = CMBlockBufferReplaceDataBytes(buf, blockBufferRef, targetSampleOffset, targetSampleSize);
									}
								}

								timeStamp = CMTimeMake(1, sampleSize);
								stat = CMAudioSampleBufferCreateWithPacketDescriptions(kCFAllocatorDefault, blockBufferRef, true, NULL,
																								NULL, audioFormat, sampleSize, timeStamp, NULL, &sample2);

								if (stat != noErr) {
									NSLog(@"Sample buffer error");
									exit(1);
								}

//								stat = CMSampleBufferSetDataBuffer(sample2, blockBufferRef);
//								size_t size = CMSampleBufferGetTotalSampleSize(sample);
//								size_t size2 = CMSampleBufferGetTotalSampleSize(sample2);
								while (1) {
									if ([writerInput isReadyForMoreMediaData]) {
										[writerInput appendSampleBuffer:sample2];
										break;
									}
								}

//								CMSampleBufferInvalidate(sample);
								CFRelease(sample);
//								CMSampleBufferInvalidate(sample2);
								CFRelease(sample2);
//								if (pixelCnt >= MidAVinfo.SoundFileSize)
//									break;
//							}
							sample = [readerOutput copyNextSampleBuffer];
						}
						
						[writerInput markAsFinished];
//						[writer finishWriting];		
						[writer finishWritingWithCompletionHandler:^(){ NSLog(@"Done!"); }];
						if (reader.status == AVAssetReaderStatusFailed || reader.status == AVAssetReaderStatusUnknown){
							fprintf(stderr, "Failed to read media file.");
						} else if (pixelCnt >= AVinfo->SoundFileSize || reader.status == AVAssetReaderStatusCompleted) {
							isRead = true;
						}
						[reader release];
					}
				}
			}
			[setting release];
		}
//	});
	return(true);
}

static BOOL silence_findFullMediaName(silence_AVinfo *AVinfo) {
	int pathLen, nameLen;
	char *rFileName;
	BOOL isFirstTimeAround;

	isFirstTimeAround = true;
	rFileName = AVinfo->mediaFPath;
	pathLen = strlen(rFileName);
tryAgain:
	strcat(rFileName, AVinfo->mediaFName);
	nameLen = strlen(rFileName);
	strcat(rFileName, ".mov");
	AVinfo->isWhatType = isVideo;
	if (access(rFileName, 0)) {
		rFileName[nameLen] = EOS;
		uS.str2FNType(rFileName, strlen(rFileName), ".mp4");
		AVinfo->isWhatType = isVideo;
		if (access(rFileName, 0)) {
			rFileName[nameLen] = EOS;
			uS.str2FNType(rFileName, strlen(rFileName), ".mp3");
			AVinfo->isWhatType = isAudio;
			if (access(rFileName, 0)) {
				rFileName[nameLen] = EOS;
				uS.str2FNType(rFileName, strlen(rFileName), ".wav");
				AVinfo->isWhatType = isAudio;
				if (access(rFileName, 0)) {
					rFileName[nameLen] = EOS;
					uS.str2FNType(rFileName, strlen(rFileName), ".m4v");
					AVinfo->isWhatType = isVideo;
					if (access(rFileName, 0)) {
						rFileName[nameLen] = EOS;
						uS.str2FNType(rFileName, strlen(rFileName), ".aif");
						AVinfo->isWhatType = isAudio;
						if (access(rFileName, 0)) {
							rFileName[nameLen] = EOS;
							uS.str2FNType(rFileName, strlen(rFileName), ".avi");
							AVinfo->isWhatType = isVideo;
							if (access(rFileName, 0)) {
								rFileName[nameLen] = EOS;
								uS.str2FNType(rFileName, strlen(rFileName), ".wmv");
								AVinfo->isWhatType = isVideo;
								if (access(rFileName, 0)) {
									rFileName[nameLen] = EOS;
									uS.str2FNType(rFileName, strlen(rFileName), ".mpg");
									AVinfo->isWhatType = isVideo;
									if (access(rFileName, 0)) {
										rFileName[nameLen] = EOS;
										uS.str2FNType(rFileName, strlen(rFileName), ".aiff");
										AVinfo->isWhatType = isAudio;
										if (access(rFileName, 0)) {
											rFileName[nameLen] = EOS;
											uS.str2FNType(rFileName, strlen(rFileName), ".dv");
											AVinfo->isWhatType = isVideo;
											if (access(rFileName, 0)) {
												if (isFirstTimeAround) {
													isFirstTimeAround = false;
													rFileName[pathLen] = EOS;
													strcat(rFileName, "media/");
													goto tryAgain;
												}
												rFileName[nameLen] = EOS;
												AVinfo->isWhatType = 0;
												return(false);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return(true);
}

void call() {
	char word[BUFSIZ];
	char err_mess[ERRMESSAGELEN];
	char mFName[FILENAME_MAX+2];
	long startI, endI, mBeg, mEnd;
	silence_AVinfo AVinfo;
	NSURL *mediaURL;
	CMTime duration;
//	Float64 durationTime;

//	AVinfo.SoundFPtr = NULL;
//	AVinfo.TopP1 = NULL;
//	AVinfo.TopP2 = NULL;
	AVinfo.mediaFPath[0] = EOS;
	AVinfo.mediaFName[0] = EOS;
	AVinfo.isWhatType = 0; // isAudio isVideo
	AVinfo.BegF = 0L;
	AVinfo.EndF = 0L;
	AVinfo.SoundFileSize = 0L;
	AVinfo.mBitsPerChannel = 0;
	AVinfo.mChannelsPerFrame = 0;
	AVinfo.mSampleRate = 0.0;
	AVinfo.mFormatID = 0;
	AVinfo.mFormatFlags = 0;
	AVinfo.mBytesPerPacket = 0;
	AVinfo.mFramesPerPacket = 0;
	AVinfo.mBytesPerFrame = 0;
	AVinfo.mReserved = 0;

	currentatt = 0;
	currentchar = (char)getc_cr(fpin, &currentatt);
	while (getwholeutter()) {
		if (uS.partcmp(utterance->speaker, MEDIAHEADER, FALSE, FALSE)) {
			getMediaName(utterance->line, AVinfo.mediaFName, FILENAME_MAX);
//			AVinfo.SoundFPtr = NULL;
			templineC[0] = EOS;
			strcpy(err_mess, "Can't locate media filename:\n");
			strcat(err_mess, AVinfo.mediaFPath);
			addFilename2Path(err_mess, AVinfo.mediaFName);
			strcat(err_mess, "\n or:\n");
			if (!silence_findFullMediaName(&AVinfo)) {
				strcat(err_mess, AVinfo.mediaFPath);
				fprintf(stderr, "%s\n", err_mess);
				freeAllMemory(FALSE, &AVinfo);
				return;
			}

			if (AVinfo.isWhatType == isVideo)
				;
			else
				;
			mediaURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:AVinfo.mediaFPath]];
			AVAsset *mediaAsset = [AVAsset assetWithURL:mediaURL];

			AVAssetTrack *track;
			track = mediaAsset.tracks[0];
			NSArray *fd = track.formatDescriptions;
			CMAudioFormatDescriptionRef desc = (CMAudioFormatDescriptionRef)fd[0];
			const AudioStreamBasicDescription *asbd = CMAudioFormatDescriptionGetStreamBasicDescription(desc);
			AVinfo.mBitsPerChannel = asbd->mBitsPerChannel;
			AVinfo.mChannelsPerFrame = asbd->mChannelsPerFrame;
			AVinfo.mSampleRate = asbd->mSampleRate;
			AVinfo.mFormatID = asbd->mFormatID;
			AVinfo.mFormatFlags = asbd->mFormatFlags;
			AVinfo.mBytesPerPacket = asbd->mBytesPerPacket;
			AVinfo.mFramesPerPacket = asbd->mFramesPerPacket;
			AVinfo.mBytesPerFrame = asbd->mBytesPerFrame;
			AVinfo.mReserved = asbd->mReserved;

			
			duration = mediaAsset.duration;
//			durationTime = CMTimeGetSeconds(duration);

		} else {
			startI = 0L;
			for (endI=startI; utterance->line[endI]; endI++) {
				if (utterance->line[endI] == HIDEN_C && (utterance->line[endI+1] == '%' || isdigit(utterance->line[endI+1]))) {
					if (utterance->line[endI+1] == '%') {
						if (getOLDMediaTagInfo(utterance->line+endI, NULL, mFName, &mBeg, &mEnd)) {
							fprintf(stderr, "\nError: Media bullet(s) in this file are in old format.\n");
							fprintf(stderr, "Please run \"fixbullets\" program to fix this data.\n\n");
							freeAllMemory(TRUE, &AVinfo);
						}
					} else if (getMediaTagInfo(utterance->line+endI, &mBeg, &mEnd)) {
						while (1) {
							while (uttline[startI] != EOS && uS.isskip(uttline,startI,&dFnt,MBF) && !uS.isRightChar(uttline,startI,'[',&dFnt,MBF))
								startI++;
							if (uttline[startI] == EOS || startI >= endI)
								break;
							startI = getword(utterance->speaker, uttline, word, NULL, startI);
							if (startI == 0)
								break;
							if (exclude(word)) {
								TimesList = AddToSilenceList(TimesList, mBeg, mEnd, &AVinfo);
								break;
							}
						}
						for (startI=endI+1; utterance->line[startI] && utterance->line[startI] != HIDEN_C ; startI++) ;
						if (utterance->line[startI] == EOS)
							break;
					}
				}
			}
		}
	}
	if (!ProcessSound(&AVinfo, TimesList)) {
	}
	freeAllMemory(FALSE, &AVinfo);
	TimesList = free_SilenceList(TimesList);
}
