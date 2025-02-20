//
//  NSWindowController+CLANOptions.m
//  Clan
//

#import "ced.h"
#import "cu.h"
#import "KidevalController.h"
#import "CommandsController.h"

#define SP_CODE_FIRST	  1
#define SP_CODE_LAST	  8

#define CODESTRLEN  6

extern int  F_numfiles;
extern int  cl_argc;
extern char *cl_argv[];
extern char wd_dir[];

static int  m_AgeAuto;
static char IndyAgeTp, OneHTp, TwoTp, TwoHTp, ThreeTp, ThreeHTp, FourTp, FourHTp, FiveTp, FiveHTp,
	MaleOnly, FemaleOnly, BothGen,
	Lang_EngTp, Lang_EngUTp, Lang_FraTp, Lang_SpaTp, Lang_JpnTp, Lang_YueTp, Lang_ZhoTp,
	NotCmpDBTp, CmpDBTp, LinkAgeTp,
	DBEngToyplayTp, DBEngNarrativeTp,
	DBEngUToyplayTp,DBEngUNarrativeTp,
	DBZhoToyplayTp, DBZhoNarrativeTp,
	DBNldToyplayTp,
	DBFraToyplayTp, DBFraNarrativeTp,
	DBJpnToyplayTp,
	DBSpaToyplayTp, DBSpaNarrativeTp;
static char spCodeKEv[SP_CODE_LAST][CODESTRLEN + 1];
static BOOL spSetKEv[SP_CODE_LAST];



void InitKidevalOptions(void) {
	DBEngToyplayTp = TRUE;
	DBEngNarrativeTp = FALSE;
	DBEngUToyplayTp = FALSE;
	DBEngUNarrativeTp = FALSE;
	DBZhoToyplayTp = FALSE;
	DBZhoNarrativeTp = FALSE;
	DBNldToyplayTp = FALSE;
	DBFraToyplayTp = FALSE;
	DBFraNarrativeTp = FALSE;
	DBJpnToyplayTp = FALSE;
	DBSpaToyplayTp = FALSE;
	DBSpaNarrativeTp = FALSE;
	LinkAgeTp = TRUE;

	NotCmpDBTp = FALSE;
	CmpDBTp = FALSE;

	IndyAgeTp = FALSE;
	OneHTp = FALSE;
	TwoTp = FALSE;
	TwoHTp = FALSE;
	ThreeTp = FALSE;
	ThreeHTp = FALSE;
	FourTp = FALSE;
	FourHTp = FALSE;
	FiveTp = FALSE;
	FiveHTp = FALSE;

	MaleOnly = FALSE;
	FemaleOnly = FALSE;
	BothGen = TRUE;
}

static void addToSpArr(char *code) {
	int i;

	for (i=SP_CODE_FIRST-1; i < SP_CODE_LAST; i++) {
		if (spCodeKEv[i][0] == EOS)
			break;
		if (uS.mStricmp(spCodeKEv[i], code) == 0) {
			return;
		}
	}
	if (i < SP_CODE_LAST) {
		strncpy(spCodeKEv[i], code, CODESTRLEN);
		spCodeKEv[i][CODESTRLEN] = EOS;
	}
}

static char getSpNamesFromFile(int *cnt, char *fname, char isFoundFile) {
	int agef, aget;
	char	*s, *e, *code, *age;
	FILE	*fp;

	fp = fopen(fname, "r");
	if (fp == NULL)
		return(isFoundFile);
	//	*cnt = *cnt + 1;
	while (fgets_cr(templineC3, UTTLINELEN, fp)) {
		if (templineC3[0] == '*') {
			if (isFoundFile)
				break;
			s = strchr(templineC3, ':');
			if (s != NULL) {
				*s = EOS;
				addToSpArr(templineC3);
				isFoundFile = TRUE;
			}
		} else if (strncmp(templineC3, "@Languages:",11) == 0) {
		} else if (strncmp(templineC3, "@ID:", 4) == 0) {
			s = templineC3 + 4;
			while (isSpace(*s))
				s++;
			code = s;
			s = strchr(templineC3, '|');
			if (s != NULL) {
				if (Lang_EngTp == FALSE && Lang_EngUTp == FALSE && Lang_FraTp == FALSE && Lang_SpaTp == FALSE &&
					Lang_JpnTp == FALSE && Lang_YueTp == FALSE && Lang_ZhoTp == FALSE) {
					*s = EOS;
					e = strchr(code, ',');
					if (e == NULL) {
						e = strchr(code, ' ');
						if (e != NULL)
							*e = EOS;
					} else
						*e = EOS;
					if (uS.mStricmp(code, "eng") == 0) {
						Lang_EngTp = TRUE;
					} else if (uS.mStricmp(code, "engu") == 0) {
						Lang_EngUTp = TRUE;
					} else if (uS.mStricmp(code, "fra") == 0) {
						Lang_FraTp = TRUE;
					} else if (uS.mStricmp(code, "spa") == 0) {
						Lang_SpaTp = TRUE;
					} else if (uS.mStricmp(code, "jpn") == 0) {
						Lang_JpnTp = TRUE;
					} else if (uS.mStricmp(code, "yue") == 0) {
						Lang_YueTp = TRUE;
					} else if (uS.mStricmp(code, "zho") == 0) {
						Lang_ZhoTp = TRUE;
					}
				}
				s++;
				code = strchr(s, '|');
				if (code != NULL) {
					*code = '*';
					s = strchr(code, '|');
					if (s != NULL) {
						isFoundFile = TRUE;
						*s = EOS;
						uS.remblanks(code);
						addToSpArr(code);
						age = s + 1;
						s = strchr(age, '|');
						if (s != NULL) {
							*s = EOS;
							uS.remblanks(age);
							if (age[0] != EOS && uS.mStricmp(code, "*CHI") == 0 && isAge(age, &agef, &aget)) {
								if (m_AgeAuto == 0) {
									m_AgeAuto = agef;
								}
							}
						}
					}
				}
			}
		}
	}
	fclose(fp);
	return(isFoundFile);
}

