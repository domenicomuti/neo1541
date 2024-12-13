#include "display.h"

#if !DEBUG
extern int device_resetted;
char _localtime[LOCALTIME_STRLEN];
static int _last_percent = -1;
#endif

WINDOW *header_window;
WINDOW *log_window;

void init_screen() {
    setlocale(LC_ALL, "");
    initscr();
    start_color();

    header_window = newwin(7, 0, 0, 0);
    log_window = newwin(0, 0, 8, 0);

    if (COLORS == 8) {
        init_pair(1, COLOR_RED, 0);
        init_pair(2, COLOR_RED, 0);
        init_pair(3, COLOR_YELLOW, 0);
        init_pair(4, COLOR_YELLOW, 0);
        init_pair(5, COLOR_GREEN, 0);
        init_pair(6, COLOR_GREEN, 0);
        init_pair(7, COLOR_CYAN, 0);
    }
    else if (COLORS == 256) {
        init_pair(1, 160, 0);
        init_pair(2, 166, 0);
        init_pair(3, 172, 0);
        init_pair(4, 178, 0);
        init_pair(5, 34, 0);
        init_pair(6, 28, 0);
        init_pair(7, 27, 0);
    }
}

void print_header(int i, int direction) {
    wattron(header_window, COLOR_PAIR(direction ? (((0 + i) % 7) + 1) : ((( 7 - i) % 7) + 1)));   mvwprintw(header_window, 0, 0, "░▒█████████▒░  ░▒██████████▒░  ░▒████████▒░     ░▒███▒░ ░▒██████████▒░ ░▒███▒  ▒███▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((1 + i) % 7) + 1) : ((( 8 - i) % 7) + 1)));     wprintw(header_window,       "░▒███▒  ▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░ ░▒██████▒░ ░▒███▒░        ░▒███▒  ▒███▒░ ░▒██████▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((2 + i) % 7) + 1) : ((( 9 - i) % 7) + 1)));     wprintw(header_window,       "░▒███▒  ▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░    ░▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((3 + i) % 7) + 1) : (((10 - i) % 7) + 1)));     wprintw(header_window,       "░▒███▒  ▒███▒░ ░▒████████▒░   ░▒███▒  ▒███▒░    ░▒███▒░ ░▒█████████▒░  ░▒██████████▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((4 + i) % 7) + 1) : (((11 - i) % 7) + 1)));     wprintw(header_window,       "░▒███▒  ▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░    ░▒███▒░        ░▒███▒░        ░▒███▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((5 + i) % 7) + 1) : (((12 - i) % 7) + 1)));     wprintw(header_window,       "░▒███▒  ▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░    ░▒███▒░        ░▒███▒░        ░▒███▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((6 + i) % 7) + 1) : (((13 - i) % 7) + 1)));     wprintw(header_window,       "░▒███▒  ▒███▒░ ░▒██████████▒░  ░▒████████▒░     ░▒███▒░ ░▒█████████▒░         ░▒███▒░    ░▒███▒░\n");
}

void create_progress_bar() {
#if !DEBUG
    _localtime[0] = '\0';
    _last_percent = -1;
    wprintw(log_window, "\n");
#endif
}

void set_progress_bar(int percent) {
    /*#if !DEBUG
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
    wprintw(log_window, "[%s] %s\n", _localtime, progress_bar);

    free(progress_bar);
    #endif*/
}