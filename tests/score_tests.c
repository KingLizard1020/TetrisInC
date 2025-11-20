#include <assert.h>
#include <stdio.h>

#include "score.h"

static void cleanup_file(const char *path) {
    if (path != NULL) {
        remove(path);
    }
}

static void test_score_init_without_file(void) {
    const char *path = "build/tests/score_missing.dat";
    cleanup_file(path);

    ScoreState state;
    assert(score_state_init(&state, path) == 0);
    assert(state.current == 0);
    assert(state.high == 0);
}

static void test_score_line_awards(void) {
    ScoreState state;
    assert(score_state_init(&state, "build/tests/score_line.dat") == 0);

    score_add_lines(&state, 2);
    assert(state.current == 300);

    score_add_lines(&state, 4);
    assert(state.current == 1100);
}

static void test_score_persistence(void) {
    const char *path = "build/tests/score_persist.dat";
    cleanup_file(path);

    ScoreState state;
    assert(score_state_init(&state, path) == 0);
    state.current = 500;
    assert(score_commit_highscore(&state));
    assert(score_state_save(&state) == 0);

    ScoreState reread;
    assert(score_state_init(&reread, path) == 0);
    assert(reread.high == 500);
    cleanup_file(path);
}

static void run_test(const char *name, void (*fn)(void)) {
    printf("[RUN] %s\n", name);
    fn();
    printf("[OK ] %s\n", name);
}

int main(void) {
    run_test("score_init_without_file", test_score_init_without_file);
    run_test("score_line_awards", test_score_line_awards);
    run_test("score_persistence", test_score_persistence);
    return 0;
}