static char makeSpeakersList(void) {
	int		i, j, len, cnt;
	char	isFoundFile;
	FILE	*fp;

	SetNewVol(wd_dir);
	isFoundFile = FALSE;
	cnt = 0;
	for (i=1; i < cl_argc; i++) {
		if (cl_argv[i][0] == '-' || cl_argv[i][0] == '+') {
		} else {
			if (!strcmp(cl_argv[i], "@")) {
				for (j=1; j <= F_numfiles; j++) {
					get_selected_file(j, FileName1, FNSize);
					isFoundFile = getSpNamesFromFile(&cnt, FileName1, isFoundFile);
				}
			} else if (cl_argv[i][0] == '@' && cl_argv[i][1] == ':') {
				uS.str2FNType(FileName1, 0L, cl_argv[i]+2);
				fp = fopen(FileName1, "r");
				if (fp == NULL) {
					return(isFoundFile);
				}
				while (fgets_cr(FileName1, FNSize, fp)) {
					uS.remFrontAndBackBlanks(FileName1);
					isFoundFile = getSpNamesFromFile(&cnt, FileName1, isFoundFile);
				}
				fclose(fp);
			} else if (strchr(cl_argv[i], '*') == NULL) {
				isFoundFile = getSpNamesFromFile(&cnt, cl_argv[i], isFoundFile);
			} else {
				strcpy(DirPathName, wd_dir);
				len = strlen(DirPathName);
				j = 1;
				while ((j=Get_File(FileName1, j)) != 0) {
					if (uS.fIpatmat(FileName1, cl_argv[i])) {
						addFilename2Path(DirPathName, FileName1);
						isFoundFile = getSpNamesFromFile(&cnt, DirPathName, isFoundFile);
						DirPathName[len] = EOS;
					}
				}
			}
		}
	}
	return(isFoundFile);
}

@implementation KidevalController

static KidevalController *KidevalWindow = nil;

- (id)init {
	KidevalWindow = nil;
	return [super initWithWindowNibName:@"Kideval"];
}

+ (const char *)KidevalDialog;
{
	int	i;
//	char *s;
//	NSString *comStr;
//	NSRect wFrame;

	NSLog(@"KidevalController: KidevalDialog\n");

	if (F_numfiles <= 0) {
		return("ERROR KidEVAL. Please specify input data files first");
	}
	cl_argc = 1;
	strcpy(templineC2, " @");
	if (!MakeArgs(templineC2))
		return("Internal Error!");
	if (KidevalWindow == nil) {
		Lang_EngTp = FALSE;
		Lang_EngUTp = FALSE;
		Lang_FraTp = FALSE;
		Lang_SpaTp = FALSE;
		Lang_JpnTp = FALSE;
		Lang_YueTp = FALSE;
		Lang_ZhoTp = FALSE;
		m_AgeAuto = 0;
		for (i=SP_CODE_FIRST-1; i < SP_CODE_LAST; i++) {
			spCodeKEv[i][0] = EOS;
			spSetKEv[i] = FALSE;
		}

		if (makeSpeakersList() == FALSE) {
			return("Can't open any of specified files. Please check for specified files in working directory");
		}

		if (CmpDBTp) {
			if (IndyAgeTp) {
				KidevalWindow = [[KidevalController alloc] initWithWindowNibName:@"KidevalDBAge"];
			} else {
				KidevalWindow = [[KidevalController alloc] initWithWindowNibName:@"KidevalDB"];
			}
		} else if (NotCmpDBTp)
			KidevalWindow = [[KidevalController alloc] initWithWindowNibName:@"KidevalNoDB"];
		else
			KidevalWindow = [[KidevalController alloc] initWithWindowNibName:@"Kideval"];

//		[KidevalWindow showWindow:nil];
		[[commandsWindow window] beginSheet:[KidevalWindow window] completionHandler:nil];

//		[NSApplication
//		[[NSApplication sharedApplication] runModalForWindow:[KidevalWindow window]];
//		[NSApp runModalForWindow:[KidevalWindow window]];
	}
	return(NULL);
}

- (void)setButton:(NSButton *)tButton index:(int)i {
	if (spCodeKEv[i][0] != EOS) {
		tButton.title = [NSString stringWithUTF8String:spCodeKEv[i]];
		if (spSetKEv[i])
			tButton.state =   NSControlStateValueOn;
		else
			tButton.state =   NSControlStateValueOff;
		[tButton setHidden:NO];
	} else {
		tButton.state =   NSControlStateValueOff;
		[tButton setHidden:YES];
	}
}

