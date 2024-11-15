#ifndef CC1541_H
#define CC1541_H

#include "../constants.h"
#include "../string_functions.h"
#include "../disk.h"

const unsigned char* basename(const unsigned char* path);
unsigned char a2p(unsigned char a);
unsigned char p2a(unsigned char p);
int cc1541(int mode);

#endif