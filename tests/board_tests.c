#include <assert.h>
#include <stdio.h>

#include "board.h"
#include "piece.h"

static const PieceShape *first_shape(void) {
    if (piece_shape_count() == 0) {
        return NULL;
    }
    return piece_shape_get(0);
}

static void test_board_can_place_empty(void) {
    Board board;
    board_reset(&board);
    const PieceShape *shape = first_shape();
    assert(shape != NULL);
    assert(board_can_place(&board, shape, 0, 0, 3));
}

static void test_board_can_place_blocked(void) {
    Board board;
    board_reset(&board);
    const PieceShape *shape = first_shape();
    assert(shape != NULL);

    board.cells[0][3] = 99;
    assert(!board_can_place(&board, shape, 0, 0, 3));
}

static void test_board_lock_and_clear(void) {
    Board board;
    board_reset(&board);
    const PieceShape *shape = first_shape();
    assert(shape != NULL);

    board_lock_shape(&board, shape, 0, 0, 3, 1);
    assert(board.cells[0][3] == 1);

    for (int col = 0; col < BOARD_WIDTH; ++col) {
        board.cells[BOARD_HEIGHT - 1][col] = 5;
    }
    int cleared = board_clear_completed_lines(&board);
    assert(cleared == 1);
    for (int col = 0; col < BOARD_WIDTH; ++col) {
        assert(board.cells[BOARD_HEIGHT - 1][col] == 0);
    }
}

static void run_test(const char *name, void (*fn)(void)) {
    printf("[RUN] %s\n", name);
    fn();
    printf("[OK ] %s\n", name);
}

int main(void) {
    run_test("board_can_place_empty", test_board_can_place_empty);
    run_test("board_can_place_blocked", test_board_can_place_blocked);
    run_test("board_lock_and_clear", test_board_lock_and_clear);
    return 0;
}
