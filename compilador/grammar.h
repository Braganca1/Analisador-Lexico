/* ======================================================================
 * grammar.h
 * Gramática efetivamente usada pelo compilador (gramática oficial +
 * marcadores semânticos + extensão 'return'). Os terminais são os
 * próprios TokenType do analisador léxico (TK_EOF faz o papel de '$').
 * ====================================================================== */
#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "lexer.h"

/* Terminais: 0 .. NUM_TERMINALS-1 (TK_ERROR não é símbolo da gramática) */
#define NUM_TERMINALS ((int)TK_EOF + 1)

/* Não terminais (cap. 4: "enumerações para os não terminais") */
typedef enum {
    START_, P_, LDE_, DE_, T_, DT_, DC_, DF_, LP_, B_, LDV_, LS_, DV_, LI_,
    S_, E_, L_, R_, Y_, F_, LE_, LV_, IDD_, IDU_, ID_,
    TRUE_, FALSE_, CHR_, STR_, NUM_,
    NB_, MF_, MC_, ML_,                  /* marcadores semânticos */
    NUM_NONTERMINALS
} t_nont;

#define NUM_SYMBOLS (NUM_TERMINALS + NUM_NONTERMINALS)
#define NONT(n)     (NUM_TERMINALS + (int)(n))   /* símbolo de um não terminal */
#define IS_TERMINAL(x) ((x) < NUM_TERMINALS)

/* Regras, na mesma ordem do texto em grammar.c */
typedef enum {
    R_START,
    R_P,
    R_LDE_REC, R_LDE,
    R_DE_DF, R_DE_DT,
    R_T_INTEGER, R_T_CHAR, R_T_BOOLEAN, R_T_STRING, R_T_IDU,
    R_DT_ARRAY, R_DT_STRUCT, R_DT_ALIAS,
    R_DC_REC, R_DC,
    R_DF,
    R_LP_REC, R_LP,
    R_B,
    R_LDV_REC, R_LDV,
    R_LS_REC, R_LS,
    R_DV,
    R_LI_REC, R_LI,
    R_S_IF, R_S_IF_ELSE, R_S_WHILE, R_S_DO, R_S_BLOCK, R_S_ASSIGN,
    R_S_BREAK, R_S_CONTINUE, R_S_RETURN,
    R_E_AND, R_E_OR, R_E_L,
    R_L_LT, R_L_GT, R_L_LE, R_L_GE, R_L_EQ, R_L_NE, R_L_R,
    R_R_PLUS, R_R_MINUS, R_R_Y,
    R_Y_TIMES, R_Y_DIV, R_Y_F,
    R_F_LV, R_F_PRE_INC, R_F_PRE_DEC, R_F_POS_INC, R_F_POS_DEC,
    R_F_PAREN, R_F_CALL, R_F_MINUS, R_F_NOT,
    R_F_TRUE, R_F_FALSE, R_F_CHR, R_F_STR, R_F_NUM,
    R_LE_REC, R_LE,
    R_LV_FIELD, R_LV_INDEX, R_LV_IDU,
    R_IDD, R_IDU, R_ID,
    R_TRUE, R_FALSE, R_CHR, R_STR, R_NUM,
    R_NB, R_MF, R_MC, R_ML,
    NUM_RULES
} t_rule;

#define MAX_RHS 10

extern int Left[NUM_RULES];             /* símbolo do lado esquerdo   */
extern int Len[NUM_RULES];              /* tamanho do lado direito    */
extern int Rhs[NUM_RULES][MAX_RHS];     /* símbolos do lado direito   */

void        InitGrammar(void);          /* converte o texto em tabelas */
const char *SymbolName(int sym);        /* nome para mensagens         */
const char *RuleText(int rule);         /* texto da regra              */

#endif
