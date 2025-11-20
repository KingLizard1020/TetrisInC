#ifndef PIECE_H
#define PIECE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int size;
    int rotation_count;
    const char *rotations[4];
} PieceShape;

typedef struct {
    int type;
    int rotation;
    int row;
    int col;
    bool active;
} ActivePiece;

size_t piece_shape_count(void);
const PieceShape *piece_shape_get(size_t index);
bool piece_shape_cell_filled(const PieceShape *shape, int rotation, int local_row, int local_col);

#endif /* PIECE_H */
