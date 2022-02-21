/* pqsh.c: Process Queue Shell */

#include "pqsh/macros.h"
#include "pqsh/options.h"
#include "pqsh/scheduler.h"
#include "pqsh/signal.h"

#include <errno.h>
#include <string.h>
#include <sys/time.h>

/* Global Variables */

Scheduler PQShellScheduler = {
    .policy    = FIFO_POLICY,
    .cores     = 1,
    .timeout   = 250000,
};

/* Help Message */

void help() {
    printf("Commands:\n");
    printf("  add    command    Add command to waiting queue.\n");
    printf("  status [queue]    Display status of specified queue (default is all).\n");
    printf("  help              Display help message.\n");
    printf("  exit|quit         Exit shell.\n");
}

/* Main Execution */

int main(int argc, char *argv[]) {
    Scheduler *s = &PQShellScheduler;

    /* TODO: Parse command line options */
    if (parse_command_line_options(argc, argv, s) == 0){
        help();
        return -1;
    }

    /* TODO Register Signal handlers */
    if (!signal_register(SIGALRM, 0, sigalrm_handler)){
        help();
         return -1;
    }
                                    
    /* TODO: Start timer interrupt */
    struct itimerval interval = {
        .it_interval = { .tv_sec = 0, .tv_usec = s->timeout },
        .it_value    = { .tv_sec = 0, .tv_usec = s->timeout },
    };

    if (setitimer(ITIMER_REAL, &interval, NULL) < 0) {
        help();
       return -1; 
    }


    /* TODO: Process shell comands */
    while (!feof(stdin)) {
        char command[BUFSIZ]  = "";
        int queue = 0;

        printf("\nPQSH> ");

        while (!fgets(command, BUFSIZ, stdin) && !feof(stdin));

        chomp(command);
        
        /* TODO: Handle add and status commands */

        char *comm = strtok(command, " ");
        char *arg = strtok(NULL, "\n");

        if(!comm)
            continue;
 

        if (streq(comm, "help")) {
            help();
        } else if (streq(comm, "exit") || streq(comm, "quit")) {
                break;
        } else if (streq(comm, "add")) {
            scheduler_add(&(PQShellScheduler), stdout, arg);
        } else if (streq(comm, "status")) {
            if(arg != NULL) {
                if(streq(arg, "running"))
                    queue = RUNNING;
                else if(streq(arg, "waiting"))
                    queue = WAITING;
                else if(streq(arg, "finished"))
                    queue = FINISHED;
            }

            scheduler_status(&(PQShellScheduler), stdout, queue);          
        } else if (strlen(command)) {
            printf("Unknown command: %s\n", comm);
        }

    }

        // free memory allocated by process create
    Process *curr = s->finished.head;
    Process *tmp;
    while(curr) {
        tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
