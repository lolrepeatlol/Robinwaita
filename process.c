#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib-unix.h>
#include "process.h"
#include "main.h"

Process* newProcess(int processType){

    Process* createdProcess = (Process*) malloc(sizeof(Process));

    if(createdProcess == NULL){
        perror("newProcess: Failed to create process struct");
        return NULL;
    }

    char newName[16];
    char testProcess[35];

    //init integer values
    createdProcess->state = 0;
    createdProcess->pid = 0;

    //set process information
    if(processType == 1) {
        strcpy(newName, "CPUBound");
        strcpy(testProcess, "../testing_processes/cpubound.py");
    } else if(processType == 2) {
        strcpy(newName, "IOBound");
        strcpy(testProcess, "../testing_processes/iobound.py");
    } else if(processType == 3) {
        strcpy(newName, "MixedBound");
        strcpy(testProcess, "../testing_processes/mixbound.py");
    }

    //allocate memory for name
    createdProcess->name = (char*) malloc(sizeof(char) * (strlen(newName)+1));
    if (createdProcess->name == NULL) {
        perror("newProcess: Failed to allocate memory for process struct name");
        free(createdProcess);
        return NULL;
    }
    strcpy(createdProcess->name, newName);

    //allocate memory for the arguments array
    createdProcess->args = (char**) malloc(3 * sizeof(char*)); //allocate python3, path, NULL terminator, and amount of addl arguments
    if (createdProcess->args == NULL) {
        perror("newProcess: Failed to allocate memory for process struct arguments");
        free(createdProcess->name);
        free(createdProcess);
        return NULL;
    }

    //allocate and set the python argument
    createdProcess->args[0] = (char*) malloc(strlen("python3")+1);
    if (createdProcess->args[0] == NULL) {
        perror("newProcess: Failed to allocate memory for python3 argument");
        free(createdProcess->args);
        free(createdProcess->name);
        free(createdProcess);
        return NULL;
    }
    strcpy(createdProcess->args[0], "python3");

    //allocate and set the name of the script
    createdProcess->args[1] = (char*) malloc(strlen(testProcess)+1);
    if (createdProcess->args[1] == NULL) {
        perror("newProcess: Failed to allocate memory for script argument");
        for (int i = 0; createdProcess->args[i] != NULL; i++) {
            free(createdProcess->args[i]);
        }
        free(createdProcess->args);
        free(createdProcess->name);
        free(createdProcess);
        return NULL;
    }
    strcpy(createdProcess->args[1], testProcess);

    //null-terminate arguments array
    createdProcess->args[2] = NULL;

    return createdProcess;
}

void startProcess(Process *process, initData *data) {
    pid_t pid;
    pid = fork(); //fork process

    if (pid == 0) { //child process
        execvp(process->args[0], process->args);

        //will only run if execvp fails
        perror("startProcess: execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { //parent process
        process->pid = pid;
        process->state = 1;

        //append message for gtktextview
        gchar *message = g_strdup_printf("Started process %d (%s)\n", pid, process->name);
        g_string_append(data->logBuffer, message);
        g_free(message);
    } else { //fork failed
        perror("startProcess: fork failed");
    }
}

void stopProcess(Process *process, initData *data) {
    int status;
    pid_t result = waitpid(process->pid, &status, WNOHANG);

    if (result == 0) {
        //process is still running, safe to send SIGSTOP
        if (kill(process->pid, SIGSTOP) == -1) {
            perror("Failed to stop process");
            exit(EXIT_FAILURE);
        } else {
            process->state = 2;

            gchar *message = g_strdup_printf("Stopped process %d (%s)\n", process->pid, process->name);
            g_string_append(data->logBuffer, message);
            g_free(message);
        }
    } else if (result > 0) {
        //process has already finished
        process->state = 3;
        fprintf(stderr, "Program %d has already completed, cannot stop\n", process->pid);
    } else {
        //an error occurred in waitpid
        perror("stopProcess: waitpid failed");
        exit(EXIT_FAILURE);
    }
}

void continueProcess(Process *process, initData *data) {
    if (kill(process->pid, SIGCONT) == -1) { //resume process by sending SIGSTOP signal
        fprintf(stderr, "Program %d failed to resume\n", process->pid); //print error to stderr
        exit(EXIT_FAILURE);
    } else {
        gchar *message = g_strdup_printf("Continued process %d (%s)\n", process->pid, process->name);
        g_string_append(data->logBuffer, message);
        g_free(message);

        process->state = 1;
    }
}