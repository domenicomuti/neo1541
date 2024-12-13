#ifndef DEVICE_H
#define DEVICE_H

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "constants.h"
#include "display.h"
#include "vic_io.h"
#include "disk.h"

void initialize_buffers();
void free_buffers();

void reset_device();
void handle_atn();
void send_bytes();
void receive_bytes();
void handle_received_bytes();
void handle_dos_command();
void print_command();

#endif