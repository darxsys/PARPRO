/**
    CS Parallel programming
    philosophers.cpp
    Purpose: Simulates the dining philosophers problem using MPI.
    Using mpich.

    @author Dario Pavlovic
    @version 1.0 30/03/2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <vector>
using namespace std;

// 0 - thinking, 1 - hungry
int mood_state = 0;

/**
    Prints text with arbitrary number of spaces to the left.

    @param indent - number of spaces
    text - text to print
    @return
*/
void print_with_indent(int indent, char *text) {
    printf("%*s" "%s", indent, " ", text);
}

/**
    Dummy function that simulates "thinking" from 1 to 4 secs.

    @param arg - from pthread_create accepts process id.
    @return NULL pointer
*/
void *think(void *arg) {
    int ID = *((int*) arg);
    int sleep_sec = rand() % 5 + 1;
    char buffer[200];
    sprintf(buffer, "Philosopher %d thinking for %d secs...\n", ID, sleep_sec);
    print_with_indent(ID+1, buffer);
    
    sleep(sleep_sec);
    mood_state = 1;
    return NULL;
}

/**
    Endlessly simulates philosophers behavior.

    @param rank - rank of the process
    comm_size - number of processes

    @return
*/
void philosopher(int rank, int comm_size) {
    // 0 - clean, 1 - dirty, 2 - don't have
    int left_fork;
    int right_fork;
    int left_request = 0;
    int right_request = 0;
    // flags to determine if this process has already asked for a fork
    int left_send = 0;
    int right_send = 0;
    int *fork;
    int *request;
    int *send;
    int left_neighbor = (rank + 1) % comm_size;
    int right_neighbor = (rank - 1 + comm_size) % comm_size;
    int left_sent = 0;
    int right_sent = 0;
    int *sent;

    pthread_t think_t;
    char buffer[200];

    // initialize forks
    if (rank == 0) {
        left_fork = 1;
        right_fork = 1;
    } else if (rank == comm_size - 1) {
        left_fork = 2;
        right_fork = 2;
    } else {
        left_fork = 1;
        right_fork = 2;
    }

    // Communication variables
    MPI_Status probe_stats;
    MPI_Status stats;
    MPI_Request req1, req2;
    int flag;
    int buf = 1;
    // 0 - requesting a fork, 1 giving a fork
    int tag = 1;

    while(1) {
        // thinking
        mood_state = 0;
        pthread_create(&think_t, NULL, think, (void*) &rank);

        while (mood_state == 0) {
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &probe_stats);
            
            if (flag == true) {
                // there is a request
                MPI_Recv((void *) &buf, 0, MPI_INT, probe_stats.MPI_SOURCE, 
                    probe_stats.MPI_TAG, MPI_COMM_WORLD, &stats);

                // serve the request if possible, otherwise just remember it
                if (stats.MPI_SOURCE == left_neighbor && left_sent == 0) {
                    // neighbor is asking for the current process left fork
                    fork = &left_fork;
                    request = &left_request;
                    sent = &left_sent;
                } else if (stats.MPI_SOURCE == right_neighbor && right_sent == 0) {
                    // neighbor is asking for the current process right fork
                    fork = &right_fork;
                    request = &right_request;
                    sent = &right_sent;
                }

                if (*fork == 1) {
                    tag = 1;
                    *fork = 2;
                    *request = 0;
                    *sent = 1;
                    MPI_Send((void *) &buf, 0, MPI_INT, stats.MPI_SOURCE, 1, MPI_COMM_WORLD);
                } else {
                    // remember the request
                    *request = 1;
                }      
            }
        }
        
        pthread_join(think_t, NULL);

        // wanna eat
        // TODO - finish this
        while (left_fork == 2 || right_fork == 2) {
            if (left_fork == 2 && left_send == 0) {
                int dest = left_neighbor;
                sprintf(buffer, "Philosopher %d asking for fork (%d)\n", rank, dest);
                print_with_indent(rank+1, buffer);

                left_sent = 0;
                MPI_Isend((void *) &buf, 0, MPI_INT, dest, 0, MPI_COMM_WORLD, &req1);
                left_send = 1;
            }

            if (right_fork == 2 && right_send == 0) {
                int dest = right_neighbor;
                sprintf(buffer, "Philosopher %d asking for fork (%d)\n", rank, dest);
                print_with_indent(rank+1, buffer);

                right_sent = 0;
                MPI_Isend((void *) &buf, 0, MPI_INT, dest, 0, MPI_COMM_WORLD, &req2);
                right_send = 1;
            }

            // wait for a message
            MPI_Recv((void *) &buf, 0, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                MPI_COMM_WORLD, &stats);
            if (stats.MPI_TAG == 0) {
                int dest;
                // fork is requested
                if (stats.MPI_SOURCE == left_neighbor && left_sent == 0) {
                    // left fork
                    fork = &left_fork;
                    request = &left_request;
                    send = &left_send;
                    dest = left_neighbor;
                    sent = &left_sent;
                } else if (stats.MPI_SOURCE == right_neighbor && right_sent == 0) {
                    fork = &right_fork;
                    request = &right_request;
                    send = &right_send;
                    dest = right_neighbor;
                    sent = &right_sent;
                }

                if (*fork == 1) {
                    // fork is dirty and send it
                    MPI_Send((void *) &buf, 0, MPI_INT, dest, 1, MPI_COMM_WORLD);
                    *fork = 2;
                    *request = 0;
                    *send = 0;
                    *sent = 1;
                } else {
                    *request = 1;
                }

            } else if (stats.MPI_TAG = 1) {
                // fork is given
                if (stats.MPI_SOURCE == left_neighbor && left_fork == 2) {
                    left_fork = 0;
                    left_send = 0;
                } else if (stats.MPI_SOURCE == right_neighbor) {
                    right_fork = 0;
                    right_send = 0;
                }
            }
        }

        sprintf(buffer, "Philosopher %d is eating...\n", rank);
        print_with_indent(rank+1, buffer);
        left_fork = 1;
        right_fork = 1;

        // check existing requests
        if (left_request == 1) {
            MPI_Send((void *) &buf, 0, MPI_INT, left_neighbor, 1, MPI_COMM_WORLD);    
            left_fork = 2;
        }

        if (right_request == 1) {
            MPI_Send((void *) &buf, 0, MPI_INT, right_neighbor, 1, MPI_COMM_WORLD);    
            right_fork = 2;
        }
    }
}

int main(int argc, char **argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    int rank;
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    srand((unsigned)time(NULL) + rank);
    philosopher(rank, size);

    // pthread_create(&think_t, NULL, think, (void*) &rank);
    MPI_Finalize();
    return 0;
}
