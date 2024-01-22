//
//  NSWindowController+CLANOptions.m
//  Clan
//

#import "ced.h"
#import "cu.h"
#import "EvaldController.h"
#import "CommandsController.h"

#define SP_CODE_FIRST	  1
#define SP_CODE_LAST	  8

#define CODESTRLEN  6

extern int  F_numfiles;
extern int  cl_argc;
extern char *cl_argv[];
extern char wd_dir[];

static char ControlTp, MCITp, MemoryTp, PossibleADTp, ProbableADTp, VascularTp,
	MaleOnly, FemaleOnly,
	CatGm, CinderellaGm, CookieGm, HometownGm, RockwellGm, SandwichGm;
static char AgeRange[256];
static char spCodeEv[SP_CODE_LAST][CODESTRLEN + 1];
static BOOL spSetEv[SP_CODE_LAST];

void InitEvaldOptions(void) {
	ControlTp = 0;
	MCITp = 0;
	MemoryTp = 0;
	PossibleADTp = 0;
	ProbableADTp = 0;
	VascularTp = 0;

	MaleOnly = 0;
	FemaleOnly = 0;
	
	CatGm = 0;
	CinderellaGm = 0;
	CookieGm = 0;
	HometownGm = 0;
	RockwellGm = 0;
	SandwichGm = 0;

	AgeRange[0] = EOS;
}

static void addToSpArr(char *code) {
	int i;

	for (i=SP_CODE_FIRST-1; i < SP_CODE_LAST; i++) {
		if (spCodeEv[i][0] == EOS)
			break;
		if (uS.mStricmp(spCodeEv[i], code) == 0) {
			return;
		}
	}
	if (i < SP_CODE_LAST) {
		strncpy(spCodeEv[i], code, CODESTRLEN);
		spCodeEv[i][CODESTRLEN] = EOS;
	}
}

static char getSpNamesFromFile(int *cnt, char *fname, char isFoundFile) {
	char *s, *code;
	FILE *fp;

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
		} else if (strncmp(templineC3, "@ID:", 4) == 0) {
			s = templineC3 + 4;
			while (isSpace(*s))
				s++;
			code = s;
			s = strchr(templineC3, '|');
			if (s != NULL) {
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

@implementation EvaldController

static EvaldController *EvalWindow = nil;

- (id)init {
	EvalWindow = nil;
	return [super initWithWindowNibName:@"EvalD"];
}

+ (const char *)EvaldDialog;
{
	int	i;
//	NSRect wFrame;

	NSLog(@"EvaldController: EvaldDialog\n");

	if (F_numfiles <= 0) {
		return("ERROR EVAL. Please specify input data files first");
	}
	cl_argc = 1;
	strcpy(templineC2, " @");
	if (!MakeArgs(templineC2))
		return("Internal Error!");
	if (EvalWindow == nil) {
		for (i=SP_CODE_FIRST-1; i < SP_CODE_LAST; i++) {
			spCodeEv[i][0] = EOS;
			spSetEv[i] = FALSE;
		}
		if (makeSpeakersList() == FALSE) {
			return("Can't open any of specified files. Please check for specified files in working directory");
		}
		EvalWindow = [[EvaldController alloc] initWithWindowNibName:@"EvalD"];

//		[EvalWindow showWindow:nil];
		[[commandsWindow window] beginSheet:[EvalWindow window] completionHandler:nil];
	}
	return(NULL);
}

- (void)setButton:(NSButton *)tButton index:(int)i {
	if (spCodeEv[i][0] != EOS) {
		tButton.title = [NSString stringWithUTF8String:spCodeEv[i]];
		if (spSetEv[i])
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

	[window setIdentifier:@"Eval"];
//	[window setRestorationClass:[self class]];
	[super windowDidLoad];  // It's documented to do nothing, but still a good idea to invoke...

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowWillClose:) name:NSWindowWillCloseNotification object:self.window];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidResize:) name:NSWindowDidResizeNotification object:self.window];


	for (i=SP_CODE_FIRST-1; i < SP_CODE_LAST; i++) {
		if (uS.mStricmp(spCodeEv[i], "*PAR") == 0) {
			spSetEv[i] = TRUE;
		} else {
			spSetEv[i] = FALSE;
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

	[AgeRangeField setStringValue:[NSString stringWithUTF8String:AgeRange]];

	ControlCH.state =    NSControlStateValueOff;
	MCICH.state =    NSControlStateValueOff;
	MemoryCH.state =   NSControlStateValueOff;
	PossibleADCH.state =     NSControlStateValueOff;
	ProbableADCH.state =  NSControlStateValueOff;
	VascularCH.state =   NSControlStateValueOff;

	if (ControlTp != 0)
		ControlCH.state =  NSControlStateValueOn;
	if (MCITp != 0)
		MCICH.state =   NSControlStateValueOn;
	if (MemoryTp != 0)
		MemoryCH.state =  NSControlStateValueOn;
	if (PossibleADTp != 0)
		PossibleADCH.state = NSControlStateValueOn;
	if (ProbableADTp != 0)
		ProbableADCH.state =  NSControlStateValueOn;
	if (VascularTp != 0)
		VascularCH.state =   NSControlStateValueOn;

	maleCH.state = NSControlStateValueOff;
	femaleCH.state = NSControlStateValueOff;
	if (MaleOnly)
		maleCH.state = NSControlStateValueOn;
	else if (FemaleOnly)
		femaleCH.state = NSControlStateValueOn;

	CatCH.state =   NSControlStateValueOff;
	CinderellaCH.state =    NSControlStateValueOff;
	CookieCH.state =   NSControlStateValueOff;
	CinderellaCH.state =  NSControlStateValueOff;
	HometownCH.state = NSControlStateValueOff;
	RockwellCH.state =   NSControlStateValueOff;
	SandwichCH.state =  NSControlStateValueOff;
	if (CatGm != 0)
		CatCH.state =   NSControlStateValueOn;
	if (CinderellaGm != 0)
		CinderellaCH.state =   NSControlStateValueOn;
	if (CookieGm != 0)
		CookieCH.state =  NSControlStateValueOn;
	if (HometownGm != 0)
		HometownCH.state =  NSControlStateValueOn;
	if (RockwellGm != 0)
		RockwellCH.state = NSControlStateValueOn;
	if (SandwichGm != 0)
		SandwichCH.state =    NSControlStateValueOn;
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
		spSetEv[i-1] = FALSE;
	}
	spSetEv[0] = TRUE;
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
		spSetEv[i-1] = FALSE;
	}
	spSetEv[1] = TRUE;
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
		spSetEv[i-1] = FALSE;
	}
	spSetEv[2] = TRUE;
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
		spSetEv[i-1] = FALSE;
	}
	spSetEv[3] = TRUE;
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
		spSetEv[i-1] = FALSE;
	}
	spSetEv[4] = TRUE;
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
		spSetEv[i-1] = FALSE;
	}
	spSetEv[5] = TRUE;
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
		spSetEv[i-1] = FALSE;
	}
	spSetEv[6] = TRUE;
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
		spSetEv[i-1] = FALSE;
	}
	spSetEv[7] = TRUE;
}

