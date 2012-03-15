COMPILER=clang
OPTIONS=-Wall

all: clean build

build:
	$(COMPILER) $(OPTIONS) tardis.c -o tardis

clean:
	rm -f *.o tardis
