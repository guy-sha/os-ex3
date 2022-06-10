#include "segel.h"
#include "request.h"
#include "requestQueue.h"

typedef struct thread_stats_t {
    unsigned int internal_id;
    unsigned int handled_count;
    unsigned int static_count;
    unsigned int dynamic_count;
} thread_stats;

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
    thread_stats stats = { .internal_id=id, .dynamic_count=0, .handled_count=0, .static_count=0 };

    req_info request;

    while(1) {
        // get request from queue
        // handle request + statistics
        // close request

        RQTakeRequest(queue, &request);
        requestHandle(request.connfd); // maybe give statistics as an arg, also maybe check if null
        Close(request.connfd);
        RQNotifyDone(queue);
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

    struct thread_args* args = malloc(sizeof(*args)*threads);
    for( int i=0; i<threads; i++ ) {
        args[i].queue = queue;
        args[i].internal_id = i;
        Pthread_create(&(args[i].thread_id), NULL, worker_thread, (void*)(args+i));
    }

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        gettimeofday(&arrival_time, NULL);
        req_info req = { .arrival_time=arrival_time, .connfd=connfd, .dispatch_interval=dispatch_interval };
        RQInsertRequest(queue, req);
    }
}


    


 
