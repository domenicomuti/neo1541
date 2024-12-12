#include "vic_io.h"

extern int device_resetted;

int addr = 0xd100;

#ifdef _WIN32
extern LARGE_INTEGER lpFrequency;
#endif

int _resetted_message_displayed = 0;
int resetted() {
    int _resetted = (INB(addr + 1) & 0x80) == 0x80;
    if (_resetted && !_resetted_message_displayed) {
        _resetted_message_displayed = 1;
        printf("%sMACHINE HALTED%s\n", COLOR_RED, COLOR_RESET);
    }
    return _resetted;
}

int atn(int value) {
    if (resetted()) {
        device_resetted = 1;
        return 0;
    }
    return (INB(addr + 1) & 0x10) == !value;
}

void wait_atn(int value) {
    #if DEBUG
    printf("watn ");
    #endif
    
    if (value) 
        value = 0x10;
    while ((INB(addr + 1) & 0x10) == value) {
        if (resetted()) {
            device_resetted = 1;
            break;
        }
    }
}

int wait_clock(int value, int timeout, int check_atn) {
    #if DEBUG
    printf("wc%d(%d) ", value, timeout);
    #endif

    if (value) 
        value = 0x20;
    suseconds_t a = get_microsec();
    vic_byte in = INB(addr + 1);

    while ((in & 0x20) == value) {
        if (check_atn && ((in & 0x10) == 0x10)) {   // if check for atn low and atn is low
            printf("EXIT ATN ");
            return 1;
        }
        else if ((timeout > 0) && ((get_microsec() - a) > timeout)) {
            return 1;
        }
        else if (resetted()) {
            device_resetted = 1;
            return 1;
        }
        in = INB(addr + 1);
    }
    return 0;
}

int wait_data(int value, int timeout) {
    #if DEBUG
    printf("wd%d(%d) ", value, timeout);
    #endif

    if (value) 
        value = 0x40;

    #ifdef __linux__
    suseconds_t a = get_microsec();
    while ((INB(addr + 1) & 0x40) == value) {
        if ((timeout > 0) && ((get_microsec() - a) > timeout)) {
            #if DEBUG
            printf("%sTIMEOUT %s%s\n", COLOR_RED, __func__, COLOR_RESET);
            #endif
            return 0;
        }       
    }
    #elif _WIN32
    LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
    QueryPerformanceCounter(&StartingTime);
    while ((INB(addr + 1) & 0x40) == value) {
        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= lpFrequency.QuadPart;

        if ((timeout > 0) && (ElapsedMicroseconds.QuadPart > timeout))
            return 0;
    }
    #endif

    return 1;
}

void set(vic_byte value) {
    vic_byte in_value = INB(addr + 2);
    vic_byte out_value = in_value;
    if ((value & CLOCK_HIGH) == CLOCK_HIGH) {
        out_value |= 2;    // set Clock to 1 (Commodore True)
        #if DEBUG
        printf("sc1 ");
        #endif
    }
    if ((value & CLOCK_LOW) == CLOCK_LOW) {
        out_value &= ~2;   // set Clock to 0 (Commodore False)
        #if DEBUG
        printf("sc0 ");
        #endif
    }
    if ((value & DATA_HIGH) == DATA_HIGH) {
        out_value &= ~4;   // set Data to 0 (Commodore True)
        #if DEBUG
        printf("sd1 ");
        #endif
    }
    if ((value & DATA_LOW) == DATA_LOW) {
        out_value |= 4;    // set Data to 1 (Commodore False)
        #if DEBUG
        printf("sd0 ");
        #endif
    }
    OUTB(out_value, addr + 2);
}

int eoi() {
    #if DEBUG
    printf("eoi? ");
    #endif

    #ifdef __linux__
    suseconds_t a = get_microsec();
    suseconds_t elapsed;
    int eoi = 0;
    while (INB(addr + 1) & 0x20) {
        elapsed = get_microsec() - a;
        if (elapsed > 200) {
            #if DEBUG
            printf("eoi ");
            #endif
            eoi = 1;
            break;
        }
    }
    #elif _WIN32
    LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
    QueryPerformanceCounter(&StartingTime);
    int eoi = 0;
    while (INB(addr + 1) & 0x20) {
        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= lpFrequency.QuadPart;

        if (ElapsedMicroseconds.QuadPart > 200) {
            //printf("EOI\n");
            eoi = 1;
            break;
        }
    }
    #endif
    
    return eoi;
}

vic_byte get_byte(int check_atn) {
    #if DEBUG
    printf("gb ");
    #endif
    
    vic_byte byte = 0;

    // bit 1
    if (wait_clock(0, 0, check_atn)) return 0; // 70 timeout
    byte = (INB(addr + 1) & 0x40) >> 6;
    if (wait_clock(1, 0, check_atn)) return 0; // 20 timeout

    // bit 2
    if (wait_clock(0, 0, check_atn)) return 0; // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 5;
    if (wait_clock(1, 0, check_atn)) return 0; // 20 timeout

    // bit 3
    if (wait_clock(0, 0, check_atn)) return 0; // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 4;
    if (wait_clock(1, 0, check_atn)) return 0; // 20 timeout

    // bit 4
    if (wait_clock(0, 0, check_atn)) return 0; // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 3;
    if (wait_clock(1, 0, check_atn)) return 0; // 20 timeout

    // bit 5
    if (wait_clock(0, 0, check_atn)) return 0; // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 2;
    if (wait_clock(1, 0, check_atn)) return 0; // 20 timeout
    
    // bit 6
    if (wait_clock(0, 0, check_atn)) return 0; // 70 timeout
    byte |= (INB(addr + 1) & 0x40) >> 1;
    if (wait_clock(1, 0, check_atn)) return 0; // 20 timeout

    // bit 7
    if (wait_clock(0, 0, check_atn)) return 0; // 70 timeout
    byte |= (INB(addr + 1) & 0x40);
    if (wait_clock(1, 0, check_atn)) return 0; // 20 timeout

    // bit 8
    if (wait_clock(0, 0, check_atn)) return 0; // 70 timeout
    byte |= (INB(addr + 1) & 0x40) << 1;
    if (wait_clock(1, 0, check_atn)) return 0; // 20 timeout

    set(DATA_HIGH);

    return byte;
}