#include "locale/locale_impl.h"
#include <errno.h>
#include <iconv.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define UTF_32BE 0300
#define UTF_16LE 0301
#define UTF_16BE 0302
#define UTF_32LE 0303
#define UCS2BE 0304
#define UCS2LE 0305
#define WCHAR_T 0306
#define US_ASCII 0307
#define UTF_8 0310
#define UTF_16 0312
#define UTF_32 0313
#define UCS2 0314
#define SHIFT_JIS 0321

int iconv_error = 0;

/* Definitions of charmaps. Each charmap consists of:
 * 1. Empty-string-terminated list of null-terminated aliases.
 * 2. Special type code or number of elided quads of entries.
 * 3. Character table (size determined by field 2), consisting
 *    of 5 bytes for every 4 characters, interpreted as 10-bit
 *    indices into the legacy_chars table. */
#include "locale/codepages.h"

/* Table of characters that appear in legacy 8-bit codepages,
 * limited to 1024 slots (10 bit indices). The first 256 entries
 * are elided since those characters are obviously all included. */
#include "locale/jis0208.h"
#include "locale/legacychars.h"
#include "locale/revjis.h"

static int fuzzycmp(const unsigned char *a, const unsigned char *b) {
    for (; *a && *b; a++, b++) {
        while (*a && (*a | 32U) - 'a' > 26 && *a - '0' > 10U)
            a++;
        if ((*a | 32U) != *b)
            return 1;
    }
    return *a != *b;
}

static size_t find_charmap(const void *name) {
    const unsigned char *s;
    if (!*(char *)name)
        name = charmaps; /* "utf8" */
    for (s = charmaps; *s;) {
        if (!fuzzycmp(name, s)) {
            for (; *s; s += strlen((void *)s) + 1)
                ;
            return s + 1 - charmaps;
        }
        s += strlen((void *)s) + 1;
        if (!*s) {
            if (s[1] > 0200)
                s += 2;
            else
                s += 2 + (64U - s[1]) * 5;
        }
    }
    return -1;
}

struct stateful_cd {
    iconv_t base_cd;
    unsigned state;
};

static iconv_t combine_to_from(size_t t, size_t f) {
    return (void *)(f << 16 | t << 1 | 1);
}

static size_t extract_from(iconv_t cd) { return (size_t)cd >> 16; }

static size_t extract_to(iconv_t cd) { return (size_t)cd >> 1 & 0x7fff; }

iconv_t iconv_open(const char *to, const char *from) {
    size_t f, t;
    struct stateful_cd *scd;

    if ((t = find_charmap(to)) == -1 || (f = find_charmap(from)) == -1 ||
        (charmaps[t] >= 0330)) {
        iconv_error = EINVAL;
        return (iconv_t)-1;
    }
    iconv_t cd = combine_to_from(t, f);

    switch (charmaps[f]) {
    case UTF_16:
    case UTF_32:
    case UCS2:
        scd = malloc(sizeof *scd);
        if (!scd)
            return (iconv_t)-1;
        scd->base_cd = cd;
        scd->state = 0;
        cd = (iconv_t)scd;
    }

    return cd;
}

static unsigned get_16(const unsigned char *s, int e) {
    e &= 1;
    return s[e] << 8 | s[1 - e];
}

static void put_16(unsigned char *s, unsigned c, int e) {
    e &= 1;
    s[e] = c >> 8;
    s[1 - e] = c;
}

static unsigned get_32(const unsigned char *s, int e) {
    e &= 3;
    return (s[e] + (0U << 24)) | s[e ^ 1] << 16 | s[e ^ 2] << 8 | s[e ^ 3];
}

static void put_32(unsigned char *s, unsigned c, int e) {
    e &= 3;
    s[e ^ 0] = c >> 24;
    s[e ^ 1] = c >> 16;
    s[e ^ 2] = c >> 8;
    s[e ^ 3] = c;
}

/* Adapt as needed */
#define mbrtowc_utf8 mbrtowc
#define wctomb_utf8 wctomb

