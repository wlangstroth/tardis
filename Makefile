OPTIONS=-Wall -lsqlite3

all: clean tardis

tardis:
	clang $(OPTIONS) tardis.c -o $@

clean:
	rm -f tardis
