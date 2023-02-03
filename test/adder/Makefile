ifeq ($(OS),Windows_NT)
	CC = gcc
endif

all: adder.wasm libadder.so

libadder.so: adder.o
	$(CC) -o libadder.so adder.o -shared

adder.o: adder.c

adder.wasm: adder.wat
	npx -p wabt wat2wasm adder.wat -o adder.wasm

clean:
	rm -f *.o *.s *.so *.wasm