- (void)windowDidLoad {
	int	i;
	NSWindow *window = [self window];

	[window setIdentifier:@"Kideval"];
//	[window setRestorationClass:[self class]];
	[super windowDidLoad];  // It's documented to do nothing, but still a good idea to invoke...
/*
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{//TODO
		[[NSApplication sharedApplication] runModalForWindow:self.window];
	});
*/
//	NSRunLoop *theRL = [NSRunLoop currentRunLoop];

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowWillClose:) name:NSWindowWillCloseNotification object:self.window];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidResize:) name:NSWindowDidResizeNotification object:self.window];

	for (i=SP_CODE_FIRST-1; i < SP_CODE_LAST; i++) {
		if (uS.mStricmp(spCodeKEv[i], "*CHI") == 0) {
			spSetKEv[i] = TRUE;
		} else {
			spSetKEv[i] = FALSE;
		}
	}
	[self setButton:sp_oneCH   index:0];
	[self setButton:sp_twoCH   index:1];
	[self setButton:sp_threeCH index:2];
	[self setButton:sp_fourCH  index:3];
	[self setButton:sp_fiveCH  index:4];
	[self setButton:sp_sixCH   index:5];
	[self setButton:sp_sevenCH index:6];
	[self setButton:sp_eightCH index:7];


	engCH.state = NSControlStateValueOff;
	engUCH.state = NSControlStateValueOff;
	fraCH.state = NSControlStateValueOff;
	spaCH.state = NSControlStateValueOff;
	jpnCH.state = NSControlStateValueOff;
	yueCH.state = NSControlStateValueOff;
	zhoCH.state = NSControlStateValueOff;
	if (Lang_EngTp)
		engCH.state = NSControlStateValueOn;
	else if (Lang_EngUTp)
		engUCH.state = NSControlStateValueOn;
	else if (Lang_FraTp)
		fraCH.state = NSControlStateValueOn;
	else if (Lang_SpaTp)
		spaCH.state = NSControlStateValueOn;
	else if (Lang_JpnTp)
		jpnCH.state = NSControlStateValueOn;
	else if (Lang_YueTp)
		yueCH.state = NSControlStateValueOn;
	else if (Lang_ZhoTp)
		zhoCH.state = NSControlStateValueOn;


	doNotCompareDB.state = NSControlStateValueOff;
	CompareDB.state      = NSControlStateValueOff;
	if (NotCmpDBTp)
		doNotCompareDB.state = NSControlStateValueOn;
	else if (CmpDBTp)
		CompareDB.state      = NSControlStateValueOn;


	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state   = NSControlStateValueOff;
	EngUNarrativeCH.state = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;
	if (DBEngToyplayTp)
		EngToyplayCH.state    = NSControlStateValueOn;
	else if (DBEngNarrativeTp)
		EngNarrativeCH.state  = NSControlStateValueOn;
	else if (DBEngUToyplayTp)
		EngUToyplayCH.state   = NSControlStateValueOn;
	else if (DBEngUNarrativeTp)
		EngUNarrativeCH.state = NSControlStateValueOn;
	else if (DBZhoToyplayTp)
		ZhoToyplayCH.state = NSControlStateValueOn;
	else if (DBZhoNarrativeTp)
		ZhoNarrativeCH.state = NSControlStateValueOn;
	else if (DBNldToyplayTp)
		NldToyplayCH.state = NSControlStateValueOn;
	else if (DBFraToyplayTp)
		FraToyplayCH.state = NSControlStateValueOn;
	else if (DBFraNarrativeTp)
		FraNarrativeCH.state = NSControlStateValueOn;
	else if (DBJpnToyplayTp)
		JpnToyplayCH.state = NSControlStateValueOn;
	else if (DBSpaToyplayTp)
		SpaToyplayCH.state = NSControlStateValueOn;
	else if (DBSpaNarrativeTp)
		SpaNarrativeCH.state = NSControlStateValueOn;


	maleCH.state = NSControlStateValueOff;
	femaleCH.state = NSControlStateValueOff;
	bothCH.state = NSControlStateValueOff;
	if (MaleOnly)
		maleCH.state = NSControlStateValueOn;
	else if (FemaleOnly)
		femaleCH.state = NSControlStateValueOn;
	else if (BothGen)
		bothCH.state = NSControlStateValueOn;

	if (IndyAgeTp)
		chooseAgeRangeCH.state = NSControlStateValueOn;
	else
		chooseAgeRangeCH.state = NSControlStateValueOff;


	OneHCH.state =   NSControlStateValueOff;
	TwoCH.state =    NSControlStateValueOff;
	TwoHCH.state =   NSControlStateValueOff;
	ThreeCH.state =  NSControlStateValueOff;
	ThreeHCH.state = NSControlStateValueOff;
	FourCH.state =   NSControlStateValueOff;
	FourHCH.state =  NSControlStateValueOff;
	FiveCH.state =   NSControlStateValueOff;
	FiveHCH.state =  NSControlStateValueOff;
	if (OneHTp)
		OneHCH.state =   NSControlStateValueOn;
	else if (TwoTp)
		TwoCH.state =    NSControlStateValueOn;
	else if (TwoHTp)
		TwoHCH.state =   NSControlStateValueOn;
	else if (ThreeTp)
		ThreeCH.state =  NSControlStateValueOn;
	else if (ThreeHTp)
		ThreeHCH.state = NSControlStateValueOn;
	else if (FourTp)
		FourCH.state =   NSControlStateValueOn;
	else if (FourHTp)
		FourHCH.state =  NSControlStateValueOn;
	else if (FiveTp)
		FiveCH.state =   NSControlStateValueOn;
	else if (FiveHTp)
		FiveHCH.state =   NSControlStateValueOn;
//	[self ResizeOptionsWindow];
}

- (void)setButtonShow:(NSButton *)tButton index:(int)i {
	if (spCodeKEv[i][0] != EOS) {
		[tButton setHidden:NO];
	} else {
		[tButton setHidden:YES];
	}
}

- (IBAction)doNotCompareDBClicked:(NSButton *)sender
{
#pragma unused (sender)
	NSLog(@"KidevalController: doNotCompareDBClicked\n");

	doNotCompareDB.state = NSControlStateValueOn;
	CompareDB.state = NSControlStateValueOff;
	NotCmpDBTp = TRUE;
	CmpDBTp = FALSE;
	[[self window] close];
	[KidevalController KidevalDialog];
//	[self ResizeOptionsWindow];
}

- (IBAction)CompareDBClicked:(NSButton *)sender
{
#pragma unused (sender)
	NSLog(@"KidevalController: CompareDBClicked\n");

	doNotCompareDB.state = NSControlStateValueOff;
	CompareDB.state = NSControlStateValueOn;
	NotCmpDBTp = FALSE;
	CmpDBTp = TRUE;
	[[self window] close];
	[KidevalController KidevalDialog];
//	[self ResizeOptionsWindow];
}


