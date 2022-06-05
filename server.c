#include "segel.h"
#include "request.h"

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


int main(int argc, char *argv[])
{
    int port, threads, max_queue_size;
    RQPolicy policy;
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, &threads, &max_queue_size, &policy, argc, argv);
    requestQueue* queue = RQInit(max_queue_size, policy);

    // 
    // HW3: Create some threads...
    //

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	requestHandle(connfd);

	Close(connfd);
    }

}


    


 