- (IBAction)DeselectDatabaseClicked:(NSButton *)sender
{
#pragma unused (sender)
	ControlCH.state = NSControlStateValueOff;
	MCICH.state = NSControlStateValueOff;
	MemoryCH.state = NSControlStateValueOff;
	PossibleADCH.state = NSControlStateValueOff;
	ProbableADCH.state = NSControlStateValueOff;
	VascularCH.state = NSControlStateValueOff;
}

- (IBAction)ControlClicked:(NSButton *)sender
{
#pragma unused (sender)
	ControlCH.state = NSControlStateValueOn;
	MCICH.state = NSControlStateValueOff;
	MemoryCH.state = NSControlStateValueOff;
	PossibleADCH.state = NSControlStateValueOff;
	ProbableADCH.state = NSControlStateValueOff;
	VascularCH.state = NSControlStateValueOff;
}

- (IBAction)MCIClicked:(NSButton *)sender
{
#pragma unused (sender)
	ControlCH.state = NSControlStateValueOff;
	MCICH.state = NSControlStateValueOn;
	MemoryCH.state = NSControlStateValueOff;
	PossibleADCH.state = NSControlStateValueOff;
	ProbableADCH.state = NSControlStateValueOff;
	VascularCH.state = NSControlStateValueOff;
}

- (IBAction)MemoryClicked:(NSButton *)sender
{
#pragma unused (sender)
	ControlCH.state = NSControlStateValueOff;
	MCICH.state = NSControlStateValueOff;
	MemoryCH.state = NSControlStateValueOn;
	PossibleADCH.state = NSControlStateValueOff;
	ProbableADCH.state = NSControlStateValueOff;
	VascularCH.state = NSControlStateValueOff;
}

- (IBAction)PossibleADClicked:(NSButton *)sender
{
#pragma unused (sender)
	ControlCH.state = NSControlStateValueOff;
	MCICH.state = NSControlStateValueOff;
	MemoryCH.state = NSControlStateValueOff;
	PossibleADCH.state = NSControlStateValueOn;
	ProbableADCH.state = NSControlStateValueOff;
	VascularCH.state = NSControlStateValueOff;
}

