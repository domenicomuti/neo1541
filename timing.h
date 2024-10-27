#ifndef TIMING_H
#define TIMING_H

#include <sys/time.h>
#include <time.h>

suseconds_t get_microsec();
void microsleep(int duration);
void probe_microsleep_offset();

#endif