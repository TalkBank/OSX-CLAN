#include "ced.h"

extern DialogPtr GetProgressDialog(StringPtr mess);
extern void UpdateProgressDialog(DialogPtr myDlg, short percentFilled);

DialogPtr GetProgressDialog(StringPtr mess) {
	GrafPtr		oldPort;
	DialogPtr	myDlg;
	
	myDlg = GetNewDialog(145, (void*)nil, (WindowPtr)-1);	
	if (!myDlg || MemError()) {
		if (myDlg)
			DisposeDialog(myDlg);
		return NULL;
	}
	ParamText(mess, 0L, 0L, 0L);
	GetPort(&oldPort);
	SetPort(myDlg);
	ShowWindow(myDlg);
	DrawDialog(myDlg);
	SetPort(oldPort);
	return myDlg;
}

void UpdateProgressDialog(DialogPtr	myDlg, short percentFilled) {
	Rect		box, fBox;
	Handle		hdl;
	short		type;
	RGBColor	rgbBlack 	= {0L, 0L, 0L};
	RGBColor 	rgbBarColor = {17476L, 17476L, 17476L};	
	RGBColor	oldForeColor;
	PenState	oldPen;
	GrafPtr		oldPort;
	short		pixelsDone;

	if (myDlg == NULL)
		return;
	
	GetPort(&oldPort);
	SetPort(myDlg);
	GetPenState(&oldPen);
	GetForeColor(&oldForeColor);

	PenNormal(); /* Reset the drawing pen */

	GetDialogItem(myDlg, 1, &type, &hdl, &box); /* Get rect of userItem */

	RGBForeColor(&rgbBlack);/* Frame progress bar */
	FrameRect(&box);

	InsetRect(&box, 1, 1); /* Setup rect for amount filled */
	fBox = box;
	pixelsDone = (box.right - box.left) * percentFilled / 100;
	fBox.right = fBox.left + pixelsDone;

	RGBForeColor(&rgbBarColor); /* Draw full part of the bar */
	PaintRect(&fBox);
	
	SetPenState(&oldPen);
	RGBForeColor(&oldForeColor);
	SetPort(oldPort);
}
