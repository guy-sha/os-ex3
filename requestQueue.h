#ifndef OS_EX3_REQUESTQUEUE_H
#define OS_EX3_REQUESTQUEUE_H

#include "segel.h"

typedef enum RQStatus_t {RQ_SUCCESS, RQ_FAILURE} RQStatus;
typedef enum RQPolicy_t {RQ_BLOCK, RQ_DROP_TAIL, RQ_DROP_RANDOM, RQ_DROP_HEAD} RQPolicy;

typedef struct req_info_t {
    int connfd;
    struct timeval arrival_time;
    struct timeval dispatch_interval; // (now - arrival_time) when this request goes out of the queue
} req_info;

typedef struct requestQueue_t requestQueue;

requestQueue* RQInit(int max_queue_size, RQPolicy policy);
void RQDestroy(requestQueue* queue);

RQStatus RQInsertRequest(requestQueue* queue, req_info request);
RQStatus RQTakeRequest(requestQueue* queue, req_info* request_ptr);
RQStatus RQNotifyDone(requestQueue* queue);

#endif //OS_EX3_REQUESTQUEUE_H
