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

void wait_atn(int value) {
    //printf("WAIT ATN\n");
    if (value) 
        value = 0x10;
    while ((inb(addr+1) & 0x10) == value) {}
}

void wait_clock(int value) {
    //printf("WAIT CLOCK LINE %s\n", value ? "TRUE" : "FALSE");
    if (value) 
        value = 0x20;
    while ((inb(addr+1) & 0x20) == value) {}
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

int get_data() {
    int data = (inb(addr+1) & 0x40) >> 6;
    //printf("GET DATA: %d\n", data);    
    return data;
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

void send_bytes() {
    printf("TURNAROUND\n");

    set_data(0);
    set_clock(1);

    FILE* fptr = fopen("/home/noelyoung/Synthesong.prg", "rb");
    vic_byte c;

    fseek(fptr, 0, SEEK_END);
    int file_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    int i_file = 0;
    int retry = 0;

    do {
        if (retry == 20) {
            set_clock(0);
            set_data(0);
            device_attentioned = device_listening = device_talking = 0;
            break;
        }

        microsleep(100);
        set_clock(0); // DEVICE IS READY TO SEND
        wait_data(0, 0); // COMPUTER IS READY FOR DATA

        int eoi = 0;
        
        c = getc(fptr);
        //printf("%X - %c - 0x%X\n", i_file, c, c);
        i_file++;

        if (i_file == file_size) {
            eoi = 1;
            wait_data(1, 0);
            wait_data(0, 0);
            microsleep(30);
        }

        for (int i=0; i<8; i++) {
            suseconds_t a = get_microsec();
            set_clock(1);
            set_data((~c >> i) & 1);
            while ((get_microsec() - a) < 60) {}

            a = get_microsec();
            set_clock(0);
            while ((get_microsec() - a) < 60) {}
        }

        set_clock(1);
        set_data(0);

        if (!wait_data(1, 1000)) {
            retry++;
            //printf("%sTIMEOUT, RETRY%s\n", KRED, KNRM);
            i_file--;
            printf("%X - %c - 0x%X - %d\n", i_file, c, c, retry);
            fseek(fptr, i_file, SEEK_SET);
            continue;
        }
        retry = 0;

        if (eoi) break;
    }
    while (1);

    set_clock(0);
    set_data(0);

    fclose(fptr);
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

    vic_byte secondary = command & 0x0F;
    if (secondary != 0x0F) printf("%X", secondary);
    printf("\n");

}