#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"
#include "timing.h"

#define COLOR_RESET   "\x1B[0m"
#define COLOR_RED     "\x1B[31m"
#define COLOR_GREEN   "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"
#define COLOR_BLUE    "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN    "\x1B[36m"
#define COLOR_WHITE   "\x1B[37m"

#define WHITE_SQUARE  "\033[37;47m"
#define GREEN_SQUARE  "\033[32;42m"

#define CURSOR_UP(n)  printf("\033[%dA", n)

void create_progress_bar();
void set_progress_bar(int percent);

#endif