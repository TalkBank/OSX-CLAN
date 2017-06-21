/*
    File: mUPControl.h
    
    Description:
        Scrolling Text User Pane (mUP) control support routines.
	
	Routines defined in this header file are implemented in the
	file mUPControl.c
	
	These routines allow you to create (or use an existing) user
	pane control as a scrolling edit text field.
	
	These routines use the refcon field inside of the user pane
	record for storage of interal variables.  You should not
	use the reference value field in the user pane control if you
	are calling these routines.

    Copyright:
        © Copyright 2000 Apple Computer, Inc. All rights reserved.
    
    Disclaimer:
        IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
        ("Apple") in consideration of your agreement to the following terms, and your
        use, installation, modification or redistribution of this Apple software
        constitutes acceptance of these terms.  If you do not agree with these terms,
        please do not use, install, modify or redistribute this Apple software.

        In consideration of your agreement to abide by the following terms, and subject
        to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
        copyrights in this original Apple software (the "Apple Software"), to use,
        reproduce, modify and redistribute the Apple Software, with or without
        modifications, in source and/or binary forms; provided that if you redistribute
        the Apple Software in its entirety and without modifications, you must retain
        this notice and the following text and disclaimers in all such redistributions of
        the Apple Software.  Neither the name, trademarks, service marks or logos of
        Apple Computer, Inc. may be used to endorse or promote products derived from the
        Apple Software without specific prior written permission from Apple.  Except as
        expressly stated in this notice, no other rights or licenses, express or implied,
        are granted by Apple herein, including but not limited to any patent rights that
        may be infringed by your derivative works or by other works in which the Apple
        Software may be incorporated.

        The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
        WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
        WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
        PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
        COMBINATION WITH YOUR PRODUCTS.

        IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
        CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
        GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
        ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
        OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
        (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
        ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    Change History (most recent first):
        Fri, Jan 28, 2000 -- created
*/

#define TARGET_API_MAC_CARBON 1


#ifndef __mUPCONTROL__
#define __mUPCONTROL__


#define ACT_FUNC_INFO struct act_func_info
struct act_func_info {
    int (*act_func_ptr)(WindowPtr, EventRecord *, short);
    WindowPtr win;
	short id;
} ;

/* STPTextPaneVars is a structure used for storing the the mUP Control's
	internal variables and state information.  A handle to this record is
	stored in the pane control's reference value field using the
	SetControlReference routine. */

typedef struct {
		/* OS records referenced */
	TXNObject fTXNRec; /* the txn record */
	TXNFrameID fTXNFrame; /* the txn frame ID */
	ControlRef fUserPaneRec;  /* handle to the user pane control */
	WindowPtr fOwner; /* window containing control */
	CGrafPtr fDrawingEnvironment; /* grafport where control is drawn */
		/* flags */
	Boolean fInFocus; /* true while the focus rect is drawn around the control */
	Boolean fIsActive; /* true while the control is drawn in the active state */
	Boolean fTEActive; /* reflects the activation state of the text edit record */ 
	Boolean fInDialogWindow; /* true if displayed in a dialog window */ 
		/* calculated locations */
	Rect fRTextArea; /* area where the text is drawn */
	Rect fRFocusOutline;  /* rectangle used to draw the focus box */
	Rect fRTextOutline; /* rectangle used to draw the border */
	RgnHandle fTextBackgroundRgn; /* background region for the text, erased before calling TEUpdate */
		/* our focus advance override routine */
	EventHandlerUPP handlerUPP;
	EventHandlerRef handlerRef;
	ACT_FUNC_INFO *keyActions;
} STPTextPaneVars;


/* part codes */

/* kUserClickedToFocusPart is a part code we pass to the SetKeyboardFocus
	routine.  In our focus switching routine this part code is understood
	as meaning 'the user has clicked in the control and we need to switch
	the current focus to ourselves before we can continue'. */
#define kUserClickedToFocusPart 100


/* kmUPClickScrollDelayTicks is a time measurement in ticks used to
	slow the speed of 'auto scrolling' inside of our clickloop routine.
	This value prevents the text from wizzzzzing by while the mouse
	is being held down inside of the text area. */
#define kmUPClickScrollDelayTicks 3


/* kmUPTextPart is the part code we return to indicate the user has clicked
	in the text area of our control */
#define kmUPTextPart 1

