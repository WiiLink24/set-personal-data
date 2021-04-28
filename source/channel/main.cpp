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
#include <libpatcher/libpatcher.h>

#include "pd_data.h"
#include "pd_info.h"
#include "region.h"
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
        pd_t::info_block_t *infoBlock = data.info();

        std::cout << "First name: " << infoBlock->first_name().c_str()
                  << std::endl;
        std::cout << "Surname: " << infoBlock->surname().c_str() << std::endl;
        std::cout << "Postal code: " << infoBlock->postal_code().c_str()
                  << std::endl;
        std::cout << "State/Prefecture: "
                  << infoBlock->state_or_prefecture().c_str() << std::endl;
        std::cout << "City: " << infoBlock->city().c_str() << std::endl;
        std::cout << "Address: " << infoBlock->address().c_str() << std::endl;
        std::cout << "Apartment Number: "
                  << infoBlock->apartment_number().c_str() << std::endl;
        std::cout << "Phone number: " << infoBlock->phone_number().c_str()
                  << std::endl;
        std::cout << "Email address: " << infoBlock->email_address().c_str()
                  << std::endl;
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    bool result = PD_SaveFileContents();
    std::cout << "Was " << (result ? "able" : "not able") << " to save pd.dat."
              << std::endl;

    // MainMenu(1);
    sleep(110000);

    return 0;
}
