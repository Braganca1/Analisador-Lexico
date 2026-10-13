/* ======================================================================
 * parser.h
 * Integração das fases: léxico -> sintático (la-LR(1)) -> semântica.
 * ====================================================================== */
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

typedef enum { COMPILE_OK = 0, COMPILE_LEX_ERROR, COMPILE_SYNTAX_ERROR,
               COMPILE_SEM_ERROR, COMPILE_IO_ERROR } t_result;

void     InitCompiler(void);               /* gramática + tabela (uma vez) */
t_result Compile(const char *path);        /* mensagens vão para errOut    */
int      PrintTokens(const char *path, FILE *out);   /* modo --tokens (VC) */

extern int nLexErrors;

#endif
