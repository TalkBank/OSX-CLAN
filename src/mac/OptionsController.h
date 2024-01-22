//
//  NSWindowController+CLANOptions.h
//  Clan
//

#import <Cocoa/Cocoa.h>

@interface OptionsPanelController : NSWindowController
{
	IBOutlet NSTextField *disambiguateField;
	IBOutlet NSButton *NewfilePosSizeButton;
	IBOutlet NSButton *OpenCommandsButton;
	IBOutlet NSButton *RestoreCursorButton;
	IBOutlet NSButton *doMixedSTWaveButton;
	IBOutlet NSButton *CloseButton;
	IBOutlet NSPopUpButton *alphaPopUp;
	IBOutlet NSPopUpButton *colorPopUp;
	IBOutlet NSPopUpButton *AutoSaveDelayPopUp;
}

- (IBAction)optionsNewfilePosSizeClicked:(id)sender;
- (IBAction)optionsOpenCommandsClicked:(id)sender;
- (IBAction)optionsRestoreCursorClicked:(id)sender;
- (IBAction)optionsdoMixedSTWaveClicked:(id)sender;
- (IBAction)optionsCloseClicked:(id)sender;
- (IBAction)optionsAlphaClicked:(id)sender;
- (IBAction)optionsColorClicked:(id)sender;
- (IBAction)autosaveDelayClicked:(id)sender;

- (IBAction)resetOptions:(id)sender;

- (IBAction)disambiguateFieldChanged:(id)sender;

@end
