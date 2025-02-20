//
//  NSWindowController+CLANOptions.h
//  Clan
//

#import <Cocoa/Cocoa.h>

@interface OptionsPanelController : NSWindowController
{
	IBOutlet NSTextField *disambiguateField;
	IBOutlet NSButton *AutoSetWorkigDirButton;
	IBOutlet NSButton *NewfilePosSizeButton;
	IBOutlet NSButton *OpenCommandsButton;
	IBOutlet NSButton *RestoreCursorButton;
	IBOutlet NSButton *doMixedSTWaveButton;
	IBOutlet NSButton *CloseButton;
	IBOutlet NSPopUpButton *alphaPopUp;
	IBOutlet NSPopUpButton *highlightColorPopUp;
	IBOutlet NSPopUpButton *lemmasColorPopUp;
	IBOutlet NSPopUpButton *caretWidthPopUp;
	IBOutlet NSPopUpButton *AutoSaveDelayPopUp;
}

- (IBAction)optionsAutoSetWorkigDirClicked:(id)sender;
- (IBAction)optionsNewfilePosSizeClicked:(id)sender;
- (IBAction)optionsOpenCommandsClicked:(id)sender;
- (IBAction)optionsRestoreCursorClicked:(id)sender;
- (IBAction)optionsdoMixedSTWaveClicked:(id)sender;
- (IBAction)optionsCloseClicked:(id)sender;
- (IBAction)optionsAlphaClicked:(id)sender;
- (IBAction)optionsColorClicked:(id)sender;
- (IBAction)optionsLemmasColorClicked:(id)sender;
- (IBAction)optionsCaretWidthClicked:(id)sender;
- (IBAction)autosaveDelayClicked:(id)sender;

- (IBAction)resetOptions:(id)sender;

- (IBAction)disambiguateFieldChanged:(id)sender;

@end
