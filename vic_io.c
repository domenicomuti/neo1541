#include "vic_io.h"

extern int device_resetted;

int addr = 0xd100;

int _resetted_message_displayed = 0;
int resetted() {
    int _resetted = (inb(addr+1) & 0x80) == 0x80;
    if (_resetted && !_resetted_message_displayed) {
        _resetted_message_displayed = 1;
        printf("%sMACHINE RESET%s\n", COLOR_RED, COLOR_RESET);
    }
    return _resetted;
}

int atn(int value) {
    if (resetted()) {
        device_resetted = 1;
        return 0;
    }
    return (inb(addr+1) & 0x10) == !value;
}

void wait_atn(int value) {
    //printf("WAIT ATN\n");
    if (value) 
        value = 0x10;
    while ((inb(addr+1) & 0x10) == value) {
        if (resetted()) {
            device_resetted = 1;
            break;
        }
    }
}

void wait_clock(int value) {
    //printf("WAIT CLOCK LINE %s\n", value ? "TRUE" : "FALSE");
    if (value) 
        value = 0x20;
    while ((inb(addr+1) & 0x20) == value) {
        if (resetted()) {
            device_resetted = 1;
            break;
        }
    }
}

int wait_data(int value, int timeout) {
    //printf("WAIT DATA LINE %s\n", value ? "TRUE" : "FALSE");
    if (value) 
        value = 0x40;
    suseconds_t a = get_microsec();
    while ((inb(addr+1) & 0x40) == value) {
        if ((timeout > 0) && ((get_microsec() - a) > timeout))
            return 0;
    }
    return 1;
}

void set_clock(int value) {
    //printf("SET CLOCK %s\n", value ? "TRUE" : "FALSE");
    if (value)
        outb((inb(addr+2) | 2), addr+2); // set Clock to 1 (Commodore True)
    else
        outb((inb(addr+2) & ~2), addr+2); // set Clock to 0 (Commodore False)
        
}

void set_data(int value) {
    //printf("SET DATA %s\n", value ? "TRUE" : "FALSE");
    if (value)
        outb((inb(addr+2) & ~4), addr+2); // set Data to 0 (Commodore True)
    else
        outb((inb(addr+2) | 4), addr+2); // set Data to 1 (Commodore False)
}

int eoi() {
    //printf("WAIT CLOCK LINE TRUE - CHECK EOI\n");
    suseconds_t a = get_microsec();
    suseconds_t elapsed;
    int eoi = 0;
    while (inb(addr+1) & 0x20) {
        elapsed = get_microsec() - a;
        if (elapsed > 200) {
            //printf("EOI\n");
            eoi = 1;
            break;
        }
    }
    return eoi;
}