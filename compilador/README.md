# Compilador SSL — VF (Sintático, Escopo e Tipos)

Continuação do trabalho da VC de Compiladores (Prof. Luiz Guedes – IME). A partir do analisador léxico da VC, este diretório implementa em **C99** a análise sintática ascendente la-LR(1), a análise de escopo e a análise de tipos da Simple Script Language (SSL).

**A geração de código (cap. 7) não faz parte desta entrega** e fica para um trabalho futuro. As inconsistências do cap. 7 aparecem na seção 7 só como registro.

```
analisador/   VC original, sem nenhuma alteração
compilador/   VF (este diretório)
  lexer.c/h       cópia do léxico da VC + 'return' + coluna (diff na seção 7)
  grammar.c/h     gramática em texto -> Left[], Len[], Rhs[]
  lalr.c/h        construção da Tabela de Ação la-LR(1)
  parser.c/h      leitura dos tokens + algoritmo da figura 4.6
  symtab.c/h      tabela de símbolos (cap. 5) e object estendido (cap. 6)
  semantics.c/h   pilha semântica e Semantics(rule) (caps. 4, 5 e 6)
  main.c          programa sslc
  testes.c        executável único de testes
  exemplos/       programas válidos, programas com erro, tokens.txt
```

## 1. Entendimento

| Fase | Capítulo | O que faz aqui |
|---|---|---|
| Léxica | 2 (VC) | `nextToken()` da VC lê o arquivo inteiro. Se houver erro léxico, os erros são listados e a análise sintática **não** é iniciada. |
| Sintática | 3.8 + 4.1 | O programa constrói a Tabela de Ação la-LR(1) a partir da gramática. O parser segue a figura 4.6: `Action[q][a]`, `Len[r]`, `Left[r]` e `Semantics(r)` a cada redução. O primeiro erro sintático encerra a análise. |
| Atributos | 4 | Pilha semântica paralela à sintática, com um nó `t_attrib` por não terminal (inclusive os marcadores). Os terminais não vão para a pilha: `IDD`, `IDU`, `NUM` etc. leem `tokenSecundario`, que é o token secundário do último terminal empilhado. |
| Escopo | 5 | Tabela de símbolos como pilha de níveis (`NewBlock`, `EndBlock`, `Define`, `Search`, `Find`), com os não terminais `IDD`/`IDU` e o marcador `NB`. |
| Tipos | 6 | Kinds, tipos pré-definidos, tipo universal, `CheckTypes`, marcadores `MF` e `MC`, e regras de declarações, comandos e expressões. |

Os erros de escopo e de tipo são **acumulados**: a análise não para no primeiro.

## 2. Gramática efetivamente usada

`./sslc --gramatica` imprime as 83 regras numeradas (a regra 0 é a aumentada). Diferenças em relação à gramática oficial (`Gramática.pdf`):

| Diferença | Justificativa |
|---|---|
| `P' → P $` | Gramática aumentada (cap. 3). Empilhar `$` a partir de `P' → P . $` é a aceitação (ACC). |
| `DC → DC ';' LI ':' T \| LI ':' T` | Na gramática, `'T'` entre aspas é o não terminal `T` (decisão da VC). |
| `LV → LV '.' ID` e `ID → Id` | O campo só pode ser resolvido pelo tipo da struct, então não há verificação de escopo (cap. 5, figura 6.22). A gramática oficial usa `IDU`. |
| `DT → 'type' IDD '=' 'struct' NB '{' DC '}'` | Marcador `NB`: os campos ficam em um nível próprio (figura 5.9). |
| `DF → 'function' IDD NB '(' LP ')' ':' T MF B` | `NB` antes dos parâmetros (5.10) e `MF` para preencher a função antes do corpo, o que permite recursão (6.13). |
| `S → NB B` (no lugar de `S → B`) | Bloco-comando com nível próprio (6.14). |
| `S → 'while' '(' E ')' ML S` e `S → 'do' ML S 'while' '(' E ')' ';'` | Marcador `ML` (extensão), que conta os laços abertos para verificar `break`/`continue`. |
| `F → IDU MC '(' LE ')'` | Marcador `MC`: verifica o kind e guarda a lista de parâmetros antes dos argumentos (6.23). |
| `S → 'return' E ';'` (extensão) | Sem `return` uma função não devolve valor. Como os parâmetros são por valor e não há globais, nenhum programa teria efeito observável. É a única alteração da linguagem, e exige a palavra reservada `return` no léxico. |
| `NB → ε`, `MF → ε`, `MC → ε`, `ML → ε` | Marcadores semânticos (cap. 5.1). |

