OPTIONS=-Wall -lsqlite3

all: clean t

t:
	clang $(OPTIONS) tardis.c -o $@

clean:
	rm -f t
