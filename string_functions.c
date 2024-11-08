#include "string_functions.h"

void trim(char *dest, char *src) {
    int i_start = 0;
    char c = src[i_start];
    while (c == ' ') {
        i_start++;
        c = src[i_start];
    }

    int i_end = strlen(src) - 1;
    c = src[i_end];
    while (c == ' ') {
        i_end--;
        c = src[i_end];
    }

    memcpy(dest, src + i_start, i_end - i_start + 1);
}

void substr(char *dest, char *src, int start, int length) {
    if (start >= 0) 
        memcpy(dest, src + start, start + length);
    else
        memcpy(dest, src + strlen(src) + start, length);
}

void strtolower(char *string) {
    for (int i = 0; i < strlen(string); i++) {
        string[i] = tolower(string[i]);
    }
}

void strtoupper(char *string) {
    for (int i = 0; i < strlen(string); i++) {
        string[i] = toupper(string[i]);
    }
}