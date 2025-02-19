#include "vic_io.h"

extern int device_resetted;
extern int port;

#ifdef _WIN32
extern LARGE_INTEGER lpFrequency;
#endif

extern WINDOW *header_window;
extern WINDOW *log_window;

int resetted() {
    return (INB(port + 1) & 0x80) == 0x80;
}

int atn(int value) {
    if (resetted()) {
        device_resetted = 1;
        return 0;
    }
    return (INB(port + 1) & 0x10) == !value;
}

void wait_atn(int value) {
    #if DEBUG
    printf("watn ");
    #endif
    
    if (value) 
        value = 0x10;
    while ((INB(port + 1) & 0x10) == value) {
        if (resetted()) {
            device_resetted = 1;
            break;
        }
    }
}

int wait_clock(int value, int timeout, int check_atn) {
    #if DEBUG
    printf("wc%d(%d) ", value, timeout);
    suseconds_t b = get_microsec();
    #endif

    if (value) 
        value = 0x20;
    suseconds_t a = get_microsec();
    vic_byte in = INB(port + 1);

    while ((in & 0x20) == value) {
        if (check_atn && ((in & 0x10) == 0x10)) {   // if check for atn low and atn is low
            #if DEBUG
            printf("EXIT ATN ");
            #endif
            return 1;
        }
        if (!check_atn && ((in & 0x10) == 0)) {
            #if DEBUG
            printf("ENTER ATN ");
            #endif
            return 1;
        }
        else if ((timeout > 0) && ((get_microsec() - a) > timeout)) {
            #if DEBUG
            char _localtime[LOCALTIME_STRLEN];
            get_localtime(_localtime);
            printf("[%s] TIMEOUT(%ld) %s ", _localtime, (get_microsec() - a), __func__);
            #endif
            return 1;
        }
        else if (resetted()) {
            device_resetted = 1;
            return 1;
        }
        in = INB(port + 1);
    }

    #if DEBUG
    printf("%ld ", get_microsec() - b);
    #endif

    return 0;
}

int wait_data(int value, int timeout) {
    #if DEBUG
    printf("wd%d(%d) ", value, timeout);
    suseconds_t b = get_microsec();
    #endif

    if (value) 
        value = 0x40;

    #ifdef __linux__
    suseconds_t a = get_microsec();
    while ((INB(port + 1) & 0x40) == value) {
        if ((timeout > 0) && ((get_microsec() - a) > timeout)) {
            #if DEBUG
            char _localtime[LOCALTIME_STRLEN];
            get_localtime(_localtime);
            printf("[%s] TIMEOUT(%ld) %s ", _localtime, (get_microsec() - a), __func__);
            #endif
            return 1;
        }       
    }
    #elif _WIN32
    LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
    QueryPerformanceCounter(&StartingTime);
    while ((INB(port + 1) & 0x40) == value) {
        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= lpFrequency.QuadPart;

        if ((timeout > 0) && (ElapsedMicroseconds.QuadPart > timeout))
            return 1;
    }
    #endif

    #if DEBUG
    printf("%ld ", get_microsec() - b);
    #endif

    return 0;
}

void set(vic_byte value) {
    vic_byte in_value = INB(port + 2);
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
    OUTB(out_value, port + 2);
}

int eoi() {
    #if DEBUG
    printf("eoi? ");
    #endif

    #ifdef __linux__
    suseconds_t a = get_microsec();
    suseconds_t elapsed;
    int eoi = 0;
    while (INB(port + 1) & 0x20) {
        elapsed = get_microsec() - a;
        if (elapsed > 255) {
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
    while (INB(port + 1) & 0x20) {
        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= lpFrequency.QuadPart;

        if (ElapsedMicroseconds.QuadPart > 200) {
            //print_log("EOI\n", 0, 0, 1);
            eoi = 1;
            break;
        }
    }
    #endif
    
    return eoi;
}

vic_byte get_byte(int check_atn) {
    #if DEBUG
    print_log("gb ", 0, 0, 1);
    #endif
    
    vic_byte byte = 0;

    // bit 1
    if (wait_clock(0, 200, check_atn)) goto end;   // ~70 ms
    byte = (INB(port + 1) & 0x40) >> 6;
    if (wait_clock(1, 200, check_atn)) goto end;   // ~20 ms

    // bit 2
    if (wait_clock(0, 200, check_atn)) goto end;   // ~70 ms
    byte |= (INB(port + 1) & 0x40) >> 5;
    if (wait_clock(1, 200, check_atn)) goto end;   // ~20 ms

    // bit 3
    if (wait_clock(0, 200, check_atn)) goto end;   // ~70 ms
    byte |= (INB(port + 1) & 0x40) >> 4;
    if (wait_clock(1, 200, check_atn)) goto end;   // ~20 ms

    // bit 4
    if (wait_clock(0, 200, check_atn)) goto end;   // ~70 ms
    byte |= (INB(port + 1) & 0x40) >> 3;
    if (wait_clock(1, 200, check_atn)) goto end;   // ~20 ms

    // bit 5
    if (wait_clock(0, 200, check_atn)) goto end;   // ~70 ms
    byte |= (INB(port + 1) & 0x40) >> 2;
    if (wait_clock(1, 200, check_atn)) goto end;   // ~20 ms
    
    // bit 6
    if (wait_clock(0, 200, check_atn)) goto end;   // ~70 ms
    byte |= (INB(port + 1) & 0x40) >> 1;
    if (wait_clock(1, 200, check_atn)) goto end;   // ~20 ms

    // bit 7
    if (wait_clock(0, 200, check_atn)) goto end;   // ~70 ms
    byte |= (INB(port + 1) & 0x40);
    if (wait_clock(1, 200, check_atn)) goto end;   // ~20 ms

    // bit 8
    if (wait_clock(0, 200, check_atn)) goto end;   // ~70 ms
    byte |= (INB(port + 1) & 0x40) << 1;
    if (wait_clock(1, 200, check_atn)) goto end;   // ~20 ms

    set(DATA_HIGH);

    end:
    return byte;
}