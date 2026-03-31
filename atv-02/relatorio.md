# Relatório — Pipeline, Vetorização e ILP em C

**Disciplina:** Computação de Alto Desempenho
**Curso:** Tecnologia da Informação — 8º Período

---

## 1. Introdução

Processadores modernos executam instruções de forma mais eficiente do que uma por vez. Técnicas como **pipeline**, **execução fora de ordem** e **vetorização SIMD** permitem que múltiplas operações aconteçam simultaneamente — fenômeno conhecido como **Paralelismo ao Nível de Instrução (ILP, do inglês *Instruction-Level Parallelism*)**.

No entanto, o código escrito pelo programador pode limitar ou facilitar o aproveitamento dessas capacidades. Este relatório investiga como o estilo do laço e o nível de otimização do compilador afetam o desempenho de três variações de laços em C, utilizando um vetor de 100 milhões de elementos do tipo `double`.

---

## 2. Fundamentação Teórica

### 2.1 Pipeline de Instruções

O pipeline divide a execução de uma instrução em estágios (busca, decodificação, execução, escrita). Enquanto uma instrução está em um estágio, a próxima já entra no estágio anterior — similar a uma linha de montagem. Isso aumenta o throughput, mas funciona melhor quando as instruções são independentes entre si.

### 2.2 Dependência entre Iterações (Loop-Carried Dependency)

Quando o resultado de uma iteração é necessário para calcular a próxima, temos uma **dependência entre iterações**. Nesse caso, o processador é forçado a esperar o resultado anterior antes de avançar, impedindo o uso pleno do pipeline e da vetorização.

Exemplo clássico:
```c
double soma = 0.0;
for (int i = 0; i < N; i++) {
    soma += v[i];  // cada iteração depende do valor de soma da anterior
}
```

### 2.3 Vetorização SIMD

Instruções SIMD (*Single Instruction, Multiple Data*) operam em múltiplos dados ao mesmo tempo. Por exemplo, uma instrução AVX pode somar 4 valores `double` simultaneamente. O compilador pode aplicar vetorização automaticamente quando não há dependências entre as iterações.

### 2.4 Níveis de Otimização do GCC

| Flag | Otimizações aplicadas |
|------|-----------------------|
| `-O0` | Nenhuma — código gerado diretamente do fonte |
| `-O2` | Pipeline, eliminação de código morto, unrolling parcial |
| `-O3` | Tudo do O2 + vetorização automática (SIMD) |

---

## 3. Implementação

Foram implementados três laços operando sobre um vetor de 100 milhões de elementos `double`:

**Loop 1 — Inicialização sem dependência:**
Cada posição do vetor recebe um valor calculado apenas a partir do índice `i`. Não há dependência entre iterações — totalmente paralelizável.

```c
for (int i = 0; i < N; i++) {
    v[i] = i * 2.0 + 1.0;
}
```

**Loop 2 — Soma acumulativa com dependência:**
A variável `soma` acumula o vetor elemento a elemento. Cada iteração depende do resultado da anterior — dependência explícita que limita o paralelismo.

```c
double soma = 0.0;
for (int i = 0; i < N; i++) {
    soma += v[i];
}
```

**Loop 3 — Soma com múltiplos acumuladores:**
A dependência é quebrada ao usar 4 acumuladores independentes (`s0`, `s1`, `s2`, `s3`), que acumulam partes diferentes do vetor e são somados ao final. O compilador e o processador podem trabalhar nos quatro fluxos em paralelo.

```c
double s0 = 0.0, s1 = 0.0, s2 = 0.0, s3 = 0.0;
for (int i = 0; i < N; i += 4) {
    s0 += v[i];
    s1 += v[i + 1];
    s2 += v[i + 2];
    s3 += v[i + 3];
}
soma = s0 + s1 + s2 + s3;
```

A medição de tempo foi feita com `clock()` da `<time.h>`, isolando cada laço individualmente. Os testes foram compilados com `-O0`, `-O2` e `-O3` usando GCC no Windows.

---

## 4. Resultados

| Loop                       | `-O0`   | `-O2`   | `-O3`   |
|:--------------------------|:-------:|:-------:|:-------:|
| Loop 1 — Inicialização    | 0,333 s | 0,125 s | 0,119 s |
| Loop 2 — Soma dependente  | 0,319 s | 0,088 s | 0,085 s |
| Loop 3 — Múlt. acumuladores | 0,104 s | 0,046 s | 0,042 s |

**Speedup em relação ao `-O0`:**

| Loop                       | Speedup O2 | Speedup O3 |
|:--------------------------|:----------:|:----------:|
| Loop 1 — Inicialização    | 2,66×      | 2,80×      |
| Loop 2 — Soma dependente  | 3,63×      | 3,75×      |
| Loop 3 — Múlt. acumuladores | 2,26×    | 2,48×      |

---

## 5. Análise dos Resultados

### 5.1 Loop 1 — Inicialização

