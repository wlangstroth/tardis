CC=clang
OPTIONS=-Wall -lsqlite3

all: clean tardis

tardis:
	$(CC) $(OPTIONS) tardis.c -o $@

install:
	install tardis /usr/bin

clean:
	rm -f tardis
