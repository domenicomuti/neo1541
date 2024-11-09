#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>

#include "timing.h"
#include "display.h"
#include "vic_io.h"
#include "device.h"

#include "cc1541.h"
#include "string_functions.h"

extern int device_resetted;
extern int device_attentioned;
extern int device_listening;
extern int device_talking;

extern int _resetted_message_displayed;

extern int addr;

char *disk_path;
struct vic_disk_info disk_info;

#ifdef _WIN32
    extern LARGE_INTEGER lpFrequency;
#endif

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--disk") == 0) {
            if (argc < i + 2) {
                fprintf(stderr, "ERROR: Error parsing argument for --disk\n");
                exit(EXIT_FAILURE);
            }
            disk_path = argv[i + 1];
        }
    }

    /*disk_path = calloc(12, sizeof(char));
    strcpy(disk_path, "     C:    ");*/

    int trimmed = trim(disk_path);
    int disk_path_len = strlen(disk_path);

    #ifdef __linux__
        int remove_final_slash = (disk_path_len > 1) && (disk_path[disk_path_len - 1] == FILESEPARATOR);
        int add_final_slash = 0;
    #elif _WIN32
        int remove_final_slash = (disk_path_len > 3) && (disk_path[disk_path_len - 1] == FILESEPARATOR);
        int add_final_slash = (disk_path_len == 2) && (disk_path[disk_path_len - 1] == ':');
    #endif
    if (remove_final_slash) {
        disk_path[disk_path_len - 1] = '\0';
    }
    else if (add_final_slash) {
        if (!trimmed) {
            char *new_disk_path = calloc(disk_path_len + 2, sizeof(char));
            if (new_disk_path == NULL) {
                printf("ERROR: Memory allocation error\n");
                exit(EXIT_FAILURE);
            }
            strcpy(new_disk_path, disk_path);
            new_disk_path[disk_path_len] = FILESEPARATOR;
            free(disk_path);
            disk_path = new_disk_path;
        }
        else {
            disk_path[disk_path_len] = FILESEPARATOR;
            disk_path[disk_path_len + 1] = '\0';
        }
    }

    printf("%s\n", disk_path);

    get_disk_info();

    

    read_directory();

    printf("OK\n");

    #ifdef _WIN32
        //system("pause");
    #endif

    return 0;

    /*printf("TEST\n");

    vic_byte prg_buffer[16338];
    int prg_buffer_i = 0;
    argv[0] = "";
    argv[1] = "-X";
    argv[2] = "omega race";
    argv[3] = "/home/noelyoung/omega.d64";
    
    cc1541(4, argv, prg_buffer, &prg_buffer_i);

    FILE* fptr = fopen("/home/noelyoung/omega_race_neo", "w");

    for (int i = 0; i < prg_buffer_i; i++) {
        //printf("%X ", prg_buffer[i]);
        putc(prg_buffer[i], fptr);
    }

    fclose(fptr);
    printf("%d\n", prg_buffer_i);

    return 0;*/
#ifdef __linux__
    if (ioperm(addr, 3, 1) == -1) {
        if (errno == EPERM) {
            printf("NO ROOT\n");
        }
        return 0;
    }

    struct sched_param _sched_param;
    _sched_param.sched_priority = 99;
    sched_setscheduler(0, SCHED_FIFO, &_sched_param);

    probe_microsleep_offset();
#elif _WIN32
    QueryPerformanceFrequency(&lpFrequency);
#endif
    /*for (int i = 0; i < 20; i++) {
        suseconds_t a = get_microsec();
        microsleep(60);
        printf("%ld\n", get_microsec() - a);
    }
    return 0;*/

    while (1) {
        OUTB(0xC0, addr+2); // Reset PCR

        while (resetted()) {
            microsleep(1000);
        }
        device_resetted = _resetted_message_displayed = 0;
        printf("%sDEVICE RESET OK%s\n", COLOR_GREEN, COLOR_RESET);
        
        wait_atn(0);
        printf("WAITING ATN\n");

        while (1) {
            if (!device_attentioned)
                microsleep(900);
                
            if (atn(1))
                handle_atn();
            else if (device_listening)
                read_bytes();
            else if (device_talking)
                send_bytes();

            if (device_resetted) {
                reset_device();
                break;
            }
        }
    }

    return 0;
}