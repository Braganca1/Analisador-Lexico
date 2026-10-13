CC     = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g
OBJS   = lexer.o grammar.o lalr.o symtab.o semantics.o parser.o

all: sslc testes

sslc: main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ main.o $(OBJS)

testes: testes.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ testes.o $(OBJS)

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $<

# testes da VF + programa da VC + modo --tokens igual a saída da VC
test: all
	./testes
	$(MAKE) -s -C ../analisador
	cd ../analisador && ./lexer_test entrada.txt > /dev/null 2>&1; echo "VC: lexer_test executado (codigo $$?)"
	../analisador/lexer_test exemplos/tokens.txt > vc.out 2>&1
	./sslc --tokens exemplos/tokens.txt > vf.out 2>&1
	diff vc.out vf.out && echo "--tokens: saida identica a da VC"
	rm -f vc.out vf.out

clean:
	rm -f *.o sslc testes vc.out vf.out
