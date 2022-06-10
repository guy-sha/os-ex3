#
# To compile, type "make" or make "all"
# To remove files, type "make clean"
#
OBJS = server.o request.o requestQueue.o segel.o client.o
TARGET = server

CC = gcc
CFLAGS = -g -Wall

LIBS = -lm -lpthread

.SUFFIXES: .c .o 

all: server client output.cgi
	-mkdir -p public
	-cp output.cgi favicon.ico home.html public

server: server.o request.o segel.o requestQueue.o
	$(CC) $(CFLAGS) -o server server.o request.o segel.o requestQueue.o $(LIBS)

client: client.o segel.o
	$(CC) $(CFLAGS) -o client client.o segel.o $(LIBS)

output.cgi: output.c
	$(CC) $(CFLAGS) -o output.cgi output.c

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	-rm -f $(OBJS) server client output.cgi
	-rm -rf public
