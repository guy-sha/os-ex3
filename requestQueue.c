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

typedef RQStatus(*full_queue_handler_func)(requestQueue*,Node*);

struct requestQueue_t {
    list wait_queue;
    int max_queue_size;
    RQPolicy policy;
    int currently_handled_count;
    full_queue_handler_func full_queue_handler;

    /* Sync */
    pthread_mutex_t lock;
    pthread_cond_t can_add_req;
    pthread_cond_t can_take_req;
};

/* Node methods */

Node NodeCreate(req_info req) {
    Node new_node = (Node)malloc(sizeof(*new_node));
    if (new_node == NULL) {
        return NULL;
    }

    new_node->req.connfd = req.connfd;
    new_node->req.arrival_time = req.arrival_time;
    new_node->req.dispatch_interval = req.dispatch_interval;

    new_node->prev = NULL;
    new_node->next = NULL;

    return new_node;
}

// Close the connection (connfd) stored in the *node_ptr and free
void dropConnection(Node* node_ptr) {
    if (node_ptr == NULL || *node_ptr == NULL)
        return;

    Close((*node_ptr)->req.connfd);
    free(*node_ptr);
    *node_ptr = NULL;
}

/* End of Node methods */

/* List methods */
void ListInit(list* lst) {
    // lst is already allocated within requestQueue
    lst->head = NULL;
    lst->tail = NULL;
    lst->size = 0;
}

// Drop connection for each node and free (the node)
// lst was allocated was part of requestQueue malloc, and wasn't malloced itself!
void ListCleanup(list* lst) {
    Node next, curr = lst->head;
    while (curr != NULL) {
        next = curr->next;
        dropConnection(&curr);
        curr = next;
    }
    lst->head = NULL;
    lst->tail = NULL;
    lst->size = 0;
}

bool ListEmpty(list* lst) {
    if (lst->head == NULL) {
        assert(lst->size == 0 && lst->tail == NULL); // wait_queue.size should be 0 iff head is NULL iif tail is NULL
        return true;
    }

    assert(lst->size > 0 && lst->tail != NULL); // wait_queue.size should be 0 iff head is NULL iif tail is NULL
    return false;

}

void ListRemoveNode(list* lst, Node node_to_remove) {
    if (lst == NULL || node_to_remove == NULL)
        return;

    if (node_to_remove == lst->head && node_to_remove == lst->tail) {
        lst->head = NULL;
        lst->tail = NULL;
    } else if (node_to_remove == lst->head)
        lst->head = node_to_remove->next;
    else if (node_to_remove == lst->tail)
        lst->tail = node_to_remove->prev;
    else {
        node_to_remove->prev->next = node_to_remove->next;
        node_to_remove->next->prev = node_to_remove->prev;
    }

    if (lst->head != NULL)
        lst->head->prev = NULL;
    if (lst->tail !=NULL)
        lst->tail->next = NULL;

    dropConnection(&(node_to_remove));
    lst->size--;
}

/* End of List methods */

/* Full queue handlers */

// All handlers assume the lock was already acquired, and return while the lock is still held

// Block until there's room in the queue for the new request node
RQStatus doBlock(requestQueue* queue, Node* node_ptr) {
    int total_in_queue = queue->wait_queue.size + queue->currently_handled_count;
    while (total_in_queue >= queue->max_queue_size)
        pthread_cond_wait(&(queue->can_add_req), &(queue->lock)); // release lock and sleep until new nodes can be added, then wake up and acquire lock

    return RQ_SUCCESS;
}

// Drop the new request - close the connection and free the node
RQStatus doDropTail(requestQueue* queue, Node* node_ptr) {
    dropConnection(node_ptr);
    return RQ_SUCCESS;
}

int getRandomInRange(int lower, int upper) {
    return ((rand() % (upper - lower + 1))) + lower;
}

RQStatus doDropRandom(requestQueue* queue, Node* node_ptr) {
    if (ListEmpty(&(queue->wait_queue))) {
        dropConnection(node_ptr);
        return RQ_SUCCESS;
    }

    int waiting_count = queue->wait_queue.size;
    int drop_count = ceil((double)(waiting_count + queue->currently_handled_count) * 0.3);

    if (drop_count >= waiting_count) {
        ListCleanup(&(queue->wait_queue));
        return RQ_SUCCESS;
    }

    // not all requests are dropped, need to choose randomly
    bool* queue_entries = (bool*)malloc(sizeof(*queue_entries) * waiting_count);
    for (int i=0; i<waiting_count; i++)
        queue_entries[i] = false;
    while (drop_count > 0) {
        int random_index = getRandomInRange(0, waiting_count-1);
        if (queue_entries[random_index] == false) {
            queue_entries[random_index] = true;
            drop_count--;
        }
    }

    Node curr = queue->wait_queue.head;
    for (int idx=0; idx<waiting_count && curr != NULL; idx++) {
        Node next = curr->next;
        if (queue_entries[idx] == true)
            ListRemoveNode(&(queue->wait_queue), curr);
        curr = next;
    }

    free(queue_entries);
    return RQ_SUCCESS;
}

// Drop the oldest request in the queue by closing its connection and freeing the head node
RQStatus doDropHead(requestQueue* queue, Node* node_ptr) {
    if (ListEmpty(&(queue->wait_queue))) {
        dropConnection(node_ptr);
        return RQ_SUCCESS;
    }

    ListRemoveNode(&(queue->wait_queue), queue->wait_queue.head);
    return RQ_SUCCESS;
}


/* End of full queue handlers */

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

    if (policy == RQ_DROP_TAIL)
        queue->full_queue_handler = doDropTail;
    else if (policy == RQ_DROP_RANDOM)
        queue->full_queue_handler = doDropRandom;
    else if (policy == RQ_DROP_HEAD)
        queue->full_queue_handler = doDropHead;
    else
        queue->full_queue_handler = doBlock;

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


RQStatus RQInsertRequest(requestQueue* queue, req_info request) {
    Node req_node = NodeCreate(request); // check if NULL?

    pthread_mutex_lock(&(queue->lock));
    int total_in_queue = queue->wait_queue.size + queue->currently_handled_count;
    if (total_in_queue >= queue->max_queue_size)
        queue->full_queue_handler(queue, &req_node);

    // req_node will be NULL if the current request was dropped.
    // This will happen if the policy is RQ_DROP_TAIL and the list is full,
    // or the policy RQ_DROP_HEAD or RQ_DROP_RANDOM and there are no waiting requests.
    // in that case, the handler frees the new node and closes the connection (connfd)
    if (req_node != NULL) {
        if (ListEmpty(&(queue->wait_queue))) {
            queue->wait_queue.head = req_node;
        } else {
            queue->wait_queue.tail->next = req_node;
            req_node->prev = queue->wait_queue.tail;
        }
        queue->wait_queue.tail = req_node;
        queue->wait_queue.size++;

        pthread_cond_signal(&(queue->can_take_req));
    }

    pthread_mutex_unlock(&(queue->lock));
    return RQ_SUCCESS;
}


/* End of requestQueue methods */