#ifndef VIC_IO_H
#define VIC_IO_H

#ifdef __linux__
#include <sys/io.h>
#define INB(port) inb(port)
#define OUTB(value, port) outb(value, port)
#elif _WIN32
#define INB(port) printf("INB(%d)", port)
#define OUTB(value, port) printf("OUTB(%d, %d)", value, port)
#endif

#include <stdio.h>

#include "timing.h"
#include "display.h"

int resetted();
int atn(int value);
void wait_atn(int value);
void wait_clock(int value);
int wait_data(int value, int timeout);
void set_clock(int value);
void set_data(int value);
int eoi();

#endif