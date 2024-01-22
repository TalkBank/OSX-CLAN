
#import <Cocoa/Cocoa.h>

@interface EvaldController : NSWindowController <NSLayoutManagerDelegate, NSTableViewDelegate>
{
	IBOutlet NSButton *sp_oneCH;
	IBOutlet NSButton *sp_twoCH;
	IBOutlet NSButton *sp_threeCH;
	IBOutlet NSButton *sp_fourCH;
	IBOutlet NSButton *sp_fiveCH;
	IBOutlet NSButton *sp_sixCH;
	IBOutlet NSButton *sp_sevenCH;
	IBOutlet NSButton *sp_eightCH;

	IBOutlet NSButton *ControlCH;
	IBOutlet NSButton *MCICH;
	IBOutlet NSButton *MemoryCH;
	IBOutlet NSButton *PossibleADCH;
	IBOutlet NSButton *ProbableADCH;
	IBOutlet NSButton *VascularCH;

	IBOutlet NSTextField *AgeRangeField;

	IBOutlet NSButton *maleCH;
	IBOutlet NSButton *femaleCH;

	IBOutlet NSButton *CatCH;
	IBOutlet NSButton *CinderellaCH;
	IBOutlet NSButton *CookieCH;
	IBOutlet NSButton *HometownCH;
	IBOutlet NSButton *RockwellCH;
	IBOutlet NSButton *SandwichCH;
}

- (IBAction)sp_oneClicked:(id)sender;
- (IBAction)sp_twoClicked:(id)sender;
- (IBAction)sp_threeClicked:(id)sender;
- (IBAction)sp_fourClicked:(id)sender;
- (IBAction)sp_fiveClicked:(id)sender;
- (IBAction)sp_sixClicked:(id)sender;
- (IBAction)sp_sevenClicked:(id)sender;
- (IBAction)sp_eightClicked:(id)sender;

- (IBAction)DeselectDatabaseClicked:(id)sender;

- (IBAction)ControlClicked:(id)sender;
- (IBAction)MCIClicked:(id)sender;
- (IBAction)MemoryClicked:(id)sender;
- (IBAction)PossibleADClicked:(id)sender;
- (IBAction)ProbableADClicked:(id)sender;
- (IBAction)VascularClicked:(id)sender;

- (IBAction)maleClicked:(id)sender;
- (IBAction)femaleClicked:(id)sender;

- (IBAction)DeselectAllGemsClicked:(id)sender;
- (IBAction)SelectAllGemsClicked:(id)sender;

- (IBAction)CatClicked:(id)sender;
- (IBAction)CinderellaClicked:(id)sender;
- (IBAction)CookieClicked:(id)sender;
- (IBAction)HometownClicked:(id)sender;
- (IBAction)RockwellClicked:(id)sender;
- (IBAction)SandwichClicked:(id)sender;

- (IBAction)OKClicked:(id)sender;
- (IBAction)CancelClicked:(id)sender;

+ (const char *)EvaldDialog;

@end
