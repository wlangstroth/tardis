CC=clang
CFLAGS=-Wall -O2
LDFLAGS=-lsqlite3
OBJ=main.o util.o

all: tardis

tardis: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $<

install:
	install tardis /usr/bin

clean:
	rm -f tardis *.o
