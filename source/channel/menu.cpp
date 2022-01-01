/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <unistd.h>

#include "gui/gui.h"
#include "gui/gettext.h"
#include "main.h"
#include "menu.h"
#include "pd_info.h"

#define THREAD_SLEEP 100
// 48 KiB was chosen after many days of testing.
// It horrifies the author.
#define GUI_STACK_SIZE 48 * 1024
#define _(string) gettext (string)

static GuiImageData *pointer[4];
static GuiImage *bgImg = NULL;
static GuiSound *bgMusic = NULL;
static GuiWindow *mainWindow = NULL;
static lwp_t guithread = LWP_THREAD_NULL;
static bool guiHalt = true;
static bool ExitRequested = false;

// From pd_info.cpp
extern struct PDInfoData currentData;

/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void ResumeGui() {
    guiHalt = false;
    LWP_ResumeThread(guithread);
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void HaltGui() {
    guiHalt = true;

    // wait for thread to finish
    while (!LWP_ThreadIsSuspended(guithread))
        usleep(THREAD_SLEEP);
}

/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
int WindowPrompt(const char *title, const char *msg, const char *btn1Label,
                 const char *btn2Label) {
    int choice = -1;

    GuiWindow promptWindow(448, 288);
    promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    promptWindow.SetPosition(0, -10);
    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);

    GuiImageData dialogBox(dialogue_box_png);
    GuiImage dialogBoxImg(&dialogBox);

    GuiText titleTxt(title, 26, (GXColor){0, 0, 0, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 40);
    GuiText msgTxt(msg, 22, (GXColor){0, 0, 0, 255});
    msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    msgTxt.SetPosition(0, -20);
    msgTxt.SetWrap(true, 400);

    GuiText btn1Txt(btn1Label, 22, (GXColor){0, 0, 0, 255});
    GuiImage btn1Img(&btnOutline);
    GuiImage btn1ImgOver(&btnOutlineOver);
    GuiButton btn1(btnOutline.GetWidth(), btnOutline.GetHeight());

    if (btn2Label) {
        btn1.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
        btn1.SetPosition(20, -25);
    } else {
        btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
        btn1.SetPosition(0, -25);
    }

    btn1.SetLabel(&btn1Txt);
    btn1.SetImage(&btn1Img);
    btn1.SetImageOver(&btn1ImgOver);
    btn1.SetSoundOver(&btnSoundOver);
    btn1.SetTrigger(&trigA);
    btn1.SetState(STATE_SELECTED);
    btn1.SetEffectGrow();

    GuiText btn2Txt(btn2Label, 22, (GXColor){0, 0, 0, 255});
    GuiImage btn2Img(&btnOutline);
    GuiImage btn2ImgOver(&btnOutlineOver);
    GuiButton btn2(btnOutline.GetWidth(), btnOutline.GetHeight());
    btn2.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    btn2.SetPosition(-20, -25);
    btn2.SetLabel(&btn2Txt);
    btn2.SetImage(&btn2Img);
    btn2.SetImageOver(&btn2ImgOver);
    btn2.SetSoundOver(&btnSoundOver);
    btn2.SetTrigger(&trigA);
    btn2.SetEffectGrow();

    promptWindow.Append(&dialogBoxImg);
    promptWindow.Append(&titleTxt);
    promptWindow.Append(&msgTxt);
    promptWindow.Append(&btn1);

    if (btn2Label)
        promptWindow.Append(&btn2);

    promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
    HaltGui();
    mainWindow->SetState(STATE_DISABLED);
    mainWindow->Append(&promptWindow);
    mainWindow->ChangeFocus(&promptWindow);
    ResumeGui();

    while (choice == -1) {
        usleep(THREAD_SLEEP);

        if (btn1.GetState() == STATE_CLICKED)
            choice = 1;
        else if (btn2.GetState() == STATE_CLICKED)
            choice = 0;
    }

    promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
    while (promptWindow.GetEffect() > 0)
        usleep(THREAD_SLEEP);
    HaltGui();
    mainWindow->Remove(&promptWindow);
    mainWindow->SetState(STATE_DEFAULT);
    ResumeGui();
    return choice;
}

/****************************************************************************
 * Message Window
 *
 * Displays a window with text for a specified number of seconds
 ***************************************************************************/
void MessageWindow(const char *title, const char *msg, u32 time) {
    GuiWindow promptWindow(448, 288);
    promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    promptWindow.SetPosition(0, -10);
    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);

    GuiImageData dialogBox(dialogue_box_png);
    GuiImage dialogBoxImg(&dialogBox);

    GuiText titleTxt(title, 26, (GXColor){0, 0, 0, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 40);
    GuiText msgTxt(msg, 22, (GXColor){0, 0, 0, 255});
    msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    msgTxt.SetPosition(0, -20);
    msgTxt.SetWrap(true, 400);

    promptWindow.Append(&dialogBoxImg);
    promptWindow.Append(&titleTxt);
    promptWindow.Append(&msgTxt);


    promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
    HaltGui();
    mainWindow->SetState(STATE_DISABLED);
    mainWindow->Append(&promptWindow);
    mainWindow->ChangeFocus(&promptWindow);
    ResumeGui();

    sleep(time);

    promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
    while (promptWindow.GetEffect() > 0)
        usleep(THREAD_SLEEP);
    HaltGui();
    mainWindow->Remove(&promptWindow);
    mainWindow->SetState(STATE_DEFAULT);
    ResumeGui();
}

/****************************************************************************
 * UpdateGUI
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/

static void *UpdateGUI(void *arg) {
    int i;

    while (1) {
        if (guiHalt) {
            LWP_SuspendThread(guithread);
        } else {
            UpdatePads();
            mainWindow->Draw();

            // so that player 1's cursor appears on top!
            for (i = 3; i >= 0; i--) {
                if (userInput[i].wpad->ir.valid)
                    Menu_DrawImg(userInput[i].wpad->ir.x - 48,
                                 userInput[i].wpad->ir.y - 48, 96, 96,
                                 pointer[i]->GetImage(),
                                 userInput[i].wpad->ir.angle, 1, 1, 255);
            }

            Menu_Render();

            for (i = 0; i < 4; i++)
                mainWindow->Update(&userInput[i]);

            if (ExitRequested) {
                for (i = 0; i <= 255; i += 15) {
                    mainWindow->Draw();
                    Menu_DrawRectangle(0, 0, screenwidth, screenheight,
                                       (GXColor){0, 0, 0, (u8)i}, 1);
                    Menu_Render();
                }
                ExitApp();
            }
        }
    }
    return NULL;
}

/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
static u8 *_gui_stack[GUI_STACK_SIZE] ATTRIBUTE_ALIGN(8);
void InitGUIThreads() {
    LWP_CreateThread(&guithread, UpdateGUI, NULL, _gui_stack, GUI_STACK_SIZE,
                     70);
}

// Opens a popup screen to choose which channel you would like to load
void Selection() {
    s32 digicam = ISFS_Open("/title/00010001/4843444a/content/00000019.app",
                            ISFS_OPEN_WRITE);

    s32 demae = ISFS_Open("/title/00010001/4843484a/content/00000001.app",
                          ISFS_OPEN_WRITE);

    if (digicam < 0 && demae < 0) {
        ExitApp();
    }

    if (digicam > 0 && demae < 0) {
        int result =
            WindowPrompt(_("Choose Channel"),
                         _("Please choose which channel you would like to load."),
                         _("Wii Menu"), "Digicam");
        if (result == 1) {
            ExitApp();
        } else {
            Exit_to_Digicam();
        }
    }

    if (digicam < 0 && demae > 0) {
        int result =
            WindowPrompt(_("Choose Channel"),
                         _("Please choose which channel you would like to load."),
                         _("Wii Menu"), "Demae");
        if (result == 1) {
            ExitApp();
        } else {
            Exit_to_Demae();
        }
    }

    if (digicam > 0 && demae > 0) {
        int result =
            WindowPrompt(_("Choose Channel"),
                         _("Please choose which channel you would like to load."),
                         "Digicam", "Demae");
        if (result == 1) {
            Exit_to_Digicam();
        } else {
            Exit_to_Demae();
        }
    }

    ISFS_Close(digicam);
    ISFS_Close(demae);
}

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
void OnScreenKeyboard(wchar_t *var, u16 maxlen, const char *name) {
    int save = -1;

    GuiKeyboard keyboard(var, maxlen);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);

    GuiText titleTxt(name, 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiText okBtnTxt("OK", 22, (GXColor){0, 0, 0, 255});
    GuiImage okBtnImg(&btnOutline);
    GuiImage okBtnImgOver(&btnOutlineOver);
    GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

    okBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    okBtn.SetPosition(-50, 25);

    okBtn.SetLabel(&okBtnTxt);
    okBtn.SetImage(&okBtnImg);
    okBtn.SetImageOver(&okBtnImgOver);
    okBtn.SetSoundOver(&btnSoundOver);
    okBtn.SetTrigger(&trigA);
    okBtn.SetEffectGrow();

    GuiText cancelBtnTxt(_("Cancel"), 22, (GXColor){0, 0, 0, 255});
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(50, 25);
    cancelBtn.SetLabel(&cancelBtnTxt);
    cancelBtn.SetImage(&cancelBtnImg);
    cancelBtn.SetImageOver(&cancelBtnImgOver);
    cancelBtn.SetSoundOver(&btnSoundOver);
    cancelBtn.SetTrigger(&trigA);
    cancelBtn.SetEffectGrow();

    keyboard.Append(&okBtn);
    keyboard.Append(&cancelBtn);

    HaltGui();
    mainWindow->SetState(STATE_DISABLED);
    mainWindow->Append(&keyboard);
    mainWindow->Append(&titleTxt);
    mainWindow->ChangeFocus(&keyboard);
    ResumeGui();

    while (save == -1) {
        usleep(THREAD_SLEEP);

        if (okBtn.GetState() == STATE_CLICKED)
            save = 1;
        else if (cancelBtn.GetState() == STATE_CLICKED)
            save = 0;
    }

    if (var == currentData.user_email_address) {
        if (wcsstr(keyboard.kbtextstr, L"@") == 0) {
            int result = WindowPrompt(_("Error"),
                                      _("You have entered an invalid email address. Please enter a valid one."),
                                      _("Retry"), _("Main Menu"));
            if (result == 1) {
                HaltGui();
                mainWindow->Remove(&keyboard);
                mainWindow->Remove(&titleTxt);
                mainWindow->SetState(STATE_DEFAULT);
                OnScreenKeyboard(var, maxlen, name);
            }
        }
    }

    if (wcslen(keyboard.kbtextstr) == 0) {
        int result = WindowPrompt(_("Error"),
                                  _("You cannot have an empty field. Either try again or return to the main menu."),
                                  _("Retry"), _("Main Menu"));
        if (result == 1) {
            HaltGui();
            mainWindow->Remove(&keyboard);
            mainWindow->Remove(&titleTxt);
            mainWindow->SetState(STATE_DEFAULT);
            OnScreenKeyboard(var, maxlen, name);
        } else {
            save = 0;
        }
    } else if (save) {
        swprintf(var, maxlen, L"%ls", keyboard.kbtextstr);
    }

    HaltGui();
    mainWindow->Remove(&keyboard);
    mainWindow->Remove(&titleTxt);
    mainWindow->SetState(STATE_DEFAULT);
    ResumeGui();
}

/****************************************************************************
 * Credits
 ***************************************************************************/
/*static int MenuCredits() {

    int menu = MENU_NONE;

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiImageData btnLargeOutline(button_large_png);
    GuiImageData btnLargeOutlineOver(button_large_over_png);

    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);

    GuiText titleTxt(_("Credits"), 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiText nameTxt1("-Spotlight", 28, (GXColor){255, 255, 255, 255});
    GuiText nameTxt2("-SketchMaster2001", 28, (GXColor){255, 255, 255, 255});
    nameTxt1.SetPosition(0, 100);
    nameTxt2.SetPosition(0, 150);
    nameTxt1.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    nameTxt2.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);

    GuiText saveBtnTxt(_("Back"), 22, (GXColor){0, 0, 0, 255});
    GuiImage saveBtnImg(&btnOutline);
    GuiImage saveBtnImgOver(&btnOutlineOver);
    GuiButton saveBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    saveBtn.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
    saveBtn.SetPosition(0, -15);
    saveBtn.SetLabel(&saveBtnTxt);
    saveBtn.SetImage(&saveBtnImg);
    saveBtn.SetImageOver(&saveBtnImgOver);
    saveBtn.SetSoundOver(&btnSoundOver);
    saveBtn.SetTrigger(&trigA);
    saveBtn.SetEffectGrow();

    GuiImage *logo = new GuiImage(new GuiImageData(logo_png));
    logo->SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
    logo->SetPosition(0, -150);

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    mainWindow->Append(logo);
    mainWindow->Append(&w);
    mainWindow->Append(&titleTxt);

    w.Append(&nameTxt1);
    w.Append(&nameTxt2);
    w.Append(&saveBtn);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (saveBtn.GetState() == STATE_CLICKED) {
            menu = MENU_PRIMARY;
        }
    }

    HaltGui();

    mainWindow->Remove(&w);
    mainWindow->Remove(&titleTxt);
    mainWindow->Remove(logo);
    return menu;
} */

/****************************************************************************
 * InitialPin
 *
 * Prompts the user to enter their 4-digit PIN if possible
 ***************************************************************************/
static int InitialPin() {
    int menu = MENU_NONE;

    if (!currentData.passwordProtected) {
        return MENU_PRIMARY;
    }

    MessageWindow(_("Set Personal Data"), _("Your personal data is PIN protected. Please enter your 4-digit PIN."), 3);

    // Place an empty wchar_t to satisfy the function
    wchar_t empty = L'\0';
    GuiNumberpad keyboard(&empty, 5);

    GuiText titleTxt(_("Input PIN"), 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);


    GuiText okBtnTxt(_("Enter"), 22, (GXColor){0, 0, 0, 255});
    GuiImage okBtnImg(&btnOutline);
    GuiImage okBtnImgOver(&btnOutlineOver);
    GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

    okBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    okBtn.SetPosition(-50, 25);

    okBtn.SetLabel(&okBtnTxt);
    okBtn.SetImage(&okBtnImg);
    okBtn.SetImageOver(&okBtnImgOver);
    okBtn.SetSoundOver(&btnSoundOver);
    okBtn.SetTrigger(&trigA);
    okBtn.SetEffectGrow();

    GuiText cancelBtnTxt(_("Exit"), 22, (GXColor){0, 0, 0, 255});
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(50, 25);
    cancelBtn.SetLabel(&cancelBtnTxt);
    cancelBtn.SetImage(&cancelBtnImg);
    cancelBtn.SetImageOver(&cancelBtnImgOver);
    cancelBtn.SetSoundOver(&btnSoundOver);
    cancelBtn.SetTrigger(&trigA);
    cancelBtn.SetEffectGrow();

    keyboard.Append(&okBtn);
    keyboard.Append(&cancelBtn);

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&titleTxt);
    w.Append(&keyboard);
    w.ChangeFocus(&keyboard);
    mainWindow->Append(&w);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (okBtn.GetState() == STATE_CLICKED) {
            if ((wcsstr(keyboard.kbtextstr, currentData.user_pin) == NULL)) {
                int result = WindowPrompt(_("Incorrect PIN"),
                                          _("You have entered the incorrect PIN. Either try again or return to the main menu."),
                                          _("Wii Menu"), _("Retry"));
                if (result == 1) {
                    ExitApp();
                } else {
                    swprintf(keyboard.kbtextstr, 1, L"\0");
                    keyboard.kbTextfield->SetText(keyboard.kbtextstr);
                }
            } else {
                menu = MENU_PRIMARY;
            }
        }
        else if (cancelBtn.GetState() == STATE_CLICKED) {
            ExitApp();
        }
    }

    HaltGui();
    mainWindow->Remove(&w);

    return menu;
}