/* kmUPScrollPart is the part code we return to indicate the user has clicked
	in the scroll bar part of the control. */
#define kmUPScrollPart 2


/* routines for using existing user pane controls.
	These routines are useful for cases where you would like to use an
	existing user pane control in, say, a dialog window as a scrolling
	text edit field.*/
	
/* mUPOpenControl initializes a user pane control so it will be drawn
	and will behave as a scrolling text edit field inside of a window.
	This routine performs all of the initialization steps necessary,
	except it does not create the user pane control itself.  theControl
	should refer to a user pane control that you have either created
	yourself or extracted from a dialog's control heirarchy using
	the GetDialogItemAsControl routine.  */
OSStatus mUPOpenControl(ControlRef theControl, short FName, short FSize, char isUTF, ACT_FUNC_INFO *keyActions);

/* mUPOpenStaticControl initializes a user pane control so it will be drawn
	and will behave as a scrolling text edit field inside of a window.
	This routine performs all of the initialization steps necessary,
	except it does not create the user pane control itself.  theControl
	should refer to a user pane control that you have either created
	yourself or extracted from a dialog's control heirarchy using
	the GetDialogItemAsControl routine.  */
OSStatus mUPOpenStaticControl(ControlRef theControl, short FName, short FSize, char isUTF);

/* mUPCloseControl deallocates all of the structures allocated
	by mUPOpenControl.  */
OSStatus mUPCloseControl(ControlRef theControl);



/* routines for creating new scrolling text user pane controls.
	These routines allow you to create new scrolling text
	user pane controls. */

/* mUPCreateControl creates a new user pane control and then it passes it
	to mUPOpenControl to initialize it as a scrolling text user pane control. */
OSStatus mUPCreateControl(WindowPtr theWindow, Rect *bounds, ControlRef *theControl);

/* mUPDisposeControl calls mUPCloseControl and then it calls DisposeControl. */
OSStatus mUPDisposeControl(ControlRef theControl);


/* Utility Routines */

	/* mUPSetText replaces the contents of the selection with the unicode
	text described by the text and count parameters:.
		text = pointer to unicode text buffer
		count = number of bytes in the buffer.  */
OSStatus mUPSetText(ControlRef theControl, char* text, long count);

/* mUPGetText returns the current text data being displayed inside of
	the mUPControl.  When noErr is returned, *theText contain a new
	handle containing all of the Unicode text copied from the current
	selection.  It is the caller's responsibiliby to dispose of this handle. */
OSStatus mUPGetText(ControlRef theControl, Handle *theText);


/* mUPSetSelection sets the text selection and autoscrolls the text view
	so either the cursor or the selction is in the view. */
void mUPSetSelection(ControlRef theControl, long selStart, long selEnd);



/* IsmUPControl returns true if theControl is not NULL
	and theControl refers to a mUP Control.  */
Boolean IsmUPControl(ControlRef theControl);



/* Edit commands for mUP Controls. */
enum {
	kmUPCut = 1,
	kmUPCopy = 2,
	kmUPPaste = 3,
	kmUPClear = 4
};


/* mUPDoEditCommand performs the editing command specified
	in the editCommand parameter.  The mUPControl's text
	and scroll bar are redrawn and updated as necessary. */
void mUPDoEditCommand(ControlRef theControl, short editCommand);




/* mUPGetContents returns the entire contents of the control including the text
	and the formatting information. */
OSStatus mUPGetContents(ControlRef theControl, Handle *theContents);
/* mUPSetContents replaces the contents of the selection with the data stored in the handle. */
OSStatus mUPSetContents(ControlRef theControl, Handle theContents);

extern short IsArrowKeyCode(short code);
extern void GetMLTETextSelection(ControlRef iCtrl, ControlEditTextSelectionRec *selection);
extern void SetMLTETextSelection(ControlRef iCtrl, ControlEditTextSelectionRec *selection);
extern int  AgmentCurrentMLTEText(ControlRef iCtrl, EventRecord *event, char *str, short var);
extern void GetWindowMLTEText(WindowPtr win, SInt32 id, int maxLen, char *text);
extern void SetWindowMLTEText(const char *text, WindowPtr win, SInt32 id);
extern void SelectWindowMLTEText(WindowPtr win, SInt32 id, SInt16 beg, SInt16 end);

#endif

