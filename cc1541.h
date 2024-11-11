#ifndef CC1541_H
#define CC1541_H

#include "constants.h"
#include "string_functions.h"

#ifdef _WIN32
#include <windows.h>
#endif

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

vic_byte p2a(vic_byte p);
void extract_file_from_image();
void get_disk_info();

#endif