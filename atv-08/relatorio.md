# Relatório — Coerência de Cache e Falso Compartilhamento com OpenMP

**Disciplina:** Computação de Alto Desempenho
**Curso:** Tecnologia da Informação — 8º Período

---

## 1. Introdução

Programas paralelos podem sofrer degradação de desempenho por razões que vão além de condições de corrida explícitas. Dois fenômenos sutis — a **contenção no gerador de números aleatórios** e o **falso compartilhamento de cache** — podem tornar um programa paralelo mais lento que o esperado, mesmo quando o código aparenta estar correto.

Este relatório implementa quatro versões paralelas da estimativa de π pelo método de Monte Carlo, combinando duas estratégias de contagem e dois geradores de números aleatórios, para isolar e medir o impacto de cada fenômeno.

---

## 2. Fundamentação Teórica

### 2.1 Coerência de cache

Cada núcleo do processador possui seu próprio cache. Quando múltiplos núcleos leem a mesma região de memória, cada um mantém uma cópia local. Se um núcleo modifica sua cópia, o hardware precisa invalidar as cópias dos outros núcleos — forçando-os a recarregar os dados da memória RAM. Esse mecanismo é chamado de **coerência de cache** e tem um custo de desempenho quando ocorre frequentemente.

### 2.2 Falso compartilhamento

O cache não opera em bytes individuais — ele move blocos de memória chamados **cache lines**, tipicamente de 64 bytes. Se duas threads modificam variáveis diferentes que residem na mesma cache line, o hardware trata como se estivessem compartilhando dados e invalida o cache mutuamente — mesmo que logicamente não haja compartilhamento.

Isso é chamado de **falso compartilhamento**: o compartilhamento é físico (mesma cache line), não lógico (mesma variável).

Exemplo: um vetor `int acertos[6]` ocupa apenas 24 bytes — cabe inteiramente em uma cache line. Quando a thread 0 incrementa `acertos[0]`, o hardware invalida o cache de todos os outros núcleos, mesmo que cada um mexa apenas na sua posição.

### 2.3 Contenção no `rand()`

O `rand()` mantém um estado interno global compartilhado entre todas as threads. Quando múltiplas threads chamam `rand()` simultaneamente, elas competem por esse estado — uma precisa aguardar a outra terminar. Isso serializa parcialmente o loop paralelo.

Um gerador com estado privado por thread, como o `rand_r()` (POSIX) ou uma implementação própria, elimina essa contenção — cada thread opera sobre sua própria semente sem interferir nas demais.

### 2.4 Xorshift — gerador de estado privado

Na ausência do `rand_r()` no Windows, foi utilizada uma implementação de **xorshift**, um gerador de números pseudoaleatórios que recebe a semente por ponteiro e a atualiza a cada chamada:

```c
unsigned int meu_rand(unsigned int* semente) {
    *semente ^= *semente << 13;
    *semente ^= *semente >> 17;
    *semente ^= *semente << 5;
    return *semente;
}
```

Como a semente é uma variável privada de cada thread, não há estado compartilhado — equivalente funcional ao `rand_r()`.

---

## 3. Implementação

Foram implementadas quatro versões da estimativa de π com 100 milhões de pontos e 6 threads:

**Versão 1 — `rand()` com `acertos_local` e `critical`:**
Cada thread acumula acertos em uma variável privada local. Ao final do loop, soma em `contagem` com `critical` — chamado apenas uma vez por thread.

```c
#pragma omp parallel
{
    srand(omp_get_thread_num() + 1);
    int acertos_local = 0;

    #pragma omp for
    for (int i = 0; i < N; i++) {
        double x = (double)rand() / RAND_MAX;
        double y = (double)rand() / RAND_MAX;
        if (x*x + y*y <= 1.0) acertos_local++;
    }

    #pragma omp critical
    { contagem += acertos_local; }
}
```

**Versão 2 — `rand()` com vetor compartilhado:**
Cada thread escreve diretamente em `acertos[omp_get_thread_num()]`. A soma é feita em loop serial após a região paralela.

**Versão 3 — `meu_rand()` com `acertos_local` e `critical`:**
Igual à versão 1, substituindo `rand()` por `meu_rand(&semente)` com semente privada por thread.

**Versão 4 — `meu_rand()` com vetor compartilhado:**
Igual à versão 2, substituindo `rand()` por `meu_rand(&semente)`.

---

## 4. Resultados

| Versão | Gerador | Contagem | Tempo (s) | π estimado |
|--------|---------|----------|-----------|-----------|
| 1 | `rand()` | `acertos_local` + critical | 0.98 | 3.141382 |
| 2 | `rand()` | vetor `acertos[]` | 3.64 | 3.141382 |
| 3 | `meu_rand()` | `acertos_local` + critical | 0.87 | 3.141834 |
| 4 | `meu_rand()` | vetor `acertos[]` | 3.32 | 3.141834 |

---

## 5. Análise

### 5.1 Versão 2 vs Versão 1 — falso compartilhamento

A versão 2 foi **3.7× mais lenta** que a versão 1, apesar de não usar `critical` e não ter condição de corrida. O motivo é o falso compartilhamento: as 6 posições do vetor `acertos[6]` (24 bytes) cabem em uma única cache line de 64 bytes. Toda vez que uma thread incrementa sua posição, o hardware invalida o bloco inteiro no cache dos outros núcleos, forçando recarga constante da memória.

