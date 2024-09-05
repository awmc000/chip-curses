#include "chip8.hpp"
#include "ncurses.h"
#include <locale.h>
#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <cctype>

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

#define NS_IN_SECOND                1000000000
#define CURSE_CHIP_FRAMERATE        20

struct chip_frontend {
    Chip8 * sys;
    WINDOW * display_win;
    WINDOW * sidebar_win;
    WINDOW * helpbar_win;
    bool paused;
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

    mvwprintw(fe.helpbar_win, 1, 1, "QUIT: [Esc] PLAY/PAUSE: [.]");
    wrefresh(fe.helpbar_win);
}

void write_debug_info(chip_frontend &fe) {
    wattron(fe.sidebar_win, COLOR_PAIR(2));
    mvwprintw(fe.sidebar_win, 1, 1, "PC: %04x", fe.sys->programCounter);
    
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


void run_cycle(chip_frontend &fe, timespec &last_frame, timespec &now)
{
    // Cycle
    fe.sys->cycle();

    // Write info to debug area
    write_debug_info(fe);

    // If sound flag set, make a sound and unset
    if (fe.sys->sound)
    {
        printf("\07");
        fe.sys->sound = false;
    }

    // If draw flag set, draw and unset
    if (fe.sys->draw)
    {
        draw_display(&fe);
        fe.sys->draw = false;
        refresh();
        wrefresh(fe.display_win);
        if (last_frame.tv_nsec != -1) {
            last_frame.tv_nsec = now.tv_nsec;
            last_frame.tv_sec = now.tv_sec;
        }
    }
}

char handle_input(chip_frontend &fe) {
    char ch = getch();
    if (isalnum(ch)) {
        mvwprintw(fe.helpbar_win, 1, 40, "GOT KEY: %c", ch);
        wrefresh(fe.helpbar_win);
        refresh();
    }

    if (ch == '.') {
        fe.paused ^= 1;
    }

    if (fe.paused && ch == ',') {
        fe.sys->cycle();
        timespec stupid_hack {-1, -1};
        run_cycle(fe, stupid_hack, stupid_hack);
    }

    return ch;
}

byte * load_file_buf(const std::string &filename) {
	byte * fileBuf = new byte[CHIP8_ROM_BYTES];
	std::ifstream in(filename, std::ios_base::in | std::ios_base::binary);

	do {
		in.read((char *) fileBuf, CHIP8_ROM_BYTES);
	} while (in.gcount() > 0);

	return fileBuf;
}

timespec timespec_sub(timespec start, timespec end) {
    timespec temp;
    
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec  = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec  = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    
    return temp;
};


int main(void)
{

    // Set locale for unicode
    setlocale(LC_ALL, "");

    // Set up backend
    struct chip_frontend fe;
    fe.paused = true;
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
    fe.sys->load(load_file_buf("particle.ch8"));
    
    // Main loop
    bool waiting = false;
    timespec last_frame;
    clock_gettime(CLOCK_REALTIME, &last_frame);
    timespec now;
    
    while ((handle_input(fe) != 27)) {
        
        if (fe.paused) {
        
        } else {
            // Wait if it's not time for the next frame!
            clock_gettime(CLOCK_REALTIME, &now);

            if (timespec_sub(last_frame, now).tv_nsec > (1000000000 / CURSE_CHIP_FRAMERATE)) {
                run_cycle(fe, last_frame, now);
            }
        }

        
    }

    endwin();

    return 0;
}
