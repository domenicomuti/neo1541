#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>

#include "timing.h"
#include "display.h"
#include "vic_io.h"
#include "ieee488.h"

extern int reset;
extern int device_attentioned;
extern int device_listening;
extern int device_talking;

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
        reset = 0;
        printf("RESET OK\n");
        
        wait_clock(0);
        printf("WAIT\n");

        while (1) {
            if (!device_attentioned) {
                microsleep(900);
            }

            if (atn(1)) {
                handle_atn();
            }
            else if (device_listening) {
                read_bytes();
                device_listening = 0;
            }
            else if (device_talking) {
                send_bytes();
                device_talking = 0;
            }

            if (reset) {
                reset = device_attentioned = device_listening = device_talking = 0;
                break;
            }
        }
    }

    return 0;
}