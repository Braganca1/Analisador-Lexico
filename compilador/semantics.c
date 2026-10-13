/* ======================================================================
 * semantics.c
 * Semantics(rule): um caso por regra da gramática (figura 4.9). Antes do
 * switch o nó do lado esquerdo é preparado; depois, os nós dos não
 * terminais do lado direito são desempilhados e o novo nó é empilhado.
 * Convenção do cap. 4: todo não terminal (inclusive marcadores) empilha
 * um nó, de modo que TopSem(k) aponta sempre para o mesmo símbolo.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"

int tokenSecundario;
int nLastLine;

FILE   *errOut = NULL;
int     nSemErrors;
t_error semErrors[MAX_SEM_ERRORS];

/* ---------------------------------------------------------------------- *
 * Pilha semântica (figura 4.8)                                           *
 * ---------------------------------------------------------------------- */
#define MAX_STACK_SIZE 1024

static t_attrib StackSem[MAX_STACK_SIZE];
static int      nSem;

static void PushSem(const t_attrib *attrib) {
    if (nSem == MAX_STACK_SIZE) { fprintf(stderr, "Pilha semantica cheia\n"); exit(2); }
    StackSem[nSem++] = *attrib;
}
static void     PopSem(int nToPop)      { nSem -= nToPop; }
static t_attrib TopSem(int offFromTop)  { return StackSem[nSem - 1 - offFromTop]; }

static int nNonTerms[NUM_RULES];     /* não terminais no lado direito */

/* ---------------------------------------------------------------------- *
 * Estado global da análise                                               *
 * ---------------------------------------------------------------------- */
static pobject pCurrentFunction;     /* função cujo corpo está sendo lido */
static int     nLoopDepth;           /* laços abertos (break/continue)    */

/* ---------------------------------------------------------------------- *
 * Nomes e mensagens de erro                                              *
 * ---------------------------------------------------------------------- */
static char **names;
static int    nNames;

void SetName(int nName, const char *lexeme) {
    if (nName >= nNames) {
        int n = nName + 64, i;
        names = (char **)realloc(names, (size_t)n * sizeof(char *));
        if (names == NULL) { fprintf(stderr, "Erro: memoria insuficiente\n"); exit(2); }
        for (i = nNames; i < n; i++) names[i] = NULL;
        nNames = n;
    }
    if (names[nName] == NULL) {
        names[nName] = (char *)malloc(strlen(lexeme) + 1);
        strcpy(names[nName], lexeme);
    }
}

const char *NameOf(int nName) {
    return (nName >= 0 && nName < nNames && names[nName]) ? names[nName] : "?";
}

void FreeNames(void) {
    int i;
    for (i = 0; i < nNames; i++) free(names[i]);
    free(names);
    names = NULL;
    nNames = 0;
}

static const char *MESSAGE[NUM_SEM_ERRORS] = {
    [ERR_REDECL]             = "identificador '%s' ja declarado neste escopo",
    [ERR_NOT_DECL]           = "identificador '%s' nao declarado",
    [ERR_TYPE_EXPECTED]      = "'%s' nao e um tipo",
    [ERR_BOOL_TYPE_EXPECTED] = "expressao do tipo boolean esperada",
    [ERR_TYPE_MISMATCH]      = "tipos incompativeis",
    [ERR_INVALID_TYPE]       = "tipo invalido para o operador",
    [ERR_KIND_NOT_STRUCT]    = "acesso ao campo '%s' de algo que nao e struct",
    [ERR_FIELD_NOT_DECL]     = "campo '%s' nao declarado na struct",
    [ERR_KIND_NOT_ARRAY]     = "indexacao de algo que nao e array nem string",
    [ERR_INVALID_INDEX_TYPE] = "indice deve ser do tipo integer",
    [ERR_KIND_NOT_VAR]       = "'%s' nao e variavel nem parametro",
    [ERR_KIND_NOT_FUNC]      = "'%s' nao e funcao",
    [ERR_TOO_MANY_ARGS]      = "argumentos demais na chamada de '%s'",
    [ERR_PARAM_TYPE]         = "argumento %d de '%s' com tipo incompativel",
    [ERR_TOO_FEW_ARGS]       = "argumentos de menos na chamada de '%s'",
    [ERR_RETURN_TYPE]        = "valor de retorno incompativel com o tipo de '%s'",
    [ERR_NOT_IN_LOOP]        = "'%s' fora de um laco"
};