/****************************************************************************
 * PinMenu
 ***************************************************************************/
static int PinMenu() {
    int menu = MENU_NONE;

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiImageData btnLargeOutline(button_large_png);
    GuiImageData btnLargeOutlineOver(button_large_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);

    GuiText titleTxt("PIN", 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiText addPinBtnTxt(_("Add PIN"), 22, (GXColor){0, 0, 0, 255});
    GuiImage addPinBtnImg(&btnLargeOutline);
    GuiImage addPinBtnImgOver(&btnLargeOutlineOver);
    GuiButton addPinBtn(btnLargeOutline.GetWidth(),
                           btnLargeOutline.GetHeight());
    addPinBtn.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    addPinBtn.SetPosition(0, 0);
    addPinBtn.SetLabel(&addPinBtnTxt);
    addPinBtn.SetImage(&addPinBtnImg);
    addPinBtn.SetImageOver(&addPinBtnImgOver);
    addPinBtn.SetSoundOver(&btnSoundOver);
    addPinBtn.SetTrigger(&trigA);
    addPinBtn.SetEffectGrow();

    GuiText editPinBtnTxt(_("Edit PIN"), 22, (GXColor){0, 0, 0, 255});
    GuiImage editPinBtnImg(&btnLargeOutline);
    GuiImage editPinBtnImgOver(&btnLargeOutlineOver);
    GuiButton editPinBtn(btnLargeOutline.GetWidth(),
                        btnLargeOutline.GetHeight());
    editPinBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    editPinBtn.SetPosition(0, 120);
    editPinBtn.SetLabel(&editPinBtnTxt);
    editPinBtn.SetImage(&editPinBtnImg);
    editPinBtn.SetImageOver(&editPinBtnImgOver);
    editPinBtn.SetSoundOver(&btnSoundOver);
    editPinBtn.SetTrigger(&trigA);
    editPinBtn.SetEffectGrow();

    GuiText deletePinBtnTxt(_("Delete PIN"), 22, (GXColor){0, 0, 0, 255});
    GuiImage deletePinBtnImg(&btnLargeOutline);
    GuiImage deletePinBtnImgOver(&btnLargeOutlineOver);
    GuiButton deletePinBtn(btnLargeOutline.GetWidth(),
                         btnLargeOutline.GetHeight());
    deletePinBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    deletePinBtn.SetPosition(0, 250);
    deletePinBtn.SetLabel(&deletePinBtnTxt);
    deletePinBtn.SetImage(&deletePinBtnImg);
    deletePinBtn.SetImageOver(&deletePinBtnImgOver);
    deletePinBtn.SetSoundOver(&btnSoundOver);
    deletePinBtn.SetTrigger(&trigA);
    deletePinBtn.SetEffectGrow();

    GuiText backBtnTxt(_("Back"), 22, (GXColor){0, 0, 0, 255});
    GuiImage backBtnImg(&btnOutline);
    GuiImage backBtnImgOver(&btnOutlineOver);
    GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    backBtn.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
    backBtn.SetPosition(0, -15);
    backBtn.SetLabel(&backBtnTxt);
    backBtn.SetImage(&backBtnImg);
    backBtn.SetImageOver(&backBtnImgOver);
    backBtn.SetSoundOver(&btnSoundOver);
    backBtn.SetTrigger(&trigA);
    backBtn.SetEffectGrow();

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    mainWindow->Append(&w);
    mainWindow->Append(&titleTxt);

    if (!currentData.passwordProtected)
        w.Append(&addPinBtn);
    else {
        w.Append(&editPinBtn);
        w.Append(&deletePinBtn);
    }

    w.Append(&backBtn);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (backBtn.GetState() == STATE_CLICKED) {
            menu = MENU_PRIMARY;
        } else if (editPinBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EDIT_PIN;
        } else if (deletePinBtn.GetState() == STATE_CLICKED) {
            menu = MENU_DELETE_PIN;
        } else if (addPinBtn.GetState() == STATE_CLICKED) {
            menu = MENU_ADD_PIN;
        }
    }

    HaltGui();

    mainWindow->Remove(&w);
    mainWindow->Remove(&titleTxt);
    return menu;
}

