#include <aes/aes.h>
#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "pd_data.h"

// Keep two static variables in order to retain state.
static bool PDHasLoaded = false;
static void *PDFilePointer = NULL;

// Used to house key info when loading.
struct KeyInfo {
    unsigned char key[32];
    unsigned char iv[16];
};

char *PD_GetTitleDataPath() {
    // TODO: Flesh out

    switch (CONF_GetRegion()) {
    case CONF_REGION_JP:
        return "/title/00010008/4843434a/data/nocopy/pd.dat";
    case CONF_REGION_US:
    case CONF_REGION_EU:
    case CONF_REGION_KR:
    case CONF_REGION_CN:
    default:
        return "/tmp/FILE_SHOULD_NOT_EXIST_PLEASE_FAIL";
    }
}

struct KeyInfo *PD_GetKeyData() {
    struct KeyInfo *info = malloc(sizeof(struct KeyInfo));

    u32 deviceId;
    s32 ret = ES_GetDeviceID(&deviceId);
    if (ret != 0) {
        printf("Failed to obtain device ID.\n");
        return NULL;
    }

    // Copy 1:1. We're not going to bother with string terminators
    // as we know our exact value ahead of time.
    memcpy(info->key, &PERSONAL_DATA_KEY, 32);
    memcpy(info->iv, &PERSONAL_DATA_IV, 16);

    unsigned char firstByte = (unsigned char)deviceId;
    unsigned char secondByte = (deviceId >> 8);
    unsigned char thirdByte = (deviceId >> 16);
    unsigned char fourthByte = (deviceId >> 24);

    // We need to update values within the key based off of the device's ID.
    info->key[7] = fourthByte;
    info->key[15] = thirdByte;
    info->key[23] = secondByte;
    info->key[31] = firstByte;

    // Similarly for the IV - its last 4 bytes are the device's ID as-is.
    info->iv[12] = fourthByte;
    info->iv[13] = thirdByte;
    info->iv[14] = secondByte;
    info->iv[15] = firstByte;

    return info;
}

void *PD_DecryptFile() {
    struct KeyInfo *keyInfo = PD_GetKeyData();
    if (keyInfo == NULL) {
        // There's not a whole ton we can do.
        return NULL;
    }

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, keyInfo->key, keyInfo->iv);

    void *fileBuffer = malloc(PD_FILE_LENGTH);
    if (fileBuffer == NULL) {
        printf("malloc() for fileBuffer failed.\n");
        return NULL;
    }

    // Read the contents of this file in NAND.
    char *filepath = PD_GetTitleDataPath();
    s32 fd = ISFS_Open(filepath, ISFS_OPEN_READ);
    if (fd < 0) {
        printf("Error opening file at %s\n", filepath);
        return NULL;
    }

    s32 ret = ISFS_Read(fd, fileBuffer, PD_FILE_LENGTH);
    if (ret < 0) {
        printf("Error reading file at %s\n", filepath);
        return NULL;
    }

    ret = ISFS_Close(fd);
    if (ret < 0) {
        printf("Error closing file at %s\n", filepath);
        return NULL;
    }

    // Decrypt file.
    AES_CBC_decrypt_buffer(&ctx, fileBuffer, PD_FILE_LENGTH);

    // Extremely simple validation.
    if (memcmp(fileBuffer, EXPECTED_FILE_MAGIC, 5) != 0) {
        // This doesn't appear to be valid.
        free(fileBuffer);
        return NULL;
    }

    // We have loaded succesfully.
    PDHasLoaded = true;

    free(keyInfo);
    return fileBuffer;
}

void *PD_GetFileContents() {
    // Check if we've previously loaded pd.dat.
    if (!PDHasLoaded) {
        printf("Loading...\n");
        PDFilePointer = PD_DecryptFile();
    } else {
        printf("Not loading.\n");
    }

    return PDFilePointer;
}
