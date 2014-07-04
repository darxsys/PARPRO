#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#include "serial.h"

State* initialize_state(State* state) {
    int i, j;
    state->x = -1;
    state->y = -1;
    state->over = 0;
    for (i = 0; i < state->height; i++) {
        for (j = 0; j < WIDTH; j++) {
            state->table[i * WIDTH + j] = EMPTY;
            // state->heights[j] = 0;
        }
    }

    return state;
}

void print_state(State* state) {
    int i, j;
    printf("\n");
    for (i = 0; i < state->height; i++) {
        for (j = 0; j < WIDTH; j++) {
            if (state->table[i * WIDTH + j] == EMPTY) {
                printf("- ");
            } else if (state->table[i * WIDTH + j] == AI) {
                printf("x ");
            } else if (state->table[i * WIDTH + j] == HU) {
                printf("o ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

// check if there are four in a row from the current element
int check_row(State* state, int x, int y) {
    int curr = state->table[x * WIDTH + y];
    int left = y;
    int right = y;
    while (right+1 < WIDTH && (state->table[x * WIDTH + right + 1] == curr)) {
        right++;
    }

    while (left-1 >= 0 && (state->table[x * WIDTH + left - 1] == curr)) {
        left--;
    }

    if (right - left + 1 == 4) {
        return curr;
    }
    
    return EMPTY;
}

// chech for four in a column
int check_col(State* state, int x, int y) {
    int curr = state->table[x * WIDTH + y];
    if (x < state->height-3) {
        if ((curr == state->table[(x+1) * WIDTH + y]) &&
            (curr == state->table[(x+2) * WIDTH + y]) &&
            (curr == state->table[(x+3) * WIDTH + y])) {
            return curr;
        }
    }

    return EMPTY;
}

// checks two diagonals from current point
int check_slash_diagonal(State* state, int x, int y) {
    int curr = state->table[x * WIDTH + y];
    int left_x = x;
    int left_y = y;
    int right_x = x; 
    int right_y = y;

    // stretch to the left
    while(left_x + 1 < state->height && left_y - 1 >= 0 && 
        state->table[(left_x+1) * WIDTH + left_y - 1] == curr) {
        left_x++;
        left_y--;
    }

    // stretch to the right
    while(right_x - 1 >= 0 && right_y + 1 < WIDTH &&
        state->table[(right_x-1) * WIDTH + right_y + 1] == curr) {
        right_x--;
        right_y++;
    }

    if (right_y - left_y + 1 == 4) {
        return curr;
    }

    return EMPTY;
}

int check_backslash_diagonal(State* state, int x, int y) {
    int curr = state->table[x * WIDTH + y];
    int left_x = x;
    int left_y = y;
    int right_x = x; 
    int right_y = y;

    // stretch to the left
    while(left_x - 1 >= 0 && left_y - 1 >= 0 && 
        (state->table[(left_x-1) * WIDTH + left_y - 1] == curr)) {
        left_x--;
        left_y--;
    }

    // stretch to the right
    while(right_x+1 < state->height && right_y + 1 < WIDTH &&
        (state->table[(right_x+1) * WIDTH + right_y + 1] == curr)) {
        right_x++;
        right_y++;
    }

    if (right_y - left_y + 1 == 4) {
        return curr;
    }

    return EMPTY;
}

// checks for four same in a row in the state
int check_four(State* state) {
    int row = state->x;
    int col = state->y;

    int curr = check_row(state, row, col);
    if (curr != EMPTY) {
        return curr;
    }

    curr = check_col(state, row, col);
    if (curr != EMPTY) {
        return curr;
    }

    curr = check_slash_diagonal(state, row, col);
    if (curr != EMPTY) {
        return curr;
    }

    curr = check_backslash_diagonal(state, row, col);
    return curr;

}

int make_move(State* state, int mover, int col) {
    int row = -1;
    int i;
    for (i = state->height-1; i >= 0; i--) {
        if (state->table[i * WIDTH + col] == EMPTY) {
            row = i;
            break;
        }
    }

    if (i == -1) {
        return 0;
    }

    state->table[row * WIDTH + col] = mover;
    state->x = row;
    state->y = col;
    return 1;
}

// undo move in the col and set it to x y as last
void undo_move(State* state, int col, int x, int y) {
    int i;
    int row = -1;
    for (i = 0; i < state->height; i++) {
        if (state->table[i * WIDTH + col] != EMPTY) {
            row = i;
            break;
        }
    }

    state->table[row * WIDTH + col] = EMPTY;    
    state->x = x;
    state->y = y;
    // state->heights[col]--;
}

float evaluate(State* state, int mover, int depth) {
    // printf("Current state:");
    // printf("Depth: %d\n", depth);
    // print_state(state);
    float eval = 0;
    int i;
    int count = 0;
    float temp_eval;
    int last_x = state->x;
    int last_y = state->y;
    int cant_move = 1;

    int check;
    check = check_four(state);
    if (check == AI && (mover^1) == AI) {
        return 1;
    } else if (check == HU && (mover^1) == HU) {
        return -1;
    }

    if (depth > 0) {
        for (i = 0; i < WIDTH; i++) {
            if (make_move(state, mover, i)) {
                cant_move = 0;
                temp_eval = evaluate(state, mover^1, depth-1);
                // winning state
                if (fabs(temp_eval - 1) < EPS && mover == AI) {
                    undo_move(state, i, last_x, last_y);
                    return 1;
                }

                // losing state
                if (fabs(temp_eval + 1) < EPS && mover == HU) {
                    undo_move(state, i, last_x, last_y);
                    return -1;
                }

                count++;
                eval += temp_eval;   
                undo_move(state, i, last_x, last_y);        
            }
        }    
    }

    // cant move
    if (cant_move && depth > 0) {
        check = check_four(state);
    }
        // int check;
    if (cant_move) {
        if (check == AI && (mover^1) == AI) {
            return 1;
        } else if (check == HU && (mover^1) == HU) {
            return -1;
        } else {
            return 0;
        }
    }

    return eval/count;
}

State* play_move(State* state, int depth) {
    // printf("play_move\n");
    // print_state(state);
    int mover = AI;
    int i;
    float eval = -2;
    float temp_eval;
    int opt;
    int check;
    int last_x, last_y;
    last_x = state->x;
    last_y = state->y;

    for (i = 0; i < WIDTH; i++) {
        if (make_move(state, mover, i)) {
            if ((check = check_four(state)) == mover) {
                // if it is already winning
                // printf("Here\n");
                return state;
            }

            temp_eval = evaluate(state, mover^1, depth-1);
            if (temp_eval > eval) {
                eval = temp_eval;
                opt = i;
            }
            undo_move(state, i, last_x, last_y);
        }
    }

    printf("best move: %d\n", opt);
    printf("Evaluation %f\n", eval);
    make_move(state, mover, opt);
    return state;
}

State* input_player(State* state) {
    int x;
    printf("Input a move:\n");
    scanf("%d", &x);

    while(x < 0 || x > WIDTH || make_move(state, HU, x) == 0) {
        printf("Invalid column.\n");
        scanf("%d", &x);
    }

    return state;
}

// check if state is full
int full(State* state) {
    int i;
    for (i = 0; i < WIDTH; i++) {
        if (state->table[i] == EMPTY) {
            return 0;
        }
    }
    return 1;
}

// check if game is over and return the winner if any
// -1 -> over and no winner
// 2 -> not ov
int game_over(State* state) {
    int check;
    if ((check=check_four(state)) != EMPTY) {
        return check;
    }

    if (full(state)) {
        return GAME_OVER;
    }

    return CONTINUE;
}

void play(int height, int depth) {
    State *s = (State* )malloc(sizeof(State));
    s->table = (char* )malloc(sizeof(char) * height * WIDTH);
    s->height = height;

    printf("Welcome.\n");
    s = initialize_state(s);
    print_state(s);
    struct timeval before, after;

    // int over = game_over(s);
    int over;
    while (1) {
        input_player(s);
        over = game_over(s);
        print_state(s);
        if (over != CONTINUE) {
            break;
        }

        gettimeofday(&before, NULL);        
        play_move(s, depth);
        gettimeofday(&after, NULL);

        double time = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
        time /= 1000000;
        printf("Move time: %lf[s]\n", time);

                
        print_state(s);
        over = game_over(s);
        if (over != CONTINUE) {
            break;
        }
        
    }

    printf("GAME OVER. ");
    if (over == AI) {
        printf("AI won.\n");
    } else if (over == HU) {
        printf("HU won.\n");
    } else {
        printf("Tie.\n");
    }

    free(s->table);
    free(s);
}

// int main(void) {
//     int height = 4;
//     play(height);
//     return 0;
// }