/*------------------------------------------------------------------------
 *  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include <zbar.h>

zbar_decoder_t *decoder;

zbar_symbol_type_t expect_sym;
char *expect_data = NULL;

unsigned seed = 0;
int verbosity = 1;
int rnd_size = 9;  /* NB should be odd */
int iter = 0;      /* test iteration */

#define zprintf(level, format, ...) do {                                \
        if(verbosity >= (level)) {                                      \
            fprintf((level) ? stdout : stderr, format , ##__VA_ARGS__); \
        }                                                               \
    } while(0)

static void symbol_handler (zbar_decoder_t *decoder)
{
    zbar_symbol_type_t sym = zbar_decoder_get_type(decoder);
    if(sym <= ZBAR_PARTIAL || sym == ZBAR_QRCODE)
        return;
    const char *data = zbar_decoder_get_data(decoder);

    int pass = (sym == expect_sym) && !strcmp(data, expect_data);
    pass *= 3;

    zprintf(pass, "decode %s:%s\n", zbar_get_symbol_name(sym), data);

    if(!expect_sym)
        zprintf(0, "UNEXPECTED!\n");
    else
        zprintf(pass, "expect %s:%s\n", zbar_get_symbol_name(expect_sym),
                expect_data);
    if(!pass) {
        zprintf(0, "SEED=%d\n", seed);
        abort();
    }

    expect_sym = ZBAR_NONE;
    free(expect_data);
    expect_data = NULL;
}

static void expect (zbar_symbol_type_t sym,
                    const char *data)
{
    if(expect_sym) {
        zprintf(0, "MISSING %s:%s\n"
                "SEED=%d\n",
                zbar_get_symbol_name(expect_sym), expect_data, seed);
        abort();
    }
    expect_sym = sym;
    expect_data = (data) ? strdup(data) : NULL;
}

static void encode_junk (int n)
{
    if(n > 1)
        zprintf(3, "encode random junk...\n");
    int i;
    for(i = 0; i < n; i++)
        zbar_decode_width(decoder, 20. * (rand() / (RAND_MAX + 1.)) + 1);
}

#define FWD 1
#define REV 0

static void encode (uint64_t units,
                    int fwd)
{
    zprintf(3, " raw=%x%x%c\n", (unsigned)(units >> 32),
            (unsigned)(units & 0xffffffff), (fwd) ? '<' : '>');
    if(!fwd)
        while(units && !(units >> 0x3c))
            units <<= 4;

    while(units) {
        unsigned char w = (fwd) ? units & 0xf : units >> 0x3c;
        zbar_decode_width(decoder, w);
        if(fwd)
            units >>= 4;
        else
            units <<= 4;
    }
}


/*------------------------------------------------------------*/
/* Code 128 encoding */

typedef enum code128_char_e {
    FNC3        = 0x60,
    FNC2        = 0x61,
    SHIFT       = 0x62,
    CODE_C      = 0x63,
    CODE_B      = 0x64,
    CODE_A      = 0x65,
    FNC1        = 0x66,
    START_A     = 0x67,
    START_B     = 0x68,
    START_C     = 0x69,
    STOP        = 0x6a,
} code128_char_t;

static const unsigned int code128[107] = {
    0x212222, 0x222122, 0x222221, 0x121223, /* 00 */
    0x121322, 0x131222, 0x122213, 0x122312,
    0x132212, 0x221213, 0x221312, 0x231212, /* 08 */
    0x112232, 0x122132, 0x122231, 0x113222,
    0x123122, 0x123221, 0x223211, 0x221132, /* 10 */
    0x221231, 0x213212, 0x223112, 0x312131,
    0x311222, 0x321122, 0x321221, 0x312212, /* 18 */
    0x322112, 0x322211, 0x212123, 0x212321,
    0x232121, 0x111323, 0x131123, 0x131321, /* 20 */
    0x112313, 0x132113, 0x132311, 0x211313,
    0x231113, 0x231311, 0x112133, 0x112331, /* 28 */
    0x132131, 0x113123, 0x113321, 0x133121,
    0x313121, 0x211331, 0x231131, 0x213113, /* 30 */
    0x213311, 0x213131, 0x311123, 0x311321,
    0x331121, 0x312113, 0x312311, 0x332111, /* 38 */
    0x314111, 0x221411, 0x431111, 0x111224,
    0x111422, 0x121124, 0x121421, 0x141122, /* 40 */
    0x141221, 0x112214, 0x112412, 0x122114,
    0x122411, 0x142112, 0x142211, 0x241211, /* 48 */
    0x221114, 0x413111, 0x241112, 0x134111,
    0x111242, 0x121142, 0x121241, 0x114212, /* 50 */
    0x124112, 0x124211, 0x411212, 0x421112,
    0x421211, 0x212141, 0x214121, 0x412121, /* 58 */
    0x111143, 0x111341, 0x131141, 0x114113,
    0x114311, 0x411113, 0x411311, 0x113141, /* 60 */
    0x114131, 0x311141, 0x411131,
    0xa211412, 0xa211214, 0xa211232,        /* START_A-START_C (67-69) */
    0x2331112a,                             /* STOP (6a) */
};

