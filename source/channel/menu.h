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

enum ExitType {
    POWER,
    WII_MENU,
    DEMAE,
    DIGICAM
};

void InitGUIThreads();
void MainMenu(int menuitem);
extern bool ExitRequested;
extern bool startApp;
extern ExitType exitType;

enum {
    MENU_INITIAL_PIN = -2,
    MENU_EXIT,
    MENU_NONE,
    MENU_OPTIONS1,
    MENU_OPTIONS2,
    MENU_EDIT_FIRST_NAME,
    MENU_EDIT_LAST_NAME,
    MENU_EDIT_EMAIL_ADDRESS,
    MENU_EDIT_ADDRESS,
    MENU_EDIT_HOME_ADDRESS,
    MENU_EDIT_APT_NUMBER,
    MENU_EDIT_ZIP_CODE,
    MENU_EDIT_CITY,
    MENU_PHONE,
    MENU_PIN,
    MENU_EDIT_PIN,
    MENU_DELETE_PIN,
    MENU_ADD_PIN,
    MENU_EDIT_STATE,
   // MENU_CREDITS
};

#endif
