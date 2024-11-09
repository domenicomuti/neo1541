#ifndef STRING_FUNCTIONS_H
#define STRING_FUNCTIONS_H

#include <string.h>
#include <ctype.h>

int trim(char *string);
void substr(char *dest, char *src, int start, int length);
void strtolower(char *string, int length);
void strtoupper(char *string, int length);

#endif