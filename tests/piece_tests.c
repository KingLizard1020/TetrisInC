#include <assert.h>
#include <stdio.h>

#include "piece.h"

static void run_test(const char *name, void (*fn)(void)) {
    printf("[RUN] %s\n", name);
    fn();
    printf("[OK ] %s\n", name);
}

static void test_piece_count_and_rotations(void) {
    size_t count = piece_shape_count();
    assert(count >= 7);

    for (size_t i = 0; i < count; ++i) {
        const PieceShape *shape = piece_shape_get(i);
        assert(shape != NULL);
        assert(shape->rotation_count >= 1);
        for (int r = 0; r < shape->rotation_count; ++r) {
            assert(shape->rotations[r] != NULL);
        }
    }
}

static void test_piece_shape_cell_accessor(void) {
    const PieceShape *shape = piece_shape_get(0);
    assert(shape != NULL);

    assert(!piece_shape_cell_filled(shape, 0, 0, 0));
    assert(piece_shape_cell_filled(shape, 0, 1, 2));
    assert(piece_shape_cell_filled(shape, 1, 0, 2));
    assert(piece_shape_cell_filled(shape, 1, 3, 2));
    assert(!piece_shape_cell_filled(shape, 1, 0, 1));
}

int main(void) {
    run_test("piece_count_and_rotations", test_piece_count_and_rotations);
    run_test("piece_shape_cell_accessor", test_piece_shape_cell_accessor);
    return 0;
}