static void encode_code128b (char *data)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    zprintf(3, "----------------------------------------------------------\n");
    zprintf(2, "CODE-128(B): %s\n", data);
    zprintf(3, "    encode START_B: %02x", START_B);
    encode(code128[START_B], 0);
    int i, chk = START_B;
    for(i = 0; data[i]; i++) {
        zprintf(3, "    encode '%c': %02x", data[i], data[i] - 0x20);
        encode(code128[data[i] - 0x20], 0);
        chk += (i + 1) * (data[i] - 0x20);
    }
    chk %= 103;
    zprintf(3, "    encode checksum: %02x", chk);
    encode(code128[chk], 0);
    zprintf(3, "    encode STOP: %02x", STOP);
    encode(code128[STOP], 0);
    zprintf(3, "----------------------------------------------------------\n");
}

static void encode_code128c (char *data)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    zprintf(3, "----------------------------------------------------------\n");
    zprintf(2, "CODE-128(C): %s\n", data);
    zprintf(3, "    encode START_C: %02x", START_C);
    encode(code128[START_C], 0);
    int i, chk = START_C;
    for(i = 0; data[i]; i += 2) {
        assert(data[i] >= '0');
        assert(data[i + 1] >= '0');
        unsigned char c = (data[i] - '0') * 10 + (data[i + 1] - '0');
        zprintf(3, "    encode '%c%c': %02d", data[i], data[i + 1], c);
        encode(code128[c], 0);
        chk += (i / 2 + 1) * c;
    }
    chk %= 103;
    zprintf(3, "    encode checksum: %02x", chk);
    encode(code128[chk], 0);
    zprintf(3, "    encode STOP: %02x", STOP);
    encode(code128[STOP], 0);
    zprintf(3, "----------------------------------------------------------\n");
}

/*------------------------------------------------------------*/
/* Code 39 encoding */

static const unsigned int code39[91-32] = {
    0x0c4, 0x000, 0x000, 0x000,  0x0a8, 0x02a, 0x000, 0x000, /* 20 */
    0x000, 0x000, 0x094, 0x08a,  0x000, 0x085, 0x184, 0x0a2, /* 28 */
    0x034, 0x121, 0x061, 0x160,  0x031, 0x130, 0x070, 0x025, /* 30 */
    0x124, 0x064, 0x000, 0x000,  0x000, 0x000, 0x000, 0x000, /* 38 */
    0x000, 0x109, 0x049, 0x148,  0x019, 0x118, 0x058, 0x00d, /* 40 */
    0x10c, 0x04c, 0x01c, 0x103,  0x043, 0x142, 0x013, 0x112, /* 48 */
    0x052, 0x007, 0x106, 0x046,  0x016, 0x181, 0x0c1, 0x1c0, /* 50 */
    0x091, 0x190, 0x0d0,                                     /* 58 */
};

/* FIXME configurable/randomized ratio, ics */
/* FIXME check digit option, ASCII escapes */

static void convert_code39 (char *data)
{
    char *src, *dst;
    for(src = data, dst = data; *src; src++) {
        char c = *src;
        if(c >= 'a' && c <= 'z')
            *(dst++) = c - ('a' - 'A');
        else if(c == ' ' ||
                c == '$' || c == '%' ||
                c == '+' || c == '-' ||
                (c >= '.' && c <= '9') ||
                (c >= 'A' && c <= 'Z'))
            *(dst++) = c;
        else
            /* skip (FIXME) */;
    }
    *dst = 0;
}

