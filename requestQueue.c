#include <stdbool.h>
#include <assert.h>
#include "requestQueue.h"

struct node_t {
    req_info req;
    struct node_t* prev;
    struct node_t* next;
};
typedef struct node_t* Node;

typedef struct list_t {
    Node head;  // oldest request in queue
    Node tail;  // newest request in queue
    int size;
} list;

struct requestQueue_t {
    list wait_queue;
    int max_queue_size;
    RQPolicy policy;
    int currently_handled_count;

    /* Sync */
    pthread_mutex_t lock;
    pthread_cond_t can_add_req;
    pthread_cond_t can_take_req;
};

/* List methods */
void ListInit(list* lst) {
    // lst is already allocated within requestQueue
    lst->head = NULL;
    lst->tail = NULL;
    lst->size = 0;
}

void ListCleanup(list* lst) {
    // lst was allocated was part of requestQueue malloc, and wasn't malloced itself!
    Node next, curr = lst->head;
    while (curr != NULL) {
        next = curr->next;
        free(curr);
        curr = next;
    }
    lst->size = 0;
}
/* End of List methods */

/* requestQueue methods */
requestQueue* RQInit(int max_queue_size, RQPolicy policy) {
    requestQueue* queue = (requestQueue*)malloc(sizeof(*queue));
    if (queue == NULL) {
        return NULL;
    }
    ListInit(&(queue->wait_queue));
    queue->max_queue_size = max_queue_size;
    queue->policy = policy;
    queue->currently_handled_count = 0;

    pthread_mutex_init(&(queue->lock), NULL);
    pthread_cond_init(&(queue->can_add_req), NULL);
    pthread_cond_init(&(queue->can_take_req), NULL);

    return queue;
}

void RQDestroy(requestQueue* queue) {
    ListCleanup(&(queue->wait_queue));

    // these might fail, what should we do?
    // TODO: check with segel when should we clean up, and what to do upon failures
    pthread_mutex_destroy(&(queue->lock));
    pthread_cond_destroy(&(queue->can_add_req));
    pthread_cond_destroy(&(queue->can_take_req));

    free(queue);
}

/* End of requestQueue methods */