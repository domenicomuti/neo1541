#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>

#include <string.h>

#include "timing.h"
#include "display.h"
#include "vic_io.h"
#include "device.h"

extern int device_resetted;
extern int device_attentioned;
extern int device_listening;
extern int device_talking;

extern int _resetted_message_displayed;

extern int addr;

int main() {
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