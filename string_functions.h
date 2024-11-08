#ifndef STRING_FUNCTIONS_H
#define STRING_FUNCTIONS_H

#include <string.h>
#include <ctype.h>

void trim(char *dest, char *src);
void substr(char *dest, char *src, int start, int length);
void strtolower(char *string);
void strtoupper(char *string);

#endif