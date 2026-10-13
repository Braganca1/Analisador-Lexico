/* ======================================================================
 * lalr.h
 * Construção da Tabela de Ação la-LR(1) a partir da gramática (cap. 3.8):
 * FIRST, closure, goto e fusão dos estados com o mesmo núcleo.
 * ====================================================================== */
#ifndef LALR_H
#define LALR_H

#include <stdio.h>
#include "grammar.h"

#define MAX_STATES 512

/* Codificação das entradas de Action[q][x] (como na figura 4.6):
 *   0          erro
 *   p > 0      empilha (terminal) / desvio (não terminal) para o estado p
 *   p < 0      reduz pela regra -p
 *   ACC        aceita (empilhar '$' a partir de P' -> P . $)            */
#define ACC              0x7FFF
#define IS_SHIFT(p)      ((p) > 0 && (p) != ACC)
#define IS_REDUCTION(p)  ((p) < 0)
#define RULE(p)          (-(p))

extern int Action[MAX_STATES][NUM_SYMBOLS];
extern int nStates;
extern int nShiftReduce;    /* conflitos empilha/reduz (resolvidos: empilha) */
extern int nReduceReduce;   /* conflitos reduz/reduz (resolvidos: menor regra) */

void BuildLALRTable(void);
void PrintTableReport(FILE *out);   /* estatísticas e conflitos */

#endif
