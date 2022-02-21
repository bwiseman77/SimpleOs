/* process.c: PQSH Process */

#include "pqsh/macros.h"
#include "pqsh/process.h"
#include "pqsh/timestamp.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

//struct Process {
//    char    command[BUFSIZ];    /* Command to execute */
//    pid_t   pid;                /* Process identifier (0 == invalid) */
//    double  arrival_time;       /* Process arrival time (is placed into waiting queue) */
//    double  start_time;         /* Process start time (is first placed into running queue) */
//    double  end_time;           /* Process end time (is placed into finished queue) */
//
//  Process *next;              /* Pointer to next process */
//};


/**
 * Create new process structure given command.
 * @param   command     String with command to execute.
 * @return  Pointer to new process structure
 **/
Process *process_create(const char *command) {
    /* TODO: Implement */
    Process *n = malloc(sizeof(Process));
    
    // if allocation fails
    if (!n){
        return NULL;
    }

    // initalize values
    strcpy(n->command, command);
    n->arrival_time = timestamp();
    n->start_time = 0;
    n->end_time = 0;
    n->pid = 0;
    n->next =  NULL;
    
    return n;
}

/**
 * Start process by forking and executing the command.
 * @param   p           Pointer to Process structure.
 * @return  Whether or not starting the process was successful
 **/
bool process_start(Process *p) {
    /* TODO: Implement */
    p->pid = fork();

    // if allocation fails
    if (p->pid < 0){
        return false;
    }

    if (p->pid == 0){ // for child
        char *argv[MAX_ARGUMENTS] = {0};
        int i = 0;
        for (char *token = strtok(p->command, " "); token; token=strtok(NULL, " ")){
            argv[i++] = token;
        }

       execvp(argv[0], argv);
    }

    p->start_time = timestamp(); 

    return true;
}

/**
 * Pause process by sending it the appropriate signal.
 * @param   p           Pointer to Process structure.
 * @return  Whether or not sending the signal was successful.
 **/
bool process_pause(Process *p) {
    /* TODO: Implement */
    int response;
    response = kill(p->pid, SIGSTOP);
    return !response;
}

/**
 * Resume process by sending it the appropriate signal.
 * @param   p           Pointer to Process structure.
 * @return  Whether or not sending the signal was successful.
 **/
bool process_resume(Process *p) {
    /* TODO: Implement */ 
    int response;
    response = kill(p->pid, SIGCONT);
    return !response;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
