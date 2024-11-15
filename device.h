#ifndef DEVICE_H
#define DEVICE_H

#include <stdlib.h>
#include <errno.h>
#include "constants.h"
#include "display.h"
#include "vic_io.h"
#include "disk.h"

void initialize_buffers();
void free_buffers();

void reset_device();
void handle_atn();
vic_byte get_byte();
void send_bytes();
void receive_bytes();
int valid_command(vic_byte command);

#endif