static void encode_char39 (unsigned char c,
                           unsigned ics)
{
    assert(0x20 <= c && c <= 0x5a);
    unsigned int raw = code39[c - 0x20];
    if(!raw)
        return; /* skip (FIXME) */

    uint64_t enc = 0;
    int j;
    for(j = 0; j < 9; j++) {
        enc = (enc << 4) | ((raw & 0x100) ? 2 : 1);
        raw <<= 1;
    }
    enc = (enc << 4) | ics;
    zprintf(3, "    encode '%c': %02x%08x: ", c,
            (unsigned)(enc >> 32), (unsigned)(enc & 0xffffffff));
    encode(enc, REV);
}

static void encode_code39 (char *data)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    zprintf(3, "----------------------------------------------------------\n");
    zprintf(2, "CODE-39: %s\n", data);
    encode(0xa, 0);  /* leading quiet */
    encode_char39('*', 1);
    int i;
    for(i = 0; data[i]; i++)
        if(data[i] != '*') /* skip (FIXME) */
            encode_char39(data[i], 1);
    encode_char39('*', 0xa);  /* w/trailing quiet */
    zprintf(3, "----------------------------------------------------------\n");
}

#if 0
/*------------------------------------------------------------*/
/* PDF417 encoding */

/* hardcoded test message: "hello world" */
#define PDF417_ROWS 3
#define PDF417_COLS 3
static const unsigned pdf417_msg[PDF417_ROWS][PDF417_COLS] = {
    { 007, 817, 131 },
    { 344, 802, 437 },
    { 333, 739, 194 },
};

#define PDF417_START UINT64_C(0x81111113)
#define PDF417_STOP  UINT64_C(0x711311121)
#include "pdf417_encode.h"

static int calc_ind417 (int mod,
                        int r,
                        int cols)
{
    mod = (mod + 3) % 3;
    int cw = 30 * (r / 3);
    if(!mod)
        return(cw + cols - 1);
    else if(mod == 1)
        return(cw + (PDF417_ROWS - 1) % 3);
    assert(mod == 2);
    return(cw + (PDF417_ROWS - 1) / 3);
}

static void encode_row417 (int r,
                           const unsigned *cws,
                           int cols,
                           int dir)
{
    int k = r % 3;

    zprintf(3, "    [%d] encode %s:", r, (dir) ? "stop" : "start");
    encode((dir) ? PDF417_STOP : PDF417_START, dir);

    int cw = calc_ind417(k + !dir, r, cols);
    zprintf(3, "    [%d,%c] encode %03d(%d): ", r, (dir) ? 'R' : 'L', cw, k);
    encode(pdf417_encode[cw][k], dir);

    int c;
    for(c = 0; c < cols; c++) {
        cw = cws[c];
        zprintf(3, "    [%d,%d] encode %03d(%d): ", r, c, cw, k);
        encode(pdf417_encode[cw][k], dir);
    }

    cw = calc_ind417(k + dir, r, cols);
    zprintf(3, "    [%d,%c] encode %03d(%d): ", r, (dir) ? 'L' : 'R', cw, k);
    encode(pdf417_encode[cw][k], dir);

    zprintf(3, "    [%d] encode %s:", r, (dir) ? "start" : "stop");
    encode((dir) ? PDF417_START : PDF417_STOP, dir);
}

static void encode_pdf417 (char *data)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    zprintf(3, "----------------------------------------------------------\n");
    zprintf(2, "PDF417: hello world\n");
    encode(0xa, 0);

    int r;
    for(r = 0; r < PDF417_ROWS; r++) {
        encode_row417(r, pdf417_msg[r], PDF417_COLS, r & 1);
        encode(0xa, 0);
    }

    zprintf(3, "----------------------------------------------------------\n");
}
#endif

/*------------------------------------------------------------*/
/* Interleaved 2 of 5 encoding */

static const unsigned char i25[10] = {
    0x06, 0x11, 0x09, 0x18, 0x05, 0x14, 0x0c, 0x03, 0x12, 0x0a,
};

