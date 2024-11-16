#include "display.h"

#if !DEBUG
extern int device_resetted;
char _localtime[LOCALTIME_STRLEN];
static int _last_percent = -1;
#endif

void create_progress_bar() {
#if !DEBUG
    _localtime[0] = '\0';
    _last_percent = -1;
    printf("\n");
#endif
}

void set_progress_bar(int percent) {
#if !DEBUG
    if (device_resetted || _last_percent == percent)
        return;
    _last_percent = percent;

    int square_size = strlen(GREEN_SQUARE);
    int color_reset_size = strlen(COLOR_RESET);
    int progress_bar_size = ((2 * square_size) + color_reset_size + 106) * sizeof(char);

    char *progress_bar = malloc(progress_bar_size);
    memset(progress_bar, ' ', progress_bar_size);

    memcpy(progress_bar, GREEN_SQUARE, square_size);
    memcpy(progress_bar + square_size + percent, WHITE_SQUARE, square_size);
    memcpy(progress_bar + (2 * square_size) + 100, COLOR_RESET, color_reset_size);

    char _percent[6] = {0};
    sprintf(_percent, " %d%%", percent);
    memcpy(progress_bar + (2 * square_size) + 100 + color_reset_size, _percent, 6);

    CURSOR_UP(1);
    if (_localtime[0] == '\0')
        get_localtime(_localtime);
    printf("[%s] %s\n", _localtime, progress_bar);

    free(progress_bar);
#endif
}