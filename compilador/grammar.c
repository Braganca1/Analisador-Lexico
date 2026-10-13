/* ======================================================================
 * grammar.c
 * A gramática é escrita como texto (uma alternativa por linha, na ordem
 * do enum t_rule) e convertida em Left[], Len[] e Rhs[] na inicialização.
 * Terminais aparecem entre aspas simples (ou Id, n, c, s, $).
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grammar.h"

static const char *GRAMMAR_TEXT[] = {
    /* R_START      */ "P' -> P $",
    /* R_P          */ "P -> LDE",
    /* R_LDE_REC    */ "LDE -> LDE DE",
    /* R_LDE        */ "LDE -> DE",
    /* R_DE_DF      */ "DE -> DF",
    /* R_DE_DT      */ "DE -> DT",
    /* R_T_INTEGER  */ "T -> 'integer'",
    /* R_T_CHAR     */ "T -> 'char'",
    /* R_T_BOOLEAN  */ "T -> 'boolean'",
    /* R_T_STRING   */ "T -> 'string'",
    /* R_T_IDU      */ "T -> IDU",
    /* R_DT_ARRAY   */ "DT -> 'type' IDD '=' 'array' '[' NUM ']' 'of' T",
    /* R_DT_STRUCT  */ "DT -> 'type' IDD '=' 'struct' NB '{' DC '}'",
    /* R_DT_ALIAS   */ "DT -> 'type' IDD '=' T",
    /* R_DC_REC     */ "DC -> DC ';' LI ':' T",
    /* R_DC         */ "DC -> LI ':' T",
    /* R_DF         */ "DF -> 'function' IDD NB '(' LP ')' ':' T MF B",
    /* R_LP_REC     */ "LP -> LP ',' IDD ':' T",
    /* R_LP         */ "LP -> IDD ':' T",
    /* R_B          */ "B -> '{' LDV LS '}'",
    /* R_LDV_REC    */ "LDV -> LDV DV",
    /* R_LDV        */ "LDV -> DV",
    /* R_LS_REC     */ "LS -> LS S",
    /* R_LS         */ "LS -> S",
    /* R_DV         */ "DV -> 'var' LI ':' T ';'",
    /* R_LI_REC     */ "LI -> LI ',' IDD",
    /* R_LI         */ "LI -> IDD",
    /* R_S_IF       */ "S -> 'if' '(' E ')' S",
    /* R_S_IF_ELSE  */ "S -> 'if' '(' E ')' S 'else' S",
    /* R_S_WHILE    */ "S -> 'while' '(' E ')' ML S",
    /* R_S_DO       */ "S -> 'do' ML S 'while' '(' E ')' ';'",
    /* R_S_BLOCK    */ "S -> NB B",
    /* R_S_ASSIGN   */ "S -> LV '=' E ';'",
    /* R_S_BREAK    */ "S -> 'break' ';'",
    /* R_S_CONTINUE */ "S -> 'continue' ';'",
    /* R_S_RETURN   */ "S -> 'return' E ';'",
    /* R_E_AND      */ "E -> E '&&' L",
    /* R_E_OR       */ "E -> E '||' L",
    /* R_E_L        */ "E -> L",
    /* R_L_LT       */ "L -> L '<' R",
    /* R_L_GT       */ "L -> L '>' R",
    /* R_L_LE       */ "L -> L '<=' R",
    /* R_L_GE       */ "L -> L '>=' R",
    /* R_L_EQ       */ "L -> L '==' R",
    /* R_L_NE       */ "L -> L '!=' R",
    /* R_L_R        */ "L -> R",
    /* R_R_PLUS     */ "R -> R '+' Y",
    /* R_R_MINUS    */ "R -> R '-' Y",
    /* R_R_Y        */ "R -> Y",
    /* R_Y_TIMES    */ "Y -> Y '*' F",
    /* R_Y_DIV      */ "Y -> Y '/' F",
    /* R_Y_F        */ "Y -> F",
    /* R_F_LV       */ "F -> LV",
    /* R_F_PRE_INC  */ "F -> '++' LV",
    /* R_F_PRE_DEC  */ "F -> '--' LV",
    /* R_F_POS_INC  */ "F -> LV '++'",
    /* R_F_POS_DEC  */ "F -> LV '--'",
    /* R_F_PAREN    */ "F -> '(' E ')'",
    /* R_F_CALL     */ "F -> IDU MC '(' LE ')'",
    /* R_F_MINUS    */ "F -> '-' F",
    /* R_F_NOT      */ "F -> '!' F",
    /* R_F_TRUE     */ "F -> TRUE",
    /* R_F_FALSE    */ "F -> FALSE",
    /* R_F_CHR      */ "F -> CHR",
    /* R_F_STR      */ "F -> STR",
    /* R_F_NUM      */ "F -> NUM",
    /* R_LE_REC     */ "LE -> LE ',' E",
    /* R_LE         */ "LE -> E",
    /* R_LV_FIELD   */ "LV -> LV '.' ID",
    /* R_LV_INDEX   */ "LV -> LV '[' E ']'",
    /* R_LV_IDU     */ "LV -> IDU",
    /* R_IDD        */ "IDD -> Id",
    /* R_IDU        */ "IDU -> Id",
    /* R_ID         */ "ID -> Id",
    /* R_TRUE       */ "TRUE -> 'true'",
    /* R_FALSE      */ "FALSE -> 'false'",
    /* R_CHR        */ "CHR -> c",
    /* R_STR        */ "STR -> s",
    /* R_NUM        */ "NUM -> n",
    /* R_NB         */ "NB -> ",
    /* R_MF         */ "MF -> ",
    /* R_MC         */ "MC -> ",
    /* R_ML         */ "ML -> "
};

