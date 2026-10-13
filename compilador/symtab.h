/* ======================================================================
 * symtab.h
 * Tabela de símbolos como pilha de blocos (cap. 5) com a estrutura
 * object estendida para a análise de tipos (cap. 6, figura 6.2).
 * ====================================================================== */
#ifndef SYMTAB_H
#define SYMTAB_H

#define MAX_NEST_LEVEL 64

typedef enum { NO_KIND_DEF_ = -1, VAR_, PARAM_, FUNCTION_, FIELD_,
               ARRAY_TYPE_, STRUCT_TYPE_, ALIAS_TYPE_, SCALAR_TYPE_,
               UNIVERSAL_ } t_kind;

#define IS_TYPE_KIND(k) ((k) == ARRAY_TYPE_ || (k) == STRUCT_TYPE_ || \
                         (k) == ALIAS_TYPE_ || (k) == SCALAR_TYPE_)

typedef struct object {
    int            nName;
    struct object *pNext;
    t_kind         eKind;
    union {
        struct { struct object *pType; } Var, Param, Field;
        struct { struct object *pRetType; struct object *pParams; } Function;
        struct { struct object *pElemType; int nNumElems; } Array;
        struct { struct object *pFields; } Struct;
        struct { struct object *pBaseType; } Alias;
    } _;
} object, *pobject;

/* Tipos pré-definidos (figura 6.3) e tipo universal */
extern pobject pInteger, pChar, pBool, pString, pUniversal;

int     NewBlock(void);
int     EndBlock(void);
pobject Define(int aName);
pobject Search(int aName);
pobject Find(int aName);

void    InitSymbolTable(void);   /* nível -1, sem objetos */
void    FreeSymbolTable(void);   /* libera todos os objects criados */

#endif
