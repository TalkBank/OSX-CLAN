#ifndef MAC_DIAL
#define MAC_DIAL

extern "C"
{

enum {
	myOK=500,
	myNO,
	myCANCEL
} ;
	
extern char dial_isF1Pressed;
extern char dial_isF2Pressed;

extern char GetWindowBoolValue(WindowPtr myDlg, SInt32 id);
extern pascal short MyDlg(short item, DialogPtr theDialogPtr);
extern pascal ControlKeyFilterResult myStringFilter(ControlRef iCtrl, SInt16 *keyCode, SInt16 *charCode, EventModifiers *modifiers);
extern void SetWindowUnicodeTextValue(ControlRef iCtrl, char isClearAll, unCH *str, wchar_t c);
extern void SetDialogItemValue(WindowPtr myDlg, short itemID, short val);
extern void SetDialogItemUTF8(const char *text, WindowPtr myDlg, short itemID, char isWinLim);
extern void GetWindowUnicodeTextValue(ControlRef iCtrl,  unCH *str, int lenMax);
extern void SelectWindowItemText(WindowPtr myDlg, SInt32 id, SInt16 beg, SInt16 end);
extern void showWindow(WindowPtr win);

}

#endif // MAC_DIAL
