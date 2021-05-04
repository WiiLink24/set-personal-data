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
#include "gui/gui.h"
#include "menu.h"
#include "noto_sans_jp_regular_otf.h"

// PD related
#include "pd_info.h"

#define TITLE_ID(x,y)		(((u64)(x) << 32) | (y))

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

void Exit_to_Digicam() {
    ShutoffRumble();
    StopGX();
    WII_LaunchTitle(TITLE_ID(0x00010001,0x4843444a));
}

void Exit_to_Demae() {
    ShutoffRumble();
    StopGX();
    WII_LaunchTitle(TITLE_ID(0x00010001,0x4843484a));
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

    bool result = PD_PopulateData();
    if (result == false) {
        printf("Failed to read pd.dat!\n");
        sleep(5);
        ExitApp();
    }

    MainMenu(1);

    return 0;
}
