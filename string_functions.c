#include "string_functions.h"

int trim(char *string) {
    int orig_string_len = strlen(string);

    int i_start = 0;
    char c = string[i_start];
    while (c == ' ') {
        i_start++;
        c = string[i_start];
    }

    int new_string_len = orig_string_len - i_start;
    if (i_start > 0) {
        int j = 0;
        for(int i = i_start; i <= orig_string_len; i++) string[j++] = string[i];
    }

    int i_end = new_string_len - 1;
    c = string[i_end];
    while (c == ' ') c = string[--i_end];

    if (i_end != (new_string_len - 1)) {
        new_string_len = i_end + 1;
        string[new_string_len] = '\0';
    }

    return orig_string_len - new_string_len;
}

void substr(char *dest, char *src, int start, int length) {
    if (start >= 0) 
        memcpy(dest, src + start, start + length);
    else
        memcpy(dest, src + strlen(src) + start, length);
}

void strtolower(char *string, int length) {
    if (length == 0) length = strlen(string);
    else if (length < 0) length = strlen(string) + length;

    for (int i = 0; i < length; i++) {
        string[i] = tolower(string[i]);
    }
}

void strtoupper(char *string, int length) {
    if (length == 0) length = strlen(string);
    else if (length < 0) length = strlen(string) + length;

    for (int i = 0; i < length; i++) {
        string[i] = toupper(string[i]);
    }
}

int vic_string_equal_string(vic_string *string1, char *string2) {
    int n1 = string1->length;
    int n2 = strlen(string2);
    if (n1 != n2) return 0;
    for (int i = 0; i < n1; i++) {
        if (string1->string[i] != string2[i])
            return 0;
    }
    return 1;
}

int vic_string_equal_vic_string(vic_string *string1, vic_string *string2) {
    if (string1->length != string2->length) return 0;
    for (int i = 0; i < string1->length; i++) {
        if (string1->string[i] != string2->string[i])
            return 0;
    }
    return 1;
}