/****************************************************************************
 * AddPin
 *
 * Prompts the user to add a 4-digit PIN to protect their data
 ***************************************************************************/
static int AddPin() {
    int menu = MENU_NONE;

    MessageWindow(_("Add PIN"), _("Please enter a 4-digit PIN that you will remember."), 3);

    // Place an empty wchar_t to satisfy the function
    wchar_t empty = L'\0';
    GuiNumberpad keyboard(&empty, 5);

    GuiText titleTxt(_("Edit PIN"), 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);


    GuiText okBtnTxt(_("Enter"), 22, (GXColor){0, 0, 0, 255});
    GuiImage okBtnImg(&btnOutline);
    GuiImage okBtnImgOver(&btnOutlineOver);
    GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

    okBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    okBtn.SetPosition(-50, 25);

    okBtn.SetLabel(&okBtnTxt);
    okBtn.SetImage(&okBtnImg);
    okBtn.SetImageOver(&okBtnImgOver);
    okBtn.SetSoundOver(&btnSoundOver);
    okBtn.SetTrigger(&trigA);
    okBtn.SetEffectGrow();

    GuiText cancelBtnTxt(_("Back"), 22, (GXColor){0, 0, 0, 255});
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(50, 25);
    cancelBtn.SetLabel(&cancelBtnTxt);
    cancelBtn.SetImage(&cancelBtnImg);
    cancelBtn.SetImageOver(&cancelBtnImgOver);
    cancelBtn.SetSoundOver(&btnSoundOver);
    cancelBtn.SetTrigger(&trigA);
    cancelBtn.SetEffectGrow();

    keyboard.Append(&okBtn);
    keyboard.Append(&cancelBtn);

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&titleTxt);
    w.Append(&keyboard);
    w.ChangeFocus(&keyboard);
    mainWindow->Append(&w);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (okBtn.GetState() == STATE_CLICKED) {
            if (wcslen(keyboard.kbtextstr) != 4) {
                MessageWindow(_("Error"), _("You cannot have a PIN shorter than 4 numbers!"), 3);
                swprintf(keyboard.kbtextstr, 1, L"\0");
                keyboard.kbTextfield->SetText(keyboard.kbtextstr);
            } else {
                currentData.passwordProtected = true;
                swprintf(currentData.user_pin, 8, L"%ls", keyboard.kbtextstr);
                menu = MENU_PIN;
            }
        }
        else if (cancelBtn.GetState() == STATE_CLICKED) {
            menu = MENU_PIN;
        }
    }

    HaltGui();
    mainWindow->Remove(&w);

    return menu;
}

