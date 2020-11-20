#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <unistr.h>

#include "network.h"

#include "pd_data.h"
#include "pd_info.h"

#define textPos(x, y) printf("\x1b[%d;%dH", y, x)

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void do_reset(u32 irq, void *ctx) {
    // cya later!
    exit(0);
}

void do_poweroff() { exit(0); }

int main(void) {
    VIDEO_Init();
    WPAD_Init();
    ISFS_Initialize();
    CONF_Init();

    // Make the Reset button functional.
    SYS_SetResetCallback(do_reset);
    SYS_SetPowerCallback(do_poweroff);

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

    // Decrypt file
    void *pdLocation = PD_GetFileContents();
    if (pdLocation == NULL) {
        // It appears something went awry down the line.
        // TODO: please do proper error handling!
        printf("An error occurred reading.\n");
        goto stall;
    }

    // Print on screen because we have no other operation for the moment.
    // hexDump(pdLocation, 464);
    // hexDump(pdLocation, 0x53B);
    struct PDInfoBlock *infoBlock = PD_ParseInfoBlock();
    if (infoBlock == NULL) {
        printf("Failed to obtain INFO block.\n");
        goto stall;
    }

    // printf("%d\n", u16_strlen(infoBlock->firstname));

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