- (IBAction)ProbableADClicked:(NSButton *)sender
{
#pragma unused (sender)
	ControlCH.state = NSControlStateValueOff;
	MCICH.state = NSControlStateValueOff;
	MemoryCH.state = NSControlStateValueOff;
	PossibleADCH.state = NSControlStateValueOff;
	ProbableADCH.state = NSControlStateValueOn;
	VascularCH.state = NSControlStateValueOff;
}

- (IBAction)VascularClicked:(NSButton *)sender
{
#pragma unused (sender)
	ControlCH.state = NSControlStateValueOff;
	MCICH.state = NSControlStateValueOff;
	MemoryCH.state = NSControlStateValueOff;
	PossibleADCH.state = NSControlStateValueOff;
	ProbableADCH.state = NSControlStateValueOff;
	VascularCH.state = NSControlStateValueOn;
}


- (IBAction)maleClicked:(NSButton *)sender
{
#pragma unused (sender)

	maleCH.state = NSControlStateValueOn;
	femaleCH.state = NSControlStateValueOff;
}

- (IBAction)femaleClicked:(NSButton *)sender
{
#pragma unused (sender)

	maleCH.state = NSControlStateValueOff;
	femaleCH.state = NSControlStateValueOn;
}


- (IBAction)DeselectAllGemsClicked:(NSButton *)sender
{
#pragma unused (sender)
	CatCH.state = NSControlStateValueOff;
	CinderellaCH.state = NSControlStateValueOff;
	CookieCH.state = NSControlStateValueOff;
	HometownCH.state = NSControlStateValueOff;
	RockwellCH.state = NSControlStateValueOff;
	SandwichCH.state = NSControlStateValueOff;
}

- (IBAction)SelectAllGemsClicked:(NSButton *)sender
{
#pragma unused (sender)
	CatCH.state = NSControlStateValueOn;
	CinderellaCH.state = NSControlStateValueOn;
	CookieCH.state = NSControlStateValueOn;
	HometownCH.state = NSControlStateValueOn;
	RockwellCH.state = NSControlStateValueOn;
	SandwichCH.state = NSControlStateValueOn;
}


- (IBAction)CatClicked:(NSButton *)sender
{
#pragma unused (sender)
	CatCH.state = NSControlStateValueOn;
	CinderellaCH.state = NSControlStateValueOff;
	CookieCH.state = NSControlStateValueOff;
	HometownCH.state = NSControlStateValueOff;
	RockwellCH.state = NSControlStateValueOff;
	SandwichCH.state = NSControlStateValueOff;
}

- (IBAction)CinderellaClicked:(NSButton *)sender
{
#pragma unused (sender)
	CatCH.state = NSControlStateValueOff;
	CinderellaCH.state = NSControlStateValueOn;
	CookieCH.state = NSControlStateValueOff;
	HometownCH.state = NSControlStateValueOff;
	RockwellCH.state = NSControlStateValueOff;
	SandwichCH.state = NSControlStateValueOff;
}

- (IBAction)CookieClicked:(NSButton *)sender
{
#pragma unused (sender)
	CatCH.state = NSControlStateValueOff;
	CinderellaCH.state = NSControlStateValueOff;
	CookieCH.state = NSControlStateValueOn;
	HometownCH.state = NSControlStateValueOff;
	RockwellCH.state = NSControlStateValueOff;
	SandwichCH.state = NSControlStateValueOff;
}

- (IBAction)HometownClicked:(NSButton *)sender
{
#pragma unused (sender)
	CatCH.state = NSControlStateValueOff;
	CinderellaCH.state = NSControlStateValueOff;
	CookieCH.state = NSControlStateValueOff;
	HometownCH.state = NSControlStateValueOn;
	RockwellCH.state = NSControlStateValueOff;
	SandwichCH.state = NSControlStateValueOff;
}

- (IBAction)RockwellClicked:(NSButton *)sender
{
#pragma unused (sender)
	CatCH.state = NSControlStateValueOff;
	CinderellaCH.state = NSControlStateValueOff;
	CookieCH.state = NSControlStateValueOff;
	HometownCH.state = NSControlStateValueOff;
	RockwellCH.state = NSControlStateValueOn;
	SandwichCH.state = NSControlStateValueOff;
}

- (IBAction)SandwichClicked:(NSButton *)sender
{
#pragma unused (sender)
	CatCH.state = NSControlStateValueOff;
	CinderellaCH.state = NSControlStateValueOff;
	CookieCH.state = NSControlStateValueOff;
	HometownCH.state = NSControlStateValueOff;
	RockwellCH.state = NSControlStateValueOff;
	SandwichCH.state = NSControlStateValueOn;
}

//	WriteCedPreference();

