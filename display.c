#include "display.h"

extern int device_resetted;

void create_progress_bar() {
    printf("                                                                                                     0%%\n");
}

void set_progress_bar(int percent) {
    if (device_resetted) return;

    char progress_bar[200] = GREEN_SQUARE;
    for (int i=0; i<percent; i++) {
        strcat(progress_bar, " ");
    }
    strcat(progress_bar, COLOR_RESET);
    for (int i=percent; i<100; i++) {
        strcat(progress_bar, " ");
    }

    char _percent[6];
    sprintf(_percent, " %d%%", percent);
    strcat(progress_bar, _percent);
    
    CURSOR_UP(1);
    printf("%s\n", progress_bar);
}