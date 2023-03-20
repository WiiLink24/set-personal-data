#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiiuse/wpad.h>

// IOS patches
extern "C" {
#include <libpatcher/libpatcher.h>
}

// GUI
#include "../gui/gui.h"
#include "menu.h"
#include "noto_sans_jp_regular_otf.h"

// PD related
#include "pd_info.h"

#define TITLE_ID(x, y) (((u64)(x) << 32) | (y))

static void power_cb() {
    ExitRequested = true;
    exitType = ExitType::POWER;
}

void ExitApp(ExitType type) {
    StopGX();

    switch (type) {
    case ExitType::WII_MENU:
        WII_ReturnToMenu();
    case ExitType::POWER:
        STM_ShutdownToIdle();
    case ExitType::DEMAE:
        WII_LaunchTitle(TITLE_ID(0x00010001, 0x4843484a));
    case ExitType::DIGICAM:
        WII_LaunchTitle(TITLE_ID(0x00010001, 0x4843444a));
    }
}

static void reset_cb(u32 level, void *unk) {
    ExitRequested = true;
    exitType = ExitType::WII_MENU;
}

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
    
    bool result = PD_PopulateData();
    if (!result) {
        printf("Failed to read pd.dat!\n");
        sleep(5);
        ExitApp(exitType);
    }

    MainMenu(-2);

    return 0;
}
