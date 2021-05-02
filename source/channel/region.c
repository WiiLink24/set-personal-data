#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>

#include "helpers.h"

char *GetTicketPath() {
    // A string such as /title/00010008/4843434a/content/title.tmd is 43
    // characters. We then factor in a necessary null terminator.
    char *path = malloc(44);
    sprintf(path, "/title/00010008/4843434a/content/title.tmd");
    return path;
}

char *GetTMDPath() {
    // Similarly, /ticket/00010008/4843434a.tik will be 29 + 1 characters.
    char *path = malloc(30);
    sprintf(path, "/ticket/00010008/4843434a.tik");
    return path;
}

char *PD_InstallTemplates() { return NULL; }

char *PD_GetDataPath() {
    // /title/00010008/4843434a/data/nocopy/pd.dat\0 is 44 characters.
    char *path = malloc(44);
    sprintf(path, "/title/00010008/4843434a/data/nocopy/pd.dat");

    // Ensure this path exists.
    s32 ret = CreateParentDirs(path);
    if (ret < 0) {
        printf("Failed while creating parent directories (error %d)\n", ret);
        return NULL;
    }

    return path;
}
