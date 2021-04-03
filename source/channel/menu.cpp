/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <wiiuse/wpad.h>

#include "gui/gui.h"
#include "main.h"
#include "menu.h"

#define THREAD_SLEEP 100
// 48 KiB was chosen after many days of testing.
// It horrifies the author.
#define GUI_STACK_SIZE 48 * 1024

static GuiImageData *pointer[4];
static GuiImage *bgImg = NULL;
static GuiSound *bgMusic = NULL;
static GuiWindow *mainWindow = NULL;
static lwp_t guithread = LWP_THREAD_NULL;
static bool guiHalt = true;
static bool ExitRequested = false;

wchar_t testing[256] = L"Testing input...";

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
                DoRumble(i);
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

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
void OnScreenKeyboard(wchar_t *var, u16 maxlen) {
    int save = -1;

    GuiKeyboard keyboard(var, maxlen);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);

    GuiText okBtnTxt("OK", 22, (GXColor){0, 0, 0, 255});
    GuiImage okBtnImg(&btnOutline);
    GuiImage okBtnImgOver(&btnOutlineOver);
    GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

    okBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    okBtn.SetPosition(25, -25);

    okBtn.SetLabel(&okBtnTxt);
    okBtn.SetImage(&okBtnImg);
    okBtn.SetImageOver(&okBtnImgOver);
    okBtn.SetSoundOver(&btnSoundOver);
    okBtn.SetTrigger(&trigA);
    okBtn.SetEffectGrow();

    GuiText cancelBtnTxt("Cancel", 22, (GXColor){0, 0, 0, 255});
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(-25, -25);
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
    mainWindow->ChangeFocus(&keyboard);
    ResumeGui();

    while (save == -1) {
        usleep(THREAD_SLEEP);

        if (okBtn.GetState() == STATE_CLICKED)
            save = 1;
        else if (cancelBtn.GetState() == STATE_CLICKED)
            save = 0;
    }

    if (save) {
        swprintf(var, maxlen, L"%ls", keyboard.kbtextstr);
    }

    HaltGui();
    mainWindow->Remove(&keyboard);
    mainWindow->SetState(STATE_DEFAULT);
    ResumeGui();
}

/****************************************************************************
 * Credits
 ***************************************************************************/
static int MenuCredits() {

    int menu = MENU_NONE;

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(button_png);
    GuiImageData btnOutlineOver(button_over_png);
    GuiImageData btnLargeOutline(button_large_png);
    GuiImageData btnLargeOutlineOver(button_large_over_png);

    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A,
                           PAD_BUTTON_A);

    GuiText titleTxt("Credits", 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 25);

    GuiText nameTxt1("-Spotlight", 28, (GXColor){255, 255, 255, 255});
    GuiText nameTxt2("-SketchMaster2001", 28, (GXColor){255, 255, 255, 255});
    nameTxt1.SetPosition(0, 100);
    nameTxt2.SetPosition(0, 150);
    nameTxt1.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    nameTxt2.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);

    GuiText exitBtnTxt("Back", 22, (GXColor){0, 0, 0, 255});
    GuiImage exitBtnImg(&btnOutline);
    GuiImage exitBtnImgOver(&btnOutlineOver);
    GuiButton exitBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    exitBtn.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
    exitBtn.SetPosition(0, -15);
    exitBtn.SetLabel(&exitBtnTxt);
    exitBtn.SetImage(&exitBtnImg);
    exitBtn.SetImageOver(&exitBtnImgOver);
    exitBtn.SetSoundOver(&btnSoundOver);
    exitBtn.SetTrigger(&trigA);
    exitBtn.SetEffectGrow();

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
    w.Append(&exitBtn);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (exitBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS;
        }
    }

    HaltGui();

    mainWindow->Remove(&w);
    mainWindow->Remove(&titleTxt);
    mainWindow->Remove(logo);
    return menu;
}

/****************************************************************************
 * MenuSettings
 ***************************************************************************/
