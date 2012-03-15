OPTIONS=-Wall

all: clean build

build:
	clang $(OPTIONS) tardis.c -o tardis

clean:
	rm -f tardis
