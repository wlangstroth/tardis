OPTIONS=-Wall

all: clean build

build:
	clang $(OPTIONS) tardis.c -o t

clean:
	rm -f t
