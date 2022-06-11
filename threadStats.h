#ifndef __THREADS_STATS__
#define __THREADS_STATS__

typedef struct thread_stats_t {
    unsigned int internal_id;
    unsigned int handled_count;
    unsigned int static_count;
    unsigned int dynamic_count;
} thread_stats;


#endif