/* Registra o erro e continua a análise (os erros são acumulados) */
static void Error(t_error e, int line, const char *name, int n) {
    if (nSemErrors < MAX_SEM_ERRORS) semErrors[nSemErrors] = e;
    nSemErrors++;
    if (errOut == NULL) return;
    fprintf(errOut, "Erro %s na linha %d: ",
            (e == ERR_REDECL || e == ERR_NOT_DECL) ? "de escopo" :
            (e == ERR_NOT_IN_LOOP) ? "semantico" : "de tipo", line);
    if (e == ERR_PARAM_TYPE) fprintf(errOut, MESSAGE[e], n, name);
    else                     fprintf(errOut, MESSAGE[e], name);
    fputc('\n', errOut);
}

/* ---------------------------------------------------------------------- *
 * Compatibilidade de tipos                                               *
 * Decisão: compatibilidade por nome (figura 6.16), mas um alias é apenas *
 * um sinônimo, então os aliases são resolvidos até o tipo base antes da  *
 * comparação. A versão estrutural (6.17) não é usada.                    *
 * ---------------------------------------------------------------------- */
static pobject BaseType(pobject t) {
    while (t->eKind == ALIAS_TYPE_) t = t->_.Alias.pBaseType;
    return t;
}

int CheckTypes(pobject t1, pobject t2) {
    t1 = BaseType(t1);
    t2 = BaseType(t2);
    if (t1 == t2) return 1;
    if (t1->eKind == UNIVERSAL_ || t2->eKind == UNIVERSAL_) return 1;
    return 0;
}

/* Próximo parâmetro da lista. Decisão: parâmetros e variáveis locais da
 * função ficam no mesmo nível e na mesma lista pNext, então a lista de
 * parâmetros termina no primeiro objeto que não é PARAM_.            */
static pobject NextParam(pobject p) {
    p = p->pNext;
    return (p != NULL && p->eKind == PARAM_) ? p : NULL;
}

/* Atribui kind e tipo a todos os identificadores ainda sem kind da lista */
static void SetKindOfList(pobject p, t_kind kind, pobject t) {
    while (p != NULL && p->eKind == NO_KIND_DEF_) {
        p->eKind = kind;
        p->_.Var.pType = t;     /* Var, Param e Field têm o mesmo layout */
        p = p->pNext;
    }
}

/* Regras comuns das expressões binárias e unárias */
static void CheckBool(pobject t, int line) {
    if (!CheckTypes(t, pBool)) Error(ERR_BOOL_TYPE_EXPECTED, line, "", 0);
}
static void CheckInt(pobject t, int line) {
    if (!CheckTypes(t, pInteger)) Error(ERR_INVALID_TYPE, line, "", 0);
}
static void CheckSame(pobject t1, pobject t2, int line) {
    if (!CheckTypes(t1, t2)) Error(ERR_TYPE_MISMATCH, line, "", 0);
}

/* ---------------------------------------------------------------------- */
void InitSemantics(void) {
    int r, i;
    for (r = 0; r < NUM_RULES; r++) {
        nNonTerms[r] = 0;
        for (i = 0; i < Len[r]; i++)
            if (!IS_TERMINAL(Rhs[r][i])) nNonTerms[r]++;
    }
    nSem = 0;
    nSemErrors = 0;
    pCurrentFunction = NULL;
    nLoopDepth = 0;
}

