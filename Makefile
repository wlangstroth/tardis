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
	install tardis /usr/local/bin
	cp man/tardis.1 /usr/local/share/man/man1

clean:
	rm -f tardis *.o
