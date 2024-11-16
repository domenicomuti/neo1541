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

int wait_clock(int value, int timeout) {
#if DEBUG
    printf("wc%d(%d) ", value, timeout);
#endif
    if (value) 
        value = 0x20;
    suseconds_t a = get_microsec();
    while ((INB(addr + 1) & 0x20) == value) {
        if ((timeout > 0) && ((get_microsec() - a) > timeout)) {
        //#if DEBUG
            //printf("%stimeout %s%s", COLOR_RED, __func__, COLOR_RESET);
        //#endif
            return 0;
        }
        if (resetted()) {
            device_resetted = 1;
            break;
        }
    }
    return 1;
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
        if ((timeout > 0) && ((get_microsec() - a) > (timeout))) {
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

void set_clock(int value) {
    #if DEBUG
    printf("sc%d ", value);
    #endif

    if (value)
        OUTB((INB(addr + 2) | 2), addr + 2);    // set Clock to 1 (Commodore True)
    else
        OUTB((INB(addr + 2) & ~2), addr + 2);   // set Clock to 0 (Commodore False)
        
}

void set_data(int value) {
    #if DEBUG
    printf("sd%d ", value);
    #endif

    if (value)
        OUTB((INB(addr + 2) & ~4), addr + 2);   // set Data to 0 (Commodore True)
    else
        OUTB((INB(addr + 2) | 4), addr + 2);    // set Data to 1 (Commodore False)
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