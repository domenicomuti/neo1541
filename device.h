#ifndef DEVICE_H
#define DEVICE_H

#include "display.h"
#include "vic_io.h"

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

void reset_device();
char get_byte();
void handle_atn();
void read_bytes();
void send_bytes();

void print_command_name(vic_byte command);

#endif