ifeq ($(OS),Windows_NT)
	CC = gcc
endif

libadder.so: adder.o
	$(CC) -o libadder.so adder.o -shared

adder.o: adder.c

clean:
	rm -f *.o *.so
