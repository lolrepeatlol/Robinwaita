#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <time.h>
#include <gtk/gtk.h>
#include "main.h"

void displayProcess(Process *p, Queue *q, initData *data){
    gchar *output = g_strdup_printf("Process %d: %s\n", q->numElements, p->name);
    g_string_append(data->logBuffer, output);
    g_free(output);
}

void initQueue(Queue *q){
    q->front = NULL;
    q->rear = NULL;
    q->numElements = 0;
}

int isEmpty(Queue *q){
    return (q->front == NULL);
}

void enQueue(Queue *q, Process *p) {
    Node *newNode = (Node*) malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("enQueue: Failed to allocate memory for new node");
        return;
    }

    newNode->process = p;
    newNode->next = NULL;

    if (isEmpty(q)) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }

    q->numElements++;
}

void deQueue(Queue *q) {
    if (isEmpty(q)) {
        perror("deQueue: Attempt to dequeue from empty queue");
        return;
    }

    Node *temp = q->front;
    q->front = q->front->next;

    if (temp->process->state == 3) {
        //free if completed
        free(temp->process->name);
        for (int i = 0; temp->process->args[i] != NULL; i++) {
            free(temp->process->args[i]);
        }
        free(temp->process->args);
        free(temp->process);
    }

    free(temp);

    if (q->front == NULL) {
        q->rear = NULL;
    }

    q->numElements--;
}

int timerSetup(Queue *q){
    struct itimerspec its;
    int timerfd = 0;

    timerfd = timerfd_create(CLOCK_REALTIME, 0);
    if (timerfd == -1) {
        perror("timerSetup: timerfd_create failed");
        exit(EXIT_FAILURE);
    }

    //configure timer expiration intervals
    its.it_value.tv_sec = 1;  //first expiration after 1 second
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 1;  //recurring interval every 1 second
    its.it_interval.tv_nsec = 0;

    if (timerfd_settime(timerfd, 0, &its, NULL) == -1) {
        perror("timerSetup: timerfd_settime failed");
        exit(EXIT_FAILURE);
    }

    return timerfd;

}

void checkCompletion(Process *p){

    //if the process hasn't started or the process has already been marked as completed
    if(p == NULL || p->state == 0 || p->state == 3) return;

    int status;
    pid_t result = waitpid(p->pid, &status, WNOHANG);
    if (result > 0 && (WIFEXITED(status) || result == WIFSIGNALED(status))) { //process has completed, so log that
        p->state = 3;
        gettimeofday(&p->endTime, NULL);
    } else if (result < 0) { //error occurred
        perror("checkCompletion: waitpid encountered an error when checking status\n");
    }

}

void manageQueue(struct initData *data) {

    Process *currentProcess;

    //check if there's even anything to schedule
    if (isEmpty(data->allProcesses)){
        if(close(data->timerfd) == -1){
            perror("manageQueue: Error closing timerfd");
        } else {
            data->schedulerRunning = 0;
            gchar *message = g_strdup_printf("All processes have finished executing. Timer stopped.\n");
            g_string_append(data->logBuffer, message);
            g_free(message);

            updateStatistics(data);
            updateButtonSensitivity(data, TRUE);

            //remove the giochannel watch
            if (data->timerWatchID != 0) {
                g_source_remove(data->timerWatchID);
                data->timerWatchID = 0;
            }

        }

        return; //exit loop

    }

    currentProcess = data->allProcesses->front->process;
    checkCompletion(currentProcess);

    if (currentProcess->state != 1) { //if the process isn't running

        if (currentProcess->state == 3) { //if the process has completed
            data->processAmt += 1;
            addElapsedTime(currentProcess, &data->totalTime, data);
            deQueue(data->allProcesses);
        } else if (currentProcess->state == 0) { //if the process hasn't even started
            startProcess(currentProcess, data);
            gettimeofday(&currentProcess->startTime, NULL); //set process start time
        } else if (currentProcess->state == 2) { //if process has started but is paused
            data->contextSwitches += 1;
            continueProcess(currentProcess, data);
        }

        return; //start the loop again

    }

    //section: if the process is running

    //only apply timer-based scheduling logic if there's more than one process in the queue
    if(data->allProcesses->numElements > 1){ //stop current process and start next one
        data->contextSwitches += 1;
        stopProcess(currentProcess, data);
        deQueue(data->allProcesses); //increments front
        enQueue(data->allProcesses, currentProcess); //enqueue process at end of queue
        return;
    }

}

void addElapsedTime(Process *p, double *totalTime, initData *data) {

    long seconds = p->endTime.tv_sec - p->startTime.tv_sec;
    long microseconds = p->endTime.tv_usec - p->startTime.tv_usec;
    double processTime = (double) seconds + (double) microseconds * 1e-6;

    gchar *message = g_strdup_printf("The %s process took %.0f seconds to complete.\n", p->name, processTime);
    g_string_append(data->logBuffer, message);
    g_free(message);

    *totalTime += processTime;

}