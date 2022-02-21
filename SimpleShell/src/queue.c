/* queue.c: PQSH Queue */

#include "pqsh/macros.h"
#include "pqsh/queue.h"

#include <assert.h>

/**
 * Push process to back of queue.
 * @param q     Pointer to Queue structure.
 **/
void        queue_push(Queue *q, Process *p) {
    /* TODO: Implement */ 

    // new queue
    if(q->size == 0) {
        q->head = p;
        q->tail = p;
        q->size = 1;
    } else {

    // add to queue
    q->tail->next = p;
    q->tail = p;
    p->next = NULL;
    q->size++;
    }

}

/**
 * Pop process from front of queue.
 * @param q     Pointer to Queue structure.
 * @return  Process from front of queue.
 **/
Process *   queue_pop(Queue *q) {
    /* TODO: Implement */

    // if queue is empty
    if(q->size == 0)
        return NULL;

    Process *p = q->head;

    // if *head == *tail
    if(q->size == 1) {
        q->head = NULL;
        q->tail = NULL;
        q->size--;
        return p;
    } 

    // else
    q->head = q->head->next;
    q->size--;

    return p;
}

/**
 * Remove and return process with specified pid.
 * @param q     Pointer to Queue structure.
 * @param pid   Pid of process to return.
 * @return  Process from Queue with specified pid.
 **/
Process *   queue_remove(Queue *q, pid_t pid) {
    /* TODO: Implement */
  
    // if queue is empty
    if(!q->size)
       return NULL;

    Process *curr = q->head;
    Process *prev;

    // queue is size 1
    if(q->head == q->tail) {
        if(curr->pid == pid) {
            q->head = NULL;
            q->tail = NULL;
            q->size--;
            return curr;
        } else {
            return NULL;
        }
    }

    // queue > size 1
    while(curr != NULL) {

         // check curr pid
        if(curr->pid == pid) {

            // if curr = head, need to move head
            if(curr == q->head) {
                q->head = curr->next;
                q->size--;
                return curr;
            }
            
            // remove item
            prev->next = curr->next;
            q->size--;
            return curr;
        }

        // move pointers
        prev = curr;
        curr = curr->next;   
    }



    return NULL;
}

/**
 * Dump the contents of the Queue to the specified stream.
 * @param q     Queue structure.
 * @param fs    Output file stream.
 **/
void        queue_dump(Queue *q, FILE *fs) {
    fprintf(fs, "%6s %-30s %-13s %-13s %-13s\n", 
                "PID", "COMMAND", "ARRIVAL", "START", "END");
    /* TODO: Display information for each item in Queue. */
    if(q->size > 0) {
        Process *curr = q->head;

        while(curr != NULL) {
            fprintf(fs, "%6d %-30s %-13.2lf %-13.2lf %-13.2lf\n", curr->pid, curr->command, curr->arrival_time, curr->start_time, curr->end_time);
            curr = curr->next;      
        } 
    }
}
/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
