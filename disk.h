#ifndef DISK_H
#define DISK_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <dirent.h>
#include "constants.h"
#include "libs/cc1541.h"
#include "libs/uthash.h"
#include "string_functions.h"
#include "display.h"

typedef struct vic_disk_dir {
    vic_size filesize;
    unsigned short blocks;
    vic_byte filename[FILENAMEMAXSIZE + 2];
    int filename_length;
    char filename_local[NAME_MAX + 1];
    vic_byte type[5];
} vic_disk_dir;

typedef struct vic_disk_info {
    int type;
    vic_byte header[HEADER_SIZE];
    vic_disk_dir dir[MAX_DIRS];
    int n_dir;
    unsigned short blocks_free;
    vic_byte bam_message[BAMMESSAGEMAXSIZE];
    int bam_message_length;
} vic_disk_info;

void get_disk_info();
void extract_file_from_image();
void save_file_to_image();
void directory_listing();
void write_data_buffer();

#endif