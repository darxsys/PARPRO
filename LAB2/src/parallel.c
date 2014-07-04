#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <sys/time.h>
#include <mpi.h>

#include "parallel.h"
#include "serial.h"

void killall(int size) {
    int i;
    for (i = 1; i < size; i++) {
        MPI_Send(&i, 1, MPI_INT, i, FINISH, MPI_COMM_WORLD);
    } 
}

float evaluate_parallel(State* state, int mover, int depth, 
    int split_depth, int rank, int size) {

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

    // master
    if (rank == 0 && split_depth == 0) {
        MPI_Status status;
        int recv;
        int flag;
        // int flag;
        int res = 0;
        // int moved = 0;
        i = 0;
        int win_state = 0;
        int lose_state = 0;
        int advanced = 1;

        // receiving requests or results and sending tasks
        while (i < WIDTH || res < WIDTH) {
            while (advanced && i < WIDTH && (make_move(state, mover, i) == 0)) {
                i++;
                res++;
                // moved = 1;
            }

            advanced = 0;
            // until all tasks are done
            if (i < WIDTH) {
                MPI_Iprobe(MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &flag, &status);

                if (flag) {
                    MPI_Recv(&recv, 1, MPI_INT, status.MPI_SOURCE, REQUEST, 
                        MPI_COMM_WORLD, &status);

                    // printf("Received a request from: %d\n", status.MPI_SOURCE);
                    cant_move = 0;
                    // send state and current depth
                    // send there is task
                    MPI_Send(&recv, 1, MPI_INT, status.MPI_SOURCE, TASK, MPI_COMM_WORLD);
                    // send data
                    MPI_Send(state, sizeof(State), MPI_BYTE, status.MPI_SOURCE, 
                        DATA, MPI_COMM_WORLD);
                    MPI_Send(state->table, state->height * WIDTH, MPI_CHAR, 
                        status.MPI_SOURCE, DATA, MPI_COMM_WORLD);

                    MPI_Send(&depth, 1, MPI_INT, status.MPI_SOURCE, 
                        DATA, MPI_COMM_WORLD);

                    MPI_Send(&mover, 1, MPI_INT, status.MPI_SOURCE, 
                        DATA, MPI_COMM_WORLD);

                    undo_move(state, i, last_x, last_y);
                    advanced = 1;
                    i++;
                }

            } 

            if (res < WIDTH) {
                MPI_Iprobe(MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &flag, &status);
                if (flag) {
                    MPI_Recv(&recv, 1, MPI_INT, status.MPI_SOURCE, RESULT, 
                        MPI_COMM_WORLD, &status);

                    res++;
                    MPI_Recv(&temp_eval, 1, MPI_FLOAT, status.MPI_SOURCE, 
                        DATA, MPI_COMM_WORLD, &status);
                        // winning state
                    if (fabs(temp_eval - 1) < EPS && mover == AI) {
                        // make_move()
                        // undo_move(state, i, last_x, last_y);
                        win_state = 1;
                    }

                    // losing state
                    if (fabs(temp_eval + 1) < EPS && mover == HU) {
                        // undo_move(state, i, last_x, last_y);
                        lose_state =  -1;
                    }

                    count++;
                    eval += temp_eval;
                }
            }
        }

        if (win_state) {
            return 1;
        }

        if (lose_state) {
            return -1;
        }

    } else {
        if (depth > 0) {
            for (i = 0; i < WIDTH; i++) {
                if (make_move(state, mover, i)) {
                    // printf("Made a move\n");
                    // print_state(state);
                    // printf("")
                    cant_move = 0;
                    temp_eval = evaluate_parallel(state, mover^1, depth-1, 
                        split_depth-1, rank, size);
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

void wait(int rank, int size) {
    // workers
    int msg = REQUEST;
    int depth;
    int task;
    int mover;
    // int move;
    MPI_Status status;
    State* state = (State* )malloc(sizeof(State));

    // send request
    while (1) {
        MPI_Send(&msg, 1, MPI_INT, 0, REQUEST, MPI_COMM_WORLD);
        MPI_Recv(&task, 1, MPI_INT, 0, MPI_ANY_TAG, 
            MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == TASK) {
            // receive the task and process it
            MPI_Recv(state, sizeof(State), MPI_BYTE, 0, DATA, 
                MPI_COMM_WORLD, &status);

            state->table = (char* )malloc(sizeof(char) * WIDTH * state->height);

            MPI_Recv(state->table, WIDTH * state->height, MPI_CHAR, 0, DATA, 
                MPI_COMM_WORLD, &status);

            MPI_Recv(&depth, 1, MPI_INT, 0, DATA, MPI_COMM_WORLD, &status);
            depth--;
            // receive mover
            MPI_Recv(&mover, 1, MPI_INT, 0, DATA, MPI_COMM_WORLD, &status);
            mover ^= 1;

            float eval = evaluate_parallel(state, mover, depth, 0, rank, size);
            // printf("%f\n", eval);

            MPI_Send(&msg, 1, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
            MPI_Send(&eval, 1, MPI_FLOAT, 0, DATA, MPI_COMM_WORLD);
            // MPI_Send(&move, 1, MPI_INT, 0, DATA, MPI_COMM_WORLD);

            free(state->table);
        } else {
            // if no more tasks, return
            free(state);
            return;
        }
        
    }
}

State* play_move_parallel(State* state, int rank, int size, int depth) {
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
                return state;

            }
            temp_eval = evaluate_parallel(state, mover^1, depth-1, 
                depth/3-1, rank, size);

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

void play_parallel(int height, int depth, int rank, int size) {
    State *s = (State* )malloc(sizeof(State));
    s->table = (char* )malloc(sizeof(char) * height * WIDTH);
    s->height = height;

    printf("Welcome.\n");
    s = initialize_state(s);
    print_state(s);

    // measuring stuff
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
        play_move_parallel(s, rank, size, depth);
        gettimeofday(&after, NULL);

        double time = (after.tv_sec - before.tv_sec) * 1000000 + (after.tv_usec - before.tv_usec);
        time /= 1000000;
        printf("Move time: %lf [s]\n", time);

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

    // send everyone a notification of finishing and exit
    killall(size);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3) {
        printf("Depth and height needed.\n");
        exit(1);
    }

    int depth = atoi(argv[1]);
    int height = atoi(argv[2]);
    if (depth < 4) {
        printf("Depth has to be at least 4.\n");
        killall(size);
        return 0;
    }

    if (size == 1) {
        //serial version
        play(height, depth);
    } else {
        if (rank == 0) {
            play_parallel(height, depth, rank, size);
        } else {
            wait(rank, size);
        }
    }

    MPI_Finalize();
    return 0;
}