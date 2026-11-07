/* creative 4 bit adpcm unpacker, shared by play and audiorcv */

#include "adpcm.h"

#define ADPCM_STATES    4

/* all the sums done ahead of time so the 8086 just looks up and adds */

static const signed char adpcm_delta[ADPCM_STATES * 16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 0, -1, -2, -3, -4, -5, -6, -7,
    1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15,
    2, 6, 10, 14, 18, 22, 26, 30, -2, -6, -10, -14, -18, -22, -26, -30,
    4, 12, 20, 28, 36, 44, 52, 60, -4, -12, -20, -28, -36, -44, -52, -60
};

static const unsigned char adpcm_next[ADPCM_STATES * 16] = {
    0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1,
    0, 1, 1, 1, 1, 2, 2, 2, 0, 1, 1, 1, 1, 2, 2, 2,
    1, 2, 2, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 3, 3, 3,
    2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3
};

static int adpcm_ref = 128;
static int adpcm_state;
static int adpcm_primed;

void adpcm_reset(void)
{
    adpcm_primed = 0;
}

int adpcm_expand(unsigned char *dst, unsigned char *src, int n)
{
    unsigned char *out = dst;
    unsigned char *end = src + n;
    const signed char *delta = adpcm_delta;
    const unsigned char *next = adpcm_next;
    int ref = adpcm_ref;
    int base = adpcm_state << 4;

    if (!adpcm_primed && src < end)
    {                           /* first byte is a plain sample the rest build on */
        adpcm_primed = 1;
        ref = *src;
        base = 0;
        *out++ = *src++;
    }
    while (src < end)
    {
        unsigned char b = *src++;
        int k;

        k = base + (b >> 4);
        ref += delta[k];
        if (ref < 0)
            ref = 0;
        else if (ref > 255)
            ref = 255;
        *out++ = (unsigned char) ref;
        base = next[k] << 4;

        k = base + (b & 0x0F);
        ref += delta[k];
        if (ref < 0)
            ref = 0;
        else if (ref > 255)
            ref = 255;
        *out++ = (unsigned char) ref;
        base = next[k] << 4;
    }
    adpcm_ref = ref;
    adpcm_state = base >> 4;
    return (int) (out - dst);
}
