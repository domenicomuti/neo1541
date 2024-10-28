#include "ieee488.h"

extern int addr;

vic_byte last_command;

int reset = 1;
int device_attentioned = 0;
int device_listening = 0;
int device_talking = 0;
int open_mode;

vic_byte data_buffer[174848];
int i_data_buffer = 0;

vic_byte filename[100];
int i_filename = 0;

char get_byte() {
    vic_byte byte = 0;

    //printf("START BYTE TRANSMISSION\n");

    //suseconds_t a = get_microsec();

    // bit 1
    wait_clock(0);
    byte = (inb(addr+1) & 0x40) >> 6;
    wait_clock(1);

    // bit 2
    wait_clock(0);
    byte |= (inb(addr+1) & 0x40) >> 5;
    wait_clock(1);

    // bit 3
    wait_clock(0);
    byte |= (inb(addr+1) & 0x40) >> 4;
    wait_clock(1);

    // bit 4
    wait_clock(0);
    byte |= (inb(addr+1) & 0x40) >> 3;
    wait_clock(1);

    // bit 5
    wait_clock(0);
    byte |= (inb(addr+1) & 0x40) >> 2;
    wait_clock(1);
    
    // bit 6
    wait_clock(0);
    byte |= (inb(addr+1) & 0x40) >> 1;
    wait_clock(1);

    // bit 7
    wait_clock(0);
    byte |= (inb(addr+1) & 0x40);
    wait_clock(1);

    // bit 8
    wait_clock(0);
    byte |= (inb(addr+1) & 0x40) << 1;
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
        else if ((command & 0xF0) == OPEN_CHANNEL) {
            //
        }
        else if ((command & 0xF0) == CLOSE) {
            //
        }
        else {
            printf("NO COMMAND RECEIVED %d\n", reset);
            device_attentioned = device_listening = device_talking = 0;
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
    while (atn(1) && !reset);

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

    microsleep(80);

    do {
        if (resetted()) {
            printf("--- RESET ---\n");
            reset = 1;
            return;
        }
        if (atn(1)) {
            set_clock(0);
            printf("--- ERROR ---\n");
            return;
        }
        
        set_clock(0); // DEVICE IS READY TO SEND
        wait_data(0, 0); // COMPUTER IS READY FOR DATA

        int eoi = 0;
        
        c = getc(fptr);
        //printf("%X - %c - 0x%X\n", i_file, c, c);
        i_file++;

        if (i_file == file_size) {
            eoi = 1;
            microsleep(200);
            wait_data(1, 0);
            wait_data(0, 0);
            microsleep(30);
        }

        suseconds_t a;
        for (int i=0; i<8; i++) {
            a = get_microsec();
            set_clock(1);
            set_data((~c >> i) & 1);
            while ((get_microsec() - a) < 60) {}
            //microsleep(60 - (get_microsec() - a));

            a = get_microsec();
            set_clock(0);
            while ((get_microsec() - a) < 60) {}
            //microsleep(60 - (get_microsec() - a));
            set_data(0);
        }

        set_clock(1);

        if (!wait_data(1, 1000)) {
            retry++;
            //printf("%sTIMEOUT, RETRY%s\n", KRED, KNRM);
            i_file--;
            printf("%X - %c - 0x%X - %d\n", i_file, c, c, retry);
            fseek(fptr, i_file, SEEK_SET);
            microsleep(100);
            continue;
        }
        retry = 0;

        microsleep(100); // BETWEEN BYTES TIME

        if (eoi) break;
    }
    while (1);

    set_clock(0);
    set_data(0);

    fclose(fptr);
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
        default:
            printf("%X\n", command);
            return;
    }

    vic_byte secondary = command & 0x0F;
    if (secondary != 0x0F) printf("%X", secondary);
    printf("\n");
}