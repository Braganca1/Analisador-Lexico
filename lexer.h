/* ======================================================================
 * lexer.h
 * Analisador Léxico para a gramática fornecida.
 * ====================================================================== */
#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

/* ----------------------------------------------------------------------
 * 1) Enumeração dos tokens: palavras reservadas, símbolos, tokens
 *    regulares e um token para "desconhecido/erro".
 * ---------------------------------------------------------------------- */
typedef enum {
    /* ---- Palavras reservadas ---- */
    TK_INTEGER = 0,
    TK_CHAR,
    TK_BOOLEAN,
    TK_STRING,
    TK_TYPE,
    TK_ARRAY,
    TK_OF,
    TK_STRUCT,
    TK_FUNCTION,
    TK_VAR,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_DO,
    TK_BREAK,
    TK_CONTINUE,
    TK_TRUE,
    TK_FALSE,

    /* ---- Símbolos / operadores ---- */
    TK_LPAREN,      /* (  */
    TK_RPAREN,      /* )  */
    TK_LBRACE,      /* {  */
    TK_RBRACE,      /* }  */
    TK_LBRACKET,    /* [  */
    TK_RBRACKET,    /* ]  */
    TK_COMMA,       /* ,  */
    TK_SEMI,        /* ;  */
    TK_COLON,       /* :  */
    TK_DOT,         /* .  */
    TK_ASSIGN,      /* =  */
    TK_AND,         /* && */
    TK_OR,          /* || */
    TK_LT,          /* <  */
    TK_GT,          /* >  */
    TK_LE,          /* <= */
    TK_GE,          /* >= */
    TK_EQ,          /* == */
    TK_NE,          /* != */
    TK_PLUS,        /* +  */
    TK_MINUS,       /* -  */
    TK_MULT,        /* *  */
    TK_DIV,         /* /  */
    TK_INC,         /* ++ */
    TK_DEC,         /* -- */
    TK_NOT,         /* !  */

    /* ---- Tokens regulares (identificador e literais) ---- */
    TK_ID,          /* identificador (Id)      -> tabela de nomes     */
    TK_NUMERAL,     /* número        (n)       -> tabela de constantes */
    TK_CHARACTER,   /* caractere     (c)       -> tabela de constantes */
    TK_STRINGVAL,   /* string        (s)       -> tabela de constantes */

    /* ---- Fim de arquivo e erro ---- */
    TK_EOF,
    TK_ERROR
} TokenType;

/* Estrutura devolvida por nextToken(): o token principal e o token
 * secundário (posição na tabela de nomes ou na tabela de constantes,
 * quando aplicável). */
typedef struct {
    TokenType token;        /* token principal                     */
    int       tokenSec;     /* token secundário ("nome"); -1 se não usado */
    int       linha;        /* linha onde o token começou (p/ mensagens) */
    char      lexema[256];  /* lexema lido (útil para depuração)   */
} Token;

/* ----------------------------------------------------------------------
 * 2) Busca de palavras reservadas
 * ---------------------------------------------------------------------- */
int searchKeyWord(const char *lexema); /* retorna o TokenType da palavra
                                           reservada, ou -1 se não encontrada */

/* ----------------------------------------------------------------------
 * 3/4) Tabela (hash) de identificadores e busca/inserção
 * ---------------------------------------------------------------------- */
int searchName(const char *lexema); /* devolve o token secundário do
                                        identificador, inserindo-o na
                                        tabela caso ainda não exista   */

/* ----------------------------------------------------------------------
 * 5/6/7/8) Tabela de constantes (union) e funções de inserção/consulta
 * ---------------------------------------------------------------------- */
typedef enum { CONST_CHAR, CONST_INT, CONST_STRING } TipoConst;

typedef struct {
    TipoConst tipo;
    union {
        char  charVal;
        int   intVal;
        char *strVal;   /* alocada dinamicamente */
    } valor;
} t_const;

int  addCharConst(char c);
int  addIntConst(int n);
int  addStringConst(const char *s);

char        getCharConst(int pos);
int         getIntConst(int pos);
const char *getStringConst(int pos);

/* ----------------------------------------------------------------------
 * 9) Leitura de caracteres do arquivo fonte
 * ---------------------------------------------------------------------- */
int  readChar(void);        /* lê (consome) o próximo caractere do arquivo */
void unreadChar(int c);     /* "devolve" um caractere lido (lookahead)     */

/* Inicialização / finalização do analisador léxico */
int  initLexer(const char *nomeArquivo);
void closeLexer(void);
void freeLexerTables(void); /* libera tabela de nomes e de constantes */

/* ----------------------------------------------------------------------
 * 10) Função principal do analisador léxico
 * ---------------------------------------------------------------------- */
Token nextToken(void);

/* Utilitário: nome textual de um TokenType (para impressão/depuração) */
const char *tokenName(TokenType t);

#endif /* LEXER_H */