/* Grafia de cada terminal, indexada pelo TokenType do léxico */
static const char *TERMINAL_NAME[NUM_TERMINALS] = {
    [TK_INTEGER] = "integer", [TK_CHAR] = "char", [TK_BOOLEAN] = "boolean",
    [TK_STRING] = "string", [TK_TYPE] = "type", [TK_ARRAY] = "array",
    [TK_OF] = "of", [TK_STRUCT] = "struct", [TK_FUNCTION] = "function",
    [TK_VAR] = "var", [TK_IF] = "if", [TK_ELSE] = "else",
    [TK_WHILE] = "while", [TK_DO] = "do", [TK_BREAK] = "break",
    [TK_CONTINUE] = "continue", [TK_TRUE] = "true", [TK_FALSE] = "false",
    [TK_RETURN] = "return",
    [TK_LPAREN] = "(", [TK_RPAREN] = ")", [TK_LBRACE] = "{", [TK_RBRACE] = "}",
    [TK_LBRACKET] = "[", [TK_RBRACKET] = "]", [TK_COMMA] = ",", [TK_SEMI] = ";",
    [TK_COLON] = ":", [TK_DOT] = ".", [TK_ASSIGN] = "=", [TK_AND] = "&&",
    [TK_OR] = "||", [TK_LT] = "<", [TK_GT] = ">", [TK_LE] = "<=",
    [TK_GE] = ">=", [TK_EQ] = "==", [TK_NE] = "!=", [TK_PLUS] = "+",
    [TK_MINUS] = "-", [TK_MULT] = "*", [TK_DIV] = "/", [TK_INC] = "++",
    [TK_DEC] = "--", [TK_NOT] = "!",
    [TK_ID] = "Id", [TK_NUMERAL] = "n", [TK_CHARACTER] = "c",
    [TK_STRINGVAL] = "s", [TK_EOF] = "$"
};

static const char *NONTERMINAL_NAME[NUM_NONTERMINALS] = {
    "P'", "P", "LDE", "DE", "T", "DT", "DC", "DF", "LP", "B", "LDV", "LS",
    "DV", "LI", "S", "E", "L", "R", "Y", "F", "LE", "LV", "IDD", "IDU", "ID",
    "TRUE", "FALSE", "CHR", "STR", "NUM", "NB", "MF", "MC", "ML"
};

/* O texto deve ter exatamente uma linha por regra do enum */
typedef char check_rules[(sizeof GRAMMAR_TEXT / sizeof *GRAMMAR_TEXT
                          == NUM_RULES) ? 1 : -1];

int Left[NUM_RULES];
int Len[NUM_RULES];
int Rhs[NUM_RULES][MAX_RHS];

const char *SymbolName(int sym) {
    return IS_TERMINAL(sym) ? TERMINAL_NAME[sym]
                            : NONTERMINAL_NAME[sym - NUM_TERMINALS];
}

const char *RuleText(int rule) { return GRAMMAR_TEXT[rule]; }

/* Converte um nome do texto da gramática em número de símbolo */
static int LookupSymbol(const char *name, int rule) {
    char buf[32];
    int i, n = (int)strlen(name);

    if (name[0] == '\'' && n > 2 && name[n - 1] == '\'') {   /* 'xxx' */
        memcpy(buf, name + 1, (size_t)(n - 2));
        buf[n - 2] = '\0';
        name = buf;
    }
    for (i = 0; i < NUM_TERMINALS; i++)
        if (strcmp(TERMINAL_NAME[i], name) == 0) return i;
    for (i = 0; i < NUM_NONTERMINALS; i++)
        if (strcmp(NONTERMINAL_NAME[i], name) == 0) return NONT(i);

    fprintf(stderr, "Gramatica: simbolo '%s' desconhecido na regra %d\n",
            name, rule);
    exit(2);
}

void InitGrammar(void) {
    int r;
    for (r = 0; r < NUM_RULES; r++) {
        char line[128], *tok;
        strcpy(line, GRAMMAR_TEXT[r]);

        tok = strtok(line, " ");
        Left[r] = LookupSymbol(tok, r);
        strtok(NULL, " ");                       /* "->" */
        Len[r] = 0;
        while ((tok = strtok(NULL, " ")) != NULL)
            Rhs[r][Len[r]++] = LookupSymbol(tok, r);
    }
}
