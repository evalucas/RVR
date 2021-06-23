CC=g++
CFLAGS=-g -I.
DEPS = Socket.h 4T.h
OBJ = Socket.o 4T.o
LIBS=-lpthread

%.o: %.cc $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

all: 4ts 4tc

4ts: $(OBJ) 4TServer.o
	g++ -o $@ $^ $(CFLAGS) $(LIBS)

4tc: $(OBJ) 4TClient.o
	g++ -o $@ $^ $(CFLAGS) $(LIBS)


.PHONY: clean

clean:
	rm -f *.o 4ts 4tc

