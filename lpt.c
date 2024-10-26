#include <sys/io.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

typedef unsigned char vic_byte;

int addr = 0xd100;
int microsleep_offset = 0;

suseconds_t get_microsec();
void microsleep(int duration);

int atn(int value);
int eoi();
void wait_clock(int value);
void set_clock(int value);
void set_data(int value);
int get_data();
char get_byte();

vic_byte last;

int _clock(int value) {
    return (inb(addr+1) & 0x20) != value;
}

int __get_clock() {
    return !(inb(addr+1) & 0x20);
}

int __get_data() {
    return !(inb(addr+1) & 0x40);
}

int wait_clock_true_until_atn() {
    printf("WAIT CLOCK LINE TRUE UNTIL ATN\n");
    while ((inb(addr+1) & 0x20)) {
        if (atn(0))
            return 1;
    }
    return 0;
}

void wait_data(int value) {
    //printf("WAIT DATA LINE %s\n", value ? "TRUE" : "FALSE");
    if (value) 
        value = 0x40;
    while ((inb(addr+1) & 0x40) == value) {}
}


void get_command() {
    printf("%sATN ON\n%s", KRED, KNRM);
            
    set_data(1);
    wait_clock(1);

    do {
        wait_clock(0);
        // TALKER IS READY TO SEND

        set_data(0); // LISTENER IS READY FOR DATA

        wait_clock(1);
        vic_byte byte = get_byte();
        last = byte;
        printf("%sBYTE: %c - %X\n%s", KYEL, byte, byte, KNRM);

        // Frame Handshake
        set_data(1);

        wait_clock(0);
    }
    while (atn(1));

    printf("%sATN OFF\n%s", KRED, KNRM);

}

void _get_data() {
    printf("%sGET DATA ON\n%s", KGRN, KNRM);

    do {
        wait_clock(0);
        // TALKER IS READY TO SEND

        set_data(0); // LISTENER IS READY FOR DATA

        // Intermission: EOI (End-or-Identify)
        int _eoi = 0;
        if (eoi()) {
            set_data(1);
            microsleep(60);
            set_data(0);
            _eoi = 1;
        }

        wait_clock(1);
        vic_byte byte = get_byte();
        printf("%sBYTE: %c - %X\n%s", KYEL, byte, byte, KNRM);

        FILE* fptr;
        fptr = fopen("/home/noelyoung/test", "a");
        putc(byte, fptr);
        fclose(fptr);

        // Frame Handshake
        set_data(1);

        microsleep(100);

        if (_eoi) break;
    }
    while (1);

    
    

    printf("%sGET DATA OFF\n%s", KGRN, KNRM);
}




int main() {
    /*printf("%sred\n", KRED);
    printf("%sgreen\n", KGRN);
    printf("%syellow\n", KYEL);
    printf("%sblue\n", KBLU);
    printf("%smagenta\n", KMAG);
    printf("%scyan\n", KCYN);
    printf("%swhite\n", KWHT);
    printf("%snormal\n", KNRM);*/

    if (ioperm(addr, 3, 1) == -1) {
        if (errno == EPERM) {
            printf("NO ROOT\n");
        }
        return 0;
    }
    
    outb(0xC0, addr+2); // Reset PCR

    int _microsleep_offset = 0;
    for (int i=0; i<100; i++) {
        suseconds_t a = get_microsec();
        microsleep(1);
        _microsleep_offset += get_microsec() - a;
    }
    microsleep_offset = _microsleep_offset * 10;

    while (inb(addr+1) & 0x80) {}
    printf("RESET OK\n");
    
    wait_clock(0);
    
    printf("WAIT\n");

    int _atn = 0;

    while (1) {
        if (atn(1)) {
            _atn = 1;
            get_command();
        }
        if (last != 0x5F && last != 0x60 && last != 0x3F && _atn && _clock(0)) {
            _get_data();
            _atn = 0;
        }
        if (last == 0x60) {
            suseconds_t a;

            printf("TURNAROUND\n");
            
            printf("CLOCK: %d - DATA: %d\n", __get_clock(), __get_data());

            set_data(0);
            set_clock(1);

            printf("CLOCK: %d - DATA: %d\n", __get_clock(), __get_data());

            FILE* fptr = fopen("/home/noelyoung/test2", "r");
            unsigned char c;

            do {
                microsleep(100);
                set_clock(0); // SONO PRONTO A TRASMETTERE
                
                a = get_microsec();
                wait_data(0); 
                printf("%ld\n", get_microsec() - a);
                printf("COMPUTER IS READY FOR DATA\n"); // VIC 20 PRONTO A RICEVERE

                int _eoi = 0;
                c = getc(fptr);
                printf("CARATTERE: %X -> ", c);
                if (c == 0xFF) {
                    _eoi = 1;
                    c = 0;

                    printf("EOI - CLOCK: %d - DATA: %d\n", __get_clock(), __get_data());
                    a = get_microsec();
                    wait_data(1);
                    printf("%ld\n", get_microsec() - a);
                    a = get_microsec();
                    wait_data(0);
                    printf("%ld\n", get_microsec() - a);
                    printf("EOI AKN\n");

                    microsleep(30);
                }

                vic_byte out = 0;

                for (int i=0; i<8; i++) {
                    suseconds_t b = get_microsec();
                    set_clock(1);
                    unsigned char test = (~c >> i) & 1;
                    set_data(test);
                    out |= test << (7-i);
                    while (get_microsec() - b < 60) {}

                    b = get_microsec();
                    set_clock(0);
                    while (get_microsec() - b < 60) {}
                }

                printf("%X\n", out);

                set_clock(1);
                set_data(0);

                wait_data(1);
                

                if (_eoi) {
                    break;
                    microsleep(100);
                }
            }
            while (1);

            fclose(fptr);

            set_clock(0);
            set_data(0);

            last = 0;
            _atn = 0;
        }
    }

    return 0;
}

