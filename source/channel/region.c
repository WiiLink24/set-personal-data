#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>

#include "helpers.h"

char *GetTicketPath() {
    return "/title/00010008/4843434a/content/title.tmd";
}

char *GetTMDPath() {
    return "/ticket/00010008/4843434a.tik";
}

char *PD_InstallTemplates() { return NULL; }

char *PD_GetDataPath() {
    // /title/00010008/4843434a/data/nocopy/pd.dat\0 is 44 characters.
    char *path = "/title/00010008/4843434a/data/nocopy/pd.dat";

    // Ensure this path exists.
    s32 ret = CreateParentDirs(path);
    if (ret < 0) {
        printf("Failed while creating parent directories (error %d)\n", ret);
        return NULL;
    }

    return path;
}
