#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistr.h>
#include <wiiuse/wpad.h>

// GUI
#include "gui/gui.h"
#include "menu.h"
#include "noto_sans_jp_regular_otf.h"

extern "C" {
// Patcher
#include "charmangler.h"
#include "pd_data.h"
#include "pd_info.h"

#include <libpatcher/libpatcher.h>
}

static void power_cb() {
    ShutoffRumble();
    StopGX();
    STM_ShutdownToIdle();
}

void ExitApp() {
    ShutoffRumble();
    StopGX();
    WII_ReturnToMenu();
}

static void reset_cb(u32 level, void *unk) { ExitApp(); }

int main(void) {
    // Make hardware buttons functional.
    SYS_SetPowerCallback(power_cb);
    SYS_SetResetCallback(reset_cb);

    InitVideo();

    bool success = apply_patches();
    if (!success) {
        printf("Failed to apply patches!\n");
        sleep(5);
        WII_ReturnToMenu();
    }

    ISFS_Initialize();
    CONF_Init();
    SetupPads();
    InitAudio();
    InitFreeType((u8 *)noto_sans_jp_regular_otf, noto_sans_jp_regular_otf_size);
    InitGUIThreads();

    MainMenu(1);

    // Decrypt file
    void *pdLocation = PD_GetFileContents();
    if (pdLocation == NULL) {
        // It appears something went awry down the line.
        // TODO: please do proper error handling!
        printf("An error occurred reading.\n");
        ExitApp();
    }

    // Print on screen because we have no other operation for the moment.
    struct PDInfoBlock *infoBlock = PD_ParseInfoBlock();
    if (infoBlock == NULL) {
        printf("Failed to obtain INFO block.\n");
        ExitApp();
    }

    printf("Email: %s\n", u16_to_char(infoBlock->emailAddress));
    printf("First name: %s\n", u16_to_char(infoBlock->firstname));
    printf("Surname: %s\n", u16_to_char(infoBlock->surname));
    printf("Zip code: %s\n", u16_to_char(infoBlock->zipCode));
    printf("City: %s\n", u16_to_char(infoBlock->city));
    printf("Address: %s\n", u16_to_char(infoBlock->address));
    printf("Apartment number: %s\n", u16_to_char(infoBlock->apartmentNumber));
    printf("Phone number: %s\n", u16_to_char(infoBlock->phoneNumber));

    printf("\n\nEverything is created! Press the HOME button to exit.\n");

    return 0;
}
