#include "segel.h"
#include "request.h"
#include "requestQueue.h"
#include "threadStats.h"

#include "common.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

RQPolicy strToPolicy(char* str) {
    if (strcmp(str, "block") == 0)
        return RQ_BLOCK;
    if (strcmp(str, "dt") == 0)
        return RQ_DROP_TAIL;
    if (strcmp(str, "dh") == 0)
        return RQ_DROP_HEAD;
    if (strcmp(str, "random") == 0)
        return RQ_DROP_RANDOM;

    return RQ_BLOCK;
}

// HW3: Parse the new arguments too
void getargs(int* port, int* threads, int* max_queue_size, RQPolicy* policy, int argc, char* argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    // consider enforcing port number to be > 1023
    *port = atoi(argv[1]);
    long temp = strtol(argv[2], NULL, 10);
    if (temp <= 0) {
        fprintf(stderr, "Invalid arg: threads == %s\n", argv[2]);
        exit(1);
    }
    *threads = (int)temp;

    temp = strtol(argv[3], NULL, 10);
    if (temp <= 0) {
        fprintf(stderr, "Invalid arg: max_queue_size == %s\n", argv[3]);
        exit(1);
    }
    *max_queue_size = (int)temp;
    *policy = strToPolicy(argv[4]);

}

struct thread_args {
    requestQueue* queue;
    int internal_id;
    pthread_t thread_id;
};

void* worker_thread(void* arg) {
    int id = ((struct thread_args*)arg)->internal_id;
    requestQueue* queue = ((struct thread_args*)arg)->queue;
    thread_stats stats = { .internal_id=id, .handled_count=0, .dynamic_count=0, .static_count=0 };

    req_info request;
    struct timeval dispatch_time;

    debug_print("Thread number %d starting to work\n", id);
    while(1) {
        // get request from queue
        // handle request + statistics
        // close request

        RQTakeRequest(queue, &request);

        gettimeofday(&dispatch_time, NULL);
        timersub(&dispatch_time, &(request.arrival_time), &(request.dispatch_interval));

        debug_print("Thread %d:\tFETCHED request %d on fd %d\n", id, request.req_id, request.connfd);
        requestHandle(request, &stats);
        debug_print("Thread %d:\tFINISHED request %d on fd %d\n", id, request.req_id, request.connfd);
        Close(request.connfd);
        RQNotifyDone(queue);
        debug_print("Thread %d:\tNOTIFIED about finishing request %d on fd %d\n", id, request.req_id, request.connfd);
    }
}

int main(int argc, char *argv[])
{
    int port, threads, max_queue_size;
    RQPolicy policy;
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;
    struct timeval arrival_time, dispatch_interval = { .tv_sec=-1, .tv_usec=-1 };

    getargs(&port, &threads, &max_queue_size, &policy, argc, argv);
    requestQueue* queue = RQInit(max_queue_size, policy);

    struct thread_args* args = Malloc(sizeof(*args)*threads);
    for( int i=0; i<threads; i++ ) {
        args[i].queue = queue;
        args[i].internal_id = i;
        Pthread_create(&(args[i].thread_id), NULL, worker_thread, (void*)(args+i));
    }
    int request_count = 0;
    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        request_count++;
        debug_print("Server:\t\tACCEPTED request %d on fd %d from %d on port %d\n", request_count, connfd, clientaddr.sin_addr.s_addr, clientaddr.sin_port);
        gettimeofday(&arrival_time, NULL);
        req_info req = { .arrival_time=arrival_time, .connfd=connfd, .dispatch_interval=dispatch_interval, .req_id=request_count };
        RQInsertRequest(queue, req);
        debug_print("Server:\t\tINSERTED request %d to queue\n", request_count);
    }
}


    


 