suseconds_t get_microsec() {
    struct timeval ret;
    gettimeofday(&ret, NULL);
    return (suseconds_t)(ret.tv_sec * 1000000) + ret.tv_usec;
}

void microsleep(int duration) {    
    struct timespec _ts;
    clock_gettime(CLOCK_BOOTTIME, &_ts);

    struct timespec ts;
    ts.tv_sec = _ts.tv_sec;
    ts.tv_nsec = _ts.tv_nsec + (1000 * duration) - microsleep_offset;

    clock_nanosleep(CLOCK_BOOTTIME, TIMER_ABSTIME, &ts, NULL);
}

int atn(int value) {
    return (inb(addr+1) & 0x10) == !value;
}

int eoi() {
    printf("WAIT CLOCK LINE TRUE - CHECK EOI\n");
    suseconds_t a = get_microsec();
    int eoi = 0;
    while (inb(addr+1) & 0x20) {
        suseconds_t elapsed = get_microsec() - a;
        if (elapsed > 200) {
            printf("EOI\n");
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
    printf("GET DATA: %d\n", data);    
    return data;
}

char get_byte() {
    printf("START BYTE TRANSMISSION\n");

    vic_byte byte = 0;

    suseconds_t a = get_microsec();

    // bit 1
    //a = get_microsec();
    wait_clock(0);
    //printf("%ld\n", get_microsec() -a);
    // TALKER HAS DATA READY
    //a = get_microsec();
    byte = get_data();
    wait_clock(1);
    //printf("%ld\n", get_microsec() -a);
    // END BIT TRANSMISSION

    // bit 2
    //a = get_microsec();
    wait_clock(0);
    //printf("%ld\n", get_microsec() -a);
    // TALKER HAS DATA READY
    //a = get_microsec();
    byte |= get_data() << 1;
    wait_clock(1);
    //printf("%ld\n", get_microsec() -a);
    // END BIT TRANSMISSION

    // bit 3
    //a = get_microsec();
    wait_clock(0);
    //printf("%ld\n", get_microsec() -a);
    // TALKER HAS DATA READY
    //a = get_microsec();
    byte |= get_data() << 2;
    wait_clock(1);
    //printf("%ld\n", get_microsec() -a);
    // END BIT TRANSMISSION

    // bit 4
    //a = get_microsec();
    wait_clock(0);
    //printf("%ld\n", get_microsec() -a);
    // TALKER HAS DATA READY
    //a = get_microsec();
    byte |= get_data() << 3;
    wait_clock(1);
    //printf("%ld\n", get_microsec() -a);
    // END BIT TRANSMISSION

    // bit 5
    //a = get_microsec();
    wait_clock(0);
    //printf("%ld\n", get_microsec() -a);
    // TALKER HAS DATA READY
    //a = get_microsec();
    byte |= get_data() << 4;
    wait_clock(1);
    //printf("%ld\n", get_microsec() -a);
    // END BIT TRANSMISSION
    
    // bit 6
    //a = get_microsec();
    wait_clock(0);
    //printf("%ld\n", get_microsec() -a);
    // TALKER HAS DATA READY
    //a = get_microsec();
    byte |= get_data() << 5;
    wait_clock(1);
    //printf("%ld\n", get_microsec() -a);
    // END BIT TRANSMISSION

    // bit 7
    //a = get_microsec();
    wait_clock(0);
    //printf("%ld\n", get_microsec() -a);
    // TALKER HAS DATA READY
    //a = get_microsec();
    byte |= get_data() << 6;
    wait_clock(1);
    //printf("%ld\n", get_microsec() -a);
    // END BIT TRANSMISSION

    // bit 8
    //a = get_microsec();
    wait_clock(0);
    //printf("%ld\n", get_microsec() -a);
    // TALKER HAS DATA READY
    //a = get_microsec();
    byte |= get_data() << 7;
    wait_clock(1);
    //printf("%ld\n", get_microsec() -a);
    // END BIT TRANSMISSION

    printf("%ld\n", get_microsec() -a);
    //while(1){}

    printf("END BYTE TRANSMISSION\n");

    return byte;
}