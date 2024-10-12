#ifndef ROBINWAITA_SCHEDULER_H
#define ROBINWAITA_SCHEDULER_H
#include "process.h"

typedef struct initData initData;
struct Queue;

typedef struct Node {
    Process *process;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *front;
    Node *rear;
    int numElements;
} Queue;

void displayProcess(Process *p, Queue *q, initData *data); //display new process when added to queue
void initQueue(Queue *q); //initialize process queue
int isEmpty(Queue *q); //check if queue empty
void enQueue(Queue *q, Process *p); //add process struct to queue
void deQueue(Queue *q); //remove process struct from queue
int timerSetup(Queue *q); //setup timerfd
void manageQueue(struct initData *data); //scheduling logic, gets called repeatedly
void addElapsedTime(Process *p, double *totalTime, initData *data); //get total time for running processes and display

#endif //ROBINWAITA_SCHEDULER_H
