/*
 * Architecture Specific stubs
 */
#include <stdio.h>
#include <stdlib.h>
#if __ia16__
#include <arch/io.h>
#else
#define inb(port)           0
#define inw(port)           0
#define outb(value,port)
#define outw(value,port)
#endif
#include "host.h"
#include "basic.h"

#define VIDEO_05_G320x200   0x0005
#define VIDEO_03_T80x25     0x0003

extern FILE *outfile;

static int gmode = 0;

typedef struct {
    int x;
    int y;
    int fgc;
    int bgc;
    int r;
} xyc_t;

static xyc_t gxyc = {0, 0, 7, 0, 1};

#if __ia16__
void int_10(unsigned int ax, unsigned int bx,
            unsigned int cx, unsigned int dx)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("int $0x10;"
                      :
                      :"a" (ax), "b" (bx), "c" (cx), "d" (dx)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}
#endif

void host_digitalWrite(int pin,int state) {
}

int host_digitalRead(int pin) {
    return 0;
}

int host_analogRead(int pin) {
    return 0;
}

void host_pinMode(int pin,int mode) {
}

void host_mode(int mode) {
    gmode = mode;

    if (gmode)
        int_10(VIDEO_05_G320x200, 0, 0, 0);
    else
        int_10(VIDEO_03_T80x25, 0, 0, 0);
}

void host_cls() {
    fprintf(outfile, "\033[H\033[2J");
}

void host_color(int fgc, int bgc) {

    if (gmode) {
        gxyc.fgc = fgc;
        gxyc.bgc = bgc;
    }
}

void host_plot(int x, int y) {

    if (gmode) {
        y = 199 - y;

        int_10((0x0C00 | (0xFF & gxyc.fgc)), 0, x, y);

        gxyc.x = x;
        gxyc.y = y;
    }
}

void host_draw(int x, int y) {

    int nx;
    int ny;
    int ydiff;
    int nydiff;

    nx = gxyc.x;
    ny = gxyc.y;

    if (gmode) {
        if (nx < x)
            ydiff = (y - ny) / (x - nx);
        else if (nx > x)
            ydiff = (y - ny) / (nx - x);
        else
            ydiff = 0;

        if (nx < x) {
            while (nx < x) {
                nx++;
                nydiff = ny + ydiff;
                if (ny < nydiff) {
                    while (ny < nydiff) {
                        ny++;
                        host_plot(nx, ny);
                    }
                }
                else if (ny > nydiff) {
                    while (ny > nydiff) {
                        ny--;
                        host_plot(nx, ny);
                    }
                }
                else
                    host_plot(nx, ny);
            }
        }
        else if (nx > x) {
            while (nx > x) {
                nx--;
                nydiff = ny + ydiff;
                if (ny < nydiff) {
                    while (ny < nydiff) {
                        ny++;
                        host_plot(nx, ny);
                    }
                }
                else if (ny > nydiff) {
                    while (ny > nydiff) {
                        ny--;
                        host_plot(nx, ny);
                    }
                }
                else
                    host_plot(nx, ny);
            }
        }
        else if (nx == x) {
            if (ny < y) {
                while (ny < y) {
                    ny++;
                    host_plot(nx, ny);
                }
            }
            else if (ny > y) {
                while (ny > y) {
                    ny--;
                    host_plot(nx, ny);
                }
            }
        }

        host_plot(x, y);
    }
}

void host_circle(int x, int y, int r) {
}

void host_outb(int port, int value) {
    outb(value, port);
}

void host_outw(int port, int value) {
    outw(value, port);
}

int host_inpb(int port) {
    return inb(port);
}

int host_inpw(int port) {
    return inw(port);
}
