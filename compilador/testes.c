/* ======================================================================
 * testes.c
 * Testes da VF (executar a partir de compilador/):
 *  - a tabela la-LR(1) tem 1 conflito empilha/reduz e 0 reduz/reduz;
 *  - todo programa de exemplos/validos compila sem erro;
 *  - cada programa de exemplos/erros dispara o erro esperado;
 *  - mensagens de erro sintático trazem linha, coluna e esperados.
 * ====================================================================== */
#include <stdio.h>
#include <string.h>
#include "grammar.h"
#include "lalr.h"
#include "semantics.h"
#include "parser.h"

static int nTests, nFailed;

#define CHECK(cond, file) do { nTests++; if (!(cond)) { nFailed++; \
    printf("FALHOU: %s  (%s)\n", #cond, file); } } while (0)

static int HasError(t_error e) {
    int i;
    for (i = 0; i < nSemErrors && i < MAX_SEM_ERRORS; i++)
        if (semErrors[i] == e) return 1;
    return 0;
}

static const char *VALID[] = {
    "exemplos/validos/fatorial.ssl", "exemplos/validos/strings.ssl",
    "exemplos/validos/ordena.ssl",   "exemplos/validos/tipos.ssl"
};

#define ANY -1
static const struct {
    const char *file;
    t_result    result;
    int         error;      /* t_error esperado (ANY se não semântico) */
    int         count;      /* número exato de erros (ANY = não verifica) */
} CASES[] = {
    { "exemplos/erros/e01_redeclaracao.ssl",        COMPILE_SEM_ERROR, ERR_REDECL,             1 },
    { "exemplos/erros/e02_nao_declarado.ssl",       COMPILE_SEM_ERROR, ERR_NOT_DECL,           1 },
    { "exemplos/erros/e03_tipo_esperado.ssl",       COMPILE_SEM_ERROR, ERR_TYPE_EXPECTED,      1 },
    { "exemplos/erros/e04_bool_esperado.ssl",       COMPILE_SEM_ERROR, ERR_BOOL_TYPE_EXPECTED, 1 },
    { "exemplos/erros/e05_tipos_incompativeis.ssl", COMPILE_SEM_ERROR, ERR_TYPE_MISMATCH,      1 },
    { "exemplos/erros/e06_tipo_invalido.ssl",       COMPILE_SEM_ERROR, ERR_INVALID_TYPE,       1 },
    { "exemplos/erros/e07_nao_struct.ssl",          COMPILE_SEM_ERROR, ERR_KIND_NOT_STRUCT,    1 },
    { "exemplos/erros/e08_campo_nao_declarado.ssl", COMPILE_SEM_ERROR, ERR_FIELD_NOT_DECL,     1 },
    { "exemplos/erros/e09_nao_array.ssl",           COMPILE_SEM_ERROR, ERR_KIND_NOT_ARRAY,     1 },
    { "exemplos/erros/e10_indice_invalido.ssl",     COMPILE_SEM_ERROR, ERR_INVALID_INDEX_TYPE, 1 },
    { "exemplos/erros/e11_nao_variavel.ssl",        COMPILE_SEM_ERROR, ERR_KIND_NOT_VAR,       1 },
    { "exemplos/erros/e12_nao_funcao.ssl",          COMPILE_SEM_ERROR, ERR_KIND_NOT_FUNC,      1 },
    { "exemplos/erros/e13_argumentos_demais.ssl",   COMPILE_SEM_ERROR, ERR_TOO_MANY_ARGS,      1 },
    { "exemplos/erros/e14_tipo_argumento.ssl",      COMPILE_SEM_ERROR, ERR_PARAM_TYPE,         1 },
    { "exemplos/erros/e15_argumentos_de_menos.ssl", COMPILE_SEM_ERROR, ERR_TOO_FEW_ARGS,       1 },
    { "exemplos/erros/e16_tipo_retorno.ssl",        COMPILE_SEM_ERROR, ERR_RETURN_TYPE,        1 },
    { "exemplos/erros/e17_fora_de_laco.ssl",        COMPILE_SEM_ERROR, ERR_NOT_IN_LOOP,        1 },
    /* tipo universal: só os dois identificadores não declarados */
    { "exemplos/erros/e18_sem_cascata.ssl",         COMPILE_SEM_ERROR, ERR_NOT_DECL,           2 },
    /* erros acumulados, sem parar no primeiro */
    { "exemplos/erros/e19_varios_erros.ssl",        COMPILE_SEM_ERROR, ERR_RETURN_TYPE,        4 },
    /* structs iguais com nomes diferentes são incompatíveis; alias não */
    { "exemplos/erros/e20_structs_por_nome.ssl",    COMPILE_SEM_ERROR, ERR_TYPE_MISMATCH,      1 },
    /* identificador não declarado é definido e não é reportado de novo */
    { "exemplos/erros/e21_nao_declarado_uma_vez.ssl", COMPILE_SEM_ERROR, ERR_NOT_DECL,         1 },
    { "exemplos/erros/s01_falta_ponto_virgula.ssl", COMPILE_SYNTAX_ERROR, ANY, ANY },
    { "exemplos/erros/s02_bloco_sem_declaracao.ssl", COMPILE_SYNTAX_ERROR, ANY, ANY },
    { "exemplos/erros/s03_funcao_sem_parametros.ssl", COMPILE_SYNTAX_ERROR, ANY, ANY },
    { "exemplos/erros/s04_comando_expressao.ssl",   COMPILE_SYNTAX_ERROR, ANY, ANY },
    { "exemplos/erros/l01_lexico.ssl",              COMPILE_LEX_ERROR, ANY, 2 },
};

/* Compila capturando as mensagens em um arquivo temporário */
static t_result CompileCapture(const char *file, char *buf, size_t size) {
    t_result r;
    size_t n;
    errOut = tmpfile();
    r = Compile(file);
    rewind(errOut);
    n = fread(buf, 1, size - 1, errOut);
    buf[n] = '\0';
    fclose(errOut);
    errOut = NULL;
    return r;
}

int main(void) {
    char msg[4096];
    int i, e;

    InitCompiler();

    /* 1) Tabela */
    CHECK(nShiftReduce == 1, "tabela");
    CHECK(nReduceReduce == 0, "tabela");

    /* 2) Programas válidos */
    for (i = 0; i < (int)(sizeof VALID / sizeof *VALID); i++) {
        t_result r = CompileCapture(VALID[i], msg, sizeof msg);
        CHECK(r == COMPILE_OK, VALID[i]);
        if (r != COMPILE_OK) printf("%s", msg);
    }

    /* 3) Programas com erro */
    for (i = 0; i < (int)(sizeof CASES / sizeof *CASES); i++) {
        t_result r = CompileCapture(CASES[i].file, msg, sizeof msg);
        CHECK(r == CASES[i].result, CASES[i].file);
        if (CASES[i].error != ANY) CHECK(HasError((t_error)CASES[i].error), CASES[i].file);
        if (CASES[i].count != ANY) {
            int n = (r == COMPILE_LEX_ERROR) ? nLexErrors : nSemErrors;
            CHECK(n == CASES[i].count, CASES[i].file);
        }
    }

    /* 4) Todo erro semântico tem um caso que o dispara */
    for (e = 0; e < NUM_SEM_ERRORS; e++) {
        int found = 0;
        for (i = 0; i < (int)(sizeof CASES / sizeof *CASES); i++)
            if (CASES[i].error == e) found = 1;
        CHECK(found, "cobertura dos codigos de erro");
    }

    /* 5) Mensagem de erro sintático: linha, coluna, token e esperados */
    CompileCapture("exemplos/erros/s01_falta_ponto_virgula.ssl", msg, sizeof msg);
    CHECK(strstr(msg, "linha 5, coluna 5") != NULL, "s01");
    CHECK(strstr(msg, "'return'") != NULL, "s01");
    CHECK(strstr(msg, "esperado: ';'") != NULL, "s01");
    CompileCapture("exemplos/erros/s02_bloco_sem_declaracao.ssl", msg, sizeof msg);
    CHECK(strstr(msg, "esperado: 'var'\n") != NULL, "s02");

    printf("%d testes, %d falha(s)\n", nTests, nFailed);
    return nFailed != 0;
}