/****************************************************************************
 * DeletePin
 *
 * Prompts the user to delete their 4-digit PIN
 ***************************************************************************/
static int DeletePin() {
    int menu = MENU_NONE;

    MessageWindow(_("Delete PIN"), _("Please enter your 4-digit PIN to confirm."), 3);

    // Place an empty wchar_t to satisfy the function
    wchar_t empty = L'\0';
    GuiNumberpad keyboard(&empty, 5);

    GuiText titleTxt(_("Input PIN"), 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);


    GuiText okBtnTxt(_("Enter"), 22, (GXColor){0, 0, 0, 255});
    GuiImage okBtnImg(&btnOutline);
    GuiImage okBtnImgOver(&btnOutlineOver);
    GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

    okBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    okBtn.SetPosition(-50, 25);

    okBtn.SetLabel(&okBtnTxt);
    okBtn.SetImage(&okBtnImg);
    okBtn.SetImageOver(&okBtnImgOver);
    okBtn.SetSoundOver(&btnSoundOver);
    okBtn.SetTrigger(&trigA);
    okBtn.SetEffectGrow();

    GuiText cancelBtnTxt(_("Back"), 22, (GXColor){0, 0, 0, 255});
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(50, 25);
    cancelBtn.SetLabel(&cancelBtnTxt);
    cancelBtn.SetImage(&cancelBtnImg);
    cancelBtn.SetImageOver(&cancelBtnImgOver);
    cancelBtn.SetSoundOver(&btnSoundOver);
    cancelBtn.SetTrigger(&trigA);
    cancelBtn.SetEffectGrow();

    keyboard.Append(&okBtn);
    keyboard.Append(&cancelBtn);

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&titleTxt);
    w.Append(&keyboard);
    w.ChangeFocus(&keyboard);
    mainWindow->Append(&w);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (okBtn.GetState() == STATE_CLICKED) {
            if ((wcsstr(keyboard.kbtextstr, currentData.user_pin) == NULL)) {
                int result = WindowPrompt(_("Incorrect PIN"),
                                          _("You have entered the incorrect PIN. Either try again or return to the main menu."),
                                          _("Back"), _("Retry"));
                if (result == 1) {
                    menu = MENU_PIN;
                } else {
                    swprintf(keyboard.kbtextstr, 1, L"\0");
                    keyboard.kbTextfield->SetText(keyboard.kbtextstr);
                }
            } else {
                currentData.passwordProtected = false;
                menu = MENU_PRIMARY;
            }
        }
        else if (cancelBtn.GetState() == STATE_CLICKED) {
            menu = MENU_PIN;
        }
    }

    HaltGui();
    mainWindow->Remove(&w);

    return menu;
}

