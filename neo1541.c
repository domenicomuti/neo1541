#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>

//#include <dirent.h>

#include "timing.h"
#include "display.h"
#include "vic_io.h"
#include "device.h"

#include "cc1541.h"

extern int device_resetted;
extern int device_attentioned;
extern int device_listening;
extern int device_talking;

extern int _resetted_message_displayed;

extern int addr;

char* image;

int main(int argc, char* argv[]) {
    for (int i=1; i<argc-1; i++) {
        if (strcmp(argv[i], "--image") == 0) {
            image = argv[i+1];
        }
    }

    image = "image_examples/omega.d64";

    /*struct vic_disk_info disk_info;
    get_disk_info("/home/noelyoung/omega.d64", &disk_info);

    printf("OK\n");

    return 0;*/




    /*DIR *d;

    struct dirent *dir;

    d = opendir(image);

    if (d) {

        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.') continue;
            printf("%s %ld\n", dir->d_name, strlen(dir->d_name));
        }
        closedir(d);
    }

    return 0;*/



    /*printf("TEST\n");

    vic_byte prg_buffer[16338];
    int prg_buffer_i = 0;
    argv[0] = "";
    argv[1] = "-X";
    argv[2] = "omega race";
    argv[3] = "/home/noelyoung/omega.d64";
    
    cc1541(4, argv, prg_buffer, &prg_buffer_i);

    FILE* fptr = fopen("/home/noelyoung/omega_race_neo", "w");

    for (int i=0; i<prg_buffer_i; i++) {
        //printf("%X ", prg_buffer[i]);
        putc(prg_buffer[i], fptr);
    }

    fclose(fptr);
    printf("%d\n", prg_buffer_i);

    return 0;*/

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

    /*for (int i=0; i<20; i++) {
        suseconds_t a = get_microsec();
        microsleep(60);
        printf("%ld\n", get_microsec() - a);
    }
    return 0;*/

    while (1) {
        outb(0xC0, addr+2); // Reset PCR

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