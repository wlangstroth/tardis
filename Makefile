CC=clang
CFLAGS=-Wall -O2 -g
LDLIBS=-lsqlite3
OBJ=main.o util.o

all: tardis

tardis: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $<

install:
	install tardis /usr/local/bin
	cp man/tardis.7 /usr/local/share/man/man1

clean:
	rm -f tardis *.o

test:
	true
