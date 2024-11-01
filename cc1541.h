#ifndef CC1541_H
#define CC1541_H

#define DIRENTRIESPERBLOCK     8
#define DIRTRACK_D41_D71       18
#define DIRTRACK_D81           40
#define SECTORSPERTRACK_D81    40
#define MAXNUMFILES_D81        ((SECTORSPERTRACK_D81 - 3) * DIRENTRIESPERBLOCK)
#define DIRENTRYSIZE           32
#define BLOCKSIZE              256
#define BLOCKOVERHEAD          2
#define TRACKLINKOFFSET        0
#define SECTORLINKOFFSET       1
#define FILETYPEOFFSET         2
#define FILETYPEDEL            0
#define FILETYPESEQ            1
#define FILETYPEPRG            2
#define FILETYPEUSR            3
#define FILETYPEREL            4
#define FILETYPETRANSWARPMASK  0x100
#define FILETRACKOFFSET        3
#define FILESECTOROFFSET       4
#define FILENAMEOFFSET         5
#define FILENAMEMAXSIZE        16
#define FILENAMEEMPTYCHAR      (' ' | 0x80)
#define BAMMESSAGEOFFSET       0xab
#define BAMMESSAGEMAXSIZE      0x100-BAMMESSAGEOFFSET
#define TRANSWARPSIGNATROFFSLO 21
#define TRANSWARPSIGNATURELO   'T'
#define TRANSWARPSIGNATROFFSHI 22
#define TRANSWARPSIGNATUREHI   'W'
#define DIRDATACHECKSUMOFFSET  23
#define TRANSWARPTRACKOFFSET   24
#define FILECHECKSUMOFFSET     25
#define LOADADDRESSLOOFFSET    26
#define LOADADDRESSHIOFFSET    27
#define ENDADDRESSLOOFFSET     28
#define ENDADDRESSHIOFFSET     29
#define FILEBLOCKSLOOFFSET     30
#define FILEBLOCKSHIOFFSET     31
#define D64NUMBLOCKS           (664 + 19)
#define D64SIZE                (D64NUMBLOCKS * BLOCKSIZE)
#define D64SIZE_EXTENDED       (D64SIZE + 5 * 17 * BLOCKSIZE)
#define D71SIZE                (D64SIZE * 2)
#define D81SIZE                (D81NUMTRACKS * SECTORSPERTRACK_D81 * BLOCKSIZE)
#define D64NUMTRACKS           35
#define D64NUMTRACKS_EXTENDED  (D64NUMTRACKS + 5)
#define D71NUMTRACKS           (D64NUMTRACKS * 2)
#define D81NUMTRACKS           80
#define BAM_OFFSET_SPEED_DOS   0xc0
#define BAM_OFFSET_DOLPHIN_DOS 0xac
#define DIRSLOTEXISTS          0
#define DIRSLOTFOUND           1
#define DIRSLOTNOTFOUND        2
/* for sector chain analysis */
#define UNALLOCATED            0 /* unused as of now */
#define ALLOCATED              1 /* part of a valid sector chain */
#define FILESTART              2 /* analysed to be the start of a sector chain */
#define FILESTART_TRUNCATED    3 /* analysed to be the start of a sector chain, was truncated */
#define POTENTIALLYALLOCATED   4 /* currently being analysed */
/* error codes for sector chain validation */
#define VALID                  0 /* valid chain */
#define ILLEGAL_TRACK          1 /* ends with illegal track pointer */
#define ILLEGAL_SECTOR         2 /* ends with illegal sector pointer */
#define LOOP                   3 /* loop in current chain */
#define COLLISION              4 /* collision with other file */
#define CHAINED                5 /* ends at another file start */
#define CHAINED_TRUNCATED      6 /* ends at start of a truncated file */
#define FIRST_BROKEN           7 /* issue already in first sector */
/* undelete levels */
#define RESTORE_DIR_ONLY        0 /* Only restore all dir entries without touching any t/s links */
#define RESTORE_VALID_FILES     1 /* Fix dir entries for files with valid t/s chains */
#define RESTORE_VALID_CHAINS    2 /* Also add wild sector chains with valid t/s chains */
#define RESTORE_INVALID_FILES   3 /* Also fix dir entries with invalid t/s chains */
#define RESTORE_INVALID_CHAINS  4 /* Also add and fix wild invalid t/s chains */
#define RESTORE_INVALID_SINGLES 5 /* Also include single block files */
/* error codes for directory */
#define DIR_OK                 0
#define DIR_ILLEGAL_TS         1
#define DIR_CYCLIC_TS          2

#define TRANSWARP                "TRANSWARP"
#define TRANSWARPBASEBLOCKSIZE   0xc0
#define TRANSWARPBUFFERBLOCKSIZE 0x1f
#define TRANSWARPBLOCKSIZE       (TRANSWARPBASEBLOCKSIZE + TRANSWARPBUFFERBLOCKSIZE)
#define TRANSWARPKEYSIZE         29 /* 232 bits */
#define TRANSWARPKEYHASHROUNDS   33

/* maximum number of files for transwarp order permutation */
#define MAXPERMUTEDFILES 11
/* maximum number of free sectors on an extended D64 */
#define MAXFREESECTORS 768
/* maximum number of patches */
#define MAXNUMPATCHES 32
/* maximum number of patterns for file extraction */
#define MAXNUMEXTRACTIONS 32

typedef struct vic_disk_dir {
    int blocks;
    unsigned char filename[FILENAMEMAXSIZE];
    int filename_length;
    unsigned char type[5];
} vic_disk_dir;

typedef struct vic_disk_info {
    unsigned char header[16];
    unsigned char id[5];
    vic_disk_dir dir[1024];
    int n_dir;
} vic_disk_info;

void extract_prg_from_image(char* image_file, char* prg_name, unsigned char* prg, int* prg_size);
void get_disk_info(char* image_file, vic_disk_info* disk_info);

#endif