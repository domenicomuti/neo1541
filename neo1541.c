#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#ifdef __linux__
#include <sched.h>
#endif

#include "constants.h"
#include "timing.h"
#include "display.h"
#include "vic_io.h"
#include "device.h"
#include "string_functions.h"

extern int device_resetted;
extern int device_attentioned;
extern int device_listening;
extern int device_talking;

extern int _resetted_message_displayed;

extern int addr;

extern char disk_path[PATH_MAX];

#ifdef _WIN32
extern LARGE_INTEGER lpFrequency;
#endif

int main(int argc, char *argv[]) {

    //fclose(stdout);

#ifdef __linux__
    if (ioperm(addr, 3, 1) == -1) {
        if (errno == EPERM) {
            fprintf(stderr, "ERROR: You must be root\n");
            exit(EXIT_FAILURE);
        }
    }

    struct sched_param _sched_param;
    _sched_param.sched_priority = 99;
    sched_setscheduler(0, SCHED_FIFO, &_sched_param);

    probe_microsleep_offset();
#elif _WIN32
    // TODO: CHECK ADMINISTRATOR MODE ?
    QueryPerformanceFrequency(&lpFrequency);
#endif

    char *_disk_path;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--disk") == 0) {
            if (argc < i + 2) {
                fprintf(stderr, "ERROR: Error parsing argument for --disk\n");
                exit(EXIT_FAILURE);
            }
            _disk_path = argv[i + 1];
        }
    }

    /*_disk_path = calloc(35, sizeof(char));
    strcpy(_disk_path, "image_examples");*/

    trim(_disk_path);
    if (realpath(_disk_path, disk_path) == NULL) {
        if (errno == ENOENT)
            fprintf(stderr, "ERROR: file or directory %s doesn't exists\n", _disk_path);
        else if (errno == EACCES)
            fprintf(stderr, "ERROR: access to file or directory %s is not allowed\n", _disk_path);
        else
            fprintf(stderr, "ERROR: can't open file or directory %s (errno %d)\n", _disk_path, errno);
        exit(EXIT_FAILURE);
    }
    DIR *dir;
    dir = opendir(disk_path);
    if (!dir) {
        char ext[4] = {0};
        substr(ext, disk_path, -3, 3);
        strtolower(ext, 0);
        if (strcmp(ext, "d64") != 0 && strcmp(ext, "d71") != 0 && strcmp(ext, "d81") != 0) {
            char *t = strrchr(disk_path, FILESEPARATOR);
            t[0] = '\0';
        }
    }

    initialize_buffers();
    get_disk_info();

    // TODO: VERIFICARE SE SU WIN FUNZIONA REALPATH (rimuove automaticamente final slash)

    while (1) {
        OUTB(0xC0, addr + 2); // Reset PCR

        while (resetted()) microsleep(1000);

        set_clock(0);   // Release clock and data line
        set_data(0);

        device_resetted = _resetted_message_displayed = 0;
        
        wait_atn(0);
        char _localtime[LOCALTIME_STRLEN];
        get_localtime(_localtime);
        printf("[%s] %sDEVICE WAITING FOR ATN%s\n", _localtime, COLOR_CYAN, COLOR_RESET);

        while (1) {
            //if (!device_attentioned) microsleep(900);
                
            if (atn(1))
                handle_atn();
            else if (device_listening)
                receive_bytes();
            else if (device_talking)
                send_bytes();

            if (device_resetted) {
                reset_device();
                break;
            }
        }
    }

    free_buffers();

    // TODO: catch ctrl + c
    printf("BYE");

    exit(EXIT_SUCCESS);
}