- (IBAction)OKClicked:(NSButton *)sender
{
	int			i;
	char		isGender;
	NSUInteger len;
	NSString *cStr;

#pragma unused (sender)
	NSLog(@"EvaldController: OKClicked\n");

	templineC3[0] = EOS;
	if (commandsWindow != nil) {
		ControlTp = (ControlCH.state == NSControlStateValueOn);
		MCITp = (MCICH.state == NSControlStateValueOn);
		MemoryTp = (MemoryCH.state == NSControlStateValueOn);
		PossibleADTp = (PossibleADCH.state == NSControlStateValueOn);
		ProbableADTp = (ProbableADCH.state == NSControlStateValueOn);
		VascularTp = (VascularCH.state == NSControlStateValueOn);
		
		MaleOnly = (maleCH.state == NSControlStateValueOn);
		FemaleOnly = (femaleCH.state == NSControlStateValueOn);
		
		CatGm = (CatCH.state == NSControlStateValueOn);
		CinderellaGm = (CinderellaCH.state == NSControlStateValueOn);
		CookieGm = (CookieCH.state == NSControlStateValueOn);
		HometownGm = (HometownCH.state == NSControlStateValueOn);
		RockwellGm = (RockwellCH.state == NSControlStateValueOn);
		SandwichGm = (SandwichCH.state == NSControlStateValueOn);

		cStr = [AgeRangeField stringValue];
		len = [cStr length];
		if (len > 256)
			len = 255;
		[cStr getCharacters:templineW range:NSMakeRange(0, len)];
		templineW[len] = EOS;
		if (templineW[0] != EOS)
			u_strcpy(AgeRange, templineW, 256);
		else
			AgeRange[0] = EOS;

		if (CatGm && CinderellaGm && CookieGm && HometownGm && RockwellGm && SandwichGm) {
			CatGm = CinderellaGm = CookieGm = HometownGm = RockwellGm = SandwichGm = 0;
		}

		for (i=SP_CODE_FIRST-1; i < SP_CODE_LAST; i++) {
			if (spSetEv[i] == TRUE && spCodeEv[i][0] != EOS) {
				strcat(templineC3, " +t");
				strcat(templineC3, spCodeEv[i]);
				strcat(templineC3, ":");
			}
		}

		if (MaleOnly)
			isGender = 1;
		else if (FemaleOnly)
			isGender = 2;
		else
			isGender = 0;
		if (ControlTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "Control");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (MCITp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "MCI");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (MemoryTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "Memory");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (PossibleADTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "PossibleAD");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (ProbableADTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "ProbableAD");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}
		if (VascularTp) {
			strcat(templineC3, " +d\"");
			strcat(templineC3, "Vascular");
			if (AgeRange[0] != EOS) {
				strcat(templineC3, "|");
				strcat(templineC3, AgeRange);
			}
			if (isGender == 1) {
				strcat(templineC3, "|");
				strcat(templineC3, "male");
			} else if (isGender == 2) {
				strcat(templineC3, "|");
				strcat(templineC3, "female");
			}
			strcat(templineC3, "\"");
		}

		if (CatGm) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Cat");
			strcat(templineC3, "\"");
		}
		if (CinderellaGm) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Cinderella");
			strcat(templineC3, "\"");
		}
		if (CookieGm) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Cookie");
			strcat(templineC3, "\"");
		}
		if (HometownGm) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Hometown");
			strcat(templineC3, "\"");
		}
		if (RockwellGm) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Rockwell");
			strcat(templineC3, "\"");
		}
		if (SandwichGm) {
			strcat(templineC3, " +g\"");
			strcat(templineC3, "Sandwich");
			strcat(templineC3, "\"");
		}

		strcat(templineC3, " +u");

//		cStr = [commandsWindow->commandString stringValue];
		cStr = [NSString stringWithUTF8String:"eval-d @"];
		cStr = [cStr stringByAppendingString:[NSString stringWithUTF8String:templineC3]];
		[commandsWindow->commandString setStringValue:cStr];
	}
	[[self window] close];
}

- (IBAction)CancelClicked:(NSButton *)sender
{
#pragma unused (sender)
	NSLog(@"EvaldController: CancelClicked\n");

	[[self window] close];
}

- (void)windowWillClose:(NSNotification *)notification {
#pragma unused (notification)

	NSLog(@"EvaldController: windowWillClose\n");
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:self.window];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidResizeNotification object:self.window];

	[[commandsWindow window] endSheet:[self window]];

	[EvalWindow release];
	EvalWindow = nil;
}

- (void)windowDidResize:(NSNotification *)notification {// 2020-01-29
#pragma unused (notification)
	NSLog(@"EvaldController: windowDidResize\n");
}

// awakeFromNib is called when this object is done being unpacked from the nib file;
// at this point, we can do any needed initialization before turning app control over to the user
- (void)awakeFromNib
{
	int i;

	i = 12;
	// We don't actually need to do anything here, so it's empty
}


@end
