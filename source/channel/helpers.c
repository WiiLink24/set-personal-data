#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ISFS_EEXIST -105

// CreateParentDirs iterates through every instance of "/"
// in order to create parent directories recursively.
// It assumes no file is named "/" in any way.
// (Seemingly, so does everything else, which helps a fair bit.)
s32 CreateParentDirs(char *path) {
    if (path == NULL || path[0] != '/') {
        return -1;
    }

    char *currentDir = calloc(1, strlen(path));

    // We want to start past the first initial "/".
    int offset = 1;
    while (true) {
        // Find the first occurrence of "/" from our offset.
        char *occurrence = strchr(path + offset, '/');
        if (occurrence == NULL) {
            // We've come to the end of our run.
            return 0;
        }

        // Find what range we need to copy from to the next directory.
        offset = occurrence - path;
        strncpy(currentDir, path, offset);

        // printf("%s\n", currentDir);
        // if (strcmp("/title", currentDir) == 0) {
        //   // We're assured this exists.
        //   offset++;
        //   continue;
        // }

        // We're okay with the directory already existing.
        s32 status = ISFS_CreateDir(currentDir, 0, 3, 3, 3);
        if (status < 0 && status != ISFS_EEXIST) {
            printf("Failed to create directory at %s (error %d)\n", currentDir,
                   status);
            return status;
        }

        // Increment in order to avoid our current "/".
        offset++;
    }

    return 0;
}
