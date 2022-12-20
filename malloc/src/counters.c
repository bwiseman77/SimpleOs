/* counters.c: Counters */

#include "malloc/block.h"
#include "malloc/counters.h"
#include "malloc/freelist.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Global Variables */

extern Block FreeList;
size_t Counters[NCOUNTERS] = {0};
int    DumpFD              = -1;

/* Functions */

/**
 * Initialize counters by doing the following:
 *
 *  1. Register the dump_counters function to run when the program terminates.
 *  2. Duplicate standard output file descriptor to the DumpFD global variable.
 *
 * Note, these actions should only be performed once regardless of how many
 * times the function is called.
 **/
void init_counters() {
    static bool initialized = false;

    if (!initialized) {
        assert(atexit(dump_counters) == 0);
        initialized = true;
        DumpFD      = dup(STDOUT_FILENO);
        assert(DumpFD >= 0);
    }
}

/**
 * Compute internal fragmentation in heap using the formula:
 *
 *  FRAGMENTATION = Sum(internal fragments) / HeapSize * 100.0
 *
 * @return  Percentage of internal fragmentation in heap.
 **/
double  internal_fragmentation() {
    // TODO: Implement internal fragmentation computation
   
    // loop through and cal diff of cap and size    
    double int_frag = 0;
    for (Block *curr = FreeList.next; curr != &FreeList; curr = curr->next){
        int_frag += curr->capacity - curr->size;
    }

    return Counters[HEAP_SIZE] ? (int_frag / Counters[HEAP_SIZE]) * 100.0 : 0.0;
}

/**
 * Compute external fragmentation in heap using the formula:
 *
 *  FRAGMENTATION = (1 - (LARGEST_FREE_BLOCK / ALL_FREE_MEMORY)) * 100.0
 *
 * https://www.edn.com/design/systems-design/4333346/Handling-memory-fragmentation
 *
 * @return  Percentage of external fragmentation in heap.
 **/
double  external_fragmentation() {
    // TODO: Implement external fragmentation computation
    
    double max_free = 0;
    double total_free = 0;

    // loop through and find max / keep track of total
    for (Block *curr = FreeList.next; curr != &FreeList; curr = curr->next){
        if (curr->capacity > max_free)
            max_free = curr->capacity;

        total_free += curr->capacity;
    }

    return total_free ? (1 - (max_free / total_free)) * 100.0 : 0.0;
}

/**
 * Display all counters to the DumpFD global file descriptor saved in
 * init_counters.
 *
 * Note, the function should close the DumpFD global file descriptor at the end
 * of the function.
 **/
void dump_counters() {
    char buffer[BUFSIZ];
    assert(DumpFD >= 0);

    fdprintf(DumpFD, buffer, "blocks:      %lu\n"   , Counters[BLOCKS]);
    fdprintf(DumpFD, buffer, "free blocks: %lu\n"   , free_list_length());
    fdprintf(DumpFD, buffer, "mallocs:     %lu\n"   , Counters[MALLOCS]);
    fdprintf(DumpFD, buffer, "frees:       %lu\n"   , Counters[FREES]);
    fdprintf(DumpFD, buffer, "callocs:     %lu\n"   , Counters[CALLOCS]);
    fdprintf(DumpFD, buffer, "reallocs:    %lu\n"   , Counters[REALLOCS]);
    fdprintf(DumpFD, buffer, "reuses:      %lu\n"   , Counters[REUSES]);
    fdprintf(DumpFD, buffer, "grows:       %lu\n"   , Counters[GROWS]);
    fdprintf(DumpFD, buffer, "shrinks:     %lu\n"   , Counters[SHRINKS]);
    fdprintf(DumpFD, buffer, "splits:      %lu\n"   , Counters[SPLITS]);
    fdprintf(DumpFD, buffer, "merges:      %lu\n"   , Counters[MERGES]);
    fdprintf(DumpFD, buffer, "requested:   %lu\n"   , Counters[REQUESTED]);
    fdprintf(DumpFD, buffer, "heap size:   %lu\n"   , Counters[HEAP_SIZE]);
    fdprintf(DumpFD, buffer, "internal:    %4.2lf\n", internal_fragmentation());
    fdprintf(DumpFD, buffer, "external:    %4.2lf\n", external_fragmentation());

    close(DumpFD);
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