static void encode_i25 (char *data,
                        int dir)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    zprintf(3, "----------------------------------------------------------\n");
    zprintf(2, "Interleaved 2 of 5: %s\n", data);
    zprintf(3, "    encode start:");
    encode((dir) ? 0xa1111 : 0xa112, 0);

    /* FIXME rev case data reversal */
    int i;
    for(i = (strlen(data) & 1) ? -1 : 0; i < 0 || data[i]; i += 2) {
        /* encode 2 digits */
        unsigned char c0 = (i < 0) ? 0 : data[i] - '0';
        unsigned char c1 = data[i + 1] - '0';
        zprintf(3, "    encode '%d%d':", c0, c1);
        assert(c0 < 10);
        assert(c1 < 10);

        c0 = i25[c0];
        c1 = i25[c1];

        /* interleave */
        uint64_t enc = 0;
        int j;
        for(j = 0; j < 5; j++) {
            enc <<= 8;
            enc |= (c0 & 1) ? 0x02 : 0x01;
            enc |= (c1 & 1) ? 0x20 : 0x10;
            c0 >>= 1;
            c1 >>= 1;
        }
        encode(enc, dir);
    }

    zprintf(3, "    encode end:");
    encode((dir) ? 0x211a : 0x1111a, 0);
    zprintf(3, "----------------------------------------------------------\n");
}

/*------------------------------------------------------------*/
/* EAN/UPC encoding */

static const unsigned int ean_digits[10] = {
    0x1123, 0x1222, 0x2212, 0x1141, 0x2311,
    0x1321, 0x4111, 0x2131, 0x3121, 0x2113,
};

static const unsigned int ean_guard[] = {
    0, 0,
    0x11,       /* [2] add-on delineator */
    0x1117,     /* [3] normal guard bars */
    0x2117,     /* [4] add-on guard bars */
    0x11111,    /* [5] center guard bars */
    0x111111    /* [6] "special" guard bars */
};

static const unsigned char ean_parity_encode[] = {
    0x3f,       /* AAAAAA = 0 */
    0x34,       /* AABABB = 1 */
    0x32,       /* AABBAB = 2 */
    0x31,       /* AABBBA = 3 */
    0x2c,       /* ABAABB = 4 */
    0x26,       /* ABBAAB = 5 */
    0x23,       /* ABBBAA = 6 */
    0x2a,       /* ABABAB = 7 */
    0x29,       /* ABABBA = 8 */
    0x25,       /* ABBABA = 9 */
};

static void calc_ean_parity (char *data,
                             int n)
{
    int i, chk = 0;
    for(i = 0; i < n; i++) {
        unsigned char c = data[i] - '0';
        chk += ((i ^ n) & 1) ? c * 3 : c;
    }
    chk %= 10;
    if(chk)
        chk = 10 - chk;
    data[i++] = '0' + chk;
    data[i] = 0;
}

static void encode_ean13 (char *data)
{
    int i;
    unsigned char par = ean_parity_encode[data[0] - '0'];
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);

    zprintf(3, "----------------------------------------------------------\n");
    zprintf(2, "EAN-13: %s (%02x)\n", data, par);
    zprintf(3, "    encode start guard:");
    encode(ean_guard[3], FWD);
    for(i = 1; i < 7; i++, par <<= 1) {
        zprintf(3, "    encode %x%c:", (par >> 5) & 1, data[i]);
        encode(ean_digits[data[i] - '0'], (par >> 5) & 1);
    }
    zprintf(3, "    encode center guard:");
    encode(ean_guard[5], FWD);
    for(; i < 13; i++) {
        zprintf(3, "    encode %x%c:", 0, data[i]);
        encode(ean_digits[data[i] - '0'], FWD);
    }
    zprintf(3, "    encode end guard:");
    encode(ean_guard[3], REV);
    zprintf(3, "----------------------------------------------------------\n");
}

static void encode_ean8 (char *data)
{
    int i;
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    zprintf(3, "----------------------------------------------------------\n");
    zprintf(2, "EAN-8: %s\n", data);
    zprintf(3, "    encode start guard:");
    encode(ean_guard[3], FWD);
    for(i = 0; i < 4; i++) {
        zprintf(3, "    encode %c:", data[i]);
        encode(ean_digits[data[i] - '0'], FWD);
    }
    zprintf(3, "    encode center guard:");
    encode(ean_guard[5], FWD);
    for(; i < 8; i++) {
        zprintf(3, "    encode %c:", data[i]);
        encode(ean_digits[data[i] - '0'], FWD);
    }
    zprintf(3, "    encode end guard:");
    encode(ean_guard[3], REV);
    zprintf(3, "----------------------------------------------------------\n");
}


/*------------------------------------------------------------*/
/* main test flow */

