/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.h
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#ifndef _MENU_H_
#define _MENU_H_

#include <ogcsys.h>

void InitGUIThreads();
void MainMenu(int menuitem);

enum {
    MENU_EXIT = -1,
    MENU_NONE,
    MENU_PRIMARY,
    MENU_EDIT_FIRST_NAME,
    MENU_EDIT_LAST_NAME,
    MENU_EDIT_EMAIL,
    MENU_CREDITS
};

#endif
