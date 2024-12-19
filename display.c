#include "display.h"

extern int device_resetted;
static int _last_percent = -1;

WINDOW *header_window;
WINDOW *progress_bar_window;
WINDOW *log_window;

void init_gui() {
    #if !DEBUG
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    start_color();

    header_window       = newwin(8, 0, 0 , 0);
    progress_bar_window = newwin(1, 0, 9 , 0);
    log_window          = newwin(0, 0, 10, 0);

    idlok(log_window, true);
    scrollok(log_window, true);

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
    init_pair(8, COLOR_WHITE, COLOR_BLACK);
    init_pair(9, COLOR_WHITE, COLOR_GREEN);

    curs_set(0);
    #endif
}

void destroy_gui() {
    #if !DEBUG
    delwin(header_window);
    delwin(progress_bar_window);
    delwin(log_window);
    endwin();
    #endif
}

void print_header(int i, int direction) {
    wattron(header_window, COLOR_PAIR(direction ? (((0 + i) % 7) + 1) : ((( 7 - i) % 7) + 1)));   mvwprintw(header_window, 1, 0, "  ░▒█████████▒░  ░▒██████████▒░  ░▒████████▒░     ░▒███▒░ ░▒██████████▒░ ░▒███▒  ▒███▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((1 + i) % 7) + 1) : ((( 8 - i) % 7) + 1)));     wprintw(header_window,       "  ░▒███▒  ▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░ ░▒██████▒░ ░▒███▒░        ░▒███▒  ▒███▒░ ░▒██████▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((2 + i) % 7) + 1) : ((( 9 - i) % 7) + 1)));     wprintw(header_window,       "  ░▒███▒  ▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░    ░▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((3 + i) % 7) + 1) : (((10 - i) % 7) + 1)));     wprintw(header_window,       "  ░▒███▒  ▒███▒░ ░▒████████▒░   ░▒███▒  ▒███▒░    ░▒███▒░ ░▒█████████▒░  ░▒██████████▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((4 + i) % 7) + 1) : (((11 - i) % 7) + 1)));     wprintw(header_window,       "  ░▒███▒  ▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░    ░▒███▒░        ░▒███▒░        ░▒███▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((5 + i) % 7) + 1) : (((12 - i) % 7) + 1)));     wprintw(header_window,       "  ░▒███▒  ▒███▒░ ░▒███▒░        ░▒███▒  ▒███▒░    ░▒███▒░        ░▒███▒░        ░▒███▒░    ░▒███▒░\n");
    wattron(header_window, COLOR_PAIR(direction ? (((6 + i) % 7) + 1) : (((13 - i) % 7) + 1)));     wprintw(header_window,       "  ░▒███▒  ▒███▒░ ░▒██████████▒░  ░▒████████▒░     ░▒███▒░ ░▒█████████▒░         ░▒███▒░    ░▒███▒░\n");
}

void print_log(char *format, int localtime, int color, int refresh, ...) {
    if (localtime) {
        char _localtime[LOCALTIME_STRLEN];
        get_localtime(_localtime);

        #if !DEBUG
        wprintw(log_window, "[%s] ", _localtime);
        #else
        printf("[%s] ", _localtime);
        #endif
    }    

    va_list va;
    va_start(va, refresh);

    #if !DEBUG
    if (color != 0)
        wattron(log_window, COLOR_PAIR(color));
    #endif
        
    #if !DEBUG
    vw_printw(log_window, format, va);
    #else
    vprintf(format, va);
    #endif

    #if !DEBUG
    if (color != 0)
        wattroff(log_window, COLOR_PAIR(color));

    if (refresh)
        wrefresh(log_window);
    #endif
}

void set_progress_bar(int percent) {
    if (device_resetted || _last_percent == percent)
        return;
    _last_percent = percent;

    wattron(progress_bar_window, COLOR_PAIR(8));
    
    if (percent == 0 || percent >= 100) {
        mvwprintw(progress_bar_window, 0, 0, "                                                                                                    ");
        wmove(progress_bar_window, 0, 0);
        wrefresh(progress_bar_window);
        return;
    }
    
    wattron(progress_bar_window, COLOR_PAIR(9));
    waddch(progress_bar_window, ' ');
    wrefresh(progress_bar_window);
}