#ifndef ROBINWAITA_PROCESS_H
#define ROBINWAITA_PROCESS_H
#include <sys/time.h>
#include <gtk/gtk.h>

typedef struct initData initData;

typedef struct {
    char *name; //process name
    char **args; //any arguments we're passing to the process
    int state; //STATES || 0: not started, not running | 1: running | 2: started, paused | 3: completed
    pid_t pid; //process ID
    struct timeval startTime; //process start time
    struct timeval endTime; //process end time
} Process;

Process *newProcess(int processType); //create new process struct
void startProcess(Process *process, initData *data); //start process (fork into child and then execute python process)
void stopProcess(Process *process, initData *data); //stop process
void continueProcess(Process *process, initData *data); //continue process from stop

#endif //ROBINWAITA_PROCESS_H