static int MenuSettings() {
    int menu = MENU_NONE;

    GuiText titleTxt("Set Personal Data", 28, (GXColor){255, 255, 255, 255});
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

    wchar_t testingContent[22] = L"仮仮仮仮仮仮仮仮";
    GuiTextField testingField(testingContent, 20);
    testingField.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    testingField.SetPosition(0, 75);

    GuiText firstNameBtnTxt("First Name", 22, (GXColor){0, 0, 0, 255});
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

    GuiText lastNameBtnTxt("Last Name", 22, (GXColor){0, 0, 0, 255});
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

    GuiText emailBtnTxt1("Email", 22, (GXColor){0, 0, 0, 255});
    GuiText emailBtnTxt2("Address", 22, (GXColor){0, 0, 0, 255});
    emailBtnTxt1.SetPosition(0, -20);
    emailBtnTxt2.SetPosition(0, +10);
    GuiImage emailBtnImg(&btnLargeOutline);
    GuiImage emailBtnImgOver(&btnLargeOutlineOver);
    GuiButton emailBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    emailBtn.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
    emailBtn.SetPosition(-65, 120);
    emailBtn.SetLabel(&emailBtnTxt1, 0);
    emailBtn.SetLabel(&emailBtnTxt2, 1);
    emailBtn.SetImage(&emailBtnImg);
    emailBtn.SetImageOver(&emailBtnImgOver);
    emailBtn.SetSoundOver(&btnSoundOver);
    emailBtn.SetTrigger(&trigA);
    emailBtn.SetEffectGrow();

    GuiText saveBtnTxt("Save", 22, (GXColor){0, 0, 0, 255});
    saveBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage saveBtnImg(&btnLargeOutline);
    GuiImage saveBtnImgOver(&btnLargeOutlineOver);
    GuiButton saveBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
    saveBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    saveBtn.SetPosition(-100, 250);
    saveBtn.SetLabel(&saveBtnTxt);
    saveBtn.SetImage(&saveBtnImg);
    saveBtn.SetImageOver(&saveBtnImgOver);
    saveBtn.SetSoundOver(&btnSoundOver);
    saveBtn.SetTrigger(&trigA);
    saveBtn.SetEffectGrow();

    GuiText creditsBtnTxt("Credits", 22, (GXColor){0, 0, 0, 255});
    saveBtnTxt.SetWrap(true, btnLargeOutline.GetWidth() - 30);
    GuiImage creditsBtnImg(&btnLargeOutline);
    GuiImage creditsBtnImgOver(&btnLargeOutlineOver);
    GuiButton creditsBtn(btnLargeOutline.GetWidth(),
                         btnLargeOutline.GetHeight());
    creditsBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    creditsBtn.SetPosition(100, 250);
    creditsBtn.SetLabel(&creditsBtnTxt);
    creditsBtn.SetImage(&creditsBtnImg);
    creditsBtn.SetImageOver(&creditsBtnImgOver);
    creditsBtn.SetSoundOver(&btnSoundOver);
    creditsBtn.SetTrigger(&trigA);
    creditsBtn.SetEffectGrow();

    GuiText exitBtnTxt("Exit", 22, (GXColor){0, 0, 0, 255});
    GuiImage exitBtnImg(&btnOutline);
    GuiImage exitBtnImgOver(&btnOutlineOver);
    GuiButton exitBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    exitBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    exitBtn.SetPosition(100, -15);
    exitBtn.SetLabel(&exitBtnTxt);
    exitBtn.SetImage(&exitBtnImg);
    exitBtn.SetImageOver(&exitBtnImgOver);
    exitBtn.SetSoundOver(&btnSoundOver);
    exitBtn.SetTrigger(&trigA);
    exitBtn.SetTrigger(&trigHome);
    exitBtn.SetEffectGrow();

    GuiText resetBtnTxt("Reset Settings", 22, (GXColor){0, 0, 0, 255});
    GuiImage resetBtnImg(&btnOutline);
    GuiImage resetBtnImgOver(&btnOutlineOver);
    GuiButton resetBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    resetBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    resetBtn.SetPosition(-100, -15);
    resetBtn.SetLabel(&resetBtnTxt);
    resetBtn.SetImage(&resetBtnImg);
    resetBtn.SetImageOver(&resetBtnImgOver);
    resetBtn.SetSoundOver(&btnSoundOver);
    resetBtn.SetTrigger(&trigA);
    resetBtn.SetEffectGrow();

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    w.Append(&testingField);
    w.Append(&titleTxt);
    w.Append(&firstNameBtn);
    w.Append(&lastNameBtn);
    w.Append(&emailBtn);
    w.Append(&saveBtn);
    w.Append(&creditsBtn);

    w.Append(&exitBtn);
    w.Append(&resetBtn);

    mainWindow->Append(&w);

    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        if (firstNameBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS_FILE;
        } else if (lastNameBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS_FILE;
        } else if (emailBtn.GetState() == STATE_CLICKED) {
            menu = MENU_SETTINGS_FILE;
        } else if (exitBtn.GetState() == STATE_CLICKED) {
            menu = MENU_EXIT;
        } else if (creditsBtn.GetState() == STATE_CLICKED) {
            menu = MENU_CREDITS;
        } else if (resetBtn.GetState() == STATE_CLICKED) {
            resetBtn.ResetState();

            int choice = WindowPrompt(
                "Reset Settings",
                "Are you sure that you want to reset your settings?", "Yes",
                "No");
            if (choice == 1) {
                // reset settings
            }
        }
    }

    HaltGui();
    mainWindow->Remove(&w);
    return menu;
}

/****************************************************************************
 * MenuSettingsFile
 ***************************************************************************/

static int MenuSettingsFile() {
    int menu = MENU_NONE;

    GuiText titleTxt("Set Personal Data", 28, (GXColor){255, 255, 255, 255});
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 10);

    HaltGui();
    GuiWindow w(screenwidth, screenheight);
    mainWindow->Append(&w);
    mainWindow->Append(&titleTxt);
    ResumeGui();

    while (menu == MENU_NONE) {
        usleep(THREAD_SLEEP);

        OnScreenKeyboard(testing, 255);
        menu = MENU_SETTINGS;
    }

    HaltGui();

    mainWindow->Remove(&w);
    mainWindow->Remove(&titleTxt);
    return menu;
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu) {
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
        case MENU_SETTINGS:
            currentMenu = MenuSettings();
            break;
        case MENU_SETTINGS_FILE:
            currentMenu = MenuSettingsFile();
            break;
        case MENU_CREDITS:
            currentMenu = MenuCredits();
            break;
        default: // unrecognized menu
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
