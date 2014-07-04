#ifndef SERIAL_H
#define SERIAL_H

#define WIDTH 7
#define MAX_DEPTH 7
#define HEIGHT 4
// moves
#define EMPTY 5
#define AI 0
#define HU 1
#define EPS 1e-6

#define GAME_OVER -1
#define CONTINUE 2

typedef struct {
    //last move
    int x;
    int y;
    int height;
    int over;
    char *table;
} State;

void play(int height, int depth);
float evaluate(State* state, int mover, int depth);
State* initialize_state(State* state);
void print_state(State* state);
State* input_player(State* state);
int game_over(State* state);
int make_move(State* state, int mover, int col);
void undo_move(State* state, int col, int x, int y);
int check_four(State* state);

#endif