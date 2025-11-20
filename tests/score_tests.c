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
    const char *path = "build/tests/score_line.dat";
    cleanup_file(path);

    ScoreState state;
    assert(score_state_init(&state, path) == 0);

    score_add_lines(&state, 2);
    assert(state.current == 300);

    score_add_lines(&state, 4);
    assert(state.current == 1100);

    cleanup_file(path);
}

static void test_score_line_overflow_award(void) {
    const char *path = "build/tests/score_line_overflow.dat";
    cleanup_file(path);

    ScoreState state;
    assert(score_state_init(&state, path) == 0);

    score_add_lines(&state, 6);
    assert(state.current > 0);
    assert(state.current >= 800 + 200);

    cleanup_file(path);
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

static void test_score_drop_award_and_reset(void) {
    const char *path = "build/tests/score_drop.dat";
    cleanup_file(path);

    ScoreState state;
    assert(score_state_init(&state, path) == 0);

    score_add_drop(&state, 5);
    assert(state.current == 10);
    score_reset_current(&state);
    assert(state.current == 0);

    cleanup_file(path);
}

static void test_score_highscore_only_increases(void) {
    ScoreState state;
    assert(score_state_init(&state, "build/tests/score_high.dat") == 0);
    state.current = 400;
    assert(score_commit_highscore(&state));
    assert(score_state_save(&state) == 0);

    ScoreState reload;
    assert(score_state_init(&reload, "build/tests/score_high.dat") == 0);
    reload.current = 200;
    assert(!score_commit_highscore(&reload));
    cleanup_file("build/tests/score_high.dat");
}

static void run_test(const char *name, void (*fn)(void)) {
    printf("[RUN] %s\n", name);
    fn();
    printf("[OK ] %s\n", name);
}

int main(void) {
    run_test("score_init_without_file", test_score_init_without_file);
    run_test("score_line_awards", test_score_line_awards);
    run_test("score_line_overflow_award", test_score_line_overflow_award);
    run_test("score_persistence", test_score_persistence);
    run_test("score_drop_award_and_reset", test_score_drop_award_and_reset);
    run_test("score_highscore_only_increases", test_score_highscore_only_increases);
    return 0;
}
