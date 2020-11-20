#include <stdio.h>
#include <stdlib.h>
#include <unistr.h>

// Dear anyone whose name does not exist within the ASCII charset, I'm sorry.
// To anyone with even the slightest use of something UTF-8 or UTF-16, I'm
// sorry. This is a hack, and a cheap one at that. Yet it works. So here it is.
char *u16_to_char(uint16_t *input) {
    int maxLen = u16_strlen(input);
    char *conversion = (char *)malloc(maxLen + 1);
    if (conversion == NULL) {
        return "FAILED";
    }

    for (int i = 0; i < maxLen; i++) {
        conversion[i] = (char)(input[i] >> 8);
    }

    // Ensure we null-terminate our string.
    conversion[maxLen] = '\0';
    return conversion;
}
