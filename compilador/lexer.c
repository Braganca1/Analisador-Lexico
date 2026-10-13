/* ======================================================================
 * lexer.c
 * Implementação do analisador léxico.
 * ====================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

/* ======================================================================
 * 9) LEITURA DE CARACTERES DO ARQUIVO
 * ====================================================================== */
static FILE *fonte = NULL;
static int   linhaAtual = 1;
static int   colunaAtual = 0;    /* VF: coluna do último caractere lido */
static int   colunaAnterior = 0; /* VF: coluna antes do último '\n'    */

int initLexer(const char *nomeArquivo) {
    fonte = fopen(nomeArquivo, "r");
    if (fonte == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir o arquivo '%s'\n", nomeArquivo);
        return 0;
    }
    linhaAtual = 1;
    colunaAtual = 0;
    return 1;
}

void closeLexer(void) {
    if (fonte != NULL) {
        fclose(fonte);
        fonte = NULL;
    }
}

/* Lê o próximo caractere do arquivo, atualizando o contador de linhas. */
int readChar(void) {
    int c = fgetc(fonte);
    if (c == '\n') { linhaAtual++; colunaAnterior = colunaAtual; colunaAtual = 0; }
    else if (c != EOF) colunaAtual++;
    return c; /* devolve EOF quando acabar o arquivo */
}

/* Permite "devolver" um caractere para ser lido novamente (lookahead de 1). */
void unreadChar(int c) {
    if (c == '\n') { linhaAtual--; colunaAtual = colunaAnterior; }
    else if (c != EOF) colunaAtual--;
    if (c != EOF) ungetc(c, fonte);
}

/* ======================================================================
 * 1) TABELA DE PALAVRAS RESERVADAS  +  2) searchKeyWord
 * ====================================================================== */
typedef struct {
    const char *palavra;
    TokenType   token;
} KeywordEntry;

static const KeywordEntry tabelaPalavrasReservadas[] = {
    { "integer",  TK_INTEGER  },
    { "char",     TK_CHAR     },
    { "boolean",  TK_BOOLEAN  },
    { "string",   TK_STRING   },
    { "type",     TK_TYPE     },
    { "array",    TK_ARRAY    },
    { "of",       TK_OF       },
    { "struct",   TK_STRUCT   },
    { "function", TK_FUNCTION },
    { "var",      TK_VAR      },
    { "if",       TK_IF       },
    { "else",     TK_ELSE     },
    { "while",    TK_WHILE    },
    { "do",       TK_DO       },
    { "break",    TK_BREAK    },
    { "continue", TK_CONTINUE },
    { "true",     TK_TRUE     },
    { "false",    TK_FALSE    },
    { "return",   TK_RETURN   }  /* VF */
};
static const int NUM_PALAVRAS_RESERVADAS =
    (int)(sizeof(tabelaPalavrasReservadas) / sizeof(KeywordEntry));

/* Procura 'lexema' na tabela de palavras reservadas.
 * Retorna o TokenType correspondente, ou -1 se não for uma palavra
 * reservada. */
int searchKeyWord(const char *lexema) {
    int i;
    for (i = 0; i < NUM_PALAVRAS_RESERVADAS; i++) {
        if (strcmp(tabelaPalavrasReservadas[i].palavra, lexema) == 0) {
            return (int)tabelaPalavrasReservadas[i].token;
        }
    }
    return -1;
}

/* ======================================================================
 * 3/4) TABELA DE IDENTIFICADORES (HASH)  +  searchName
 * ====================================================================== */
#define TAM_HASH 211  /* número primo para melhor distribuição */

typedef struct IdNode {
    char *nome;             /* lexema do identificador            */
    int   tokenSecundario;  /* ordem de leitura (id único)         */
    struct IdNode *prox;    /* encadeamento p/ tratar colisões     */
} IdNode;

static IdNode *tabelaNomes[TAM_HASH];
static int     contadorIds = 0; /* gera o próximo token secundário  */

/* Função de hash simples (djb2), adequada para strings de identificadores. */
static unsigned int hashString(const char *s) {
    unsigned int h = 5381;
    int c;
    while ((c = (unsigned char)*s++) != 0) {
        h = ((h << 5) + h) + (unsigned int)c; /* h * 33 + c */
    }
    return h % TAM_HASH;
}

/* Busca (ou insere, se ainda não existir) o identificador 'lexema' na
 * tabela de nomes. Cada identificador ocorre no máximo uma vez.
 * Retorna o token secundário associado (a ordem em que foi lido pela
 * primeira vez). */
