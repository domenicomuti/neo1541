#ifndef STRING_FUNCTIONS_H
#define STRING_FUNCTIONS_H

#include <string.h>
#include <ctype.h>

typedef unsigned char vic_byte;

#ifdef _WIN64
typedef long long int vic_size;
#else
typedef long int vic_size;
#endif

typedef struct vic_string {
    vic_byte *string;
    vic_size length;
} vic_string;

int trim(char *string);
void substr(char *dest, char *src, int start, int length);
void strtolower(char *string, int length);
void strtoupper(char *string, int length);
int vic_string_equal_string(vic_string *string1, char *string2);
int vic_string_equal_vic_string(vic_string *string1, vic_string *string2);

#endif