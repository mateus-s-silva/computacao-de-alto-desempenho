# Relatório — Memory Bound vs Compute Bound com OpenMP

**Disciplina:** Computação de Alto Desempenho
**Curso:** Tecnologia da Informação — 8º Período

---

## 1. Introdução

Programas paralelos nem sempre se beneficiam da mesma forma ao aumentar o número de threads. O comportamento depende fundamentalmente do tipo de gargalo do programa: se o limite está na capacidade de **acesso à memória** (*memory bound*) ou na capacidade de **processamento da CPU** (*compute bound*).

Este relatório implementa dois programas paralelos em C com OpenMP — um memory bound e um compute bound — e avalia o desempenho de 1 a 6 threads, analisando speedup, eficiência e o comportamento do multithreading em cada caso.

---

## 2. Fundamentação Teórica

### 2.1 Memory Bound

Um programa é dito *memory bound* quando o gargalo está no tempo de busca de dados na memória RAM, e não no tempo de processamento. A CPU executa a operação rapidamente, mas fica ociosa aguardando os dados chegarem.

O canal entre a memória RAM e o processador tem uma capacidade máxima de dados por segundo, chamada de **largura de banda** (*bandwidth*). Quando esse canal está saturado, adicionar mais threads não acelera o programa — todas as threads competem pelo mesmo recurso limitado.

### 2.2 Compute Bound

Um programa é *compute bound* quando o gargalo está na capacidade de cálculo da CPU. Os dados chegam da memória rapidamente (ou ficam no cache), mas cada elemento exige muitas operações matemáticas pesadas — como raiz quadrada, seno e cosseno.

Nesse caso, o fator limitante é o número de **unidades de cálculo de ponto flutuante** disponíveis. Cada núcleo físico possui uma dessas unidades. Quando duas threads são alocadas no mesmo núcleo físico (*hyperthreading*), elas competem pela mesma unidade, reduzindo o ganho esperado.

### 2.3 Speedup e Eficiência

**Speedup** mede quantas vezes o programa ficou mais rápido com `n` threads em relação a 1 thread:

```
Speedup(n) = Tempo com 1 thread / Tempo com n threads
```

**Eficiência** mede o aproveitamento real de cada thread adicionada:

```
Eficiência(n) = Speedup(n) / n
```

Uma eficiência de 1.0 (100%) significa paralelismo perfeito. Na prática, sempre fica abaixo disso devido a overhead e competição por recursos.

### 2.4 OpenMP

OpenMP é uma interface de programação que permite paralelizar loops em C com uma única diretiva de compilador. O trecho `#pragma omp parallel for` distribui as iterações do loop entre os threads disponíveis. A cláusula `reduction(+:variavel)` é necessária quando threads acumulam resultados em uma variável compartilhada, garantindo que cada thread trabalhe com sua própria cópia e os resultados sejam somados ao final.

---

## 3. Implementação

Foram implementados dois programas operando sobre vetores de 100 milhões de elementos `double`, testados com 1 a 6 threads, com 3 execuções por configuração para calcular a média.

**Programa 1 — Memory Bound (soma de vetores):**
Operação simples (`c[i] = a[i] + b[i]`) sobre três vetores grandes. A operação é trivial, mas exige leitura de dois vetores e escrita em um terceiro — muitos acessos à memória RAM por iteração.

```c
#pragma omp parallel for
for (int i = 0; i < n; i++) {
    c[i] = a[i] + b[i];
}
```

**Programa 2 — Compute Bound (cálculos matemáticos intensivos):**
Operações pesadas (`sqrt`, `sin`, `cos`) sobre cada elemento de um vetor. O resultado é acumulado em uma única variável, que permanece no cache — o gargalo é o tempo de cálculo, não a memória.

```c
#pragma omp parallel for reduction(+:resultado)
for (int i = 0; i < n; i++) {
    resultado += sqrt(a[i]) + sin(a[i]) + cos(a[i]);
}
```

