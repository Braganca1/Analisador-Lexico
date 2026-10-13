/* ======================================================================
 * lalr.c
 * Os estados la-LR(1) são calculados "diretamente reaproveitando estados
 * já existentes e adicionando novos look-aheads" (cap. 3.8): cada goto
 * produz um núcleo LR(1); se já existe um estado com o mesmo núcleo
 * (mesmos itens sem os look-aheads) os look-aheads são unidos e o estado
 * é reprocessado, até que nada mude.
 * Os conjuntos de look-ahead são mapas de bits sobre os terminais.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "lalr.h"

typedef uint64_t t_set;                     /* conjunto de terminais */
typedef char check_set[(NUM_TERMINALS <= 64) ? 1 : -1];
#define BIT(a) ((t_set)1 << (a))

#define MAX_KERNEL 64
#define MAX_ITEMS  512

typedef struct {            /* item LR(1): regra, posição do ponto, look-aheads */
    int   rule, dot;
    t_set la;
} t_item;

typedef struct {
    int    nKernel;
    t_item kernel[MAX_KERNEL];
    int    dirty;           /* precisa (re)calcular closure/goto */
} t_state;

int Action[MAX_STATES][NUM_SYMBOLS];
int nStates;
int nShiftReduce, nReduceReduce;

static t_state states[MAX_STATES];
static int     Goto[MAX_STATES][NUM_SYMBOLS];
static t_set   First[NUM_NONTERMINALS];
static int     Nullable[NUM_NONTERMINALS];

/* ---------------------------------------------------------------------- *
 * FIRST                                                                  *
 * ---------------------------------------------------------------------- */
static void ComputeFirst(void) {
    int changed = 1, r, i;
    while (changed) {
        changed = 0;
        for (r = 0; r < NUM_RULES; r++) {
            int A = Left[r] - NUM_TERMINALS;
            t_set f = First[A];
            int null = 1;
            for (i = 0; i < Len[r] && null; i++) {
                int x = Rhs[r][i];
                if (IS_TERMINAL(x)) { f |= BIT(x); null = 0; }
                else { f |= First[x - NUM_TERMINALS];
                       null = Nullable[x - NUM_TERMINALS]; }
            }
            if (f != First[A] || (null && !Nullable[A])) {
                First[A] = f;
                Nullable[A] |= null;
                changed = 1;
            }
        }
    }
}

/* FIRST( Rhs[r][from..] la ) */
static t_set FirstOfSuffix(int r, int from, t_set la) {
    t_set f = 0;
    int i;
    for (i = from; i < Len[r]; i++) {
        int x = Rhs[r][i];
        if (IS_TERMINAL(x)) return f | BIT(x);
        f |= First[x - NUM_TERMINALS];
        if (!Nullable[x - NUM_TERMINALS]) return f;
    }
    return f | la;
}

/* ---------------------------------------------------------------------- *
 * closure                                                                *
 * ---------------------------------------------------------------------- */
static int Closure(const t_state *s, t_item *items) {
    int n = s->nKernel, changed = 1, i, r, k;

    for (i = 0; i < n; i++) items[i] = s->kernel[i];

    while (changed) {
        changed = 0;
        for (i = 0; i < n; i++) {
            int B;
            t_set f;
            if (items[i].dot >= Len[items[i].rule]) continue;
            B = Rhs[items[i].rule][items[i].dot];
            if (IS_TERMINAL(B)) continue;
            f = FirstOfSuffix(items[i].rule, items[i].dot + 1, items[i].la);

            for (r = 0; r < NUM_RULES; r++) {
                if (Left[r] != B) continue;
                for (k = 0; k < n; k++)
                    if (items[k].rule == r && items[k].dot == 0) break;
                if (k == n) {
                    if (n == MAX_ITEMS) { fprintf(stderr, "MAX_ITEMS\n"); exit(2); }
                    items[n].rule = r; items[n].dot = 0; items[n].la = f;
                    n++; changed = 1;
                } else if ((items[k].la | f) != items[k].la) {
                    items[k].la |= f; changed = 1;
                }
            }
        }
    }
    return n;
}

/* ---------------------------------------------------------------------- *
 * goto + fusão de estados com o mesmo núcleo                             *
 * ---------------------------------------------------------------------- */
static int SameCore(const t_state *a, const t_state *b) {
    int i;
    if (a->nKernel != b->nKernel) return 0;
    for (i = 0; i < a->nKernel; i++)
        if (a->kernel[i].rule != b->kernel[i].rule ||
            a->kernel[i].dot  != b->kernel[i].dot) return 0;
    return 1;
}

/* Devolve o estado com núcleo k, criando-o ou unindo os look-aheads */
static int AddState(const t_state *k) {
    int s, i;
    for (s = 0; s < nStates; s++) {
        if (!SameCore(&states[s], k)) continue;
        for (i = 0; i < k->nKernel; i++) {
            t_set u = states[s].kernel[i].la | k->kernel[i].la;
            if (u != states[s].kernel[i].la) {
                states[s].kernel[i].la = u;
                states[s].dirty = 1;
            }
        }
        return s;
    }
    if (nStates == MAX_STATES) { fprintf(stderr, "MAX_STATES\n"); exit(2); }
    states[nStates] = *k;
    states[nStates].dirty = 1;
    return nStates++;
}

