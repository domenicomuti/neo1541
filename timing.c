#include "timing.h"

#ifdef __linux

int microsleep_offset = 0;

suseconds_t get_microsec() {
    struct timeval ret;
    gettimeofday(&ret, NULL);
    return (suseconds_t)(ret.tv_sec * 1000000) + ret.tv_usec;
}

void probe_microsleep_offset() {
    int _microsleep_offset = 0;
    for (int i = 0; i < 100; i++) {
        suseconds_t a = get_microsec();
        microsleep(1);
        _microsleep_offset += get_microsec() - a;
    }
    microsleep_offset = _microsleep_offset * 10;
}

#elif _WIN32
    LARGE_INTEGER lpFrequency;
#endif

void microsleep(int duration) {
    #ifdef __linux__
    struct timespec _ts;
    clock_gettime(CLOCK_BOOTTIME, &_ts);

    struct timespec ts;
    ts.tv_sec = _ts.tv_sec;
    ts.tv_nsec = _ts.tv_nsec + (1000 * duration) - microsleep_offset;

    clock_nanosleep(CLOCK_BOOTTIME, TIMER_ABSTIME, &ts, NULL);
    #endif
}

