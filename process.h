#ifndef ROBINWAITA_PROCESS_H
#define ROBINWAITA_PROCESS_H
#include <sys/time.h>

//callbacks for log and timer (for some separation of UI and process management)
typedef void (*logCallback)(const char *message, void *user_data);
typedef void (*timerCallback)(unsigned int timer_id, void *user_data);

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
void setLogCallback(struct initData *data, logCallback callback, void *userData); //set callback function for logs
void setTimerCallback(struct initData *data, timerCallback callback, void *userData); //set callback function for timer info

#endif //ROBINWAITA_PROCESS_H