Restrições da gramática oficial que foram **mantidas**, por não serem indispensáveis para a fase:

- `B → '{' LDV LS '}'`: todo bloco tem pelo menos uma declaração e um comando.
- `LP` e `LE` não são vazios: toda função tem pelo menos um parâmetro.
- Não existe comando-expressão: `x++;` é erro, mas `y = x++;` é válido.

## 3. Análise sintática la-LR(1)

**Construção** (`lalr.c`): os estados são calculados "diretamente, reaproveitando estados já existentes e adicionando novos look-aheads" (cap. 3.8):

1. FIRST e anuláveis dos não terminais, por ponto fixo.
2. `closure` LR(1). Os look-aheads são mapas de bits de 64 posições (há 50 terminais).
3. `goto` para cada símbolo. O núcleo obtido é ordenado e comparado só pela parte principal (regra, ponto). Se já existe um estado com o mesmo núcleo, os look-aheads são unidos (**fusão**) e o estado é reprocessado. Caso contrário, um novo estado é criado.
4. Repete até nenhum look-ahead mudar. Então `Action[q][x]` é preenchida: empilha/desvio (`p > 0`), reduz (`p < 0`), ACC ou erro (0).

**Estatísticas** (`./sslc --tabela`):

```
Tabela la-LR(1)
  regras: 83  terminais: 50  nao terminais: 34
  estados: 170  itens de nucleo: 257
  entradas: 463 empilha, 883 reduz, 320 desvio
  conflitos: 1 empilha/reduz, 0 reduz/reduz
  estado 156, 'else': empilha (mantido) x reduz por S -> 'if' '(' E ')' S
```

Para conferir, a mesma gramática foi passada ao bison (só como verificação, ele não faz parte do projeto). O bison dá 171 estados, porque cria um estado extra após `$end` que aqui é a ação ACC, e o mesmo único conflito.

**Else pendente**: o único conflito é empilha/reduz no `else`. Ele é resolvido por **empilhar**, de modo que o `else` pertence ao `if` mais interno. Um eventual conflito reduz/reduz seria resolvido pela regra de menor número; ele é contado e mostrado, mas não ocorre.

**Erros sintáticos**: como estados fundidos podem reduzir antes de detectar o erro, cada novo token é testado primeiro com `CanShift()`, que simula as reduções sem alterar a pilha. O erro é então acusado com a pilha intacta, e a lista de esperados é **exata**:

```
Erro sintatico na linha 5, coluna 5: token inesperado 'return' (TK_RETURN)
  esperado: ';', '&&', '||', '<', '>', '<=', '>=', '==', '!=', '+', '-', '*', '/'
```

## 4. Análise de escopo

- `object { nName, pNext, eKind, union _ }` e as funções das figuras 5.3 a 5.7. `SymbolTable[nível]` é uma lista encadeada com ponteiro para o último elemento.
- O nível 0 é aberto antes da análise e contém os tipos e as funções.
- `IDD → Id`: se `Search` acha o nome no nível corrente, reporta `ERR_REDECL`. `IDU → Id`: se `Find` não acha o nome em nenhum nível visível, reporta `ERR_NOT_DECL` e define o nome com kind `UNIVERSAL_`, para não repetir o erro nem gerar erros de tipo em cascata.
- `NB` (NewBlock) na struct, no bloco-comando e na função. O `EndBlock` correspondente é chamado nas reduções de `DT` struct, `S → NB B` e `DF`.
- Função: o nome fica no nível externo, e os parâmetros e as variáveis locais ficam no mesmo nível (figura 5.2). Isso permite recursão (o object da função é preenchido em `MF`) e deixa um bloco interno sombrear um parâmetro.

## 5. Análise de tipos

**Kinds**: `NO_KIND_DEF_, VAR_, PARAM_, FUNCTION_, FIELD_, ARRAY_TYPE_, STRUCT_TYPE_, ALIAS_TYPE_, SCALAR_TYPE_, UNIVERSAL_`. Os tipos pré-definidos são `pInteger, pChar, pBool, pString` (objects distintos com kind `SCALAR_TYPE_`), e há também `pUniversal`.

**Compatibilidade** (`CheckTypes`): por nome (figura 6.16), mas os aliases são resolvidos até o tipo base antes da comparação. Um tipo é compatível consigo mesmo e com o universal. Duas structs com campos iguais e nomes diferentes são incompatíveis (`exemplos/erros/e20`).

**Regras implementadas**:

