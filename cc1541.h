#ifndef CC1541_H
#define CC1541_H

#define BAMMESSAGEOFFSET  0xab
#define BAMMESSAGEMAXSIZE 0x100-BAMMESSAGEOFFSET
#define FILENAMEMAXSIZE   16
#define HEADER_SIZE       25
#define MAX_DIRS          144

typedef struct vic_disk_dir {
    unsigned short blocks;
    unsigned char filename[FILENAMEMAXSIZE + 2];
    unsigned char type[5];
} vic_disk_dir;

typedef struct vic_disk_info {
    unsigned char header[HEADER_SIZE]; // header 16 chars + id 5 chars + other chars
    vic_disk_dir dir[MAX_DIRS];
    int n_dir;
    unsigned short blocks_free;
    unsigned char bam_message[BAMMESSAGEMAXSIZE];
    int bam_message_length;
} vic_disk_info;

void extract_prg_from_image(char *prg_name, unsigned char *prg, int *prg_size);
int get_disk_info(vic_disk_info* disk_info);

#endif