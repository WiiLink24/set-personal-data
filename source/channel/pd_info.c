// We need gettime from LWP to match OSGetTime.
#include "pd_info.h"
#include "pd_data.h"
#include <assert.h>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <string.h>
// Copied to avoid using a GNU extension.
#include <musl/memmem.h>

#include <stdlib.h>

void *PD_SeekBlockType(char toFind[4]) {
    void *contents = PD_GetFileContents();

    // Unfortunately, it appears that we can't expect the file internally to be
    // aligned in any way. Nintendo's way of searching for a block type is
    // rather horrifying as well.
    void *result = memmem(contents, 0x4000, toFind, 4);
    if (result == NULL) {
        printf("Unable to find tag within memory!\n");
        return NULL;
    }

    return result;
}

struct PDInfoBlock *PD_ParseInfoBlock() {
    void *blockPointer = PD_SeekBlockType("INFO");
    if (blockPointer == NULL) {
        printf("Could not find INFO block!\n");
        return NULL;
    }

    // After INFO + two nulls, the literal %o exists.
    if (memcmp(blockPointer + 6, "%o", 2) != 0) {
        printf("INFO block not well formed!\n");
        return NULL;
    }

    // Past this constant, there's a u32 of null.
    // Past that is what's most likely a 64-bit timestamp
    // which we're additionally going to ignore.
    blockPointer = blockPointer + 6 + 4 + 10;

    struct PDInfoBlock *infoBlock = malloc(sizeof(struct PDInfoBlock));
    if (infoBlock == NULL) {
        printf("Unable to allocate info block struct!\n");
        return NULL;
    }

    // The following offsets have been observed as being true.
    memcpy(infoBlock->registrationName, blockPointer, 60);
    memcpy(infoBlock->firstname, blockPointer + 60, 64);
    memcpy(infoBlock->surname, blockPointer + 124, 64);
    memcpy(infoBlock->zipCode, blockPointer + 188, 32);
    // Observed to be 0x19E.
    infoBlock->unknown = 0x19E;
    memcpy(infoBlock->stateOrPrefecture, blockPointer + 224, 64);
    memcpy(infoBlock->city, blockPointer + 284, 64);
    memcpy(infoBlock->address, blockPointer + 348, 256);
    memcpy(infoBlock->apartmentNumber, blockPointer + 604, 256);
    memcpy(infoBlock->phoneNumber, blockPointer + 860, 64);
    memcpy(infoBlock->emailAddress, blockPointer + 924, 254);

    infoBlock->updateTime = *(uint64_t*)(blockPointer + 1183);

    // The update time will be set upon save.
    printf("last update time: %llu\n", infoBlock->updateTime);
    printf("compare to: %llu\n", gettime());

    return infoBlock;
}