- Declarações: var, param, field, array, struct, alias e function (6.5 a 6.13).
- Condições booleanas em if, while e do-while (6.15).
- Atribuição.
- Operadores lógicos, relacionais e aritméticos. O `+` aceita integer ou string (6.18 a 6.20).
- Fatores e incrementos (6.21).
- Left-values: campo, índice de array e índice de string, que dá `char` (6.22).
- Chamadas: kind, quantidade e tipo dos argumentos (6.23).
- `return`: o tipo da expressão deve ser compatível com o tipo de retorno da função corrente.

**Erros** (todos têm um caso de teste):

| Código | Situação |
|---|---|
| `ERR_REDECL` | identificador já declarado no nível corrente |
| `ERR_NOT_DECL` | identificador não declarado |
| `ERR_TYPE_EXPECTED` | `T → IDU` com algo que não é tipo |
| `ERR_BOOL_TYPE_EXPECTED` | condição, `&&` ou `\|\|` não booleano |
| `ERR_TYPE_MISMATCH` | atribuição, comparação ou operandos de tipos incompatíveis |
| `ERR_INVALID_TYPE` | operador aplicado a tipo inválido (`-`, `*`, `/`, `++`, `!`, `+` com boolean...) |
| `ERR_KIND_NOT_STRUCT` | `.campo` em algo que não é struct |
| `ERR_FIELD_NOT_DECL` | campo inexistente na struct |
| `ERR_KIND_NOT_ARRAY` | `[ ]` em algo que não é array nem string |
| `ERR_INVALID_INDEX_TYPE` | índice não inteiro |
| `ERR_KIND_NOT_VAR` | uso como valor de algo que não é variável nem parâmetro |
| `ERR_KIND_NOT_FUNC` | chamada de algo que não é função |
| `ERR_TOO_MANY_ARGS` / `ERR_TOO_FEW_ARGS` | quantidade de argumentos |
| `ERR_PARAM_TYPE` | tipo do n-ésimo argumento |
| `ERR_RETURN_TYPE` | (extensão) tipo do `return` |
| `ERR_NOT_IN_LOOP` | (extensão) `break`/`continue` fora de laço |

## 6. Geração de código

Fora do escopo desta entrega, conforme o enunciado. A estrutura já prevê o que o cap. 7 precisa: atributos por nó, marcadores e objects com kind. As decisões sobre o cap. 7 estão registradas abaixo para o trabalho futuro.

## 7. Decisões e ambiguidades

| # | Questão | Decisão |
|---|---|---|
| 1 | `'T'` entre aspas em `DC` | É o não terminal `T` (decisão da VC). |
| 2 | Acesso a campo: o cap. 5 usa `ID`, a gramática usa `IDU` | `ID`: o campo é resolvido pelo tipo da struct na regra de `LV`. |
| 3 | `return` não existe na gramática | Extensão `S → 'return' E ';'`, com a palavra reservada `return` no léxico e a verificação `ERR_RETURN_TYPE`. Não há verificação de que toda função retorna, pois isso exigiria análise de fluxo, que não está no livro. |
| 4 | `B` exige declaração e comando; os exemplos do livro violam isso | A gramática foi mantida. Os exemplos do cap. 7.4 foram reescritos (`exemplos/validos`), declarando variáveis nos blocos internos ou usando comandos simples. |
| 5 | Fig. 7.18 (`NewBlock` em `MF`) x texto (`NF` substitui `NB`) | Sem geração de código, `NF` e `NB` fazem o mesmo. Foi usado `NB` antes de `LP`, como nas figuras 5.10 e 6.13, porque os parâmetros precisam do bloco novo. |
| 6 | Universal com kind `SCALAR_TYPE_` (6.3) gera cascata | `pUniversal` tem kind `UNIVERSAL_`, e os `IDU` não declarados também recebem `UNIVERSAL_`. `exemplos/erros/e18` mostra só os 2 erros de origem. |
| 7 | Listas por `pNext` misturam parâmetros e locais | A lista de parâmetros termina no primeiro object que não é `PARAM_` (`NextParam`). As listas de `LI` param no primeiro object com kind já definido, como no livro. |
| 8 | Na figura 6.4, redeclaração sobrescreve o kind do original | Ao redeclarar, o erro é reportado e um novo object é inserido depois do original. As buscas continuam encontrando o original, e a lista `pNext` do `LI` não se quebra. |
| 9 | `CheckTypes` estrutural (6.17) entra em laço; o por nome (6.16) torna um alias incompatível com o próprio tipo base | Por nome, com resolução de aliases. Sem ponteiros não há tipos recursivos: `type A = A` dá `ERR_TYPE_EXPECTED`, porque `A` ainda não tem kind. |
| 10 | `break`/`continue` sem regras de atributos | Marcador `ML` conta os laços abertos, e fora de laço o erro é `ERR_NOT_IN_LOOP`. O destino do `continue` no do-while é assunto do cap. 7. |
| 11 | Mensagens precisam de coluna; a VC só tinha linha | Campo `coluna` no `Token`, preenchido por `readChar`/`unreadChar`. É a única outra mudança no léxico. |
| 12 | Literal `'\0'` dos exemplos do livro | O léxico da VC não tem sequências de escape (`c = ' any '`). `strcmp` e `strinv` recebem o comprimento como parâmetro. |
| 13 | `char - char` no `strcmp` do livro | Não é válido em SSL (char não é integer). O exemplo devolve -1, 0 ou 1 por comparação. |
| 14 | Exemplo do livro chama `fat` sem a função existir e atribui a `n` em `bSort` | Corrigido nos exemplos reescritos. |
| 15 | Fig. 7.27 (`LOAD_REF` com tamanho), 7.7/7.26 (`true`/`false`), `BEGIN_FUNC`, `CALL_LIB` para strings | Geração de código: trabalho futuro. Os literais já guardam `type`, `pos` e `val` (figura 6.7). |

