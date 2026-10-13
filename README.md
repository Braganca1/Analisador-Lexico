# Compiladores – SSL (VC + VF)

- `analisador/` – VC: analisador léxico em C99, **sem nenhuma alteração** (ver `analisador/README.md`).
- `compilador/` – VF: análise sintática la-LR(1), escopo e tipos, reaproveitando o léxico da VC (ver `compilador/README.md`).

```
cd compilador && make && make test
./sslc exemplos/validos/tipos.ssl
```

Autores: Bragança e Lanerson