- (IBAction)sp_oneClicked:(NSButton *)sender
{
	int i;
#pragma unused (sender)

	sp_oneCH.state =   NSControlStateValueOn;
	sp_twoCH.state =   NSControlStateValueOff;
	sp_threeCH.state = NSControlStateValueOff;
	sp_fourCH.state =  NSControlStateValueOff;
	sp_fiveCH.state =  NSControlStateValueOff;
	sp_sixCH.state =   NSControlStateValueOff;
	sp_sevenCH.state = NSControlStateValueOff;
	sp_eightCH.state = NSControlStateValueOff;
	for (i=SP_CODE_FIRST; i <= SP_CODE_LAST; i++) {
		spSetKEv[i-1] = FALSE;
	}
	spSetKEv[0] = TRUE;
}

- (IBAction)sp_twoClicked:(NSButton *)sender
{
	int i;
#pragma unused (sender)

	sp_oneCH.state =   NSControlStateValueOff;
	sp_twoCH.state =   NSControlStateValueOn;
	sp_threeCH.state = NSControlStateValueOff;
	sp_fourCH.state =  NSControlStateValueOff;
	sp_fiveCH.state =  NSControlStateValueOff;
	sp_sixCH.state =   NSControlStateValueOff;
	sp_sevenCH.state = NSControlStateValueOff;
	sp_eightCH.state = NSControlStateValueOff;
	for (i=SP_CODE_FIRST; i <= SP_CODE_LAST; i++) {
		spSetKEv[i-1] = FALSE;
	}
	spSetKEv[1] = TRUE;
}

- (IBAction)sp_threeClicked:(NSButton *)sender
{
	int i;
#pragma unused (sender)

	sp_oneCH.state =   NSControlStateValueOff;
	sp_twoCH.state =   NSControlStateValueOff;
	sp_threeCH.state = NSControlStateValueOn;
	sp_fourCH.state =  NSControlStateValueOff;
	sp_fiveCH.state =  NSControlStateValueOff;
	sp_sixCH.state =   NSControlStateValueOff;
	sp_sevenCH.state = NSControlStateValueOff;
	sp_eightCH.state = NSControlStateValueOff;
	for (i=SP_CODE_FIRST; i <= SP_CODE_LAST; i++) {
		spSetKEv[i-1] = FALSE;
	}
	spSetKEv[2] = TRUE;
}

- (IBAction)sp_fourClicked:(NSButton *)sender
{
	int i;
#pragma unused (sender)

	sp_oneCH.state =   NSControlStateValueOff;
	sp_twoCH.state =   NSControlStateValueOff;
	sp_threeCH.state = NSControlStateValueOff;
	sp_fourCH.state =  NSControlStateValueOn;
	sp_fiveCH.state =  NSControlStateValueOff;
	sp_sixCH.state =   NSControlStateValueOff;
	sp_sevenCH.state = NSControlStateValueOff;
	sp_eightCH.state = NSControlStateValueOff;
	for (i=SP_CODE_FIRST; i <= SP_CODE_LAST; i++) {
		spSetKEv[i-1] = FALSE;
	}
	spSetKEv[3] = TRUE;
}

- (IBAction)sp_fiveClicked:(NSButton *)sender
{
	int i;
#pragma unused (sender)

	sp_oneCH.state =   NSControlStateValueOff;
	sp_twoCH.state =   NSControlStateValueOff;
	sp_threeCH.state = NSControlStateValueOff;
	sp_fourCH.state =  NSControlStateValueOff;
	sp_fiveCH.state =  NSControlStateValueOn;
	sp_sixCH.state =   NSControlStateValueOff;
	sp_sevenCH.state = NSControlStateValueOff;
	sp_eightCH.state = NSControlStateValueOff;
	for (i=SP_CODE_FIRST; i <= SP_CODE_LAST; i++) {
		spSetKEv[i-1] = FALSE;
	}
	spSetKEv[4] = TRUE;
}

- (IBAction)sp_sixClicked:(NSButton *)sender
{
	int i;
#pragma unused (sender)

	sp_oneCH.state =   NSControlStateValueOff;
	sp_twoCH.state =   NSControlStateValueOff;
	sp_threeCH.state = NSControlStateValueOff;
	sp_fourCH.state =  NSControlStateValueOff;
	sp_fiveCH.state =  NSControlStateValueOff;
	sp_sixCH.state =   NSControlStateValueOn;
	sp_sevenCH.state = NSControlStateValueOff;
	sp_eightCH.state = NSControlStateValueOff;
	for (i=SP_CODE_FIRST; i <= SP_CODE_LAST; i++) {
		spSetKEv[i-1] = FALSE;
	}
	spSetKEv[5] = TRUE;
}

- (IBAction)sp_sevenClicked:(NSButton *)sender
{
	int i;
#pragma unused (sender)

	sp_oneCH.state =   NSControlStateValueOff;
	sp_twoCH.state =   NSControlStateValueOff;
	sp_threeCH.state = NSControlStateValueOff;
	sp_fourCH.state =  NSControlStateValueOff;
	sp_fiveCH.state =  NSControlStateValueOff;
	sp_sixCH.state =   NSControlStateValueOff;
	sp_sevenCH.state = NSControlStateValueOn;
	sp_eightCH.state = NSControlStateValueOff;
	for (i=SP_CODE_FIRST; i <= SP_CODE_LAST; i++) {
		spSetKEv[i-1] = FALSE;
	}
	spSetKEv[6] = TRUE;
}

- (IBAction)sp_eightClicked:(NSButton *)sender
{
	int i;
#pragma unused (sender)

	sp_oneCH.state =   NSControlStateValueOff;
	sp_twoCH.state =   NSControlStateValueOff;
	sp_threeCH.state = NSControlStateValueOff;
	sp_fourCH.state =  NSControlStateValueOff;
	sp_fiveCH.state =  NSControlStateValueOff;
	sp_sixCH.state =   NSControlStateValueOff;
	sp_sevenCH.state = NSControlStateValueOff;
	sp_eightCH.state = NSControlStateValueOn;
	for (i=SP_CODE_FIRST; i <= SP_CODE_LAST; i++) {
		spSetKEv[i-1] = FALSE;
	}
	spSetKEv[7] = TRUE;
}


