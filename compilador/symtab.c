/* ======================================================================
 * symtab.c
 * Funções das figuras 5.4 a 5.7. EndBlock() não libera os objetos do
 * bloco: parâmetros e campos continuam referenciados pelas funções e
 * structs. Todos os objetos são liberados juntos em FreeSymbolTable().
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include "symtab.h"

/* Figura 6.3. Decisão: o tipo universal tem kind UNIVERSAL_ (o livro usa
 * SCALAR_TYPE_), para que os testes "eKind != UNIVERSAL_" das regras
 * de left-value também o reconheçam e não gerem erros em cascata.   */
static object int_       = { -1, NULL, SCALAR_TYPE_, { { NULL } } };
static object char_      = { -1, NULL, SCALAR_TYPE_, { { NULL } } };
static object bool_      = { -1, NULL, SCALAR_TYPE_, { { NULL } } };
static object string_    = { -1, NULL, SCALAR_TYPE_, { { NULL } } };
static object universal_ = { -1, NULL, UNIVERSAL_,   { { NULL } } };

pobject pInteger = &int_, pChar = &char_, pBool = &bool_,
        pString = &string_, pUniversal = &universal_;

static pobject SymbolTable[MAX_NEST_LEVEL];
static pobject SymbolTableLast[MAX_NEST_LEVEL];
static int     nCurrentLevel = -1;

/* Registro de todos os objects alocados, só para liberá-los no final */
static pobject *allObjects = NULL;
static int      nAllObjects = 0, capAllObjects = 0;

int NewBlock(void) {
    if (nCurrentLevel + 1 >= MAX_NEST_LEVEL) {
        fprintf(stderr, "Erro: aninhamento maximo de blocos excedido\n");
        exit(2);
    }
    SymbolTable[++nCurrentLevel] = NULL;
    SymbolTableLast[nCurrentLevel] = NULL;
    return nCurrentLevel;
}

int EndBlock(void) {
    return --nCurrentLevel;
}

pobject Define(int aName) {
    pobject obj = (pobject)calloc(1, sizeof(object));
    if (obj == NULL) { fprintf(stderr, "Erro: memoria insuficiente\n"); exit(2); }

    if (nAllObjects == capAllObjects) {
        capAllObjects = capAllObjects ? 2 * capAllObjects : 64;
        allObjects = (pobject *)realloc(allObjects,
                                        (size_t)capAllObjects * sizeof(pobject));
        if (allObjects == NULL) { fprintf(stderr, "Erro: memoria insuficiente\n"); exit(2); }
    }
    allObjects[nAllObjects++] = obj;

    obj->nName = aName;
    obj->pNext = NULL;
    obj->eKind = NO_KIND_DEF_;

    if (SymbolTable[nCurrentLevel] == NULL) {
        SymbolTable[nCurrentLevel] = obj;
        SymbolTableLast[nCurrentLevel] = obj;
    } else {
        SymbolTableLast[nCurrentLevel]->pNext = obj;
        SymbolTableLast[nCurrentLevel] = obj;
    }
    return obj;
}

pobject Search(int aName) {
    pobject obj = SymbolTable[nCurrentLevel];
    while (obj != NULL) {
        if (obj->nName == aName) break;
        obj = obj->pNext;
    }
    return obj;
}

pobject Find(int aName) {
    int i;
    pobject obj = NULL;
    for (i = nCurrentLevel; i >= 0; --i) {
        obj = SymbolTable[i];
        while (obj != NULL) {
            if (obj->nName == aName) break;
            obj = obj->pNext;
        }
        if (obj != NULL) break;
    }
    return obj;
}

void InitSymbolTable(void) {
    nCurrentLevel = -1;
}

void FreeSymbolTable(void) {
    int i;
    for (i = 0; i < nAllObjects; i++) free(allObjects[i]);
    free(allObjects);
    allObjects = NULL;
    nAllObjects = capAllObjects = 0;
    nCurrentLevel = -1;
}