int searchName(const char *lexema) {
    unsigned int idx = hashString(lexema);
    IdNode *p = tabelaNomes[idx];

    while (p != NULL) {
        if (strcmp(p->nome, lexema) == 0) {
            return p->tokenSecundario; /* já existia: devolve o mesmo id */
        }
        p = p->prox;
    }

    /* Não encontrado: insere no início da lista da posição idx */
    IdNode *novo = (IdNode *) malloc(sizeof(IdNode));
    if (novo == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente (tabela de nomes)\n");
        exit(1);
    }
    novo->nome = (char *) malloc(strlen(lexema) + 1);
    strcpy(novo->nome, lexema);
    novo->tokenSecundario = contadorIds++;
    novo->prox = tabelaNomes[idx];
    tabelaNomes[idx] = novo;

    return novo->tokenSecundario;
}

/* Libera a memória usada pela tabela de nomes (opcional, ao final do programa) */
static void freeNameTable(void) {
    int i;
    for (i = 0; i < TAM_HASH; i++) {
        IdNode *p = tabelaNomes[i];
        while (p != NULL) {
            IdNode *tmp = p;
            p = p->prox;
            free(tmp->nome);
            free(tmp);
        }
        tabelaNomes[i] = NULL;
    }
}

/* ======================================================================
 * 5/6/7/8) TABELA DE CONSTANTES (t_const com union) + add.../get...
 * ====================================================================== */
#define MAX_CONST 1024

static t_const tabConst[MAX_CONST];
static int     nNumConsts = 0; /* controla o tamanho / próxima posição livre */

/* Insere uma constante caractere; devolve a posição (token secundário). */
int addCharConst(char c) {
    if (nNumConsts >= MAX_CONST) {
        fprintf(stderr, "Erro: tabela de constantes cheia\n");
        exit(1);
    }
    tabConst[nNumConsts].tipo = CONST_CHAR;
    tabConst[nNumConsts].valor.charVal = c;
    return nNumConsts++;
}

/* Insere uma constante inteira; devolve a posição (token secundário). */
int addIntConst(int n) {
    if (nNumConsts >= MAX_CONST) {
        fprintf(stderr, "Erro: tabela de constantes cheia\n");
        exit(1);
    }
    tabConst[nNumConsts].tipo = CONST_INT;
    tabConst[nNumConsts].valor.intVal = n;
    return nNumConsts++;
}

/* Insere uma constante string; devolve a posição (token secundário). */
int addStringConst(const char *s) {
    if (nNumConsts >= MAX_CONST) {
        fprintf(stderr, "Erro: tabela de constantes cheia\n");
        exit(1);
    }
    tabConst[nNumConsts].tipo = CONST_STRING;
    tabConst[nNumConsts].valor.strVal = (char *) malloc(strlen(s) + 1);
    strcpy(tabConst[nNumConsts].valor.strVal, s);
    return nNumConsts++;
}

char getCharConst(int pos) {
    return tabConst[pos].valor.charVal;
}

int getIntConst(int pos) {
    return tabConst[pos].valor.intVal;
}

const char *getStringConst(int pos) {
    return tabConst[pos].valor.strVal;
}

static void freeConstTable(void) {
    int i;
    for (i = 0; i < nNumConsts; i++) {
        if (tabConst[i].tipo == CONST_STRING) {
            free(tabConst[i].valor.strVal);
        }
    }
    nNumConsts = 0;
}

/* ======================================================================
 * 10) nextToken() - o coração do analisador léxico
 * ====================================================================== */

static int colunaInicio = 1; /* VF: coluna do 1o caractere do token */

/* Monta um Token simples (sem token secundário) */
static Token makeToken(TokenType t, const char *lex, int linha) {
    Token tok;
    tok.token = t;
    tok.tokenSec = -1;
    tok.linha = linha;
    tok.coluna = colunaInicio;
    strncpy(tok.lexema, lex, sizeof(tok.lexema) - 1);
    tok.lexema[sizeof(tok.lexema) - 1] = '\0';
    return tok;
}