- (IBAction)engClicked:(NSButton *)sender
{
#pragma unused (sender)

	engCH.state = NSControlStateValueOn;
	engUCH.state = NSControlStateValueOff;
	fraCH.state = NSControlStateValueOff;
	spaCH.state = NSControlStateValueOff;
	jpnCH.state = NSControlStateValueOff;
	yueCH.state = NSControlStateValueOff;
	zhoCH.state = NSControlStateValueOff;

	[[NSRunningApplication currentApplication] activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];

}

- (IBAction)engUClicked:(NSButton *)sender
{
#pragma unused (sender)

	engCH.state = NSControlStateValueOff;
	engUCH.state = NSControlStateValueOn;
	fraCH.state = NSControlStateValueOff;
	spaCH.state = NSControlStateValueOff;
	jpnCH.state = NSControlStateValueOff;
	yueCH.state = NSControlStateValueOff;
	zhoCH.state = NSControlStateValueOff;

	[[NSRunningApplication currentApplication] activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];

}

- (IBAction)fraClicked:(NSButton *)sender
{
#pragma unused (sender)

	engCH.state = NSControlStateValueOff;
	engUCH.state = NSControlStateValueOff;
	fraCH.state = NSControlStateValueOn;
	spaCH.state = NSControlStateValueOff;
	jpnCH.state = NSControlStateValueOff;
	yueCH.state = NSControlStateValueOff;
	zhoCH.state = NSControlStateValueOff;

	[[NSRunningApplication currentApplication] activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];

/*
	[spaCH setNextState];
 	[engCH setNextState];
	[engUCH setNextState];
	button.selected = Yes;
	button.highlighted = NO;
	button.enabled = Yes;

	[engCH setState:NSControlStateValueOff];
	[engUCH setState:NSControlStateValueOff];
 	[fraCH setState:NSControlStateValueOn];
	[spaCH setState:NSControlStateValueOff];
	[jpnCH setState:NSControlStateValueOff];
	[yueCH setState:NSControlStateValueOff];
	[zhoCH setState:NSControlStateValueOff];
*/
//	[[self window] displayIfNeeded];
}

- (IBAction)spaClicked:(NSButton *)sender
{
#pragma unused (sender)

	engCH.state = NSControlStateValueOff;
	engUCH.state = NSControlStateValueOff;
	fraCH.state = NSControlStateValueOff;
	spaCH.state = NSControlStateValueOn;
	jpnCH.state = NSControlStateValueOff;
	yueCH.state = NSControlStateValueOff;
	zhoCH.state = NSControlStateValueOff;

	[[NSRunningApplication currentApplication] activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];

}

- (IBAction)jpnClicked:(NSButton *)sender
{
#pragma unused (sender)

	engCH.state = NSControlStateValueOff;
	engUCH.state = NSControlStateValueOff;
	fraCH.state = NSControlStateValueOff;
	spaCH.state = NSControlStateValueOff;
	jpnCH.state = NSControlStateValueOn;
	yueCH.state = NSControlStateValueOff;
	zhoCH.state = NSControlStateValueOff;

	[[NSRunningApplication currentApplication] activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];

}

- (IBAction)yueClicked:(NSButton *)sender
{
#pragma unused (sender)

	engCH.state = NSControlStateValueOff;
	engUCH.state = NSControlStateValueOff;
	fraCH.state = NSControlStateValueOff;
	spaCH.state = NSControlStateValueOff;
	jpnCH.state = NSControlStateValueOff;
	yueCH.state = NSControlStateValueOn;
	zhoCH.state = NSControlStateValueOff;

	[[NSRunningApplication currentApplication] activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];

}

- (IBAction)zhoClicked:(NSButton *)sender
{
#pragma unused (sender)

	engCH.state = NSControlStateValueOff;
	engUCH.state = NSControlStateValueOff;
	fraCH.state = NSControlStateValueOff;
	spaCH.state = NSControlStateValueOff;
	jpnCH.state = NSControlStateValueOff;
	yueCH.state = NSControlStateValueOff;
	zhoCH.state = NSControlStateValueOn;

	[[NSRunningApplication currentApplication] activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];

}


- (IBAction)EngToyplayClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOn;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}

- (IBAction)EngNarrativeClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOn;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}
- (IBAction) EngUToyplayClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOn;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}

- (IBAction)EngUNarrativeClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOn;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}
- (IBAction)ZhoToyplayClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOn;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}

- (IBAction)ZhoNarrativeClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOn;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}

- (IBAction)NldToyplayClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOn;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}

- (IBAction)FraToyplayClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOn;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}

- (IBAction)FraNarrativeClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOn;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}

- (IBAction)JpnToyplayClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOn;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}

- (IBAction)SpaToyplayClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOn;
	SpaNarrativeCH.state = NSControlStateValueOff;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}

- (IBAction)SpaNarrativeClicked:(NSButton *)sender
{
#pragma unused (sender)

	EngToyplayCH.state    = NSControlStateValueOff;
	EngNarrativeCH.state  = NSControlStateValueOff;
	EngUToyplayCH.state    = NSControlStateValueOff;
	EngUNarrativeCH.state  = NSControlStateValueOff;
	ZhoToyplayCH.state = NSControlStateValueOff;
	ZhoNarrativeCH.state = NSControlStateValueOff;
	NldToyplayCH.state = NSControlStateValueOff;
	FraToyplayCH.state = NSControlStateValueOff;
	FraNarrativeCH.state = NSControlStateValueOff;
	JpnToyplayCH.state = NSControlStateValueOff;
	SpaToyplayCH.state = NSControlStateValueOff;
	SpaNarrativeCH.state = NSControlStateValueOn;

	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);
}


