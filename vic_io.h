#ifndef VIC_IO_H
#define VIC_IO_H

#ifdef __linux__
#include <sys/io.h>

#elif _WIN32
#define INB(port) wprintw(log_window, "INB(%d)", port)
#define OUTB(value, port) wprintw(log_window, "OUTB(%d, %d)", value, port)
#endif

#include <stdio.h>

#include "constants.h"
#include "timing.h"
#include "display.h"
#include "string_functions.h"

#define INB(port) inb(port)
#define OUTB(value, port) outb(value, port)

#define CLOCK_HIGH 0x01
#define CLOCK_LOW  0x02
#define DATA_HIGH  0x04
#define DATA_LOW   0x08

int resetted();
int atn(int value);
void wait_atn(int value);
int wait_clock(int value, int timeout, int check_atn);
int wait_data(int value, int timeout);
void set(vic_byte value);
int eoi();
vic_byte get_byte(int check_atn);

#endif