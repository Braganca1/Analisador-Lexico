CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

all: lexer_test

lexer_test: main.o lexer.o
	$(CC) $(CFLAGS) -o lexer_test main.o lexer.o

main.o: main.c lexer.h
	$(CC) $(CFLAGS) -c main.c

lexer.o: lexer.c lexer.h
	$(CC) $(CFLAGS) -c lexer.c

clean:
	rm -f *.o lexer_test

run: lexer_test
	./lexer_test entrada.txt
