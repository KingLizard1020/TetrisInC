#if defined(_WIN32)
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "board.h"
#include "game.h"
#include "piece.h"
#include "score.h"

#define GRAVITY_INTERVAL_MS 700ULL
#define CELL_EMPTY 0

static bool g_use_color = false;
static Board g_board;
static ActivePiece g_active_piece;
static uint64_t g_gravity_accumulator_ms = 0ULL;
static ScoreState g_score;
static int g_next_piece_type = -1;

static void spawn_piece(void);
static const PieceShape *current_piece_shape(void);
static const PieceShape *next_piece_shape(void);
static void ensure_next_piece(void);
static bool try_move_piece(int drow, int dcol);
static bool try_rotate_piece(int direction);
static void lock_piece(void);
static int clear_completed_lines(void);
static void reset_board_state(void);
static void handle_input(int ch, bool *running);
static void update_game(uint64_t delta_ms);
static uint64_t monotonic_millis(void);
static void settle_active_piece(int drop_bonus_cells);
static void draw_frame(void);
static bool has_enough_space(void);
static void draw_banner(void);
static void draw_board(int origin_y, int origin_x);
static void draw_active_piece(int origin_y, int origin_x);
static void draw_score_panel(int origin_y, int origin_x);
static void draw_next_piece_panel(int origin_y, int origin_x);
static void draw_piece_preview(int origin_y, int origin_x, const PieceShape *shape);

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

    score_state_init(&g_score, SCORE_DEFAULT_FILE);
    reset_board_state();
    srand((unsigned int)time(NULL));
    ensure_next_piece();
    spawn_piece();

    return 0;
}

