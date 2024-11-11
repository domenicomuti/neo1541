#include <limits.h>

#ifdef _WIN32
#define FILESEPARATOR        '\\'
#else
#define FILESEPARATOR        '/'
#endif

#define BAMMESSAGEOFFSET     0xab
#define BAMMESSAGEMAXSIZE    0x100-BAMMESSAGEOFFSET
#define FILENAMEMAXSIZE      16
#define HEADER_SIZE          25     // header 16 chars + id 5 chars + other chars
#define MAX_DIRS             144

#define DEVICE               8

#define LISTEN               0x20
#define UNLISTEN             0x30   // 0x3F
#define TALK                 0x40
#define UNTALK               0x50   // 0x5F
#define OPEN_CHANNEL         0x60
#define CLOSE                0xE0
#define OPEN                 0xF0

#define MODE_READ            0
#define MODE_WRITE           1

#define DISK_DIR             0
#define DISK_IMAGE           1

#define MAX_DATA_BUFFER_SIZE 168656