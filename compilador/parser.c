/* ======================================================================
 * parser.c
 * 1) O arquivo inteiro é lido pelo analisador léxico da VC; havendo
 *    erro léxico, a análise sintática não é iniciada.
 * 2) Análise ascendente dirigida pela Tabela de Ação (figura 4.6), com
 *    Semantics(r) a cada redução. O primeiro erro sintático encerra a
 *    análise; os erros de escopo e de tipo são acumulados.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "grammar.h"
#include "lalr.h"
#include "semantics.h"
#include "parser.h"

int nLexErrors;

static Token *tokens;
static int    nTokens, capTokens;

void InitCompiler(void) {
    static int done = 0;
    if (done) return;
    InitGrammar();
    BuildLALRTable();
    done = 1;
}

/* Lê todos os tokens; devolve 0 se o arquivo não pôde ser aberto */
static int ReadTokens(const char *path) {
    Token tok;
    nTokens = 0;
    nLexErrors = 0;
    if (!initLexer(path)) return 0;
    do {
        tok = nextToken();
        if (nTokens == capTokens) {
            capTokens = capTokens ? 2 * capTokens : 256;
            tokens = (Token *)realloc(tokens, (size_t)capTokens * sizeof(Token));
            if (tokens == NULL) { fprintf(stderr, "Erro: memoria insuficiente\n"); exit(2); }
        }
        tokens[nTokens++] = tok;
        if (tok.token == TK_ID) SetName(tok.tokenSec, tok.lexema);
        if (tok.token == TK_ERROR) {
            nLexErrors++;
            if (errOut)
                fprintf(errOut, "Erro lexico na linha %d, coluna %d: "
                        "caractere/token invalido '%s'\n",
                        tok.linha, tok.coluna, tok.lexema);
        }
    } while (tok.token != TK_EOF);
    closeLexer();
    return 1;
}

/* Simula, sem alterar a pilha, as reduções que o terminal t provocaria e
 * diz se ele acaba sendo empilhado. Como na la-LR(1) estados fundidos
 * podem reduzir antes de detectar um erro, essa verificação é feita
 * para cada novo token: o erro é acusado com a pilha ainda intacta e a
 * lista de esperados é exata. */
static int CanShift(const int *stack, int top, int t) {
    int aux[64], n = 0, p, q, r;
    for (;;) {
        q = n ? aux[n - 1] : stack[top];
        p = Action[q][t];
        if (IS_SHIFT(p) || p == ACC) return 1;
        if (!IS_REDUCTION(p)) return 0;
        r = RULE(p);
        if (Len[r] <= n) n -= Len[r];
        else { top -= Len[r] - n; n = 0; }
        q = n ? aux[n - 1] : stack[top];
        if (n == 64) return 1;      /* não ocorre nesta gramática */
        aux[n++] = Action[q][Left[r]];
    }
}

static void SyntaxError(const int *stack, int top, const Token *a) {
    int t, first = 1;
    if (errOut == NULL) return;
    fprintf(errOut, "Erro sintatico na linha %d, coluna %d: token inesperado '%s' (%s)\n",
            a->linha, a->coluna, a->lexema, tokenName(a->token));
    fprintf(errOut, "  esperado:");
    for (t = 0; t < NUM_TERMINALS; t++)
        if (CanShift(stack, top, t)) {
            fprintf(errOut, "%s '%s'", first ? "" : ",", SymbolName(t));
            first = 0;
        }
    fputc('\n', errOut);
}

/* Figura 4.6 */
#define MAX_PARSE_STACK 1024
static int Parse(void) {
    static int stack[MAX_PARSE_STACK];
    int top = 0, i = 0, q, p, r;
    const Token *a = &tokens[0];

    stack[top] = 0;
    q = 0;
    if (!CanShift(stack, top, a->token)) { SyntaxError(stack, top, a); return 0; }
    for (;;) {
        p = Action[q][a->token];
        if (p == ACC) return 1;
        if (IS_SHIFT(p)) {
            if (top + 1 == MAX_PARSE_STACK) { fprintf(stderr, "Pilha sintatica cheia\n"); exit(2); }
            stack[++top] = p;
            tokenSecundario = a->tokenSec;       /* usado por IDD, IDU, NUM... */
            nLastLine = a->linha;
            a = &tokens[++i];
            if (!CanShift(stack, top, a->token)) { SyntaxError(stack, top, a); return 0; }
        } else if (IS_REDUCTION(p)) {
            r = RULE(p);
            top -= Len[r];
            stack[top + 1] = Action[stack[top]][Left[r]];
            top++;
            Semantics(r);
        } else {                     /* não ocorre: CanShift já verificou */
            SyntaxError(stack, top, a);
            return 0;
        }
        q = stack[top];
    }
}

t_result Compile(const char *path) {
    t_result result;

    InitCompiler();
    if (!ReadTokens(path)) return COMPILE_IO_ERROR;

    if (nLexErrors > 0) {
        result = COMPILE_LEX_ERROR;
    } else {
        InitSymbolTable();
        NewBlock();                 /* nível 0: tipos e funções globais */
        InitSemantics();
        if (!Parse())            result = COMPILE_SYNTAX_ERROR;
        else if (nSemErrors > 0) result = COMPILE_SEM_ERROR;
        else                     result = COMPILE_OK;
        FreeSymbolTable();
    }
    FreeNames();
    freeLexerTables();
    return result;
}

/* Saída idêntica à do programa da VC */
int PrintTokens(const char *path, FILE *out) {
    Token tok;
    if (!initLexer(path)) return 0;
    fprintf(out, "%-6s %-14s %-15s %-10s\n", "Linha", "Token", "Lexema", "TokenSec");
    fprintf(out, "--------------------------------------------------------\n");
    do {
        tok = nextToken();
        if (tok.tokenSec >= 0)
            fprintf(out, "%-6d %-14s %-15s %-10d\n",
                    tok.linha, tokenName(tok.token), tok.lexema, tok.tokenSec);
        else
            fprintf(out, "%-6d %-14s %-15s %-10s\n",
                    tok.linha, tokenName(tok.token), tok.lexema, "-");
        if (tok.token == TK_ERROR)
            fprintf(stderr, "Erro lexico na linha %d: caractere/token invalido '%s'\n",
                    tok.linha, tok.lexema);
    } while (tok.token != TK_EOF);
    fprintf(out, "\n--- Tabela de identificadores (nome -> token secundario) ---\n");
    fprintf(out, "--- Constantes lidas (por posicao na tabela) ---\n");
    freeLexerTables();
    closeLexer();
    return 1;
}
