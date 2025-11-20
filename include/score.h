#ifndef SCORE_H
#define SCORE_H

#include <stdbool.h>

#define SCORE_DEFAULT_FILE "highscore.dat"
#define SCORE_PATH_CAPACITY 512

typedef struct {
    int current;
    int high;
    char storage_path[SCORE_PATH_CAPACITY];
} ScoreState;

int score_state_init(ScoreState *state, const char *path);
int score_state_save(const ScoreState *state);
void score_reset_current(ScoreState *state);
void score_add_lines(ScoreState *state, int cleared_lines);
void score_add_drop(ScoreState *state, int dropped_cells);
bool score_commit_highscore(ScoreState *state);

#endif /* SCORE_H */
