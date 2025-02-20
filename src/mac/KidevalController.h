
#import <Cocoa/Cocoa.h>

@interface KidevalController : NSWindowController <NSLayoutManagerDelegate, NSTableViewDelegate>
{
	IBOutlet NSTextField *CHOOSE_ONEField;
	IBOutlet NSButton *doNotCompareDB;
	IBOutlet NSButton *CompareDB;

	IBOutlet NSTextField *SEL_ONEField;
	IBOutlet NSTextField *SEE_SLPField;
	IBOutlet NSTextField *ChooseDBField;
	IBOutlet NSTextField *SpeakerField;
	IBOutlet NSTextField *LangugeField;
	IBOutlet NSTextField *AgeRangField;

	IBOutlet NSButton *sp_oneCH;
	IBOutlet NSButton *sp_twoCH;
	IBOutlet NSButton *sp_threeCH;
	IBOutlet NSButton *sp_fourCH;
	IBOutlet NSButton *sp_fiveCH;
	IBOutlet NSButton *sp_sixCH;
	IBOutlet NSButton *sp_sevenCH;
	IBOutlet NSButton *sp_eightCH;

	IBOutlet NSButton *engCH;
	IBOutlet NSButton *engUCH;
	IBOutlet NSButton *fraCH;
	IBOutlet NSButton *spaCH;
	IBOutlet NSButton *jpnCH;
	IBOutlet NSButton *yueCH;
	IBOutlet NSButton *zhoCH;

	IBOutlet NSButton *EngToyplayCH;
	IBOutlet NSButton *EngNarrativeCH;
	IBOutlet NSButton *EngUToyplayCH;
	IBOutlet NSButton *EngUNarrativeCH;
	IBOutlet NSButton *ZhoToyplayCH;
	IBOutlet NSButton *ZhoNarrativeCH;
	IBOutlet NSButton *NldToyplayCH;
	IBOutlet NSButton *FraToyplayCH;
	IBOutlet NSButton *FraNarrativeCH;
	IBOutlet NSButton *JpnToyplayCH;
	IBOutlet NSButton *SpaToyplayCH;
	IBOutlet NSButton *SpaNarrativeCH;

	IBOutlet NSButton *maleCH;
	IBOutlet NSButton *femaleCH;
	IBOutlet NSButton *bothCH;

	IBOutlet NSButton *chooseAgeRangeCH;

	IBOutlet NSButton *OneHCH;
	IBOutlet NSButton *TwoCH;
	IBOutlet NSButton *TwoHCH;
	IBOutlet NSButton *ThreeCH;
	IBOutlet NSButton *ThreeHCH;
	IBOutlet NSButton *FourCH;
	IBOutlet NSButton *FourHCH;
	IBOutlet NSButton *FiveCH;
	IBOutlet NSButton *FiveHCH;

	IBOutlet NSButton *OKbutton;
	IBOutlet NSButton *CancelButton;
}

- (IBAction)doNotCompareDBClicked:(id)sender;
- (IBAction)CompareDBClicked:(id)sender;

- (IBAction)sp_oneClicked:(id)sender;
- (IBAction)sp_twoClicked:(id)sender;
- (IBAction)sp_threeClicked:(id)sender;
- (IBAction)sp_fourClicked:(id)sender;
- (IBAction)sp_fiveClicked:(id)sender;
- (IBAction)sp_sixClicked:(id)sender;
- (IBAction)sp_sevenClicked:(id)sender;
- (IBAction)sp_eightClicked:(id)sender;

- (IBAction)engClicked:(id)sender;
- (IBAction)engUClicked:(id)sender;
- (IBAction)fraClicked:(id)sender;
- (IBAction)spaClicked:(id)sender;
- (IBAction)jpnClicked:(id)sender;
- (IBAction)yueClicked:(id)sender;
- (IBAction)zhoClicked:(id)sender;

- (IBAction)EngToyplayClicked:(id)sender;
- (IBAction)EngNarrativeClicked:(id)sender;
- (IBAction)EngUToyplayClicked:(id)sender;
- (IBAction)EngUNarrativeClicked:(id)sender;
- (IBAction)ZhoToyplayClicked:(id)sender;
- (IBAction)ZhoNarrativeClicked:(id)sender;
- (IBAction)NldToyplayClicked:(id)sender;
- (IBAction)FraToyplayClicked:(id)sender;
- (IBAction)FraNarrativeClicked:(id)sender;
- (IBAction)JpnToyplayClicked:(id)sender;
- (IBAction)SpaToyplayClicked:(id)sender;
- (IBAction)SpaNarrativeClicked:(id)sender;

- (IBAction)maleClicked:(id)sender;
- (IBAction)femaleClicked:(id)sender;
- (IBAction)bothClicked:(id)sender;

- (IBAction)chooseAgeRangeClicked:(id)sender;

- (IBAction)OneHClicked:(id)sender;
- (IBAction)TwoClicked:(id)sender;
- (IBAction)TwoHClicked:(id)sender;
- (IBAction)ThreeClicked:(id)sender;
- (IBAction)ThreeHClicked:(id)sender;
- (IBAction)FourClicked:(id)sender;
- (IBAction)FourHClicked:(id)sender;
- (IBAction)FiveClicked:(id)sender;
- (IBAction)FiveHClicked:(id)sender;

- (IBAction)OKClicked:(id)sender;
- (IBAction)CancelClicked:(id)sender;

+ (const char *)KidevalDialog;

@end