A medição de tempo foi feita com `omp_get_wtime()`, que mede o tempo real decorrido (tempo de parede), apropriado para programas com múltiplas threads — ao contrário de `clock()`, que soma o tempo de CPU de todas as threads.

---

## 4. Resultados

### 4.1 Memory Bound

| Threads | Tempo médio (s) | Speedup | Eficiência |
|---------|----------------|---------|------------|
| 1 | 0.635 | 1.00x | 100% |
| 2 | 0.309 | 2.05x | 103% |
| 3 | 0.202 | 3.13x | 104% |
| 4 | 0.158 | 4.01x | 100% |
| 5 | 0.149 | 4.24x | 85% |
| 6 | 0.146 | 4.35x | 72% |

### 4.2 Compute Bound

| Threads | Tempo médio (s) | Speedup | Eficiência |
|---------|----------------|---------|------------|
| 1 | 3.750 | 1.00x | 100% |
| 2 | 2.128 | 1.76x | 88% |
| 3 | 1.419 | 2.64x | 88% |
| 4 | 1.099 | 3.41x | 85% |
| 5 | 0.894 | 4.20x | 84% |
| 6 | 0.832 | 4.51x | 75% |

---

## 5. Análise dos Resultados

### 5.1 Memory Bound — saturação do canal de memória

O programa memory bound apresentou speedup próximo do ideal até 4 threads (4.01x), com eficiência de 100%. Isso indica que, até esse ponto, cada thread adicional conseguiu trabalhar de forma produtiva sem contenção excessiva.

A partir de 5 threads, o ganho começa a desacelerar: de 4 para 5 threads o tempo caiu apenas de 0.158 s para 0.149 s, e de 5 para 6 threads a melhora foi praticamente imperceptível. Isso indica que o canal de memória RAM estava saturado — todas as threads competem pelo mesmo barramento, e adicionar mais threads não aumenta a capacidade do canal, apenas gera mais competição.

### 5.2 Compute Bound — competição pela unidade de cálculo

O programa compute bound escalou de forma mais gradual, com eficiência consistente em torno de 85–88% até 4 threads. O salto mais expressivo ocorreu entre 4 e 5 threads (de 3.41x para 4.20x), o que pode ser explicado pela alocação das threads em núcleos físicos distintos: com 5 threads em um processador de 6 núcleos, é mais provável que cada thread tenha acesso exclusivo à sua unidade matemática de ponto flutuante.

Com menos threads, o sistema operacional pode alocar duas threads no mesmo núcleo físico (*hyperthreading*). Como as operações `sqrt`, `sin` e `cos` são pesadas e utilizam intensamente a unidade de ponto flutuante, duas threads no mesmo núcleo competem por esse recurso — uma precisa aguardar a outra, reduzindo o ganho real abaixo do esperado.

### 5.3 Comparação entre os dois programas

| Threads | Speedup Memory Bound | Speedup Compute Bound |
|---------|---------------------|-----------------------|
| 2 | 2.05x | 1.76x |
| 3 | 3.13x | 2.64x |
| 4 | 4.01x | 3.41x |
| 5 | 4.24x | 4.20x |
| 6 | 4.35x | 4.51x |

Nos primeiros threads, o memory bound escalou melhor que o compute bound. Isso parece contraintuitivo, mas faz sentido: a operação de soma é tão simples que as threads quase não competem pela unidade matemática, e o gargalo (canal de memória) ainda não estava saturado. O compute bound, por outro lado, sobrecarrega a unidade de cálculo desde o início.

Com 6 threads, o compute bound alcançou o maior speedup (4.51x vs 4.35x), pois com mais núcleos físicos disponíveis a competição pela unidade de ponto flutuante diminui.

---

## 6. Métricas de Avaliação

**Speedup** é a métrica principal para ambos os casos: mede diretamente o ganho real de performance com o aumento de threads.

