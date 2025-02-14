#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <locale.h>
#include "constants.h"
#include "timing.h"
#include "math.h"

void init_gui();
void destroy_gui();
void print_header(int i, int direction);
void print_log(char *format, int localtime, int color, int refresh, ...);
void set_progress_bar(int percent);

#endif