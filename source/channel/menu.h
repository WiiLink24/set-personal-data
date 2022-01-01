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
    MENU_INITIAL_PIN = -2,
    MENU_EXIT,
    MENU_NONE,
    MENU_PRIMARY,
    MENU_EDIT_FIRST_NAME,
    MENU_EDIT_LAST_NAME,
    MENU_EDIT_EMAIL_ADDRESS,
    MENU_EDIT_HOME_ADDRESS,
    MENU_EDIT_CITY,
    MENU_PHONE,
    MENU_PIN,
    MENU_EDIT_PIN,
    MENU_DELETE_PIN,
    MENU_ADD_PIN
   // MENU_CREDITS
};

#endif