A versão 1, com `acertos_local` declarada dentro do `parallel`, mantém a variável em registrador ou cache L1 privado do núcleo — sem nenhuma invalidação entre threads.

### 5.2 Versão 3 vs Versão 1 — contenção no `rand()`

A versão 3 foi levemente mais rápida (0.87s vs 0.98s). A diferença vem da eliminação da contenção no estado global do `rand()`: com `meu_rand`, cada thread opera sobre sua própria semente sem precisar aguardar as demais. O `rand()` serializa parcialmente o loop mesmo em uma região paralela.

### 5.3 Versão 4 vs Versão 2 — dois problemas independentes

A versão 4 melhorou ligeiramente em relação à versão 2 (3.32s vs 3.64s) pela eliminação da contenção no gerador. No entanto, o falso compartilhamento do vetor continua presente e domina o desempenho. Os dois problemas são independentes: resolver um não resolve o outro.

### 5.4 Resumo dos fatores de impacto

| Problema | Causa | Impacto observado |
|----------|-------|-------------------|
| Falso compartilhamento | Vetor na mesma cache line | +3.7× no tempo |
| Contenção no `rand()` | Estado global compartilhado | +12% no tempo |

---

## 6. Conclusão

Os experimentos demonstram que o desempenho de programas paralelos pode ser severamente degradado por fatores invisíveis no código-fonte. O falso compartilhamento — causado por variáveis logicamente independentes que residem na mesma cache line — tornou a versão com vetor quase 4× mais lenta que a versão com variável local, mesmo sem qualquer condição de corrida.

A contenção no gerador `rand()` revelou um segundo gargalo: estado global compartilhado entre threads causa serialização implícita, independente de `critical` ou sincronização explícita. A substituição por um gerador de estado privado por thread eliminou esse problema.

A versão mais eficiente (versão 3) combina variável local por thread, `critical` chamado apenas uma vez por thread, e gerador sem estado compartilhado — eliminando ambas as fontes de degradação.

---

## Apêndice — Código Completo

```c
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <limits.h>

unsigned int meu_rand(unsigned int* semente) {
    *semente ^= *semente << 13;
    *semente ^= *semente >> 17;
    *semente ^= *semente << 5;
    return *semente;
}

int main() {
    int N = 100000000;
    int contagem, acertos[6] = {0};
    double start, end, pi;

    // --- Versão 1: rand() com acertos_local + critical ---
    contagem = 0;
    start = omp_get_wtime();
    #pragma omp parallel
    {
        srand(omp_get_thread_num() + 1);
        int acertos_local = 0;
        #pragma omp for
        for (int i = 0; i < N; i++) {
            double x = (double)rand() / RAND_MAX;
            double y = (double)rand() / RAND_MAX;
            if (x*x + y*y <= 1.0) acertos_local++;
        }
        #pragma omp critical
        { contagem += acertos_local; }
    }
    end = omp_get_wtime();
    pi = 4.0 * contagem / N;
    printf("[V1] rand() + local  | Tempo: %.3fs | pi: %f\n", end - start, pi);

    // --- Versão 2: rand() com vetor compartilhado ---
    contagem = 0;
    for (int i = 0; i < 6; i++) acertos[i] = 0;
    start = omp_get_wtime();
    #pragma omp parallel
    {
        srand(omp_get_thread_num() + 1);
        #pragma omp for
        for (int i = 0; i < N; i++) {
            double x = (double)rand() / RAND_MAX;
            double y = (double)rand() / RAND_MAX;
            if (x*x + y*y <= 1.0) acertos[omp_get_thread_num()]++;
        }
    }
    for (int i = 0; i < 6; i++) contagem += acertos[i];
    end = omp_get_wtime();
    pi = 4.0 * contagem / N;
    printf("[V2] rand() + vetor  | Tempo: %.3fs | pi: %f\n", end - start, pi);

    // --- Versão 3: meu_rand() com acertos_local + critical ---
    contagem = 0;
    start = omp_get_wtime();
    #pragma omp parallel
    {
        unsigned int semente = omp_get_thread_num() + 1;
        int acertos_local = 0;
        #pragma omp for
        for (int i = 0; i < N; i++) {
            double x = (double)meu_rand(&semente) / UINT_MAX;
            double y = (double)meu_rand(&semente) / UINT_MAX;
            if (x*x + y*y <= 1.0) acertos_local++;
        }
        #pragma omp critical
        { contagem += acertos_local; }
    }
    end = omp_get_wtime();
    pi = 4.0 * contagem / N;
    printf("[V3] meu_rand() + local | Tempo: %.3fs | pi: %f\n", end - start, pi);

    // --- Versão 4: meu_rand() com vetor compartilhado ---
    contagem = 0;
    for (int i = 0; i < 6; i++) acertos[i] = 0;
    start = omp_get_wtime();
    #pragma omp parallel
    {
        unsigned int semente = omp_get_thread_num() + 1;
        #pragma omp for
        for (int i = 0; i < N; i++) {
            double x = (double)meu_rand(&semente) / UINT_MAX;
            double y = (double)meu_rand(&semente) / UINT_MAX;
            if (x*x + y*y <= 1.0) acertos[omp_get_thread_num()]++;
        }
    }
    for (int i = 0; i < 6; i++) contagem += acertos[i];
    end = omp_get_wtime();
    pi = 4.0 * contagem / N;
    printf("[V4] meu_rand() + vetor | Tempo: %.3fs | pi: %f\n", end - start, pi);

    return 0;
}
```
