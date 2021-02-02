#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>

#include "helpers.h"

s32 region = 0;

s32 GetRegionTitle() {
    // switch (CONF_GetRegion()) {
    // case CONF_REGION_JP:
    //     // "HCCJ"
    //     return 0x4843434a;
    // case CONF_REGION_US:
    //     // "HCCE"
    //     return 0x48434345;
    // case CONF_REGION_EU:
    //     // "HCCP"
    //     return 0x48434350;
    // default:
    //     // Only three regions are supported by the PD SDK library.
    //     return 0;
    // }
    return 0x4843434a;
}

char *GetTicketPath() {
    // A string such as /title/00010008/4843434a/data/title.tmd is 39
    // characters. We then factor in a necessary null terminator.
    char *path = malloc(40);
    sprintf(path, "/title/00010008/%08x/data/title.tmd", region);
    return path;
}

char *GetTMDPath() {
    // Similarly, /ticket/00010008/4843434a.tik will be 29 + 1 characters.
    char *path = malloc(30);
    sprintf(path, "/ticket/00010008/%08x.tik", region);
    return path;
}

char *PD_GetDataPath() {
    region = GetRegionTitle();
    if (region == 0) {
        return NULL;
    }

    // /title/00010008/4843434a/data/nocopy/pd.dat\0 is 44 characters.
    char *path = malloc(44);
    sprintf(path, "/title/00010008/%08x/data/nocopy/pd.dat", region);

    // Ensure this path exists.
    s32 ret = CreateParentDirs(path);
    if (ret < 0) {
        printf("Failed while creating parent directories (error %d)\n", ret);
        return NULL;
    }

    return path;
}