O Loop 1 não tem dependências entre iterações: cada `v[i]` é calculado de forma independente. Com `-O0`, o compilador gera código direto, sem qualquer otimização. Com `-O2` e `-O3`, o compilador aplica *loop unrolling* e pode vetorizar as atribuições com instruções SIMD.

A diferença entre `-O2` e `-O3` é pequena (0,125 s vs 0,119 s), sugerindo que a vetorização adicional do `-O3` ajuda, mas o gargalo principal é o tempo de escrita na memória, que não é eliminado por nenhum nível de otimização.

### 5.2 Loop 2 — Soma com Dependência

Este é o laço mais restrito em termos de ILP. A dependência `soma += v[i]` cria uma cadeia sequencial: a instrução de adição da iteração `i+1` só pode começar após a iteração `i` terminar completamente.

Ainda assim, o ganho de `-O0` para `-O2` foi expressivo (3,63×). Isso ocorre porque mesmo sem vetorizar a redução, o compilador com `-O2` reorganiza melhor as instruções, elimina overhead de função e aloca variáveis em registradores em vez de memória. A diferença entre `-O2` e `-O3` é mínima (0,088 s vs 0,085 s), confirmando que a dependência impede ganhos adicionais com vetorização.

### 5.3 Loop 3 — Múltiplos Acumuladores

O resultado mais expressivo vem da comparação direta entre Loop 2 e Loop 3 **no mesmo nível de otimização**:

| Comparação                    | Razão       |
|------------------------------|-------------|
| Loop 3 vs Loop 2 em `-O0`    | **3,07× mais rápido** |
| Loop 3 vs Loop 2 em `-O3`    | **2,02× mais rápido** |

Ao usar 4 acumuladores independentes, as 4 adições dentro do laço não dependem umas das outras — o processador pode executá-las em paralelo, aproveitando plenamente o pipeline de ponto flutuante. Isso é ILP manual: o programador expõe o paralelismo que o compilador não consegue extrair sozinho quando há dependência explícita.

Com `-O3`, o compilador ainda consegue vetorizar com SIMD, reduzindo o tempo para 0,042 s. Esse é o melhor resultado do experimento — uma combinação de ILP manual (múltiplos acumuladores) com vetorização automática do compilador.

Vale destacar que o Loop 3 com `-O0` (0,104 s) já é **mais rápido** que o Loop 2 com `-O2` (0,088 s). Isso demonstra que a estrutura do código tem impacto tão grande quanto o nível de otimização do compilador.

---

## 6. Conclusão

Os experimentos demonstram que o desempenho de laços em C é determinado por dois fatores combinados: a estrutura do código e as otimizações do compilador.

A dependência entre iterações (Loop 2) é o principal limitador de desempenho, impedindo que o processador use seu pipeline e as unidades de execução paralelas de forma eficiente. Ao quebrar essa dependência com múltiplos acumuladores (Loop 3), o programa ficou até 3× mais rápido sem nenhuma mudança no nível de otimização — apenas com uma reorganização do laço.

Os níveis `-O2` e `-O3` entregam ganhos significativos, mas com retornos decrescentes: a maior parte do ganho vem do `-O2`, e o `-O3` acrescenta pouco quando o gargalo é uma dependência estrutural no código. Isso reforça que escrever código com boas propriedades de ILP — evitando dependências desnecessárias entre iterações — é tão importante quanto escolher o nível de otimização correto.

---

## Apêndice — Código Completo

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 100000000  // 100 milhões de elementos

int main() {
    double *v = malloc(N * sizeof(double));

    if (v == NULL) {
        printf("Erro ao alocar memória.\n");
        return 1;
    }

    clock_t inicio, fim;
    double tempo;

    // Loop 1: Inicialização sem dependência entre iterações
    inicio = clock();
    for (int i = 0; i < N; i++) {
        v[i] = i * 2.0 + 1.0;
    }
    fim = clock();
    tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
    printf("Loop 1 (inicializacao):           %.4f s\n", tempo);

    // Loop 2: Soma acumulativa com dependência entre iterações
    double soma = 0.0;
    inicio = clock();
    for (int i = 0; i < N; i++) {
        soma += v[i];
    }
    fim = clock();
    tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
    printf("Loop 2 (soma dependente):         %.4f s  (resultado: %.0f)\n", tempo, soma);

    // Loop 3: Soma com múltiplos acumuladores (quebra a dependência)
    double s0 = 0.0, s1 = 0.0, s2 = 0.0, s3 = 0.0;
    inicio = clock();
    for (int i = 0; i < N; i += 4) {
        s0 += v[i];
        s1 += v[i + 1];
        s2 += v[i + 2];
        s3 += v[i + 3];
    }
    soma = s0 + s1 + s2 + s3;
    fim = clock();
    tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
    printf("Loop 3 (multiplos acumuladores):  %.4f s  (resultado: %.0f)\n", tempo, soma);

    free(v);
    return 0;
}
```
