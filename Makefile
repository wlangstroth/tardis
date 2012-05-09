#CC=cc
CC=clang
CFLAGS=-Wall
LDFLAGS=-lsqlite3
OBJ=main.o util.o

all: tardis

tardis: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o : src/%.c
	$(CC) $(CFLAGS) -c $<

install:
	install tardis /usr/local/bin

clean:
	rm -f tardis *.o
