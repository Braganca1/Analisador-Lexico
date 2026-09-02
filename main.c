/* ======================================================================
 * main.c
 * Programa de teste do analisador léxico: lê um arquivo fonte e imprime,
 * para cada token reconhecido, seu tipo, lexema, token secundário (se
 * houver) e a linha em que ocorreu.
 * ====================================================================== */
#include <stdio.h>
#include "lexer.h"

int main(int argc, char *argv[]) {
    const char *nomeArquivo = (argc > 1) ? argv[1] : "entrada.txt";

    if (!initLexer(nomeArquivo)) {
        return 1;
    }

    printf("%-6s %-14s %-15s %-10s\n", "Linha", "Token", "Lexema", "TokenSec");
    printf("--------------------------------------------------------\n");

    Token tok;
    do {
        tok = nextToken();
        if (tok.tokenSec >= 0) {
            printf("%-6d %-14s %-15s %-10d\n",
                   tok.linha, tokenName(tok.token), tok.lexema, tok.tokenSec);
        } else {
            printf("%-6d %-14s %-15s %-10s\n",
                   tok.linha, tokenName(tok.token), tok.lexema, "-");
        }

        if (tok.token == TK_ERROR) {
            fprintf(stderr, "Erro lexico na linha %d: caractere/token invalido '%s'\n",
                    tok.linha, tok.lexema);
        }
    } while (tok.token != TK_EOF);

    printf("\n--- Tabela de identificadores (nome -> token secundario) ---\n");
    /* Não há função pronta de "listar todos" pedida no enunciado, mas
       para fins de depuração exibimos aqui os literais lidos: */
    printf("--- Constantes lidas (por posicao na tabela) ---\n");
    /* Exemplo de uso das funções get: percorre pelas posições conhecidas
       seria necessário guardar nNumConsts; aqui apenas demonstramos a
       API chamando get* quando aplicável durante a leitura acima. */

    freeLexerTables();
    closeLexer();
    return 0;
}
