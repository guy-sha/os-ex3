#
# To compile, type "make" or make "all"
# To remove files, type "make clean"
#
OBJS = server.o request.o requestQueue.o segel.o client.o
TARGET = server

CC = gcc
CFLAGS = -g -Wall

# Debug flag - setting NDEBUG flag by default, so that debugging features are turned off
# When the make rule is "debug" sets DEBUG flag instead so that debugging (including asserts) is on
DBGFLAG = -DNDEBUG

LIBS = -lm -lpthread

.SUFFIXES: .c .o 

all: server client output.cgi
	-mkdir -p public
	-cp output.cgi favicon.ico home.html public

debug: DBGFLAG = -DDEBUG
debug: all

server: server.o request.o segel.o requestQueue.o
	$(CC) $(CFLAGS) $(DBGFLAG) -o server server.o request.o segel.o requestQueue.o $(LIBS)

client: client.o segel.o
	$(CC) $(CFLAGS) $(DBGFLAG) -o client client.o segel.o $(LIBS)

output.cgi: output.c
	$(CC) $(CFLAGS) $(DBGFLAG) -o output.cgi output.c

.c.o:
	$(CC) $(CFLAGS) $(DBGFLAG) -o $@ -c $<

clean:
	-rm -f $(OBJS) server client output.cgi
	-rm -rf public
