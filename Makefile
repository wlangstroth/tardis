#CC=cc
CC = clang
CFLAGS = -Wall
OBJ = main.o util.o

all: tardis

tardis: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) -lsqlite3

%.o : %.c
	$(CC) $(CFLAGS) -c $<

install:
	install tardis /usr/bin

clean:
	rm -f tardis *.o
