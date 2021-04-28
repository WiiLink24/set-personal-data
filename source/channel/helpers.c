#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static fstats stats ATTRIBUTE_ALIGN(32);
#define ISFS_EEXIST -105
#define ISFS_ENOENT -105

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

// The author apologizes for the amount of error catching here.
// In the end, blame Nintendo.
void *ISFS_GetFile(const char *path, u32 *size) {
    *size = 0;

    s32 fd = ISFS_Open(path, ISFS_OPEN_READ);
    if (fd < 0) {
        printf("ISFS_GetFile: unable to open file (error %d)\n", fd);
        return NULL;
    }

    void *buf = NULL;
    memset(&stats, 0, sizeof(fstats));

    s32 ret = ISFS_GetFileStats(fd, &stats);
    if (ret >= 0) {
        s32 length = stats.file_length;

        // We must align our length by 32.
        // memalign itself is dreadfully broken for unknown reasons.
        s32 aligned_length = length;
        s32 remainder = aligned_length % 32;
        if (remainder != 0) {
            aligned_length += 32 - remainder;
        }

        buf = aligned_alloc(32, aligned_length);

        if (buf != NULL) {
            s32 tmp_size = ISFS_Read(fd, buf, length);

            if (tmp_size == length) {
                // We were successful.
                *size = tmp_size;
            } else {
                // If positive, the file could not be fully read.
                // If negative, it is most likely an underlying /dev/fs
                // error.
                if (tmp_size >= 0) {
                    printf("ISFS_GetFile: only able to read %d out of "
                           "%d bytes!\n",
                           tmp_size, length);
                } else if (tmp_size == ISFS_ENOENT) {
                    // We ignore logging errors about files that do not exist.
                } else {
                    printf("ISFS_GetFile: ISFS_Open failed! (error %d)\n",
                           tmp_size);
                }

                free(buf);
            }
        } else {
            printf("ISFS_GetFile: failed to allocate buffer!\n");
        }
    } else {
        printf("ISFS_GetFile: unable to retrieve file stats (error %d)\n", ret);
    }
    ISFS_Close(fd);

    return buf;
}
