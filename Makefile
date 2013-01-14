CC=clang
CFLAGS=-Wall -O2 -g
LDLIBS=-lsqlite3
OBJ=main.o util.o

all: tardis

tardis: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $<

local:
	install tardis /usr/local/bin
	cp man/tardis.7 /usr/local/share/man/man7

clean:
	rm -f tardis *.o

install: tardis local clean

test:
	true
