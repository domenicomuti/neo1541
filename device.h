#ifndef DEVICE_H
#define DEVICE_H

#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include "constants.h"
#include "display.h"
#include "vic_io.h"
#include "cc1541.h"
#include "string_functions.h"

void initialize_buffers();
void free_buffers();

void reset_device();
vic_byte get_byte();
void handle_atn();
void receive_bytes();
void directory_listing();
void load_file();
void send_bytes();

int valid_command(vic_byte command);

#endif