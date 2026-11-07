#ifndef ADPCM_H
#define ADPCM_H

void adpcm_reset(void);
int adpcm_expand(unsigned char *dst, unsigned char *src, int n);

#endif
