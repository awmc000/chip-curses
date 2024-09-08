#include "chip8.hpp"
#include "ncurses.h"
#include <locale.h>
#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <cctype>
#include <iostream>

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
    int key_time_left[16];
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

    mvwprintw(fe.helpbar_win, 1, 1, "QUIT: [Esc] PLAY/PAUSE: [.] STEP: [,]");
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
    mvwprintw(fe.sidebar_win, 7, 1, "KEY: %01d%01d%01d%01d%01d%01d%01d%01d",
        fe.sys->keyState[0], fe.sys->keyState[1], fe.sys->keyState[2], fe.sys->keyState[3],
        fe.sys->keyState[4], fe.sys->keyState[5], fe.sys->keyState[6], fe.sys->keyState[7]
    );
    mvwprintw(fe.sidebar_win, 8, 1, "     %01d%01d%01d%01d%01d%01d%01d%01d",
        fe.sys->keyState[8], fe.sys->keyState[9], fe.sys->keyState[10], fe.sys->keyState[11],
        fe.sys->keyState[12], fe.sys->keyState[13], fe.sys->keyState[14], fe.sys->keyState[15]
    );
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

int map_to_keypad(char inputc) {
    switch (inputc) {
        // Row 1: 1234 == 123C
        case '1':
        case '2':
        case '3':
            return inputc - '0';
            break;
        case '4':       return 0xC;     break;
        
        // Row 2: QWER == 456D
        case 'Q':       return 0x4;     break;
        case 'W':       return 0x5;     break;
        case 'E':       return 0x6;     break;
        case 'R':       return 0xD;     break;
        
        // Row 3: ASDF == 789E
        case 'A':       return 0x7;     break;
        case 'S':       return 0x8;     break;
        case 'D':       return 0x9;     break;
        case 'F':       return 0xE;     break;

        // Row F: ZXCV == A0BF
        case 'Z':       return 0xA;     break;
        case 'X':       return 0x0;     break;
        case 'C':       return 0xB;     break;
        case 'V':       return 0xF;     break;

    }
    return -1;
}

char handle_input(chip_frontend &fe) {
    char ch = getch();
    
    for (int i = 0; i < 16; i++) {

        // Unset backend key state when the timer has gone off        
        if (fe.key_time_left[i] == 0) {
            fe.sys->keyState[i] = 0;
        } else {
            fe.key_time_left[i]--;
        }
    }

    char mapped_key = map_to_keypad(ch);
    if (mapped_key != -1) {
        if (fe.sys->blockingForKey) {
            fe.sys->lastKeyFromBlock = true;
        }
        fe.key_time_left[mapped_key] = 1000000;
        fe.sys->keyState[mapped_key] = 1;
        fe.sys->lastKey = mapped_key;
        mvwprintw(fe.helpbar_win, 1, 40, "GOT KEY: %c AS %01x", ch, mapped_key);
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


int main(int argc, char ** argv)
{

    if (argc < 2) {
        std::cout << "Usage: ./chipcurses filename.rom" << std::endl;
        exit(1);
    }

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
    std::string filename = argv[1];
    fe.sys->load(load_file_buf(filename));
    
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