- (IBAction)maleClicked:(NSButton *)sender
{
#pragma unused (sender)

	maleCH.state = NSControlStateValueOn;
	femaleCH.state = NSControlStateValueOff;
	bothCH.state = NSControlStateValueOff;
}

- (IBAction)femaleClicked:(NSButton *)sender
{
#pragma unused (sender)

	maleCH.state = NSControlStateValueOff;
	femaleCH.state = NSControlStateValueOn;
	bothCH.state = NSControlStateValueOff;
}

- (IBAction)bothClicked:(NSButton *)sender
{
#pragma unused (sender)

	maleCH.state = NSControlStateValueOff;
	femaleCH.state = NSControlStateValueOff;
	bothCH.state = NSControlStateValueOn;
}


- (IBAction)chooseAgeRangeClicked:(NSButton *)sender
{
#pragma unused (sender)
	NSLog(@"KidevalController: chooseAgeRangeClicked\n");

	if (chooseAgeRangeCH.state == NSControlStateValueOn) {
		IndyAgeTp = TRUE;
	} else {
		IndyAgeTp = FALSE;
	}
	[[self window] close];
	[KidevalController KidevalDialog];
//	[self ResizeOptionsWindow];
}



- (IBAction)OneHClicked:(NSButton *)sender
{
#pragma unused (sender)
	OneHCH.state =   NSControlStateValueOn;
	TwoCH.state =    NSControlStateValueOff;
	TwoHCH.state =   NSControlStateValueOff;
	ThreeCH.state =  NSControlStateValueOff;
	ThreeHCH.state = NSControlStateValueOff;
	FourCH.state =   NSControlStateValueOff;
	FourHCH.state =  NSControlStateValueOff;
	FiveCH.state =   NSControlStateValueOff;
	FiveHCH.state =  NSControlStateValueOff;
}

- (IBAction)TwoClicked:(NSButton *)sender
{
#pragma unused (sender)
	OneHCH.state =   NSControlStateValueOff;
	TwoCH.state =    NSControlStateValueOn;
	TwoHCH.state =   NSControlStateValueOff;
	ThreeCH.state =  NSControlStateValueOff;
	ThreeHCH.state = NSControlStateValueOff;
	FourCH.state =   NSControlStateValueOff;
	FourHCH.state =  NSControlStateValueOff;
	FiveCH.state =   NSControlStateValueOff;
	FiveHCH.state =  NSControlStateValueOff;

}

- (IBAction)TwoHClicked:(NSButton *)sender
{
#pragma unused (sender)
	OneHCH.state =   NSControlStateValueOff;
	TwoCH.state =    NSControlStateValueOff;
	TwoHCH.state =   NSControlStateValueOn;
	ThreeCH.state =  NSControlStateValueOff;
	ThreeHCH.state = NSControlStateValueOff;
	FourCH.state =   NSControlStateValueOff;
	FourHCH.state =  NSControlStateValueOff;
	FiveCH.state =   NSControlStateValueOff;
	FiveHCH.state =  NSControlStateValueOff;

}

- (IBAction)ThreeClicked:(NSButton *)sender
{
#pragma unused (sender)
	OneHCH.state =   NSControlStateValueOff;
	TwoCH.state =    NSControlStateValueOff;
	TwoHCH.state =   NSControlStateValueOff;
	ThreeCH.state =  NSControlStateValueOn;
	ThreeHCH.state = NSControlStateValueOff;
	FourCH.state =   NSControlStateValueOff;
	FourHCH.state =  NSControlStateValueOff;
	FiveCH.state =   NSControlStateValueOff;
	FiveHCH.state =  NSControlStateValueOff;

}

- (IBAction)ThreeHClicked:(NSButton *)sender
{
#pragma unused (sender)
	OneHCH.state =   NSControlStateValueOff;
	TwoCH.state =    NSControlStateValueOff;
	TwoHCH.state =   NSControlStateValueOff;
	ThreeCH.state =  NSControlStateValueOff;
	ThreeHCH.state = NSControlStateValueOn;
	FourCH.state =   NSControlStateValueOff;
	FourHCH.state =  NSControlStateValueOff;
	FiveCH.state =   NSControlStateValueOff;
	FiveHCH.state =  NSControlStateValueOff;

}

- (IBAction)FourClicked:(NSButton *)sender
{
#pragma unused (sender)
	OneHCH.state =   NSControlStateValueOff;
	TwoCH.state =    NSControlStateValueOff;
	TwoHCH.state =   NSControlStateValueOff;
	ThreeCH.state =  NSControlStateValueOff;
	ThreeHCH.state = NSControlStateValueOff;
	FourCH.state =   NSControlStateValueOn;
	FourHCH.state =  NSControlStateValueOff;
	FiveCH.state =   NSControlStateValueOff;
	FiveHCH.state =  NSControlStateValueOff;

}

- (IBAction)FourHClicked:(NSButton *)sender
{
#pragma unused (sender)
	OneHCH.state =   NSControlStateValueOff;
	TwoCH.state =    NSControlStateValueOff;
	TwoHCH.state =   NSControlStateValueOff;
	ThreeCH.state =  NSControlStateValueOff;
	ThreeHCH.state = NSControlStateValueOff;
	FourCH.state =   NSControlStateValueOff;
	FourHCH.state =  NSControlStateValueOn;
	FiveCH.state =   NSControlStateValueOff;
	FiveHCH.state =  NSControlStateValueOff;

}

- (IBAction)FiveClicked:(NSButton *)sender
{
#pragma unused (sender)
	OneHCH.state =   NSControlStateValueOff;
	TwoCH.state =    NSControlStateValueOff;
	TwoHCH.state =   NSControlStateValueOff;
	ThreeCH.state =  NSControlStateValueOff;
	ThreeHCH.state = NSControlStateValueOff;
	FourCH.state =   NSControlStateValueOff;
	FourHCH.state =  NSControlStateValueOff;
	FiveCH.state =   NSControlStateValueOn;
	FiveHCH.state =  NSControlStateValueOff;

}