Diff do léxico em relação à VC (`diff ../analisador/lexer.c lexer.c`): token `TK_RETURN` + entrada `"return"` na tabela de palavras reservadas + `tokenName`, e o campo `coluna` (variáveis `colunaAtual`, `colunaAnterior` e `colunaInicio`). Nada mais foi alterado.

**Revisão de cobertura**: todas as 83 regras têm um `case` em `Semantics`. As regras sem atributos (listas, `B`, `DE`) também, para manter a convenção de um nó por não terminal. `R_START` nunca é reduzida, porque é a ACC. Todas as regras de atributos das figuras 5.8 a 5.10 e 6.4 a 6.23 estão implementadas. A única omissão é a 6.17 (estrutural), substituída conforme a decisão 9.

## 8. Como compilar, executar e testar

```
cd compilador
make               # gera sslc e testes (gcc -Wall -Wextra -Wpedantic -std=c99, sem avisos)
make test          # testes da VF + programa da VC + comparação do modo --tokens com a VC

./sslc programa.ssl              # análise completa
./sslc --tokens programa.ssl     # saída idêntica à da VC
./sslc --tabela                  # estatísticas e conflitos da tabela la-LR(1)
./sslc --gramatica               # regras numeradas
```

`testes` (96 verificações) confere:

- a tabela tem 1 conflito empilha/reduz e 0 reduz/reduz;
- os 4 programas válidos compilam sem erro;
- cada programa de `exemplos/erros` dá o resultado e o código de erro esperados, com contagem exata;
- todo código de erro tem um caso;
- linha, coluna, token e lista de esperados aparecem na mensagem sintática.

O valgrind não acusa vazamentos nem acessos inválidos.

## 9. Exemplos

Válidos: `fatorial.ssl` e `strings.ssl` (exemplos 7.4.1 a 7.4.3 reescritos), `ordena.ssl` (7.4.4) e `tipos.ssl` (struct, alias, array de struct, recursão, sombreamento, else pendente, do-while, `break`/`continue`, concatenação).

Erros acumulados (`e19_varios_erros.ssl`):

```
function f(a: integer): integer
{
    var x: integer;
    x = 'c';
    if (x) x = y;
    return true;
}
```
```
Erro de tipo na linha 4: tipos incompativeis
Erro de escopo na linha 5: identificador 'y' nao declarado
Erro de tipo na linha 5: expressao do tipo boolean esperada
Erro de tipo na linha 6: valor de retorno incompativel com o tipo de 'f'
exemplos/erros/e19_varios_erros.ssl: 4 erro(s) de escopo/tipo
```

Sem cascata (`e18_sem_cascata.ssl`): `var v: Desconhecido;` seguido de `v.campo + v[0] * v`, `v = 'c'` e `g(v, 1)` produz só os 2 erros de origem (`Desconhecido` e `g` não declarados).

Léxico (`l01_lexico.ssl`): a análise sintática não é feita.

```
Erro lexico na linha 4, coluna 11: caractere/token invalido '@'
Erro lexico na linha 5, coluna 11: caractere/token invalido '_'
exemplos/erros/l01_lexico.ssl: 2 erro(s) lexico(s); analise sintatica nao realizada
```

Bloco sem declaração (`s02`):

```
Erro sintatico na linha 5, coluna 22: token inesperado 'x' (TK_ID)
  esperado: 'var'
```
