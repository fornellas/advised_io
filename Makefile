all: advised_io.so

advised_io.so: advised_io.c
	gcc -O2 -Wall -shared -fPIC -ldl advised_io.c -o advised_io.so