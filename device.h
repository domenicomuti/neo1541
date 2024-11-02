#ifndef DEVICE_H
#define DEVICE_H

#include "display.h"
#include "vic_io.h"
#include "cc1541.h"

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

int vic_string_equal(vic_byte* string1, vic_byte* string2, int n1, int n2);

void reset_device();
vic_byte get_byte();
void handle_atn();
void read_bytes();
void send_bytes();

void print_command_name(vic_byte command);

#endif