Token nextToken(void) {
    int c;
    int linhaInicio;

    /* 1) Ignora espaços em branco (espaço, tab, \n, \r) */
    do {
        c = readChar();
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

    linhaInicio = linhaAtual;
    colunaInicio = colunaAtual;

    /* 2) Fim de arquivo */
    if (c == EOF) {
        return makeToken(TK_EOF, "EOF", linhaInicio);
    }

    /* 3) Identificador ou palavra reservada:  letra (letra|digito)*  */
    if (isalpha(c)) {
        char buffer[256];
        int  n = 0;
        buffer[n++] = (char) c;
        c = readChar();
        while ((isalpha(c) || isdigit(c)) && n < (int)sizeof(buffer) - 1) {
            buffer[n++] = (char) c;
            c = readChar();
        }
        buffer[n] = '\0';
        unreadChar(c); /* devolve o caractere que encerrou o identificador */

        int kw = searchKeyWord(buffer);
        if (kw != -1) {
            /* É palavra reservada: token secundário não se aplica */
            return makeToken((TokenType) kw, buffer, linhaInicio);
        } else {
            /* É identificador: busca/insere na tabela de nomes */
            Token tok = makeToken(TK_ID, buffer, linhaInicio);
            tok.tokenSec = searchName(buffer);
            return tok;
        }
    }

    /* 4) Número:  n = digito digito*  */
    if (isdigit(c)) {
        char buffer[64];
        int  n = 0;
        buffer[n++] = (char) c;
        c = readChar();
        while (isdigit(c) && n < (int)sizeof(buffer) - 1) {
            buffer[n++] = (char) c;
            c = readChar();
        }
        buffer[n] = '\0';
        unreadChar(c);

        Token tok = makeToken(TK_NUMERAL, buffer, linhaInicio);
        tok.tokenSec = addIntConst(atoi(buffer));
        return tok;
    }

    /* 5) Constante caractere:  c = ''' . any . '''  */
    if (c == '\'') {
        int ch = readChar();
        if (ch == EOF) {
            return makeToken(TK_ERROR, "'", linhaInicio);
        }
        int fechamento = readChar();
        if (fechamento != '\'') {
            /* malformado: devolve o que sobrar e sinaliza erro */
            if (fechamento != EOF) unreadChar(fechamento);
            char lex[8] = { '\'', (char)ch, '\0' };
            return makeToken(TK_ERROR, lex, linhaInicio);
        }
        char lex[4] = { '\'', (char) ch, '\'', '\0' };
        Token tok = makeToken(TK_CHARACTER, lex, linhaInicio);
        tok.tokenSec = addCharConst((char) ch);
        return tok;
    }

    /* 6) Constante string:  s = '"' . any* . '"'  */
    if (c == '"') {
        char buffer[1024];
        int  n = 0;
        c = readChar();
        while (c != '"' && c != EOF && n < (int)sizeof(buffer) - 1) {
            buffer[n++] = (char) c;
            c = readChar();
        }
        buffer[n] = '\0';
        if (c == EOF) {
            /* string não fechada até o fim do arquivo */
            return makeToken(TK_ERROR, buffer, linhaInicio);
        }
        /* c == '"' (fechamento consumido) */
        Token tok = makeToken(TK_STRINGVAL, buffer, linhaInicio);
        tok.tokenSec = addStringConst(buffer);
        return tok;
    }

    /* 7) Símbolos e operadores (incluindo os de 2 caracteres) */
    switch (c) {
        case '(': return makeToken(TK_LPAREN, "(", linhaInicio);
        case ')': return makeToken(TK_RPAREN, ")", linhaInicio);
        case '{': return makeToken(TK_LBRACE, "{", linhaInicio);
        case '}': return makeToken(TK_RBRACE, "}", linhaInicio);
        case '[': return makeToken(TK_LBRACKET, "[", linhaInicio);
        case ']': return makeToken(TK_RBRACKET, "]", linhaInicio);
        case ',': return makeToken(TK_COMMA, ",", linhaInicio);
        case ';': return makeToken(TK_SEMI, ";", linhaInicio);
        case ':': return makeToken(TK_COLON, ":", linhaInicio);
        case '.': return makeToken(TK_DOT, ".", linhaInicio);

        case '=': {
            int c2 = readChar();
            if (c2 == '=') return makeToken(TK_EQ, "==", linhaInicio);
            unreadChar(c2);
            return makeToken(TK_ASSIGN, "=", linhaInicio);
        }
        case '!': {
            int c2 = readChar();
            if (c2 == '=') return makeToken(TK_NE, "!=", linhaInicio);
            unreadChar(c2);
            return makeToken(TK_NOT, "!", linhaInicio);
        }
        case '<': {
            int c2 = readChar();
            if (c2 == '=') return makeToken(TK_LE, "<=", linhaInicio);
            unreadChar(c2);
            return makeToken(TK_LT, "<", linhaInicio);
        }
        case '>': {
            int c2 = readChar();
            if (c2 == '=') return makeToken(TK_GE, ">=", linhaInicio);
            unreadChar(c2);
            return makeToken(TK_GT, ">", linhaInicio);
        }
        case '&': {
            int c2 = readChar();
            if (c2 == '&') return makeToken(TK_AND, "&&", linhaInicio);
            unreadChar(c2);
            return makeToken(TK_ERROR, "&", linhaInicio);
        }
        case '|': {
            int c2 = readChar();
            if (c2 == '|') return makeToken(TK_OR, "||", linhaInicio);
            unreadChar(c2);
            return makeToken(TK_ERROR, "|", linhaInicio);
        }
        case '+': {
            int c2 = readChar();
            if (c2 == '+') return makeToken(TK_INC, "++", linhaInicio);
            unreadChar(c2);
            return makeToken(TK_PLUS, "+", linhaInicio);
        }
        case '-': {
            int c2 = readChar();
            if (c2 == '-') return makeToken(TK_DEC, "--", linhaInicio);
            unreadChar(c2);
            return makeToken(TK_MINUS, "-", linhaInicio);
        }
        case '*': return makeToken(TK_MULT, "*", linhaInicio);
        case '/': return makeToken(TK_DIV, "/", linhaInicio);
        default: break;
    }

    /* 8) Caractere não reconhecido pela linguagem */
    {
        char lex[2] = { (char) c, '\0' };
        return makeToken(TK_ERROR, lex, linhaInicio);
    }
}

/* ======================================================================
 * Utilitário: nome textual de cada token (depuração / relatório)
 * ====================================================================== */
const char *tokenName(TokenType t) {
    switch (t) {
        case TK_INTEGER:    return "TK_INTEGER";
        case TK_CHAR:       return "TK_CHAR";
        case TK_BOOLEAN:    return "TK_BOOLEAN";
        case TK_STRING:     return "TK_STRING";
        case TK_TYPE:       return "TK_TYPE";
        case TK_ARRAY:      return "TK_ARRAY";
        case TK_OF:         return "TK_OF";
        case TK_STRUCT:     return "TK_STRUCT";
        case TK_FUNCTION:   return "TK_FUNCTION";
        case TK_VAR:        return "TK_VAR";
        case TK_IF:         return "TK_IF";
        case TK_ELSE:       return "TK_ELSE";
        case TK_WHILE:      return "TK_WHILE";
        case TK_DO:         return "TK_DO";
        case TK_BREAK:      return "TK_BREAK";
        case TK_CONTINUE:   return "TK_CONTINUE";
        case TK_TRUE:       return "TK_TRUE";
        case TK_FALSE:      return "TK_FALSE";
        case TK_RETURN:     return "TK_RETURN";

        case TK_LPAREN:     return "TK_LPAREN";
        case TK_RPAREN:     return "TK_RPAREN";
        case TK_LBRACE:     return "TK_LBRACE";
        case TK_RBRACE:     return "TK_RBRACE";
        case TK_LBRACKET:   return "TK_LBRACKET";
        case TK_RBRACKET:   return "TK_RBRACKET";
        case TK_COMMA:      return "TK_COMMA";
        case TK_SEMI:       return "TK_SEMI";
        case TK_COLON:      return "TK_COLON";
        case TK_DOT:        return "TK_DOT";
        case TK_ASSIGN:     return "TK_ASSIGN";
        case TK_AND:        return "TK_AND";
        case TK_OR:         return "TK_OR";
        case TK_LT:         return "TK_LT";
        case TK_GT:         return "TK_GT";
        case TK_LE:         return "TK_LE";
        case TK_GE:         return "TK_GE";
        case TK_EQ:         return "TK_EQ";
        case TK_NE:         return "TK_NE";
        case TK_PLUS:       return "TK_PLUS";
        case TK_MINUS:      return "TK_MINUS";
        case TK_MULT:       return "TK_MULT";
        case TK_DIV:        return "TK_DIV";
        case TK_INC:        return "TK_INC";
        case TK_DEC:        return "TK_DEC";
        case TK_NOT:        return "TK_NOT";

        case TK_ID:         return "TK_ID";
        case TK_NUMERAL:    return "TK_NUMERAL";
        case TK_CHARACTER:  return "TK_CHARACTER";
        case TK_STRINGVAL:  return "TK_STRINGVAL";

        case TK_EOF:        return "TK_EOF";
        case TK_ERROR:      return "TK_ERROR";
        default:            return "TK_DESCONHECIDO";
    }
}

/* Função de limpeza geral, chamada opcionalmente pelo programa principal */
void freeLexerTables(void) {
    freeNameTable();
    freeConstTable();
}
