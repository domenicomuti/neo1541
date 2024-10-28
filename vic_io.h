#ifndef VIC_IO_H
#define VIC_IO_H

#include <sys/io.h>
#include <stdio.h>

#include "timing.h"
#include "display.h"

typedef unsigned char vic_byte;

int resetted();
int atn(int value);
void wait_atn(int value);
void wait_clock(int value);
int wait_data(int value, int timeout);
void set_clock(int value);
void set_data(int value);
int eoi();

#endif