/****************************************************************************
 * EditPin
 *
 * Prompts the user to edit their 4-digit PIN if possible
 ***************************************************************************/
static int EditPin() {
    int menu = MENU_NONE;

    MessageWindow(_("Edit PIN"), _("Please enter a 4-digit PIN to replace your current one."), 3);

    // Place an empty wchar_t to satisfy the function
    wchar_t empty = L'\0';
    GuiNumberpad keyboard(&empty, 5);

    GuiText titleTxt(_("Edit PIN"), 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);


    GuiText okBtnTxt(_("Enter"), 22, (GXColor){0, 0, 0, 255});
    GuiImage okBtnImg(&btnOutline);
    GuiImage okBtnImgOver(&btnOutlineOver);
    GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

    okBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    okBtn.SetPosition(-50, 25);

    okBtn.SetLabel(&okBtnTxt);
    okBtn.SetImage(&okBtnImg);
    okBtn.SetImageOver(&okBtnImgOver);
    okBtn.SetSoundOver(&btnSoundOver);
    okBtn.SetTrigger(&trigA);
    okBtn.SetEffectGrow();

    GuiText cancelBtnTxt(_("Back"), 22, (GXColor){0, 0, 0, 255});
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(50, 25);
    cancelBtn.SetLabel(&cancelBtnTxt);
    cancelBtn.SetImage(&cancelBtnImg);
    cancelBtn.SetImageOver(&cancelBtnImgOver);
    cancelBtn.SetSoundOver(&btnSoundOver);
    cancelBtn.SetTrigger(&trigA);
    cancelBtn.SetEffectGrow();

    keyboard.Append(&okBtn);
    keyboard.Append(&cancelBtn);

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&titleTxt);
    w.Append(&keyboard);
    w.ChangeFocus(&keyboard);
    mainWindow->Append(&w);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (okBtn.GetState() == STATE_CLICKED) {
            if (wcslen(keyboard.kbtextstr) != 4) {
                MessageWindow(_("Error"), _("You cannot have a PIN shorter than 4 numbers!"), 3);
                swprintf(keyboard.kbtextstr, 1, L"\0");
                keyboard.kbTextfield->SetText(keyboard.kbtextstr);
            } else {
                swprintf(currentData.user_pin, 8, L"%ls", keyboard.kbtextstr);
                menu = MENU_PIN;
            }
        }
        else if (cancelBtn.GetState() == STATE_CLICKED) {
            menu = MENU_PIN;
        }
    }

    HaltGui();
    mainWindow->Remove(&w);

    return menu;
}

