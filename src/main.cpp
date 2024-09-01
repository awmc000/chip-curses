#include "chip8.hpp"
#include "ncurses.h"

struct chip_frontend {
    int display[CHIP8_SCREEN_HEIGHT][CHIP8_SCREEN_WIDTH];
    Chip8 sys;
};

int main (void) {

    // Initialize ncurses
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    refresh();

    // Set up 64x32 window
    WINDOW * chip_display;
    int cx, cy, cw, ch;

    cx = 0;
    cy = 0;
    cw = CHIP8_SCREEN_WIDTH;
    ch = CHIP8_SCREEN_HEIGHT;

    // Create display window
    chip_display = newwin(ch, cw, cx, cy);
    box(chip_display, 0, 0);

    wrefresh(chip_display);

    // Set up sidebar window
    WINDOW * chip_sidebar;
    int sx, sy, sw, sh;
    sx = CHIP8_SCREEN_WIDTH + 1;
    sy = 0;
    sw = 5;
    sh = 5;

    // Create sidebar window
    chip_sidebar = newwin(sh, sw, sx, sy);
    box(chip_sidebar, 0, 0);

    wrefresh(chip_sidebar);
    refresh();

    while (getch() != KEY_UP) {
        ;;
    }

    endwin();

    return 0;
}