void game_loop(void) {
    bool running = true;
    uint64_t last_tick = monotonic_millis();

    while (running) {
        int ch = getch();
        handle_input(ch, &running);

        uint64_t now = monotonic_millis();
        uint64_t delta = now - last_tick;
        last_tick = now;

        update_game(delta);

        draw_frame();
        napms(1);
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

    const int board_origin_y = 4;
    const int board_origin_x = 8;
    const int hud_origin_x = board_origin_x + BOARD_WIDTH * 2 + 4;

    draw_banner();
    draw_board(board_origin_y, board_origin_x);
    draw_active_piece(board_origin_y, board_origin_x);
    draw_score_panel(board_origin_y, hud_origin_x);
    draw_next_piece_panel(board_origin_y + 6, hud_origin_x);

    refresh();
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
    mvprintw(3, 2, "Arrows move, Space hard drops.");
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
            if (g_board.cells[row][col] != CELL_EMPTY) {
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

static void draw_active_piece(int origin_y, int origin_x) {
    if (!g_active_piece.active) {
        return;
    }

    const PieceShape *shape = current_piece_shape();
    const char *pattern = shape->rotations[g_active_piece.rotation];

    for (int r = 0; r < shape->size; ++r) {
        for (int c = 0; c < shape->size; ++c) {
            if (pattern[r * shape->size + c] != '1') {
                continue;
            }

            int board_row = g_active_piece.row + r;
            int board_col = g_active_piece.col + c;
            if (board_row < 0 || board_row >= BOARD_HEIGHT || board_col < 0 || board_col >= BOARD_WIDTH) {
                continue;
            }

            int screen_y = origin_y + board_row;
            int screen_x = origin_x + board_col * 2;

            move(screen_y, screen_x);
            if (g_use_color) {
                attron(COLOR_PAIR(1));
            }
            addstr("[]");
            if (g_use_color) {
                attroff(COLOR_PAIR(1));
            }
        }
    }
}

static void handle_input(int ch, bool *running) {
    if (ch == ERR) {
        return;
    }

    switch (ch) {
        case 'q':
        case 'Q':
            *running = false;
            break;
        case KEY_LEFT:
        case 'a':
        case 'A':
            try_move_piece(0, -1);
            break;
        case KEY_RIGHT:
        case 'd':
        case 'D':
            try_move_piece(0, 1);
            break;
        case KEY_DOWN:
        case 's':
        case 'S':
            if (!try_move_piece(1, 0)) {
                settle_active_piece(0);
            }
            break;
        case KEY_UP:
        case 'w':
        case 'W':
            try_rotate_piece(1);
            break;
        case ' ':
            {
                int dropped = 0;
                while (try_move_piece(1, 0)) {
                    ++dropped;
                }
                settle_active_piece(dropped);
            }
            break;
    }
}

static void update_game(uint64_t delta_ms) {
    if (!g_active_piece.active) {
        spawn_piece();
    }

    g_gravity_accumulator_ms += delta_ms;
    while (g_gravity_accumulator_ms >= GRAVITY_INTERVAL_MS) {
        g_gravity_accumulator_ms -= GRAVITY_INTERVAL_MS;
        if (!try_move_piece(1, 0)) {
            settle_active_piece(0);
            break;
        }
    }
}

static uint64_t monotonic_millis(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

static void spawn_piece(void) {
    size_t total_shapes = piece_shape_count();
    if (total_shapes == 0) {
        g_active_piece.active = false;
        return;
    }

    ensure_next_piece();
    g_active_piece.type = g_next_piece_type;
    g_next_piece_type = (int)(rand() % (int)total_shapes);
    g_active_piece.rotation = 0;
    g_active_piece.row = -2;
    const PieceShape *shape = current_piece_shape();
    g_active_piece.col = (BOARD_WIDTH - shape->size) / 2;
    g_active_piece.active = true;

    if (!board_can_place(&g_board, shape, g_active_piece.rotation, g_active_piece.row, g_active_piece.col)) {
        reset_board_state();
        ensure_next_piece();
    }
}

static bool try_move_piece(int drow, int dcol) {
    if (!g_active_piece.active) {
        return false;
    }

    int next_row = g_active_piece.row + drow;
    int next_col = g_active_piece.col + dcol;
    const PieceShape *shape = current_piece_shape();
    if (!board_can_place(&g_board, shape, g_active_piece.rotation, next_row, next_col)) {
        return false;
    }

    g_active_piece.row = next_row;
    g_active_piece.col = next_col;
    return true;
}

static bool try_rotate_piece(int direction) {
    if (!g_active_piece.active) {
        return false;
    }

    const PieceShape *shape = current_piece_shape();
    int next_rotation = (g_active_piece.rotation + direction + shape->rotation_count) % shape->rotation_count;
    if (!board_can_place(&g_board, shape, next_rotation, g_active_piece.row, g_active_piece.col)) {
        return false;
    }

    g_active_piece.rotation = next_rotation;
    return true;
}

static void lock_piece(void) {
    if (!g_active_piece.active) {
        return;
    }

    const PieceShape *shape = current_piece_shape();
    board_lock_shape(&g_board, shape, g_active_piece.rotation, g_active_piece.row, g_active_piece.col, g_active_piece.type + 1);
    g_active_piece.active = false;
}

static int clear_completed_lines(void) {
    return board_clear_completed_lines(&g_board);
}

static const PieceShape *current_piece_shape(void) {
    return piece_shape_get((size_t)g_active_piece.type);
}

static const PieceShape *next_piece_shape(void) {
    if (g_next_piece_type < 0) {
        return NULL;
    }
    return piece_shape_get((size_t)g_next_piece_type);
}

static void ensure_next_piece(void) {
    if (g_next_piece_type >= 0) {
        return;
    }

    size_t total_shapes = piece_shape_count();
    if (total_shapes == 0) {
        g_next_piece_type = -1;
        return;
    }

    g_next_piece_type = (int)(rand() % (int)total_shapes);
}

static void reset_board_state(void) {
    board_reset(&g_board);
    g_active_piece.active = false;
    g_gravity_accumulator_ms = 0ULL;
    g_next_piece_type = -1;
    score_reset_current(&g_score);
}

static void settle_active_piece(int drop_bonus_cells) {
    lock_piece();

    if (drop_bonus_cells > 0) {
        score_add_drop(&g_score, drop_bonus_cells);
    }

    int cleared = clear_completed_lines();
    if (cleared > 0) {
        score_add_lines(&g_score, cleared);
    }

    if (score_commit_highscore(&g_score)) {
        score_state_save(&g_score);
    }

    spawn_piece();
}

static void draw_score_panel(int origin_y, int origin_x) {
    mvprintw(origin_y, origin_x,   "Score     : %d", g_score.current);
    mvprintw(origin_y + 1, origin_x, "High Score: %d", g_score.high);
}

static void draw_next_piece_panel(int origin_y, int origin_x) {
    mvprintw(origin_y, origin_x, "Next Piece:");

    move(origin_y + 1, origin_x);
    addstr("+--------+");
    for (int row = 0; row < 4; ++row) {
        move(origin_y + 2 + row, origin_x);
        addch('|');
        addstr("        ");
        addch('|');
    }
    move(origin_y + 6, origin_x);
    addstr("+--------+");

    draw_piece_preview(origin_y + 2, origin_x + 1, next_piece_shape());
}

static void draw_piece_preview(int origin_y, int origin_x, const PieceShape *shape) {
    if (shape == NULL) {
        return;
    }

    const int preview_offset = (4 - shape->size) / 2;
    const char *pattern = shape->rotations[0];
    for (int r = 0; r < shape->size; ++r) {
        for (int c = 0; c < shape->size; ++c) {
            if (pattern[r * shape->size + c] != '1') {
                continue;
            }

            move(origin_y + preview_offset + r, origin_x + preview_offset * 2 + c * 2);
            if (g_use_color) {
                attron(COLOR_PAIR(1));
            }
            addstr("[]");
            if (g_use_color) {
                attroff(COLOR_PAIR(1));
            }
        }
    }
}
