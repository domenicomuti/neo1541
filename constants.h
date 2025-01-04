#include <limits.h>

#define NEO1541_VERSION           "0.1"

#define DEBUG                     0

#ifdef _WIN32
#define FILESEPARATOR             '\\'
#else
#define FILESEPARATOR             '/'
#endif

#define BAMMESSAGEOFFSET          0xab
#define BAMMESSAGEMAXSIZE         0x100-BAMMESSAGEOFFSET
#define FILENAMEMAXSIZE           16
#define HEADER_SIZE               25     // header 16 chars + id 5 chars + other chars
#define MAX_DIRS                  144

#define CC1541_EXTRACT_FILES      1
#define CC1541_PRINT_DIRECTORY    2
#define CC1541_SAVE_FILE_TO_IMAGE 3

#define LOCALTIME_STRLEN          13

#define DEVICE                    8

#define LISTEN                    (0x20 | DEVICE)
#define UNLISTEN                  0x3F
#define TALK                      (0x40 | DEVICE)
#define UNTALK                    0x5F
#define SECOND                    0x60
#define CLOSE                     0xE0
#define OPEN                      0xF0

#define DISK_DIR                  0
#define DISK_IMAGE                1

#define MAX_DATA_BUFFER_SIZE      168656