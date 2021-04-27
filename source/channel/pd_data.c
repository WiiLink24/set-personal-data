#include <aes/aes.h>
#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "helpers.h"
#include "pd_data.h"
#include "pd_decrypted_dat.h"
#include "region.h"

// Keep two static variables in order to retain state.
static void *PDFilePointer = NULL;

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

bool PD_EncryptFile() {
    struct KeyInfo *keyInfo = PD_GetKeyData();
    if (keyInfo == NULL) {
        return false;
    }

    void *fileBuffer = PD_GetFileContents();
    if (fileBuffer == NULL) {
        return false;
    }

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, keyInfo->key, keyInfo->iv);

    // Encrypt file.
    AES_CBC_encrypt_buffer(&ctx, fileBuffer, PD_FILE_LENGTH);

    free(keyInfo);
    return true;
}

bool PD_DecryptFile() {
    struct KeyInfo *keyInfo = PD_GetKeyData();
    if (keyInfo == NULL) {
        return false;
    }

    void *fileBuffer = PD_GetFileContents();
    if (fileBuffer == NULL) {
        return false;
    }

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, keyInfo->key, keyInfo->iv);

    // Encrypt file.
    AES_CBC_decrypt_buffer(&ctx, fileBuffer, PD_FILE_LENGTH);

    free(keyInfo);
    return true;
}

// TODO: Write
void *PD_WriteToNAND() {
    // char *filepath = PD_GetDataPath();
    // if (filepath == NULL) {
    //     printf("This console's region is not supported!\n");
    //     return NULL;
    // }
    //
    // // Write encrypted file.
    // ISFS_Delete(filepath);
    //
    // s32 ret = ISFS_CreateFile(filepath, 0, 3, 3, 3);
    // if (ret < 0) {
    //     printf("ISFS_CreateFile failed (%d)\n", ret);
    //     return NULL;
    // }
    //
    // s32 fd = ISFS_Open(filepath, ISFS_OPEN_WRITE);
    // if (fd < 0) {
    //     printf("Error opening file at %s\n", filepath);
    //     return NULL;
    // }
    //
    // ret = ISFS_Write(fd, fileBuffer, PD_FILE_LENGTH);
    // if (ret < 0) {
    //     printf("Error writing file at %s\n", filepath);
    //     return NULL;
    // }
    //
    // ret = ISFS_Close(fd);
    // if (ret < 0) {
    //     printf("Error closing file at %s\n", filepath);
    //     return NULL;
    // }
    return NULL;
}

void *PD_GetTemplateData() {
    void *fileBuffer = malloc(PD_FILE_LENGTH);
    if (fileBuffer == NULL) {
        printf("malloc() for fileBuffer failed.\n");
        return NULL;
    }

    // Read our prebuilt content.
    memcpy(fileBuffer, pd_decrypted_dat, PD_FILE_LENGTH);

    return fileBuffer;
}

void *PD_ReadFromNAND() {
    char *filepath = PD_GetDataPath();
    if (filepath == NULL) {
        return NULL;
    }

    u32 readSize;
    void *fileBuffer = ISFS_GetFile(filepath, &readSize);

    // Perhaps the file was not written properly?
    if (readSize != PD_FILE_LENGTH) {
        return NULL;
    }

    return fileBuffer;
}

// PD_LoadFileContents is guaranteed
void *PD_LoadFileContents() {
    // Attempt to read what may already exist.
    void *fileBuffer = PD_ReadFromNAND();
    if (fileBuffer != NULL) {
        return fileBuffer;
    } else {
        // Default to using our template data.
        return PD_GetTemplateData();
    }
}

// PD_GetFileContents may use an already decrypted version if available.
// Otherwise, it will default to loading file contents.
void *PD_GetFileContents() {
    // Check if we've previously loaded pd.dat.
    if (PDFilePointer != NULL) {
        return PDFilePointer;
    }

    PDFilePointer = PD_LoadFileContents();
    bool result = PD_DecryptFile();
    if (result == false) {
        // Decryption, for whatever reason, failed.
        PDFilePointer = NULL;
        return NULL;
    }

    return PDFilePointer;
}
