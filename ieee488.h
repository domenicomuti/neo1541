#ifndef IEEE488_H
#define IEEE488_H

#include "display.h"
#include "vic_io.h"

char get_byte();
void handle_atn();
void read_bytes();
void send_bytes();

void print_command_name(vic_byte command);

#endif