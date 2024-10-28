#ifndef VIC_IO_H
#define VIC_IO_H

#include <sys/io.h>
#include <stdio.h>

#include "timing.h"
#include "display.h"

#define DEVICE       8

#define LISTEN       0x20
#define UNLISTEN     0x30   // 0x3F
#define TALK         0x40
#define UNTALK       0x50   // 0x5F
#define OPEN_CHANNEL 0x60
#define CLOSE        0xE0
#define OPEN         0xF0

#define MODE_READ    0
#define MODE_WRITE   1

typedef unsigned char vic_byte;

int atn(int value);
void wait_atn(int value);
void wait_clock(int value);
int wait_data(int value, int timeout);
void set_clock(int value);
void set_data(int value);
int get_data();
int eoi();
void handle_atn();
void read_bytes();
void send_bytes();
char get_byte();

void print_command_name(vic_byte command);

#endif