static void ProcessState(int s) {
    static t_item items[MAX_ITEMS];
    int n = Closure(&states[s], items), x, i, j;

    for (x = 0; x < NUM_SYMBOLS; x++) {
        t_state k;
        if (x == TK_EOF) continue;      /* P' -> P . $ é aceitação, não estado */
        k.nKernel = 0;
        for (i = 0; i < n; i++) {
            t_item it = items[i];
            if (it.dot < Len[it.rule] && Rhs[it.rule][it.dot] == x) {
                it.dot++;
                /* insere ordenado por (regra, ponto): núcleo canônico */
                for (j = k.nKernel; j > 0 &&
                     (k.kernel[j-1].rule > it.rule ||
                      (k.kernel[j-1].rule == it.rule && k.kernel[j-1].dot > it.dot));
                     j--)
                    k.kernel[j] = k.kernel[j-1];
                k.kernel[j] = it;
                if (++k.nKernel == MAX_KERNEL) { fprintf(stderr, "MAX_KERNEL\n"); exit(2); }
            }
        }
        Goto[s][x] = k.nKernel ? AddState(&k) : 0;
    }
}

/* ---------------------------------------------------------------------- *
 * Tabela de Ação                                                         *
 * ---------------------------------------------------------------------- */
#define MAX_CONFLICTS 64
static struct { int state, sym, kept, other; } conflicts[MAX_CONFLICTS];
static int nConflicts;

static void SetReduction(int s, int a, int r) {
    int p = Action[s][a];
    if (p == 0) { Action[s][a] = -r; return; }
    if (nConflicts < MAX_CONFLICTS) {
        conflicts[nConflicts].state = s;
        conflicts[nConflicts].sym   = a;
        conflicts[nConflicts].kept  = p;
        conflicts[nConflicts].other = -r;
        nConflicts++;
    }
    if (IS_SHIFT(p) || p == ACC) nShiftReduce++;      /* mantém o empilha */
    else {
        nReduceReduce++;
        if (r < RULE(p)) Action[s][a] = -r;            /* mantém a menor regra */
    }
}

void BuildLALRTable(void) {
    static t_item items[MAX_ITEMS];
    int s, i, a, again = 1;

    ComputeFirst();

    nStates = 0; nConflicts = 0; nShiftReduce = nReduceReduce = 0;
    states[0].nKernel = 1;
    states[0].kernel[0].rule = R_START;
    states[0].kernel[0].dot  = 0;
    states[0].kernel[0].la   = 0;          /* look-ahead "ϵ" do item inicial */
    states[0].dirty = 1;
    nStates = 1;

    while (again) {
        again = 0;
        for (s = 0; s < nStates; s++)
            if (states[s].dirty) { states[s].dirty = 0; ProcessState(s); again = 1; }
    }

    for (s = 0; s < nStates; s++) {
        int n = Closure(&states[s], items);
        for (a = 0; a < NUM_SYMBOLS; a++) Action[s][a] = Goto[s][a];
        for (i = 0; i < n; i++)
            if (items[i].rule == R_START && items[i].dot == 1)
                Action[s][TK_EOF] = ACC;
        for (i = 0; i < n; i++) {
            if (items[i].dot < Len[items[i].rule]) continue;
            for (a = 0; a < NUM_TERMINALS; a++)
                if (items[i].la & BIT(a)) SetReduction(s, a, items[i].rule);
        }
    }
}

void PrintTableReport(FILE *out) {
    int s, a, nShift = 0, nRed = 0, nGoto = 0, nKernel = 0, i;
    for (s = 0; s < nStates; s++) {
        nKernel += states[s].nKernel;
        for (a = 0; a < NUM_SYMBOLS; a++) {
            int p = Action[s][a];
            if (IS_REDUCTION(p)) nRed++;
            else if (IS_SHIFT(p)) { if (IS_TERMINAL(a)) nShift++; else nGoto++; }
        }
    }
    fprintf(out, "Tabela la-LR(1)\n");
    fprintf(out, "  regras: %d  terminais: %d  nao terminais: %d\n",
            NUM_RULES, NUM_TERMINALS, NUM_NONTERMINALS);
    fprintf(out, "  estados: %d  itens de nucleo: %d\n", nStates, nKernel);
    fprintf(out, "  entradas: %d empilha, %d reduz, %d desvio\n", nShift, nRed, nGoto);
    fprintf(out, "  conflitos: %d empilha/reduz, %d reduz/reduz\n",
            nShiftReduce, nReduceReduce);
    for (i = 0; i < nConflicts; i++) {
        int p = conflicts[i].kept, r = RULE(conflicts[i].other);
        if (IS_SHIFT(p) || p == ACC)
            fprintf(out, "  estado %d, '%s': empilha (mantido) x reduz por %s\n",
                    conflicts[i].state, SymbolName(conflicts[i].sym), RuleText(r));
        else
            fprintf(out, "  estado %d, '%s': reduz por %s x reduz por %s\n",
                    conflicts[i].state, SymbolName(conflicts[i].sym),
                    RuleText(RULE(p)), RuleText(r));
    }
}