/****************************************************************************
 * PhoneNumber
 *
 * Prompts the user to enter their phone number
 ***************************************************************************/
static int PhoneNumber() {
    int menu = MENU_NONE;

    // Place an empty wchar_t to satisfy the function
    GuiNumberpad keyboard(currentData.user_phone_number, 33);

    GuiText titleTxt(_("Phone Number"), 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);


    GuiText okBtnTxt(_("Enter"), 22, (GXColor){0, 0, 0, 255});
    GuiImage okBtnImg(&btnOutline);
    GuiImage okBtnImgOver(&btnOutlineOver);
    GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

    okBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    okBtn.SetPosition(-50, 25);

    okBtn.SetLabel(&okBtnTxt);
    okBtn.SetImage(&okBtnImg);
    okBtn.SetImageOver(&okBtnImgOver);
    okBtn.SetSoundOver(&btnSoundOver);
    okBtn.SetTrigger(&trigA);
    okBtn.SetEffectGrow();

    GuiText cancelBtnTxt(_("Back"), 22, (GXColor){0, 0, 0, 255});
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(50, 25);
    cancelBtn.SetLabel(&cancelBtnTxt);
    cancelBtn.SetImage(&cancelBtnImg);
    cancelBtn.SetImageOver(&cancelBtnImgOver);
    cancelBtn.SetSoundOver(&btnSoundOver);
    cancelBtn.SetTrigger(&trigA);
    cancelBtn.SetEffectGrow();

    keyboard.Append(&okBtn);
    keyboard.Append(&cancelBtn);

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&titleTxt);
    w.Append(&keyboard);
    w.ChangeFocus(&keyboard);
    mainWindow->Append(&w);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (okBtn.GetState() == STATE_CLICKED) {
            swprintf(currentData.user_phone_number, 32, L"%ls", keyboard.kbtextstr);
            menu = MENU_PRIMARY;
        }
        else if (cancelBtn.GetState() == STATE_CLICKED) {
            menu = MENU_PIN;
        }
    }

    HaltGui();
    mainWindow->Remove(&w);

    return menu;
}

/****************************************************************************
 * MenuSettings
 ***************************************************************************/
