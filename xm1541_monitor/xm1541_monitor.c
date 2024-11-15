#include <unistd.h>
#include <sys/io.h>
#include <ncurses.h>

void print_register(int y, int x, int addr) {
    mvprintw(y, x,
        "%d         %d         %d         %d         %d         %d         %d         %d\n",
        (inb(addr) & 0x80) >> 7,
        (inb(addr) & 0x40) >> 6,
        (inb(addr) & 0x20) >> 5,
        (inb(addr) & 0x10) >> 4,
        (inb(addr) & 0x8) >> 3,
        (inb(addr) & 0x4) >> 2,
        (inb(addr) & 0x2) >> 1,
        (inb(addr) & 0x1)
    );
}

void print_psr(int addr) {
    mvprintw(0, 0, "PSR");
    mvprintw(1, 0, "!BUSY     ACK       PE        SELT      ERR       !INTFLAG  1         1\n");
    mvprintw(2, 0, "RESET     DATA      CLK       ATN\n");
    print_register(3, 0, addr + 1);
}

void print_pcr(int addr) {
    mvprintw(5, 0, "PCR");
    mvprintw(6, 0, "1         1         DIRIN     INTEN     !SIN      INIT      !AFD      !STB\n");
    mvprintw(7, 0, "                                        RESET     DATA      CLOCK\n");
    print_register(8, 0, addr + 2);
}

int main() {
    int addr = 0xd100;

    iopl(3);
    ioperm(addr, 2, 1);

    outb(0xC0, addr + 2); // Reset PCR

    initscr();
    cbreak();
    noecho();
    timeout(100);

    //outb(0, addr+3); // Enable SPP Mode on PXR register
    //outb(4, addr+3); // Enable EPP Mode on PXR register
    //outb(1, addr+3); // Enable ECP Mode on PXR register

    //outb(0xD0, addr + 2); // Enable Interrupt on PCR register

    int in;

    do {
        print_psr(addr);
        print_pcr(addr);
        
        in = getch();
        mvprintw(10, 0, "%d ", in);
        if (in == 100) {
            if ((inb(addr + 1) & 0x40) == 0) {
                outb((inb(addr + 2) | 4), addr + 2);
            }
            else {
                mvprintw(9, 0, "    ");
                outb((inb(addr + 2) & ~4), addr + 2);
            }
        }
        else if (in == 99) {
            if ((inb(addr + 1) & 0x20) == 0x20) {
                outb((inb(addr + 2) | 2), addr + 2);
            }
            else {
                mvprintw(10, 0, "     ");
                outb((inb(addr + 2) & ~2), addr + 2);
            }
        }
        else if (in == 114) {
            if ((inb(addr + 1) & 0x80) == 0) {
                outb((inb(addr + 2) | 8), addr + 2);
            }
            else {
                mvprintw(11, 0, "     ");
                outb((inb(addr + 2) & ~8), addr + 2);
            }
        }
        
        refresh();  
    }
    while(1);
    
    endwin();

    return 0;
}