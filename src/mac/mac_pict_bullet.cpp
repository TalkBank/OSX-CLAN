#include "ced.h"
#include "MMedia.h"

extern FNType Picture_Name_str[];

extern struct DefWin PictWinSize;

static short active = 0;
static GraphicsImportComponent gImporter = NULL;

static void CleanupPict(WindowPtr wind) {
	if (gImporter != NULL)
		CloseComponent(gImporter);
	gImporter = NULL;
	active = 0;
}

static void UpdatePict(WindowPtr win) {
    GrafPtr	oldPort;
	Rect	rect;

	GetPort(&oldPort);
    SetPortWindowPort(win);
	GetWindowPortBounds(win, &rect);
	EraseRect(&rect);
	if (gImporter != NULL)
		GraphicsImportDraw(gImporter);
	DrawGrowIcon(win);
	DrawControls(win); 		/* Draw all the controls */
	SetPort(oldPort);
}

int DisplayPhoto(FNType *fname) {
	double		  h, v, r;
	Rect		  myRect;
	FSSpec		  fss;
	FSRef		  ref;
	WindowPtr	  windPtr;
	unsigned long myCount,
				  myIndex;

	if (gImporter != NULL)
		CloseComponent(gImporter);
	my_FSPathMakeRef(fname, &ref); 
	FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, &fss, NULL);
	if (GetGraphicsImporterForFile(&fss, &gImporter) != noErr)
		return(0);
	if (OpenWindow(504, Picture_Name_str, 0L, false, 0, NULL, UpdatePict, CleanupPict)) {
		do_warning("Error opening picture window window", 0);
		return(1);
	}
	windPtr = FindAWindowNamed(Picture_Name_str);
	if (windPtr == NULL) {
		do_warning("Error opening picture window", 0);
		return(1);
	}
	if (GraphicsImportGetImageCount(gImporter, &myCount) != noErr)
		return(0);
	if (myCount < 1 || myCount > 1)
		return(0);
	myIndex = 1;
	for (myIndex = 1; myIndex <= myCount; myIndex++) {
		if (GraphicsImportSetImageIndex(gImporter, myIndex) != noErr)
			return(0);		
		if (GraphicsImportGetNaturalBounds(gImporter, &myRect) != noErr)
			return(0);		
// 28-6-01		MacOffsetRect(&myRect, 50, 50);
		h = myRect.right  - myRect.left;
		v = myRect.bottom - myRect.top;
		if (PictWinSize.width != 0) {
			if (h > PictWinSize.width) {
				r = (h / v);
				myRect.top = 0;
				myRect.right = PictWinSize.width;
				myRect.left = 0;
				h = myRect.right;
				h /= r;
				myRect.bottom = (int)h;
			}
		} else if (v > 240) {
			r = (h / v);
			myRect.top = 0;
			myRect.bottom = 240;
			myRect.left = 0;
			v = myRect.bottom;
			v *= r;
			myRect.right = (int)v;
		}
		GraphicsImportSetBoundsRect(gImporter, &myRect);
		SizeWindow(windPtr, myRect.right, myRect.bottom,true);
		GraphicsImportSetGWorld(gImporter, GetCWindowPort(windPtr), NULL);		
		GraphicsImportDraw(gImporter);
	}
	return(1);
}

void ResizePicture(WindowPtr PictWindow, short nh, short nv) {
	double	h, v, r;
	Rect	myRect;

	if (gImporter == NULL)
		return;

	if (GraphicsImportGetNaturalBounds(gImporter, &myRect) != noErr)
		return;		

	h = myRect.right  - myRect.left;
	v = myRect.bottom - myRect.top;
	r = (h / v);

	myRect.top = 0;
	myRect.right = nh;
	myRect.left = 0;
	h = myRect.right;
	h /= r;
	myRect.bottom = (int)h;

	GraphicsImportSetBoundsRect(gImporter, &myRect);
	SizeWindow(PictWindow, myRect.right, myRect.bottom,true);
	GraphicsImportSetGWorld(gImporter, GetCWindowPort(PictWindow), NULL);		
	GraphicsImportDraw(gImporter);

	PictWinSize.width  = myRect.right;
	PictWinSize.height = myRect.bottom;

	WriteCedPreference();
}