static int MenuSettings() {
    int menu = MENU_NONE;

    GuiText titleTxt(_("Set Personal Data"), 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiImageData btnLargeOutline(button_large_png);
    GuiImageData btnLargeOutlineOver(button_large_over_png);

    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);
    GuiTrigger trigHome;
    trigHome.SetButtonOnlyTrigger(
        -1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);

    GuiText firstNameBtnTxt(_("First Name"), 22, (GXColor){0, 0, 0, 255});
    firstNameBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage firstNameBtnImg(&btnLargeOutline);
    GuiImage firstNameBtnImgOver(&btnLargeOutlineOver);
    GuiButton firstNameBtn(btnLargeOutline.GetWidth(),
                           btnLargeOutline.GetHeight());
    firstNameBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    firstNameBtn.SetPosition(-175, 120);
    firstNameBtn.SetLabel(&firstNameBtnTxt);
    firstNameBtn.SetImage(&firstNameBtnImg);
    firstNameBtn.SetImageOver(&firstNameBtnImgOver);
    firstNameBtn.SetSoundOver(&btnSoundOver);
    firstNameBtn.SetTrigger(&trigA);
    firstNameBtn.SetEffectGrow();

    GuiText lastNameBtnTxt(_("Last Name"), 22, (GXColor){0, 0, 0, 255});
    lastNameBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage lastNameBtnImg(&btnLargeOutline);
    GuiImage lastNameImgOver(&btnLargeOutlineOver);
    GuiButton lastNameBtn(btnLargeOutline.GetWidth(),
                          btnLargeOutline.GetHeight());
    lastNameBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    lastNameBtn.SetPosition(0, 120);
    lastNameBtn.SetLabel(&lastNameBtnTxt);
    lastNameBtn.SetImage(&lastNameBtnImg);
    lastNameBtn.SetImageOver(&lastNameImgOver);
    lastNameBtn.SetSoundOver(&btnSoundOver);
    lastNameBtn.SetTrigger(&trigA);
    lastNameBtn.SetEffectGrow();

    GuiText email_addressBtnTxt(_("Email Address"), 22, (GXColor){0, 0, 0, 255});
    email_addressBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage email_addressBtnImg(&btnLargeOutline);
    GuiImage email_addressBtnImgOver(&btnLargeOutlineOver);
    GuiButton email_addressBtn(btnLargeOutline.GetWidth(),
                               btnLargeOutline.GetHeight());
    email_addressBtn.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
    email_addressBtn.SetPosition(-65, 250);
    email_addressBtn.SetLabel(&email_addressBtnTxt);
    email_addressBtn.SetImage(&email_addressBtnImg);
    email_addressBtn.SetImageOver(&email_addressBtnImgOver);
    email_addressBtn.SetSoundOver(&btnSoundOver);
    email_addressBtn.SetTrigger(&trigA);
    email_addressBtn.SetEffectGrow();

    GuiText home_addressBtnTxt(_("Home Address"), 22, (GXColor){0, 0, 0, 255});
    home_addressBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage home_addressBtnImg(&btnLargeOutline);
    GuiImage home_addressBtnImgOver(&btnLargeOutlineOver);
    GuiButton home_addressBtn(btnLargeOutline.GetWidth(),
                              btnLargeOutline.GetHeight());
    home_addressBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    home_addressBtn.SetPosition(-175, 250);
    home_addressBtn.SetLabel(&home_addressBtnTxt);
    home_addressBtn.SetImage(&home_addressBtnImg);
    home_addressBtn.SetImageOver(&home_addressBtnImgOver);
    home_addressBtn.SetSoundOver(&btnSoundOver);
    home_addressBtn.SetTrigger(&trigA);
    home_addressBtn.SetEffectGrow();

    GuiText phoneBtnTxt(_("Phone Number"), 22, (GXColor){0, 0, 0, 255});
    phoneBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage phoneBtnImg(&btnLargeOutline);
    GuiImage phoneBtnImgOver(&btnLargeOutlineOver);
    GuiButton phoneBtn(btnLargeOutline.GetWidth(),
                         btnLargeOutline.GetHeight());
    phoneBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    phoneBtn.SetPosition(0, 250);
    phoneBtn.SetLabel(&phoneBtnTxt);
    phoneBtn.SetImage(&phoneBtnImg);
    phoneBtn.SetImageOver(&phoneBtnImgOver);
    phoneBtn.SetSoundOver(&btnSoundOver);
    phoneBtn.SetTrigger(&trigA);
    phoneBtn.SetEffectGrow();

    GuiText cityBtnTxt(_("City"), 22, (GXColor){0, 0, 0, 255});
    GuiImage cityBtnImg(&btnLargeOutline);
    GuiImage cityBtnImgOver(&btnLargeOutlineOver);
    GuiButton cityBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    cityBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    cityBtn.SetPosition(175, 120);
    cityBtn.SetLabel(&cityBtnTxt);
    cityBtn.SetImage(&cityBtnImg);
    cityBtn.SetImageOver(&cityBtnImgOver);
    cityBtn.SetSoundOver(&btnSoundOver);
    cityBtn.SetTrigger(&trigA);
    cityBtn.SetEffectGrow();

    GuiText saveBtnTxt(_("Done"), 22, (GXColor){0, 0, 0, 255});
    GuiImage saveBtnImg(&btnOutline);
    GuiImage saveBtnImgOver(&btnOutlineOver);
    GuiButton saveBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    saveBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    saveBtn.SetPosition(-25, -15);
    saveBtn.SetLabel(&saveBtnTxt);
    saveBtn.SetImage(&saveBtnImg);
    saveBtn.SetImageOver(&saveBtnImgOver);
    saveBtn.SetSoundOver(&btnSoundOver);
    saveBtn.SetTrigger(&trigA);
    saveBtn.SetTrigger(&trigHome);
    saveBtn.SetEffectGrow();

    GuiText pinBtnTxt("PIN", 22, (GXColor){0, 0, 0, 255});
    GuiImage pinBtnImg(&btnOutline);
    GuiImage pinBtnImgOver(&btnOutlineOver);
    GuiButton pinBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    pinBtn.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
    pinBtn.SetPosition(0, -15);
    pinBtn.SetLabel(&pinBtnTxt);
    pinBtn.SetImage(&pinBtnImg);
    pinBtn.SetImageOver(&pinBtnImgOver);
    pinBtn.SetSoundOver(&btnSoundOver);
    pinBtn.SetTrigger(&trigA);
    pinBtn.SetEffectGrow();

    GuiText cancelBtnTxt(_("Cancel"), 22, (GXColor){0, 0, 0, 255});
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(25, -15);
    cancelBtn.SetLabel(&cancelBtnTxt);
    cancelBtn.SetImage(&cancelBtnImg);
    cancelBtn.SetImageOver(&cancelBtnImgOver);
    cancelBtn.SetSoundOver(&btnSoundOver);
    cancelBtn.SetTrigger(&trigA);
    cancelBtn.SetEffectGrow();

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&titleTxt);
    w.Append(&firstNameBtn);
    w.Append(&lastNameBtn);
    w.Append(&email_addressBtn);
    w.Append(&saveBtn);
    w.Append(&home_addressBtn);
    w.Append(&cityBtn);
    w.Append(&phoneBtn);

    w.Append(&saveBtn);
    w.Append(&pinBtn);
    w.Append(&cancelBtn);

    mainWindow->Append(&w);

    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (firstNameBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EDIT_FIRST_NAME;
        } else if (lastNameBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EDIT_LAST_NAME;
        } else if (email_addressBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EDIT_EMAIL_ADDRESS;
        } else if (cancelBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EXIT;
        } else if (home_addressBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EDIT_HOME_ADDRESS;
        } else if (cityBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EDIT_CITY;
        } else if (saveBtn.GetState() == STATE_CLICKED) {
            // Attempt to save the current configuration.
            bool success = PD_WriteData();
            if (success) {
                Selection();
            } else {
                int result = WindowPrompt(
                    _("Error saving"),
                    _("An error occurred while attempting to save your information. Would you like to retry?"),
                    _("Cancel"), _("Retry"));
                if (result == 1) {
                    // The user selected to cancel.
                    menu = MENU_EXIT;
                } else {
                    // The user selected to retry. We will do nothing
                    // as this while loop will repeat.
                    saveBtn.SetState(STATE_CLICKED);
                }
            }
        } else if (phoneBtn.GetState() == STATE_CLICKED) {
            menu = MENU_PHONE;
        } else if (pinBtn.GetState() == STATE_CLICKED) {
            menu = MENU_PIN;
        }
    }

    HaltGui();
    mainWindow->Remove(&w);
    return menu;
}