static unsigned legacy_map(const unsigned char *map, unsigned c) {
    if (c < 4 * map[-1])
        return c;
    unsigned x = c - 4 * map[-1];
    x = map[x * 5 / 4] >> 2 * x % 8 |
        (map[x * 5 / 4 + 1] << (8 - 2 * x % 8) & 1023);
    return x < 256 ? x : legacy_chars[x - 256];
}

static unsigned uni_to_jis(unsigned c) {
    unsigned nel = sizeof rev_jis / sizeof *rev_jis;
    unsigned d, j, i, b = 0;
    for (;;) {
        i = nel / 2;
        j = rev_jis[b + i];
        d = jis0208[j / 256][j % 256];
        if (d == c)
            return j + 0x2121;
        else if (nel == 1)
            return 0;
        else if (c < d)
            nel /= 2;
        else {
            b += i;
            nel -= nel / 2;
        }
    }
}

size_t iconv(iconv_t cd, char **restrict in, size_t *restrict inb,
             char **restrict out, size_t *restrict outb) {
    size_t x = 0;
    struct stateful_cd *scd = 0;
    if (!((size_t)cd & 1)) {
        scd = (void *)cd;
        cd = scd->base_cd;
    }
    unsigned to = extract_to(cd);
    unsigned from = extract_from(cd);
    const unsigned char *map = charmaps + from + 1;
    const unsigned char *tomap = charmaps + to + 1;
    mbstate_t st = {0};
    wchar_t wc;
    unsigned c, d;
    size_t k, l;
    int err;
    unsigned char type = map[-1];
    unsigned char totype = tomap[-1];
    locale_t ploc = CURRENT_LOCALE, loc = ploc;

    if (!in || !*in || !*inb)
        return 0;

    ploc = UTF8_LOCALE;

    for (; *inb; *in += l, *inb -= l) {
        c = *(unsigned char *)*in;
        l = 1;

        switch (type) {
        case UTF_8:
            if (c < 128)
                break;
            l = mbrtowc_utf8(&wc, *in, *inb, &st);
            if (l == (size_t)-1)
                goto ilseq;
            if (l == (size_t)-2)
                goto starved;
            c = wc;
            break;
        case US_ASCII:
            if (c >= 128)
                goto ilseq;
            break;
        case WCHAR_T:
            l = sizeof(wchar_t);
            if (*inb < l)
                goto starved;
            c = *(wchar_t *)*in;
            if (0) {
            case UTF_32BE:
            case UTF_32LE:
                l = 4;
                if (*inb < 4)
                    goto starved;
                c = get_32((void *)*in, type);
            }
            if (c - 0xd800u < 0x800u || c >= 0x110000u)
                goto ilseq;
            break;
        case UCS2BE:
        case UCS2LE:
        case UTF_16BE:
        case UTF_16LE:
            l = 2;
            if (*inb < 2)
                goto starved;
            c = get_16((void *)*in, type);
            if ((unsigned)(c - 0xdc00) < 0x400)
                goto ilseq;
            if ((unsigned)(c - 0xd800) < 0x400) {
                if (type - UCS2BE < 2U)
                    goto ilseq;
                l = 4;
                if (*inb < 4)
                    goto starved;
                d = get_16((void *)(*in + 2), type);
                if ((unsigned)(d - 0xdc00) >= 0x400)
                    goto ilseq;
                c = ((c - 0xd7c0) << 10) + (d - 0xdc00);
            }
            break;
        case UCS2:
        case UTF_16:
            l = 0;
            if (!scd->state) {
                if (*inb < 2)
                    goto starved;
                c = get_16((void *)*in, 0);
                scd->state = type == UCS2  ? c == 0xfffe ? UCS2LE : UCS2BE
                             : c == 0xfffe ? UTF_16LE
                                           : UTF_16BE;
                if (c == 0xfffe || c == 0xfeff)
                    l = 2;
            }
            type = scd->state;
            continue;
        case UTF_32:
            l = 0;
            if (!scd->state) {
                if (*inb < 4)
                    goto starved;
                c = get_32((void *)*in, 0);
                scd->state = c == 0xfffe0000 ? UTF_32LE : UTF_32BE;
                if (c == 0xfffe0000 || c == 0xfeff)
                    l = 4;
            }
            type = scd->state;
            continue;
        case SHIFT_JIS:
            if (c < 128)
                break;
            if (c - 0xa1 <= 0xdf - 0xa1) {
                c += 0xff61 - 0xa1;
                break;
            }
            l = 2;
            if (*inb < 2)
                goto starved;
            d = *((unsigned char *)*in + 1);
            if (c - 129 <= 159 - 129)
                c -= 129;
            else if (c - 224 <= 239 - 224)
                c -= 193;
            else
                goto ilseq;
            c *= 2;
            if (d - 64 <= 158 - 64) {
                if (d == 127)
                    goto ilseq;
                if (d > 127)
                    d--;
                d -= 64;
            } else if (d - 159 <= 252 - 159) {
                c++;
                d -= 159;
            }
            c = jis0208[c][d];
            if (!c)
                goto ilseq;
            break;
        default:
            if (!c)
                break;
            c = legacy_map(map, c);
            if (!c)
                goto ilseq;
        }

        switch (totype) {
        case WCHAR_T:
            if (*outb < sizeof(wchar_t))
                goto toobig;
            *(wchar_t *)*out = c;
            *out += sizeof(wchar_t);
            *outb -= sizeof(wchar_t);
            break;
        case UTF_8:
            if (*outb < 4) {
                char tmp[4];
                k = wctomb_utf8(tmp, c);
                if (*outb < k)
                    goto toobig;
                memcpy(*out, tmp, k);
            } else
                k = wctomb_utf8(*out, c);
            *out += k;
            *outb -= k;
            break;
        case US_ASCII:
            if (c > 0x7f)
            subst:
                x++, c = '*';
        default:
            if (*outb < 1)
                goto toobig;
            if (c < 256 && c == legacy_map(tomap, c)) {
            revout:
                if (*outb < 1)
                    goto toobig;
                *(*out)++ = c;
                *outb -= 1;
                break;
            }
            d = c;
            for (c = 4 * totype; c < 256; c++) {
                if (d == legacy_map(tomap, c)) {
                    goto revout;
                }
            }
            goto subst;
        case SHIFT_JIS:
            if (c < 128)
                goto revout;
            if (c == 0xa5) {
                x++;
                c = '\\';
                goto revout;
            }
            if (c == 0x203e) {
                x++;
                c = '~';
                goto revout;
            }
            if (c - 0xff61 <= 0xdf - 0xa1) {
                c += 0xa1 - 0xff61;
                goto revout;
            }
            c = uni_to_jis(c);
            if (!c)
                goto subst;
            if (*outb < 2)
                goto toobig;
            d = c % 256;
            c = c / 256;
            *(*out)++ = (c + 1) / 2 + (c < 95 ? 112 : 176);
            *(*out)++ = c % 2 ? d + 31 + d / 96 : d + 126;
            *outb -= 2;
            break;
        case UCS2:
            totype = UCS2BE;
        case UCS2BE:
        case UCS2LE:
        case UTF_16:
        case UTF_16BE:
        case UTF_16LE:
            if (c < 0x10000 || totype - UCS2BE < 2U) {
                if (c >= 0x10000)
                    c = 0xFFFD;
                if (*outb < 2)
                    goto toobig;
                put_16((void *)*out, c, totype);
                *out += 2;
                *outb -= 2;
                break;
            }
            if (*outb < 4)
                goto toobig;
            c -= 0x10000;
            put_16((void *)*out, (c >> 10) | 0xd800, totype);
            put_16((void *)(*out + 2), (c & 0x3ff) | 0xdc00, totype);
            *out += 4;
            *outb -= 4;
            break;
        case UTF_32:
            totype = UTF_32BE;
        case UTF_32BE:
        case UTF_32LE:
            if (*outb < 4)
                goto toobig;
            put_32((void *)*out, c, totype);
            *out += 4;
            *outb -= 4;
            break;
        }
    }
    ploc = loc;
    return x;
ilseq:
    err = EILSEQ;
    x = -1;
    goto end;
toobig:
    err = E2BIG;
    x = -1;
    goto end;
starved:
    err = EINVAL;
    x = -1;
end:
    iconv_error = err;
    ploc = loc;
    return x;
}
