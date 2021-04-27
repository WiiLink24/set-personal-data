#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistr.h>
#include <wiiuse/wpad.h>

#include <fstream>
#include <kaitai/kaitaistream.h>
#include <pd-kaitai-struct/pd.h>
#include <sstream>

#include <iostream>

// GUI
#include "gui/gui.h"
#include "menu.h"
#include "noto_sans_jp_regular_otf.h"

extern "C" {
// Patcher
#include "charmangler.h"
#include "pd_data.h"
#include "pd_info.h"
#include "region.h"

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

    // Decrypt file
    void *pdLocation = PD_GetFileContents();
    if (pdLocation == NULL) {
        // It appears something went awry down the line.
        // TODO: please do proper error handling!
        printf("An error occurred reading.\n");
        ExitApp();
    }

    // // Print on screen because we have no other operation for the moment.
    // struct PDInfoBlock *infoBlock = PD_ParseInfoBlock();
    // if (infoBlock == NULL) {
    //     printf("Failed to obtain INFO block.\n");
    //     ExitApp();
    // }

    try {
        std::stringstream is;
        is.write((const char *)pdLocation, 0x4000);
        kaitai::kstream ks(&is);
        pd_t data(&ks);
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }

        // printf("%s\n", u16_to_char(infoBlock->emailAddress));
        // u16_to_char(infoBlock->firstname);
        // u16_to_char(infoBlock->surname);
        // u16_to_char(infoBlock->zipCode);
        // u16_to_char(infoBlock->city);
        // u16_to_char(infoBlock->address);
        // u16_to_char(infoBlock->apartmentNumber);
        // u16_to_char(infoBlock->phoneNumber);

        // MainMenu(1);
        sleep(110000);

        return 0;
    }