- (IBAction)FiveHClicked:(NSButton *)sender
{
#pragma unused (sender)
	OneHCH.state =   NSControlStateValueOff;
	TwoCH.state =    NSControlStateValueOff;
	TwoHCH.state =   NSControlStateValueOff;
	ThreeCH.state =  NSControlStateValueOff;
	ThreeHCH.state = NSControlStateValueOff;
	FourCH.state =   NSControlStateValueOff;
	FourHCH.state =  NSControlStateValueOff;
	FiveCH.state =   NSControlStateValueOff;
	FiveHCH.state =  NSControlStateValueOn;

}


//	WriteCedPreference();

- (IBAction)OKClicked:(NSButton *)sender
{
	int			i;
	const char *dbSt, *ageSt, *langSt, *genSt;
	char       *s;

#pragma unused (sender)
	NSLog(@"KidevalController: kidevalOKClicked\n");

	templineC3[0] = EOS;
	OneHTp = (OneHCH.state == NSControlStateValueOn);
	TwoTp = (TwoCH.state == NSControlStateValueOn);
	TwoHTp = (TwoHCH.state == NSControlStateValueOn);
	ThreeTp = (ThreeCH.state == NSControlStateValueOn);
	ThreeHTp = (ThreeHCH.state == NSControlStateValueOn);
	FourTp = (FourCH.state == NSControlStateValueOn);
	FourHTp = (FourHCH.state == NSControlStateValueOn);
	FiveTp = (FiveCH.state == NSControlStateValueOn);
	FiveHTp = (FiveHCH.state == NSControlStateValueOn);

	Lang_EngTp = (engCH.state == NSControlStateValueOn);
	Lang_EngUTp= (engUCH.state == NSControlStateValueOn);
	Lang_FraTp = (fraCH.state == NSControlStateValueOn);
	Lang_SpaTp = (spaCH.state == NSControlStateValueOn);
	Lang_JpnTp = (jpnCH.state == NSControlStateValueOn);
	Lang_YueTp = (yueCH.state == NSControlStateValueOn);
	Lang_ZhoTp = (zhoCH.state == NSControlStateValueOn);

	MaleOnly = (maleCH.state == NSControlStateValueOn);
	FemaleOnly = (femaleCH.state == NSControlStateValueOn);
	BothGen = (bothCH.state == NSControlStateValueOn);

	NotCmpDBTp = (doNotCompareDB.state == NSControlStateValueOn);
	CmpDBTp = (CompareDB.state == NSControlStateValueOn);
	DBEngToyplayTp = (EngToyplayCH.state == NSControlStateValueOn);
	DBEngNarrativeTp = (EngNarrativeCH.state == NSControlStateValueOn);
	DBEngUToyplayTp = (EngUToyplayCH.state == NSControlStateValueOn);
	DBEngUNarrativeTp = (EngUNarrativeCH.state == NSControlStateValueOn);
	DBZhoToyplayTp = (ZhoToyplayCH.state == NSControlStateValueOn);
	DBZhoNarrativeTp = (ZhoNarrativeCH.state == NSControlStateValueOn);
	DBNldToyplayTp = (NldToyplayCH.state == NSControlStateValueOn);
	DBFraToyplayTp = (FraToyplayCH.state == NSControlStateValueOn);
	DBFraNarrativeTp = (FraNarrativeCH.state == NSControlStateValueOn);
	DBJpnToyplayTp = (JpnToyplayCH.state == NSControlStateValueOn);
	DBSpaToyplayTp = (SpaToyplayCH.state == NSControlStateValueOn);
	DBSpaNarrativeTp = (SpaNarrativeCH.state == NSControlStateValueOn);

	IndyAgeTp = (chooseAgeRangeCH.state == NSControlStateValueOn);

	if (MaleOnly)
		genSt = "male";
	else if (FemaleOnly)
		genSt = "female";
	else
		genSt = "";

	if (CmpDBTp) {
		if (DBEngToyplayTp) {
			dbSt = "toyplay";
			langSt = "eng";
		} else if (DBEngNarrativeTp) {
			dbSt = "narrative";
			langSt = "eng";
		} else if (DBEngUToyplayTp) {
			dbSt = "toyplay";
			langSt = "engu";
		} else if (DBEngUNarrativeTp) {
			dbSt = "narrative";
			langSt = "engu";
		} else if (DBZhoToyplayTp) {
			dbSt = "toyplay";
			langSt = "zho";
		} else if (DBZhoNarrativeTp) {
			dbSt = "narrative";
			langSt = "zho";
		} else if (DBNldToyplayTp) {
			dbSt = "toyplay";
			langSt = "nld";
		} else if (DBJpnToyplayTp) {
			dbSt = "toyplay";
			langSt = "jpn";
		} else if (DBFraToyplayTp) {
			dbSt = "toyplay";
			langSt = "fra";
		} else if (DBFraNarrativeTp) {
			dbSt = "narrative";
			langSt = "fra";
		} else if (DBSpaToyplayTp) {
			dbSt = "toyplay";
			langSt = "spa";
		} else if (DBSpaNarrativeTp) {
			dbSt = "narrative";
			langSt = "spa";
		} else {
			do_warning_sheet("Please select one database", [self window]);
			return;
		}
		if (IndyAgeTp == FALSE) {
			if (m_AgeAuto >= 18 && m_AgeAuto <= 23) {
				ageSt = "1;6-1;11";
			} else if (m_AgeAuto >= 24 && m_AgeAuto <= 29) {
				ageSt = "2;-2;5";
			} else if (m_AgeAuto >= 30 && m_AgeAuto <= 35) {
				ageSt = "2;6-2;11";
			} else if (m_AgeAuto >= 36 && m_AgeAuto <= 41) {
				ageSt = "3;-3;5";
			} else if (m_AgeAuto >= 42 && m_AgeAuto <= 47) {
				ageSt = "3;6-3;11";
			} else if (m_AgeAuto >= 48 && m_AgeAuto <= 53) {
				ageSt = "4;-4;5";
			} else if (m_AgeAuto >= 54 && m_AgeAuto <= 59) {
				ageSt = "4;6-4;11";
			} else if (m_AgeAuto >= 60 && m_AgeAuto <= 65) {
				ageSt = "5;-5;5";
			} else if (m_AgeAuto >= 66 && m_AgeAuto <= 72) {
				ageSt = "5;6-6;";
			} else {
				do_warning("Can't find @ID tier for *CHI speaker that provides child's age in range of 1;6 to 6;. Please, double check input file(s).", 0);
				[[self window] close];
				return;
			}
		} else {
			if (OneHTp) {
				ageSt = "1;6-1;11";
			} else if (TwoTp) {
				ageSt = "2;-2;5";
			} else if (TwoHTp) {
				ageSt = "2;6-2;11";
			} else if (ThreeTp) {
				ageSt = "3;-3;5";
			} else if (ThreeHTp) {
				ageSt = "3;6-3;11";
			} else if (FourTp) {
				ageSt = "4;-4;5";
			} else if (FourHTp) {
				ageSt = "4;6-4;11";
			} else if (FiveTp) {
				ageSt = "5;-5;5";
			} else if (FiveHTp) {
				ageSt = "5;6-6;";
			} else {
				do_warning_sheet("Please select one Age Range", [self window]);
				return;
			}
		}

		if (IndyAgeTp == FALSE) {
			strcat(templineC3, " +LinkAge");
		}
		strcat(templineC3, " +l");
		strcat(templineC3, langSt);
		strcat(templineC3, " +t*CHI:");
		strcat(templineC3, " +d");
		strcat(templineC3, dbSt);
		strcat(templineC3, "~\"");
		strcat(templineC3, ageSt);
		if (genSt[0] != EOS) {
			strcat(templineC3, "|");
			strcat(templineC3, genSt);
		}
		strcat(templineC3, "\"");

	} else if (NotCmpDBTp) {
		genSt = ""; // no gender in "no database"

		for (i=SP_CODE_FIRST-1; i < SP_CODE_LAST; i++) {
			if (spCodeKEv[i][0] != EOS && spSetKEv[i] == TRUE)
				break;
		}
		if (i >= SP_CODE_LAST) {
			do_warning_sheet("Please select one Speaker", [self window]);
			return;
		}

		if (Lang_EngTp == TRUE) {
			langSt = "eng";
		} else if (Lang_EngUTp == TRUE) {
			langSt = "engu";
		} else if (Lang_FraTp == TRUE) {
			langSt = "fra";
		} else if (Lang_SpaTp == TRUE) {
			langSt = "spa";
		} else if (Lang_JpnTp == TRUE) {
			langSt = "jpn";
		} else if (Lang_YueTp == TRUE) {
			langSt = "yue";
		} else if (Lang_ZhoTp == TRUE) {
			langSt = "zho";
		} else {
			do_warning_sheet("Please select one Language", [self window]);
			return;
		}
		strcat(templineC3, " +l");
		strcat(templineC3, langSt);
		for (i=SP_CODE_FIRST-1; i < SP_CODE_LAST; i++) {
			if (spSetKEv[i] == TRUE && spCodeKEv[i][0] != EOS) {
				if (genSt[0] != EOS) {
					strcat(templineC3, " +t@ID=\"*|*|");
					if (spCodeKEv[i][0] == '*')
						strcat(templineC3, spCodeKEv[i] + 1);
					else
						strcat(templineC3, spCodeKEv[i]);
					strcat(templineC3, "|*|");
					strcat(templineC3, genSt);
					strcat(templineC3, "|*\"");
				} else {
					strcat(templineC3, " +t");
					strcat(templineC3, spCodeKEv[i]);
					strcat(templineC3, ":");
				}
			}
		}
	}
	if (commandsWindow != nil) {
		NSString *comStr;

		comStr = [commandsWindow->commandString stringValue];
		strncpy(spareTier3, [comStr UTF8String], UTTLINELEN);
		spareTier3[UTTLINELEN] = EOS;
		s = strchr(spareTier3, ' ');
		if (s != NULL)
			*s = EOS;
		comStr = [NSString stringWithUTF8String:spareTier3];
		comStr = [comStr stringByAppendingString:@" @"];
		comStr = [comStr stringByAppendingString:[NSString stringWithUTF8String:templineC3]];
		[commandsWindow->commandString setStringValue:comStr];
	}
//	[NSApp stopModalWithCode:NSModalResponseOK];
	[[self window] close];
}

- (IBAction)CancelClicked:(NSButton *)sender
{
#pragma unused (sender)
	NSLog(@"KidevalController: kidevalCancelClicked\n");

//	[NSApp stopModalWithCode:NSModalResponseCancel];
	[[self window] close];
}

- (void)windowWillClose:(NSNotification *)notification {
#pragma unused (notification)

	NSLog(@"KidevalController: windowWillClose\n");
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:self.window];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidResizeNotification object:self.window];

//	[[NSApplication sharedApplication] stopModal];
	[[commandsWindow window] endSheet:[self window]];

	[KidevalWindow release];
	KidevalWindow = nil;
}

- (void)windowDidResize:(NSNotification *)notification {// 2020-01-29
#pragma unused (notification)
	NSLog(@"KidevalController: windowDidResize\n");
}

// awakeFromNib is called when this object is done being unpacked from the nib file;
// at this point, we can do any needed initialization before turning app control over to the user
- (void)awakeFromNib
{
//	int i;

//	i = 12;
	// We don't actually need to do anything here, so it's empty
}


@end
