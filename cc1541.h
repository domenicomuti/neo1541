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
    unsigned char filename[FILENAMEMAXSIZE + 2];
    int filename_length;
    char filename_local[NAME_MAX + 1];
    unsigned char type[5];
} vic_disk_dir;

typedef struct vic_disk_info {
    int type;
    unsigned char header[HEADER_SIZE];
    vic_disk_dir dir[MAX_DIRS];
    int n_dir;
    unsigned short blocks_free;
    unsigned char bam_message[BAMMESSAGEMAXSIZE];
    int bam_message_length;
} vic_disk_info;

unsigned char p2a(unsigned char p);
void extract_prg_from_image(char *filename, vic_string *data_buffer);
void get_disk_info();

#endif