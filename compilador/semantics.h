/* ======================================================================
 * semantics.h
 * Pilha semântica (cap. 4), atributos dos não terminais e a função
 * Semantics(rule), chamada a cada redução, com as regras de escopo
 * (cap. 5) e de tipos (cap. 6).
 * ====================================================================== */
#ifndef SEMANTICS_H
#define SEMANTICS_H

#include <stdio.h>
#include "grammar.h"
#include "symtab.h"

/* Atributos (figura 4.7): um nó por não terminal na pilha semântica */
typedef struct {
    t_nont nont;
    int    line;                  /* linha do último token lido na redução */
    union {
        struct { int name; pobject obj; } ID;          /* IDD, IDU, ID      */
        struct { pobject type; } T;    /* T, E, L, R, Y, F, LV              */
        struct { pobject list; } LI;   /* LI, DC, LP: início da lista       */
        struct { pobject type; int pos, val; const char *str; } Const;
                                       /* TRUE, FALSE, CHR, STR, NUM        */
        struct { pobject type, param; int err, name; } MC;
        struct { pobject param; int err, n, name; } LE;
    } _;
} t_attrib;

typedef enum {
    ERR_REDECL, ERR_NOT_DECL,                                   /* escopo */
    ERR_TYPE_EXPECTED, ERR_BOOL_TYPE_EXPECTED, ERR_TYPE_MISMATCH, /* tipos */
    ERR_INVALID_TYPE, ERR_KIND_NOT_STRUCT, ERR_FIELD_NOT_DECL,
    ERR_KIND_NOT_ARRAY, ERR_INVALID_INDEX_TYPE, ERR_KIND_NOT_VAR,
    ERR_KIND_NOT_FUNC, ERR_TOO_MANY_ARGS, ERR_PARAM_TYPE, ERR_TOO_FEW_ARGS,
    ERR_RETURN_TYPE, ERR_NOT_IN_LOOP,                           /* extensões */
    NUM_SEM_ERRORS
} t_error;

#define MAX_SEM_ERRORS 256

/* Preenchidos pelo analisador sintático a cada token empilhado */
extern int tokenSecundario;     /* token secundário do último terminal */
extern int nLastLine;           /* linha do último terminal            */

extern FILE   *errOut;          /* destino das mensagens (NULL = silêncio) */
extern int     nSemErrors;
extern t_error semErrors[MAX_SEM_ERRORS];

void InitSemantics(void);
void Semantics(int rule);
int  CheckTypes(pobject t1, pobject t2);

/* Lexemas dos identificadores, indexados pelo token secundário (nome),
 * usados apenas nas mensagens de erro. */
void        SetName(int nName, const char *lexeme);
const char *NameOf(int nName);
void        FreeNames(void);

#endif