**Eficiência** complementa o speedup ao revelar se as threads adicionais estão sendo bem aproveitadas ou gerando mais overhead do que ganho.

Para programas **memory bound**, a métrica mais reveladora é a **largura de banda utilizada** (dados lidos e escritos por segundo). Quando ela se aproxima do limite do hardware, o speedup estabiliza independentemente de quantas threads forem adicionadas.

Para programas **compute bound**, a métrica mais relevante é a **taxa de operações de ponto flutuante por segundo**. O teto de desempenho é determinado pelo número de núcleos físicos — não de threads lógicas — pois cada núcleo tem uma única unidade de cálculo.

---

## 7. Conclusão

Os experimentos confirmam que o comportamento de programas paralelos depende fundamentalmente do tipo de gargalo.

Programas memory bound saturam o canal de memória com poucos threads. Adicionar mais threads além desse ponto não traz ganho significativo e pode até introduzir overhead. O multithreading ajuda até o ponto de saturação da memória, mas não resolve o gargalo estrutural.

Programas compute bound se beneficiam do aumento de threads até o limite de núcleos físicos disponíveis. O *hyperthreading* — duas threads no mesmo núcleo — atrapalha nesses casos porque as threads competem pela mesma unidade de cálculo de ponto flutuante. O ganho real só se materializa quando cada thread tem acesso exclusivo ao seu núcleo.

Em ambos os casos, a eficiência diminui com muitas threads, reforçando que paralelismo tem retornos decrescentes — e que identificar o gargalo correto é essencial antes de otimizar.

---

## Apêndice — Código Completo

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define VECTOR_SIZE 100000000
#define THREADS_NUMBER 6

double memory_bound(double* a, double* b, double* c, int n, int threads) {
    omp_set_num_threads(threads);

    double inicio = omp_get_wtime();

    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }

    double fim = omp_get_wtime();

    printf("Threads: %d | Tempo: %.4f s\n", threads, fim - inicio);

    return fim - inicio;
}

double compute_bound(double* a, int n, int threads) {
    omp_set_num_threads(threads);

    double resultado = 0;

    double inicio = omp_get_wtime();

    #pragma omp parallel for reduction(+:resultado)
    for (int i = 0; i < n; i++) {
        resultado += sqrt(a[i]) + sin(a[i]) + cos(a[i]);
    }

    double fim = omp_get_wtime();

    printf("Threads: %d | Tempo: %.4f s\n", threads, fim - inicio);

    return fim - inicio;
}

int main() {
    double* vector_A = malloc(VECTOR_SIZE * sizeof(double));
    double* vector_B = malloc(VECTOR_SIZE * sizeof(double));
    double* vector_C = malloc(VECTOR_SIZE * sizeof(double));

    double one_thread_time;

    for (int i = 0; i < VECTOR_SIZE; i++) {
        vector_A[i] = (double)i;
        vector_B[i] = (double)i;
    }

    printf("\n--- MEMORY BOUND ---\n");
    for (int i = 1; i <= THREADS_NUMBER; i++) {
        double tempo = 0;
        for (int t = 0; t < 3; t++) {
            tempo += memory_bound(vector_A, vector_B, vector_C, VECTOR_SIZE, i);
        }
        if (i == 1) one_thread_time = tempo / 3;
        else printf("speedup(%d): %.4fx\n", i, one_thread_time / (tempo / 3));
    }

    printf("\n--- COMPUTE BOUND ---\n");
    for (int i = 1; i <= THREADS_NUMBER; i++) {
        double tempo = 0;
        for (int t = 0; t < 3; t++) {
            tempo += compute_bound(vector_A, VECTOR_SIZE, i);
        }
        if (i == 1) one_thread_time = tempo / 3;
        else printf("speedup(%d): %.4fx\n", i, one_thread_time / (tempo / 3));
    }

    return 0;
}
```
