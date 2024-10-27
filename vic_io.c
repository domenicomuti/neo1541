#include "vic_io.h"

int addr = 0xd100;

int device_attentioned = 0;
int device_listening = 0;
int device_talking = 0;
int open_mode;

vic_byte data_buffer[174848];
int i_data_buffer = 0;

vic_byte filename[100];
int i_filename = 0;

vic_byte last_command;

int atn(int value) {
    return (inb(addr+1) & 0x10) == !value;
}

int eoi() {
    //printf("WAIT CLOCK LINE TRUE - CHECK EOI\n");
    suseconds_t a = get_microsec();
    int eoi = 0;
    while (inb(addr+1) & 0x20) {
        suseconds_t elapsed = get_microsec() - a;
        if (elapsed > 200) {
            //printf("EOI\n");
            eoi = 1;
            break;
        }
    }
    return eoi;
}

void wait_clock(int value) {
    //printf("WAIT CLOCK LINE %s\n", value ? "TRUE" : "FALSE");
    if (value) 
        value = 0x20;
    while ((inb(addr+1) & 0x20) == value) {}
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

int get_data() {
    int data = (inb(addr+1) & 0x40) >> 6;
    //printf("GET DATA: %d\n", data);    
    return data;
}

char get_byte() {
    vic_byte byte = 0;

    //printf("START BYTE TRANSMISSION\n");

    //suseconds_t a = get_microsec();

    // bit 1
    wait_clock(0);
    byte = get_data();
    wait_clock(1);

    // bit 2
    wait_clock(0);
    byte |= get_data() << 1;
    wait_clock(1);

    // bit 3
    wait_clock(0);
    byte |= get_data() << 2;
    wait_clock(1);

    // bit 4
    wait_clock(0);
    byte |= get_data() << 3;
    wait_clock(1);

    // bit 5
    wait_clock(0);
    byte |= get_data() << 4;
    wait_clock(1);
    
    // bit 6
    wait_clock(0);
    byte |= get_data() << 5;
    wait_clock(1);

    // bit 7
    wait_clock(0);
    byte |= get_data() << 6;
    wait_clock(1);

    // bit 8
    wait_clock(0);
    byte |= get_data() << 7;
    wait_clock(1);

    //printf("%ld\n", get_microsec() - a);
    
    //printf("END BYTE TRANSMISSION\n");

    return byte;
}

void handle_atn() {
    printf("%sATN ON\n%s", KRED, KNRM);
            
    set_data(1);
    wait_clock(1);

    do {
        wait_clock(0); // TALKER IS READY TO SEND
        set_data(0);   // LISTENER IS READY FOR DATA

        wait_clock(1);
        vic_byte command = get_byte();
        last_command = command;

        print_command_name(command);

        //printf("BYTE: %c - %X\n", command, command);

        if ((command & 0xF0) == LISTEN) {
            device_attentioned = device_listening = (command & 0x0F) == DEVICE;
            device_talking = 0;
        }
        else if((command & 0xF0) == UNLISTEN) {
            device_listening = 0;
        }
        else if ((command & 0xF0) == OPEN) {
            open_mode = command & 0x0F;
        }
        else if ((command & 0xF0) == TALK) {
            device_listening = 0;
            device_attentioned = device_talking = (command & 0x0F) == DEVICE;
        }
        else if ((command & 0xF0) == UNTALK) {
            device_talking = 0;
        }

        if (!device_attentioned) {
            wait_atn(0);
            continue;
        }

        set_data(1); // Frame Handshake
        wait_clock(0);

        if ((command & 0xF0) == UNLISTEN) {
            device_attentioned = 0;
        }
        else if ((command & 0xF0) == UNTALK) {
            device_attentioned = 0;
        }
    }
    while (atn(1));

    printf("%sATN OFF\n%s", KRED, KNRM);

}

void read_bytes() {
    printf("%sGET DATA ON\n%s", KGRN, KNRM);

    do {
        wait_clock(0); // TALKER IS READY TO SEND
        set_data(0);   // LISTENER IS READY FOR DATA

        int _eoi = 0;
        if (eoi()) {
            set_data(1);
            microsleep(60);
            set_data(0);
            _eoi = 1;
        }

        wait_clock(1);
        data_buffer[i_data_buffer] = get_byte();
        printf("%s%c - 0x%X\n%s", KYEL, data_buffer[i_data_buffer], data_buffer[i_data_buffer], KNRM);

        i_data_buffer++;

        set_data(1); // Frame Handshake

        microsleep(100);

        if (_eoi) break;
    }
    while (1);

    if ((last_command & 0xF0) == OPEN) {
        for (int i=0; i<i_data_buffer; i++) {
            filename[i] = data_buffer[i];
        }
        i_filename = i_data_buffer;
        i_data_buffer = 0;
    }

    printf("%sGET DATA OFF\n%s", KGRN, KNRM);
}

void wait_data(int value) {
    //printf("WAIT DATA LINE %s\n", value ? "TRUE" : "FALSE");
    suseconds_t a = get_microsec();
    if (value) 
        value = 0x40;
    while ((inb(addr+1) & 0x40) == value) {}
}

void wait_atn(int value) {
    printf("WAIT ATN\n");
    if (value) 
        value = 0x10;
    while ((inb(addr+1) & 0x10) == value) {}
}

void print_command_name(vic_byte command) {
    switch (command & 0xF0) {
        case LISTEN:
            printf("LISTEN ");
            break;
        case UNLISTEN:
            printf("UNLISTEN ");
            break;
        case TALK:
            printf("TALK ");
            break;
        case UNTALK:
            printf("UNTALK ");
            break;
        case OPEN_CHANNEL:
            printf("OPEN_CHANNEL ");
            break;
        case CLOSE:
            printf("CLOSE ");
            break;
        case OPEN:
            printf("OPEN ");
            break;
    }
    printf("%X\n", command & 0x0F);
}