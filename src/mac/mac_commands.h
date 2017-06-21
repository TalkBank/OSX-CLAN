#ifndef _CLAN_MAC_COMMANDS_
#define _CLAN_MAC_COMMANDS_

#ifdef _CLAN_DEBUG
#define COMMANDS_TEST
#endif

enum {
	CANCEL,
	FILES_PAT,
	FILES_IN
};

extern "C"
{

extern char set_fbuffer(WindowPtr win, char isDel);
extern char isAtFound(char *s);
extern char isFilesSpecified(int *ProgNum);
extern void TiersDialog(WindowPtr win);
extern void EvalDialog(WindowPtr win);
extern void SetClanWinIcons(WindowPtr win, char *com);
extern void SearchDialog(WindowPtr win);
extern void AddToClan_commands(char *st);
extern void AddComStringToComWin(char *com, short show);
extern void OpenProgsWindow(void);
extern void OpenRecallWindow(void);
extern short SelectFilesDialog(char *additions);

}

#endif /* _CLAN_MAC_COMMANDS_ */
/*
#include "commands.h"
*/
