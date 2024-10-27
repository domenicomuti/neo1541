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
            suseconds_t a;

            printf("TURNAROUND\n");
            
            //printf("CLOCK: %d - DATA: %d\n", __get_clock(), __get_data());

            set_data(0);
            set_clock(1);

            //printf("CLOCK: %d - DATA: %d\n", __get_clock(), __get_data());

            FILE* fptr = fopen("/home/noelyoung/test2", "r");
            unsigned char c;

            do {
                microsleep(100);
                set_clock(0); // SONO PRONTO A TRASMETTERE
                
                //a = get_microsec();
                wait_data(0); 
                //printf("%ld\n", get_microsec() - a);
                //printf("COMPUTER IS READY FOR DATA\n"); // VIC 20 PRONTO A RICEVERE

                int _eoi = 0;
                c = getc(fptr);
                printf("%c - 0x%X\n", c, c);
                if (c == 0xFF) {
                    _eoi = 1;
                    c = 0;

                    //printf("EOI - CLOCK: %d - DATA: %d\n", __get_clock(), __get_data());
                    //a = get_microsec();
                    wait_data(1);
                    //printf("%ld\n", get_microsec() - a);
                    //a = get_microsec();
                    wait_data(0);
                    //printf("%ld\n", get_microsec() - a);
                    //printf("EOI AKN\n");

                    microsleep(30);
                }

                vic_byte out = 0;

                for (int i=0; i<8; i++) {
                    suseconds_t b = get_microsec();
                    set_clock(1);
                    set_data((~c >> i) & 1);
                    while (get_microsec() - b < 60) {}

                    b = get_microsec();
                    set_clock(0);
                    while (get_microsec() - b < 60) {}
                }

                set_clock(1);
                set_data(0);

                wait_data(1);
                

                if (_eoi) {
                    break;
                }
            }
            while (1);

            fclose(fptr);

            set_clock(0);
            set_data(0);

            device_talking = 0;
        }
    }

    return 0;
}