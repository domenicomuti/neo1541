#include "timing.h"

#ifdef __linux

int microsleep_offset = 0;

void get_localtime(char *ret) {
    struct timeval _microsec;
    gettimeofday(&_microsec, NULL);
    struct tm *_localtime = localtime(&_microsec.tv_sec);
    char _ret[9];
    strftime(_ret, 1000, "%T", _localtime);
    sprintf(ret, "%s.%04d", _ret, (int)(_microsec.tv_usec / 1000));
}

suseconds_t get_microsec() {
    struct timeval ret;
    gettimeofday(&ret, NULL);
    return (suseconds_t)(ret.tv_sec * 1000000) + ret.tv_usec;
}

#elif _WIN32
LARGE_INTEGER lpFrequency;
#endif

void microsleep(int duration) {
#ifdef __linux__
    suseconds_t a = get_microsec();
    while ((get_microsec() - a) < duration) {}
#endif
}