void Semantics(int rule) {
    t_attrib A, X0, X1, X2, X3;     /* A: lado esquerdo; Xk = TopSem(k) */
    pobject  p, t;
    int      n;

    memset(&A, 0, sizeof A);
    A.nont = (t_nont)(Left[rule] - NUM_TERMINALS);
    A.line = nLastLine;
    /* Cópias do topo; os marcadores (MF, MC) leem nós à sua esquerda */
    X0 = X1 = X2 = X3 = A;
    if (nSem > 0) X0 = TopSem(0);
    if (nSem > 1) X1 = TopSem(1);
    if (nSem > 2) X2 = TopSem(2);
    if (nSem > 3) X3 = TopSem(3);

    switch (rule) {
    /* ---- Programa e listas sem atributos ---- */
    case R_P: case R_LDE_REC: case R_LDE: case R_DE_DF: case R_DE_DT:
    case R_B: case R_LDV_REC: case R_LDV: case R_LS_REC: case R_LS:
        break;

    /* ---- Tipos (figura 6.5) ---- */
    case R_T_INTEGER: A._.T.type = pInteger; break;
    case R_T_CHAR:    A._.T.type = pChar;    break;
    case R_T_BOOLEAN: A._.T.type = pBool;    break;
    case R_T_STRING:  A._.T.type = pString;  break;
    case R_T_IDU:                                       /* IDU */
        p = X0._.ID.obj;
        if (IS_TYPE_KIND(p->eKind) || p->eKind == UNIVERSAL_) A._.T.type = p;
        else {
            A._.T.type = pUniversal;
            Error(ERR_TYPE_EXPECTED, X0.line, NameOf(X0._.ID.name), 0);
        }
        break;

    /* ---- Declarações de tipos (figuras 6.8 a 6.11) ---- */
    case R_DT_ARRAY:                                    /* IDD NUM T */
        p = X2._.ID.obj;
        p->eKind = ARRAY_TYPE_;
        p->_.Array.nNumElems = X1._.Const.val;
        p->_.Array.pElemType = X0._.T.type;
        break;
    case R_DT_STRUCT:                                   /* IDD NB DC */
        p = X2._.ID.obj;
        p->eKind = STRUCT_TYPE_;
        p->_.Struct.pFields = X0._.LI.list;
        EndBlock();
        break;
    case R_DT_ALIAS:                                    /* IDD T */
        p = X1._.ID.obj;
        p->eKind = ALIAS_TYPE_;
        p->_.Alias.pBaseType = X0._.T.type;
        break;
    case R_DC_REC:                                      /* DC LI T */
        SetKindOfList(X1._.LI.list, FIELD_, X0._.T.type);
        A._.LI.list = X2._.LI.list;
        break;
    case R_DC:                                          /* LI T */
        SetKindOfList(X1._.LI.list, FIELD_, X0._.T.type);
        A._.LI.list = X1._.LI.list;
        break;

    /* ---- Funções (figuras 6.12 e 6.13) ---- */
    case R_DF:                                          /* IDD NB LP T MF B */
        EndBlock();
        pCurrentFunction = NULL;
        break;
    case R_MF:                                          /* ... IDD NB LP T */
        p = X3._.ID.obj;
        p->eKind = FUNCTION_;
        p->_.Function.pRetType = X0._.T.type;
        p->_.Function.pParams  = X1._.LI.list;
        pCurrentFunction = p;
        break;
    case R_LP_REC:                                      /* LP IDD T */
        p = X1._.ID.obj;
        p->eKind = PARAM_;
        p->_.Param.pType = X0._.T.type;
        A._.LI.list = X2._.LI.list;
        break;
    case R_LP:                                          /* IDD T */
        p = X1._.ID.obj;
        p->eKind = PARAM_;
        p->_.Param.pType = X0._.T.type;
        A._.LI.list = p;
        break;

    /* ---- Variáveis (figura 6.6) ---- */
    case R_DV:                                          /* LI T */
        SetKindOfList(X1._.LI.list, VAR_, X0._.T.type);
        break;
    case R_LI_REC: A._.LI.list = X1._.LI.list; break;   /* LI IDD */
    case R_LI:     A._.LI.list = X0._.ID.obj;  break;   /* IDD */

    /* ---- Comandos (figuras 6.14 e 6.15) ---- */
    case R_S_IF:      CheckBool(X1._.T.type, X1.line); break;   /* E S */
    case R_S_IF_ELSE: CheckBool(X2._.T.type, X2.line); break;   /* E S S */
    case R_S_WHILE:                                     /* E ML S */
        CheckBool(X2._.T.type, X2.line);
        nLoopDepth--;
        break;
    case R_S_DO:                                        /* ML S E */
        CheckBool(X0._.T.type, X0.line);
        nLoopDepth--;
        break;
    case R_S_BLOCK: EndBlock(); break;                  /* NB B */
    case R_S_ASSIGN:                                    /* LV E */
        CheckSame(X1._.T.type, X0._.T.type, X0.line);
        break;
    case R_S_BREAK: case R_S_CONTINUE:
        if (nLoopDepth == 0)
            Error(ERR_NOT_IN_LOOP, nLastLine,
                  rule == R_S_BREAK ? "break" : "continue", 0);
        break;
    case R_S_RETURN:                                    /* E */
        if (pCurrentFunction != NULL &&
            !CheckTypes(pCurrentFunction->_.Function.pRetType, X0._.T.type))
            Error(ERR_RETURN_TYPE, X0.line, NameOf(pCurrentFunction->nName), 0);
        break;

    /* ---- Expressões lógicas e relacionais (6.18 e 6.19) ---- */
    case R_E_AND: case R_E_OR:                          /* E L */
        CheckBool(X1._.T.type, X1.line);
        CheckBool(X0._.T.type, X0.line);
        A._.T.type = pBool;
        break;
    case R_L_LT: case R_L_GT: case R_L_LE:              /* L R */
    case R_L_GE: case R_L_EQ: case R_L_NE:
        CheckSame(X1._.T.type, X0._.T.type, X0.line);
        A._.T.type = pBool;
        break;

    /* ---- Expressões aritméticas (6.20) ---- */
    case R_R_PLUS:                                      /* R Y */
        CheckSame(X1._.T.type, X0._.T.type, X0.line);
        if (!CheckTypes(X1._.T.type, pInteger) && !CheckTypes(X1._.T.type, pString))
            Error(ERR_INVALID_TYPE, X1.line, "", 0);
        A._.T.type = X1._.T.type;
        break;
    case R_R_MINUS: case R_Y_TIMES: case R_Y_DIV:       /* R Y | Y F */
        CheckSame(X1._.T.type, X0._.T.type, X0.line);
        CheckInt(X1._.T.type, X1.line);
        A._.T.type = X1._.T.type;
        break;

    /* ---- Regras de cópia ---- */
    case R_E_L: case R_L_R: case R_R_Y: case R_Y_F:
    case R_F_LV: case R_F_PAREN:
        A._.T.type = X0._.T.type;
        break;

    /* ---- Fatores (6.21) ---- */
    case R_F_PRE_INC: case R_F_PRE_DEC:                 /* LV */
    case R_F_POS_INC: case R_F_POS_DEC: case R_F_MINUS: /* LV | F */
        CheckInt(X0._.T.type, X0.line);
        A._.T.type = pInteger;
        break;
    case R_F_NOT:                                       /* F */
        if (!CheckTypes(X0._.T.type, pBool)) Error(ERR_INVALID_TYPE, X0.line, "", 0);
        A._.T.type = pBool;
        break;
    case R_F_TRUE: case R_F_FALSE: case R_F_CHR: case R_F_STR: case R_F_NUM:
        A._.T.type = X0._.Const.type;
        break;

    /* ---- Chamada de função (6.23) ---- */
    case R_MC:                                          /* IDU (à esquerda) */
        p = X0._.ID.obj;
        A._.MC.name = X0._.ID.name;
        if (p->eKind != FUNCTION_) {
            if (p->eKind != UNIVERSAL_)
                Error(ERR_KIND_NOT_FUNC, X0.line, NameOf(X0._.ID.name), 0);
            A._.MC.type  = pUniversal;
            A._.MC.param = NULL;
            A._.MC.err   = 1;
        } else {
            A._.MC.type  = p->_.Function.pRetType;
            A._.MC.param = p->_.Function.pParams;
            A._.MC.err   = 0;
        }
        break;
    case R_LE:                                          /* (MC) E */
    case R_LE_REC:                                      /* LE E   */
        if (rule == R_LE) {                             /* X1 é o MC */
            p = X1._.MC.param; A._.LE.err = X1._.MC.err;
            A._.LE.name = X1._.MC.name; n = 1;
        } else {
            p = X1._.LE.param; A._.LE.err = X1._.LE.err;
            A._.LE.name = X1._.LE.name; n = X1._.LE.n;
        }
        A._.LE.param = NULL;
        if (!A._.LE.err) {
            if (p == NULL) {
                Error(ERR_TOO_MANY_ARGS, X0.line, NameOf(A._.LE.name), 0);
                A._.LE.err = 1;
            } else {
                if (!CheckTypes(p->_.Param.pType, X0._.T.type))
                    Error(ERR_PARAM_TYPE, X0.line, NameOf(A._.LE.name), n);
                A._.LE.param = NextParam(p);
            }
        }
        A._.LE.n = n + 1;
        break;
    case R_F_CALL:                                      /* IDU MC LE */
        A._.T.type = X1._.MC.type;
        if (!X1._.MC.err && !X0._.LE.err && X0._.LE.param != NULL)
            Error(ERR_TOO_FEW_ARGS, nLastLine, NameOf(X1._.MC.name), 0);
        break;

    /* ---- Left-values (6.22) ---- */
    case R_LV_FIELD:                                    /* LV ID */
        t = BaseType(X1._.T.type);
        if (t->eKind != STRUCT_TYPE_) {
            if (t->eKind != UNIVERSAL_)
                Error(ERR_KIND_NOT_STRUCT, X0.line, NameOf(X0._.ID.name), 0);
            A._.T.type = pUniversal;
        } else {
            for (p = t->_.Struct.pFields; p != NULL; p = p->pNext)
                if (p->nName == X0._.ID.name) break;
            if (p == NULL) {
                Error(ERR_FIELD_NOT_DECL, X0.line, NameOf(X0._.ID.name), 0);
                A._.T.type = pUniversal;
            } else
                A._.T.type = p->_.Field.pType;
        }
        break;
    case R_LV_INDEX:                                    /* LV E */
        t = BaseType(X1._.T.type);
        if (t == pString)
            A._.T.type = pChar;
        else if (t->eKind != ARRAY_TYPE_) {
            if (t->eKind != UNIVERSAL_) Error(ERR_KIND_NOT_ARRAY, X1.line, "", 0);
            A._.T.type = pUniversal;
        } else
            A._.T.type = t->_.Array.pElemType;
        if (!CheckTypes(X0._.T.type, pInteger))
            Error(ERR_INVALID_INDEX_TYPE, X0.line, "", 0);
        break;
    case R_LV_IDU:                                      /* IDU */
        p = X0._.ID.obj;
        if (p->eKind != VAR_ && p->eKind != PARAM_) {
            if (p->eKind != UNIVERSAL_)
                Error(ERR_KIND_NOT_VAR, X0.line, NameOf(X0._.ID.name), 0);
            A._.T.type = pUniversal;
        } else
            A._.T.type = p->_.Var.pType;
        break;

    /* ---- Identificadores (figuras 5.8 e 6.4) ---- */
    case R_IDD:
        A._.ID.name = tokenSecundario;
        if (Search(tokenSecundario) != NULL)
            Error(ERR_REDECL, nLastLine, NameOf(tokenSecundario), 0);
        /* Decisão: mesmo redeclarado, um novo objeto é inserido (depois do
         * original, que continua sendo o encontrado pelas buscas), para que
         * as listas por pNext e o kind do original fiquem intactos. */
        A._.ID.obj = Define(tokenSecundario);   /* kind = NO_KIND_DEF_ */
        break;
    case R_IDU:
        A._.ID.name = tokenSecundario;
        if ((p = Find(tokenSecundario)) == NULL) {
            Error(ERR_NOT_DECL, nLastLine, NameOf(tokenSecundario), 0);
            p = Define(tokenSecundario);
            p->eKind = UNIVERSAL_;      /* evita erros de tipo em cascata */
        }
        A._.ID.obj = p;
        break;
    case R_ID:
        A._.ID.name = tokenSecundario;
        A._.ID.obj = NULL;
        break;

    /* ---- Literais (figura 6.7) ---- */
    case R_TRUE:  A._.Const.type = pBool; A._.Const.val = 1; break;
    case R_FALSE: A._.Const.type = pBool; A._.Const.val = 0; break;
    case R_CHR:
        A._.Const.type = pChar;
        A._.Const.pos  = tokenSecundario;
        A._.Const.val  = getCharConst(tokenSecundario);
        break;
    case R_STR:
        A._.Const.type = pString;
        A._.Const.pos  = tokenSecundario;
        A._.Const.str  = getStringConst(tokenSecundario);
        break;
    case R_NUM:
        A._.Const.type = pInteger;
        A._.Const.pos  = tokenSecundario;
        A._.Const.val  = getIntConst(tokenSecundario);
        break;

    /* ---- Marcadores ---- */
    case R_NB: NewBlock();   break;
    case R_ML: nLoopDepth++; break;

    default:   /* R_START nunca é reduzida (ACC) */
        break;
    }

    PopSem(nNonTerms[rule]);
    PushSem(&A);
}
