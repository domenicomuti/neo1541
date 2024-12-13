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

void init_screen();
void print_header(int i, int direction);
void create_progress_bar();
void set_progress_bar(int percent);

#endif