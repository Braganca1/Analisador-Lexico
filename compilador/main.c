/* ======================================================================
 * main.c
 * Uso: sslc [--tokens | --tabela | --gramatica] arquivo.ssl
 *   (sem opção)  analisa léxica, sintática, escopo e tipos
 *   --tokens     lista de tokens no formato da VC
 *   --tabela     estatísticas e conflitos da tabela la-LR(1)
 *   --gramatica  regras numeradas efetivamente usadas
 * ====================================================================== */
#include <stdio.h>
#include <string.h>
#include "grammar.h"
#include "lalr.h"
#include "semantics.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    const char *opt = (argc > 1 && argv[1][0] == '-') ? argv[1] : NULL;
    const char *path = argv[argc - 1];
    t_result r;

    InitCompiler();

    if (opt && strcmp(opt, "--tabela") == 0) { PrintTableReport(stdout); return 0; }
    if (opt && strcmp(opt, "--gramatica") == 0) {
        int i;
        for (i = 0; i < NUM_RULES; i++) printf("%3d  %s\n", i, RuleText(i));
        return 0;
    }
    if (argc < 2 || (opt && argc < 3)) {
        fprintf(stderr, "uso: %s [--tokens | --tabela | --gramatica] arquivo.ssl\n", argv[0]);
        return 2;
    }
    if (opt && strcmp(opt, "--tokens") == 0) return PrintTokens(path, stdout) ? 0 : 2;
    if (opt) { fprintf(stderr, "opcao desconhecida: %s\n", opt); return 2; }

    errOut = stdout;
    r = Compile(path);
    switch (r) {
    case COMPILE_OK:           printf("%s: compilado sem erros\n", path); break;
    case COMPILE_LEX_ERROR:    printf("%s: %d erro(s) lexico(s); analise sintatica nao realizada\n",
                                      path, nLexErrors); break;
    case COMPILE_SYNTAX_ERROR: printf("%s: erro sintatico\n", path); break;
    case COMPILE_SEM_ERROR:    printf("%s: %d erro(s) de escopo/tipo\n", path, nSemErrors); break;
    case COMPILE_IO_ERROR:     break;
    }
    return r == COMPILE_OK ? 0 : 1;
}
