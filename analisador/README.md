# Analisador Léxico

Trabalho da disciplina de Compiladores: implementação de um Analisador Léxico em C para a gramática fornecida.

## Implementação

O analisador foi construído em C (padrão C99) seguindo o roteiro proposto no trabalho:

1. **Enumeração de tokens** ([lexer.h:14-72](lexer.h#L14-L72)) — `TokenType` cobre palavras reservadas, símbolos/operadores, os tokens regulares (`TK_ID`, `TK_NUMERAL`, `TK_CHARACTER`, `TK_STRINGVAL`) e os tokens especiais `TK_EOF`/`TK_ERROR`.

2. **`searchKeyWord`** ([lexer.c:81-89](lexer.c#L81-L89)) — busca sequencial em `tabelaPalavrasReservadas`, um vetor fixo com as 18 palavras reservadas da linguagem. Retorna o `TokenType` correspondente ou `-1` se o lexema não for reservado.

3. **Token secundário / tabela de identificadores** ([lexer.c:96-158](lexer.c#L96-L158)) — quando `searchKeyWord` não encontra o lexema, ele é tratado como identificador (`TK_ID`). Uma tabela hash (`tabelaNomes`, 211 posições, hash djb2, colisões por encadeamento) associa cada identificador único a um "nome" numérico: a ordem em que foi lido pela primeira vez (`contadorIds`).

4. **`searchName`** ([lexer.c:119-143](lexer.c#L119-L143)) — busca o identificador na tabela hash; se já existir, devolve o token secundário existente; caso contrário, insere e devolve um novo.

5. **`t_const`** ([lexer.h:100-109](lexer.h#L100-L109)) — struct com um `union` (`charVal`, `intVal`, `strVal`) e uma tag `TipoConst` (`CONST_CHAR`, `CONST_INT`, `CONST_STRING`), usada para os literais associados a `TK_CHARACTER`, `TK_NUMERAL` e `TK_STRINGVAL`.

6. **Tabela de constantes** ([lexer.c:163-222](lexer.c#L163-L222)) — vetor `tabConst[MAX_CONST]` (1024 posições); `nNumConsts` controla o tamanho ocupado e a próxima posição livre, que é usada diretamente como token secundário do literal.

7. **`addCharConst` / `addIntConst` / `addStringConst`** ([lexer.c:169-200](lexer.c#L169-L200)) — inserem uma constante na tabela e retornam sua posição.

8. **`getCharConst` / `getIntConst` / `getStringConst`** ([lexer.c:202-212](lexer.c#L202-L212)) — recuperam o valor armazenado a partir da posição (token secundário).

9. **`readChar` / `unreadChar`** ([lexer.c:35-45](lexer.c#L35-L45)) — leitura caractere a caractere do arquivo-fonte via `fgetc`, com contagem de linha, e lookahead de 1 caractere via `ungetc`.

10. **`nextToken`** ([lexer.c:239-408](lexer.c#L239-L408)) — função central do lexer: ignora espaços em branco, reconhece identificadores/palavras reservadas, números, literais de caractere/string e símbolos (incluindo os compostos de 2 caracteres, como `==`, `!=`, `<=`, `>=`, `&&`, `||`, `++`, `--`), devolvendo um `Token` com o token principal, o token secundário (`-1` quando não se aplica), a linha e o lexema.

## Tratamento de erros

`nextToken` devolve `TK_ERROR` para: caractere de literal não fechado (`'` sem `'` de fechamento), string não fechada até o fim do arquivo, e qualquer caractere não reconhecido pela linguagem (ex.: `&` ou `|` isolados, sem o par). O erro é reportado em `stderr` com o número da linha ([main.c:31-34](main.c#L31-L34)), e a análise **continua** até `TK_EOF` — um erro léxico não interrompe o programa.

## Limitações conhecidas

A gramática usada não define `_` como parte de identificadores válidos (`ID = letra . (letra + digito)*`), então o lexer não o aceita. Isso é visível ao rodar o próprio [entrada.txt](entrada.txt) fornecido: a linha `return_val = 'x';` é lida como `TK_ID "return"`, seguido de um `TK_ERROR` no caractere `_`, seguido de `TK_ID "val"` — e um erro léxico é reportado na linha 15. A gramática também não define uma palavra reservada `return`; a linguagem não possui comando de retorno explícito.

## Como compilar e rodar

Com o Makefile (gcc/MinGW, Linux ou WSL):

```
make          # compila e gera lexer_test
make run      # equivale a ./lexer_test entrada.txt
make clean    # remove objetos e o executável
```

Manualmente, com gcc:

```
gcc -Wall -Wextra -std=c99 -g -o lexer_test main.c lexer.c
./lexer_test entrada.txt
```

O programa aceita opcionalmente o caminho do arquivo-fonte como argumento (`./lexer_test caminho.txt`); sem argumento, usa `entrada.txt` por padrão.

### Saída (trecho, rodando sobre `entrada.txt`)

```
Linha  Token          Lexema          TokenSec
--------------------------------------------------------
1      TK_TYPE        type            -
1      TK_ID          Ponto           0
1      TK_ASSIGN      =               -
1      TK_STRUCT      struct          -
1      TK_LBRACE      {               -
...
15     TK_ID          return          7
15     TK_ERROR       _               -
15     TK_ID          val             8
15     TK_ASSIGN      =               -
15     TK_CHARACTER   'x'             1
...
20     TK_EOF         EOF             -

Erro lexico na linha 15: caractere/token invalido '_'
```

## Prompt utilizado

> Escreva um analisador léxico para a gramática do pdf. Para isso, siga as seguintes orientações
> 1. Defina uma enumeração para os tokens (palavras reservadas, símbolos, tokens regulares e um para token desconhecido)
> 2. Uma função searchKeyWord para buscar palavras reservadas na tabela de palavras reservadas
> 3. ao concluir que o token retornado não é uma palavra reservada, determine o token secundário. Uma tabela de identificadores deve ser criada para associar a cada identificador o seu token secundário. Esse token secundário, chamado de 'nome', pode ser apenas um número inteiro representando a ordem em que foi lido do arquivo de entrada. Cada identificador deve ocorrer apenas uma vez nesta tabela e estar associado ao token que lhe foi atribuído. Um algoritmo de Hash é recomendado para esta tabela.
> 4. Para buscar identificadores na tabela de nomes, crie uma função searchName
> 5. Os literais que estão no programa representam as constantes e estão associados aos tokens CHARACTER, NUMERAL e STRINGVAL. Para isso, basta criar um struct que englobe as três opções a partir de uma união.
> 6. As constantes podem ser armazenadas em um vetor de t_const, e sua posição já será o seu token secundário, bastando criar uma variavel nNumConsts para constrolar o tamanho.
> 7. Para inserir as constantes e retornar a posição, use as funções addCharConst, addIntConst e addStringConst
> 8. Para recuperar basta criar funções get
> 9. Para ler os caracteres do arquivo, crie a função readChar
> 10. Crie uma função nextToken para devolver o token lido e e token secundário
> 11. Crie usando a linguagem C/C++

## Autores

Bragança e Lanerson
