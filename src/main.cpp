#include "chip8.hpp"
#include "ncurses.h"
#include <locale.h>
#include <fstream>
#include <iostream>
#include <string>

#define FRONTEND_SCREEN_WIDTH       ((CHIP8_SCREEN_WIDTH))
#define FRONTEND_SCREEN_HEIGHT      ((CHIP8_SCREEN_HEIGHT / 2))
#define FRONTEND_SCREEN_X           0
#define FRONTEND_SCREEN_Y           0

#define FRONTEND_SIDEBAR_HEIGHT     (FRONTEND_SCREEN_HEIGHT)
#define FRONTEND_SIDEBAR_WIDTH      18
#define FRONTEND_SIDEBAR_X          (FRONTEND_SCREEN_WIDTH + 2)
#define FRONTEND_SIDEBAR_Y          0

#define FRONTEND_HELPBAR_HEIGHT     1
#define FRONTEND_HELPBAR_WIDTH      (FRONTEND_SCREEN_WIDTH + FRONTEND_SIDEBAR_WIDTH)
#define FRONTEND_HELPBAR_X          0
#define FRONTEND_HELPBAR_Y          (FRONTEND_SCREEN_HEIGHT + 2)

#define FRONTEND_PIX_TOP            "▀"
#define FRONTEND_PIX_BTM            "▄"
#define FRONTEND_PIX_BOTH           "█"

struct chip_frontend {
    Chip8 * sys;
    WINDOW * display_win;
    WINDOW * sidebar_win;
    WINDOW * helpbar_win;
};

WINDOW * create_window(int x, int y, int w, int h) {
    WINDOW * new_win = newwin(h, w, y, x);
    box(new_win, 0, 0);
    wrefresh(new_win);

    return new_win;
}

void draw_display(struct chip_frontend * fe) {
    WINDOW * display = fe->display_win;

    for (int y = 0; y < 32; y += 2) {
        // each y is 2 rows
        // so check y and y+1 and set that pix to ▀, ▄, or █
        for (int x = 0; x < 64; x++) {
            int top_pix = fe->sys->displayBuffer[y][x]; 
            int bot_pix = fe->sys->displayBuffer[y+1][x];

            if (top_pix && bot_pix) {
                mvwprintw(display, (y/2)+1, x+1, FRONTEND_PIX_BOTH);
            }
            else if (top_pix) {
                mvwprintw(display, (y/2)+1, x+1, FRONTEND_PIX_TOP);
            }
            else if (bot_pix) {
                mvwprintw(display, (y/2)+1, x+1, FRONTEND_PIX_BTM);
            }
            else {
                mvwprintw(display, (y/2)+1, x+1, " ");
            }
        }
    }

    wrefresh(display);
}


void setup_windows(struct chip_frontend * fe)
{
    fe->display_win = create_window(
        FRONTEND_SCREEN_X,
        FRONTEND_SCREEN_Y,
        FRONTEND_SCREEN_WIDTH + 2,
        FRONTEND_SCREEN_HEIGHT + 2);

    fe->sidebar_win = create_window(
        FRONTEND_SIDEBAR_X,
        FRONTEND_SIDEBAR_Y,
        FRONTEND_SIDEBAR_WIDTH + 2,
        FRONTEND_SIDEBAR_HEIGHT + 2);

    fe->helpbar_win = create_window(
        FRONTEND_HELPBAR_X,
        FRONTEND_HELPBAR_Y,
        FRONTEND_HELPBAR_WIDTH + 4,
        FRONTEND_HELPBAR_HEIGHT + 2);
}

void write_starting_info(chip_frontend &fe)
{
    mvwprintw(fe.display_win, 0, FRONTEND_SCREEN_WIDTH / 2 - 5 + 1, "CURSEDCHIP");
    wrefresh(fe.display_win);

    mvwprintw(fe.sidebar_win, 0, 3, "DEBUG & INFO");
    wrefresh(fe.sidebar_win);

    mvwprintw(fe.helpbar_win, 1, 1, "QUIT: [Esc] PAUSE: [Space]");
    wrefresh(fe.helpbar_win);
}

void write_debug_info(chip_frontend &fe) {
    wattron(fe.sidebar_win, COLOR_PAIR(2));
    mvwprintw(fe.sidebar_win, 1, 1, "PC: %x", fe.sys->programCounter);
    
    // Print 4 rows of variable register contents
    for (int i = 0; i <= 12; i += 4) {
        mvwprintw(
            fe.sidebar_win, 2 + (i / 4), 1, "V%x: %02x %02x %02x %02x", 
            i,
            fe.sys->variableRegisters[i+0], fe.sys->variableRegisters[i+1],
            fe.sys->variableRegisters[i+2], fe.sys->variableRegisters[i+3]
        );
    }
    mvwprintw(fe.sidebar_win, 6, 1, "IR: %04x SP: %02x", fe.sys->indexRegister, fe.sys->stackPointer);
    wattroff(fe.sidebar_win, COLOR_PAIR(2));
    refresh();
    wrefresh(fe.sidebar_win);
}

byte * load_file_buf(const std::string &filename) {
	byte * fileBuf = new byte[CHIP8_ROM_BYTES];
	std::ifstream in(filename, std::ios_base::in | std::ios_base::binary);

	do {
		in.read((char *) fileBuf, CHIP8_ROM_BYTES);
	} while (in.gcount() > 0);

	return fileBuf;
}

int main(void)
{

    // Set locale for unicode
    setlocale(LC_ALL, "");

    // Set up backend
    struct chip_frontend fe;
    fe.sys = new Chip8();
    fe.sys->reset();

    // Initialize ncurses
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    nodelay(stdscr, TRUE);
    
    // Set up terminal colours
    if (has_colors() == FALSE) {
        endwin();
		printf("Your terminal does not support color\n");
		exit(1);
	}

    // start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);

    refresh();

    setup_windows(&fe);

    // Write some title cards
    write_starting_info(fe);

    // Both created, update parent window
    refresh();

    // Load file
    fe.sys->load(load_file_buf("chip8logo.ch8"));

    // Main loop
    while (getch() != 27) {
        // Cycle
        fe.sys->cycle();

        // Check for input

        // Write info to debug area
        write_debug_info(fe);

        // If sound flag set, make a sound and unset
        if (fe.sys->sound) {
            printf("\07");
            fe.sys->sound = false;
        }

        // If draw flag set, draw and unset
        if (fe.sys->draw) {
            draw_display(&fe);
            fe.sys->draw = false;
            refresh();
            wrefresh(fe.display_win);
        }
        
    }

    endwin();

    return 0;
}

