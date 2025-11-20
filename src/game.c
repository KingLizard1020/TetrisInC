#if defined(_WIN32)
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include <stdbool.h>
#include <string.h>

#include "game.h"

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define CELL_EMPTY 0
#define CELL_STATIC 1

static bool g_use_color = false;
static int g_board[BOARD_HEIGHT][BOARD_WIDTH];

static void board_reset(void);
static void draw_frame(void);
static bool has_enough_space(void);
static void draw_banner(void);
static void draw_board(int origin_y, int origin_x);

int game_init(void) {
    if (initscr() == NULL) {
        return -1;
    }

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_CYAN, -1);
        g_use_color = true;
    }

    board_reset();

    return 0;
}

void game_loop(void) {
    bool running = true;

    while (running) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            running = false;
        }

        draw_frame();
        napms(16); /* ~60 FPS placeholder */
    }
}

void game_shutdown(void) {
    endwin();
}

static void draw_frame(void) {
    erase();
    box(stdscr, 0, 0);

    if (!has_enough_space()) {
        mvprintw(LINES / 2, (COLS - 30) / 2, "Enlarge the terminal window.");
        refresh();
        return;
    }

    draw_banner();
    draw_board(4, 8);

    refresh();
}

static void board_reset(void) {
    memset(g_board, 0, sizeof(g_board));

    for (int col = 2; col < 8; ++col) {
        g_board[BOARD_HEIGHT - 1][col] = CELL_STATIC;
    }

    for (int row = BOARD_HEIGHT - 5; row < BOARD_HEIGHT - 1; ++row) {
        g_board[row][1] = CELL_STATIC;
        g_board[row][BOARD_WIDTH - 2] = CELL_STATIC;
    }

    int center = BOARD_WIDTH / 2;
    if (center > 1) {
        g_board[8][center] = CELL_STATIC;
        g_board[9][center] = CELL_STATIC;
        g_board[9][center + 1] = CELL_STATIC;
        g_board[10][center + 1] = CELL_STATIC;
    }
}

static bool has_enough_space(void) {
    const int min_rows = BOARD_HEIGHT + 8;
    const int min_cols = BOARD_WIDTH * 2 + 18;
    return (LINES >= min_rows) && (COLS >= min_cols);
}

static void draw_banner(void) {
    if (g_use_color) {
        attron(COLOR_PAIR(1));
    }
    mvprintw(1, 2, "Terminal Tetris Prototype");
    if (g_use_color) {
        attroff(COLOR_PAIR(1));
    }

    mvprintw(2, 2, "Press 'q' to quit");
    mvprintw(3, 2, "Prototype board rendering active.");
}

static void draw_board(int origin_y, int origin_x) {
    const int inner_width = BOARD_WIDTH * 2;

    move(origin_y - 1, origin_x - 1);
    addch('+');
    for (int i = 0; i < inner_width; ++i) {
        addch('-');
    }
    addch('+');

    for (int row = 0; row < BOARD_HEIGHT; ++row) {
        move(origin_y + row, origin_x - 1);
        addch('|');
        for (int col = 0; col < BOARD_WIDTH; ++col) {
            if (g_board[row][col] == CELL_STATIC) {
                if (g_use_color) {
                    attron(COLOR_PAIR(1));
                }
                addstr("[]");
                if (g_use_color) {
                    attroff(COLOR_PAIR(1));
                }
            } else {
                addstr("  ");
            }
        }
        addch('|');
    }

    move(origin_y + BOARD_HEIGHT, origin_x - 1);
    addch('+');
    for (int i = 0; i < inner_width; ++i) {
        addch('-');
    }
    addch('+');
}