/****************************************************************************
 * KeyboardDataEntry
 ***************************************************************************/

static int KeyboardDataEntry(wchar_t *input, const char *name) {
    int menu = MENU_NONE;

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    mainWindow->Append(&w);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        OnScreenKeyboard(input, 255, name);
        menu = MENU_PRIMARY;
    }

    HaltGui();

    mainWindow->Remove(&w);
    return menu;
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu) {
    if (!text_language()) {
        printf("Unable to load language");
        sleep(5);
        ExitApp();
    }

    int currentMenu = menu;

    pointer[0] = new GuiImageData(player1_point_png);
    pointer[1] = new GuiImageData(player2_point_png);
    pointer[2] = new GuiImageData(player3_point_png);
    pointer[3] = new GuiImageData(player4_point_png);

    mainWindow = new GuiWindow(screenwidth, screenheight);

    bgImg = new GuiImage(screenwidth, screenheight, (GXColor){0, 0, 0, 255});

    // Create a white stripe beneath the title and above actionable buttons.
    bgImg->ColorStripe(75, (GXColor){0xff, 0xff, 0xff, 255});
    bgImg->ColorStripe(76, (GXColor){0xff, 0xff, 0xff, 255});

    bgImg->ColorStripe(screenheight - 77, (GXColor){0xff, 0xff, 0xff, 255});
    bgImg->ColorStripe(screenheight - 78, (GXColor){0xff, 0xff, 0xff, 255});

    GuiImage *topChannelGradient =
        new GuiImage(new GuiImageData(channel_gradient_top_png));
    topChannelGradient->SetTile(screenwidth / 4);

    GuiImage *bottomChannelGradient =
        new GuiImage(new GuiImageData(channel_gradient_bottom_png));
    bottomChannelGradient->SetTile(screenwidth / 4);
    bottomChannelGradient->SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);

    mainWindow->Append(bgImg);
    mainWindow->Append(topChannelGradient);
    mainWindow->Append(bottomChannelGradient);

    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);

    ResumeGui();

    // bgMusic = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
    // bgMusic->SetVolume(50);
    // bgMusic->Play(); // startup music

    while (currentMenu != MENU_EXIT) {
        switch (currentMenu) {
        case MENU_INITIAL_PIN:
            currentMenu = InitialPin();
            break;
        case MENU_PRIMARY:
            currentMenu = MenuSettings();
            break;
        case MENU_EDIT_FIRST_NAME:
            currentMenu =
                KeyboardDataEntry(currentData.user_first_name, _("First Name"));
            break;
        case MENU_EDIT_LAST_NAME:
            currentMenu =
                KeyboardDataEntry(currentData.user_last_name, _("Last Name"));
            break;
        case MENU_EDIT_EMAIL_ADDRESS:
            currentMenu = KeyboardDataEntry(currentData.user_email_address,
                                            _("Email Address"));
            break;
        case MENU_EDIT_HOME_ADDRESS:
            currentMenu = KeyboardDataEntry(currentData.user_home_address,
                                            _("Home Address"));
            break;
        case MENU_EDIT_CITY:
            currentMenu = KeyboardDataEntry(currentData.user_city, _("City"));
            break;
        case MENU_PHONE:
            currentMenu = PhoneNumber();
            break;
        case MENU_PIN:
            currentMenu = PinMenu();
            break;
        case MENU_EDIT_PIN:
            currentMenu = EditPin();
            break;
        case MENU_DELETE_PIN:
            currentMenu = DeletePin();
            break;
        case MENU_ADD_PIN:
            currentMenu = AddPin();
            break;
       /* case MENU_CREDITS:
            currentMenu = MenuCredits();
            break;*/
        default:
            currentMenu = MenuSettings();
            break;
        }
    }

    ResumeGui();
    ExitRequested = true;
    while (1)
        usleep(THREAD_SLEEP);

    HaltGui();

    bgMusic->Stop();
    delete bgMusic;
    delete bgImg;
    delete mainWindow;

    delete pointer[0];
    delete pointer[1];
    delete pointer[2];
    delete pointer[3];

    mainWindow = NULL;
}
