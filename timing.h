#ifndef TIMING_H
#define TIMING_H

#ifdef __linux__
    #include <sys/time.h>
    #include <time.h>
    suseconds_t get_microsec();
    void probe_microsleep_offset();
#elif _WIN64
    #include <windows.h>
#endif

void microsleep(int duration);

#endif