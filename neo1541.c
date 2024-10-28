#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "timing.h"
#include "display.h"
#include "vic_io.h"

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
    
    outb(0xC0, addr+2); // Reset PCR

    probe_microsleep_offset();

    while (inb(addr+1) & 0x80) {}
    printf("RESET OK\n");
    
    wait_clock(0);
    
    printf("WAIT\n");

    while (1) {
        if (!device_attentioned) {
            microsleep(950);
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
    }

    return 0;
}