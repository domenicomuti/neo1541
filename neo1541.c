#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
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


int screen_width;
int screen_height;

extern WINDOW *header_window;
extern WINDOW *log_window;

int main(int argc, char *argv[]) {

    
    //cbreak();
    //noecho();


    /*screen_width = getmaxx(stdscr);
    screen_height = getmaxy(stdscr);*/

    init_screen();





    //refresh();
    //wrefresh(header_window);
    //wrefresh(log_window);


    //delwin(header);
    //delwin(log_window);



    //fclose(stdout);

    #ifdef __linux__
    if (ioperm(addr, 3, 1) == -1) {
        if (errno == EPERM) {
            wprintw(log_window, "ERROR: You must be root\n");
            exit(EXIT_FAILURE);
        }
    }

    #elif _WIN32
    // TODO: CHECK ADMINISTRATOR MODE ?
    QueryPerformanceFrequency(&lpFrequency);
    #endif

    char *_disk_path;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--disk") == 0) {
            if (argc < i + 2) {
                wprintw(log_window, "ERROR: Error parsing argument for --disk\n");
                exit(EXIT_FAILURE);
            }
            _disk_path = argv[i + 1];
        }
    }

    trim(_disk_path);
    if (realpath(_disk_path, disk_path) == NULL) {
        if (errno == ENOENT)
            wprintw(log_window, "ERROR: file or directory %s doesn't exists\n", _disk_path);
        else if (errno == EACCES)
            wprintw(log_window, "ERROR: access to file or directory %s is not allowed\n", _disk_path);
        else
            wprintw(log_window, "ERROR: can't open file or directory %s (errno %d)\n", _disk_path, errno);
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

    print_header(0, 0);
    wrefresh(header_window);

    // TODO: VERIFICARE SE SU WIN FUNZIONA REALPATH (rimuove automaticamente final slash)

    while (1) {
        OUTB(0xC0, addr + 2); // Reset PCR
        set(CLOCK_LOW | DATA_LOW);   // Release clock and data line
        device_resetted = _resetted_message_displayed = 0;
        wait_atn(0);
        char _localtime[LOCALTIME_STRLEN];
        get_localtime(_localtime);
        wprintw(log_window, "[%s] DEVICE WAITING FOR ATN\n", _localtime);

        while (1) {
            if (atn(1))
                handle_atn();

            if (device_resetted) {
                reset_device();
                break;
            }

            wrefresh(log_window);
        }
    }

    free_buffers();
    endwin();

    // TODO: catch ctrl + c
    printf("BYE");

    exit(EXIT_SUCCESS);
}