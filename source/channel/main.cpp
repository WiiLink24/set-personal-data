#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistr.h>
#include <wiiuse/wpad.h>

// GUI
#include "font_ttf.h"
#include "gui/gui.h"

#include "charmangler.h"
#include "pd_data.h"
#include "pd_info.h"

extern "C" {
  #include <libpatcher/libpatcher.h>
}

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static void power_cb() { STM_ShutdownToStandby(); }
static void reset_cb(u32 level, void *unk) { STM_RebootSystem(); }

int main(void) {
    VIDEO_Init();

    // Make hardware buttons functional.
    SYS_SetPowerCallback(power_cb);
    SYS_SetResetCallback(reset_cb);

    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight,
                 rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    bool success = apply_patches();
    if (!success) {
        printf("Failed to apply patches!\n");
        goto stall;
    }

    printf("\n\n\n\n\n\n");

    WPAD_Init();
    ISFS_Initialize();
    CONF_Init();

    InitVideo();
    SetupPads();
    InitAudio();
    InitFreeType((u8 *)font_ttf, font_ttf_size);
    InitGUIThreads();

    char test[256];
    OnScreenKeyboard(test, 255);

    goto stall;

    // Decrypt file
    // void *pdLocation = PD_GetFileContents();
    // if (pdLocation == NULL) {
    //     // It appears something went awry down the line.
    //     // TODO: please do proper error handling!
    //     printf("An error occurred reading.\n");
    //     goto stall;
    // }
    //
    // // Print on screen because we have no other operation for the moment.
    // struct PDInfoBlock *infoBlock = PD_ParseInfoBlock();
    // if (infoBlock == NULL) {
    //     printf("Failed to obtain INFO block.\n");
    //     goto stall;
    // }
    //
    // printf("Email: %s\n", u16_to_char(infoBlock->emailAddress));
    // printf("First name: %s\n", u16_to_char(infoBlock->firstname));
    // printf("Surname: %s\n", u16_to_char(infoBlock->surname));
    // printf("Zip code: %s\n", u16_to_char(infoBlock->zipCode));
    // printf("City: %s\n", u16_to_char(infoBlock->city));
    // printf("Address: %s\n", u16_to_char(infoBlock->address));
    // printf("Apartment number: %s\n",
    // u16_to_char(infoBlock->apartmentNumber)); printf("Phone number: %s\n",
    // u16_to_char(infoBlock->phoneNumber));
    //
    // printf("\n\nEverything is created! Press the HOME button to exit.\n");
stall:
    while (1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME)
            exit(0);
        VIDEO_WaitVSync();
    }

    return 0;
}