int test_numeric (char *data)
{
    data[strlen(data) & ~1] = 0;
    expect(ZBAR_CODE128, data);
    encode_code128c(data);

    encode_junk(rnd_size);

    expect(ZBAR_I25, data);
    encode_i25(data, FWD);

    encode_junk(rnd_size);

#if 0 /* FIXME encoding broken */
    encode_i25(data, REV);

    encode_junk(rnd_size);
#endif

    calc_ean_parity(data + 2, 12);
    expect(ZBAR_EAN13, data + 2);
    encode_ean13(data + 2);

    encode_junk(rnd_size);

    calc_ean_parity(data + 7, 7);
    expect(ZBAR_EAN8, data + 7);
    encode_ean8(data + 7);

    encode_junk(rnd_size);

    expect(ZBAR_NONE, NULL);
    return(0);
}

int test_alpha (char *data)
{
    expect(ZBAR_CODE128, data);
    encode_code128b(data);

    encode_junk(rnd_size);

    /*encode_code39("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%");*/
    convert_code39(data);
    expect(ZBAR_CODE39, data);
    encode_code39(data);

    encode_junk(rnd_size);

#if 0 /* FIXME decoder unfinished */
    encode_pdf417(data);

    encode_junk(rnd_size);
#endif

    expect(ZBAR_NONE, NULL);
    return(0);
}

int test1 ()
{
    zprintf(2, "----------------------------------------------------------\n");
    if(!seed)
        seed = 0xbabeface;
    zprintf(1, "[%d] SEED=%d\n", iter++, seed);
    srand(seed);

    int i;
    char data[32] = { 0, };
    for(i = 0; i < 14; i++)
        data[i] = (rand() % 10) + '0';

    test_numeric(data);

    for(i = 0; i < 10; i++)
        data[i] = (rand() % 0x5f) + 0x20;
    data[i] = 0;

    test_alpha(data);
    return(0);
}

/* FIXME TBD:
 *   - random module width (!= 1.0)
 *   - simulate scan speed variance
 *   - simulate dark "swelling" and light "blooming"
 *   - inject parity errors
 */

int main (int argc, char **argv)
{
    int n, i, j;
    char *end;

    decoder = zbar_decoder_create();
    zbar_decoder_set_handler(decoder, symbol_handler);
    zbar_decoder_set_config(decoder, 0, ZBAR_CFG_MIN_LEN, 0);

    encode_junk(rnd_size + 1);

    for(i = 1; i < argc; i++) {
        if(argv[i][0] != '-') {
            fprintf(stderr, "ERROR: unknown argument: %s\n", argv[i]);
            return(2);
        }
        for(j = 1; argv[i][j]; j++) {
            switch(argv[i][j])
            {
            case 'q': verbosity = 0; break;
            case 'v': verbosity++; break;
            case 'r':
                seed = time(NULL);
                srand(seed);
                seed = (rand() << 8) ^ rand();
                zprintf(0, "-r SEED=%d\n", seed);
                break;

            case 's':
                if(!argv[i][++j] && !(j = 0) && ++i >= argc) {
                    fprintf(stderr, "ERROR: -s needs <seed> argument\n");
                    return(2);
                }
                seed = strtol(argv[i] + j, &end, 0);
                if((!isdigit(argv[i][j]) && argv[i][j] != '-') ||
                   !seed || seed == LONG_MAX || seed == LONG_MIN) {
                    fprintf(stderr, "ERROR: invalid <seed>: \"%s\"\n",
                            argv[i] + j);
                    return(2);
                }
                j = end - argv[i] - 1;
                break;

            case 'n':
                if(!argv[i][++j] && !(j = 0) && ++i >= argc) {
                    fprintf(stderr, "ERROR: -n needs <num> argument\n");
                    return(2);
                }
                n = strtol(argv[i] + j, &end, 0);
                if(!isdigit(argv[i][j]) || !n) {
                    fprintf(stderr, "ERROR: invalid <num>: \"%s\"\n",
                            argv[i] + j);
                    return(2);
                }
                j = end - argv[i] - 1;

                while(n--) {
                    test1();
                    seed = (rand() << 8) ^ rand();
                }
                break;
            }
        }
    }

    if(!iter)
        test1();

    /* FIXME "Ran %d iterations in %gs\n\nOK\n" */

    zbar_decoder_destroy(decoder);
    return(0);
}
