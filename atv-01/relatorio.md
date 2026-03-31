# Relatório — Multiplicação de Matriz por Vetor e Impacto da Cache

**Disciplina:** Computação de Alto Desempenho
**Curso:** Tecnologia da Informação — 8º Período

---

## 1. Introdução

A multiplicação de matriz por vetor (MxV) é uma operação fundamental em diversas áreas da computação, como computação científica, aprendizado de máquina e processamento de sinais. Embora matematicamente equivalentes, diferentes formas de percorrer os elementos de uma matriz durante essa operação podem gerar diferenças significativas de desempenho, mesmo produzindo o mesmo resultado.

Este relatório investiga o impacto do padrão de acesso à memória no tempo de execução da operação MxV, comparando duas implementações em C: uma que percorre a matriz por linhas e outra por colunas. Os experimentos foram realizados com matrizes quadradas de tamanhos variados, e os resultados são analisados sob a perspectiva da hierarquia de memória e do comportamento da memória cache.

---

## 2. Fundamentação Teórica

### 2.1 Hierarquia de Memória

Processadores modernos não acessam a RAM diretamente a cada operação — isso seria lento demais. Em vez disso, utilizam uma hierarquia de memórias intermediárias chamadas **caches** (L1, L2 e L3), que são menores, mas muito mais rápidas que a memória principal.

Quando o processador precisa de um dado, ele verifica primeiro a L1. Se não estiver lá (**cache miss**), busca na L2, depois na L3, e por fim na RAM. Cada nível mais distante tem uma penalidade de latência maior.

| Nível | Tamanho típico | Latência aproximada |
|-------|----------------|---------------------|
| L1    | 32–64 KB       | ~1 ns               |
| L2    | 256 KB – 1 MB  | ~5 ns               |
| L3    | 4–32 MB        | ~20 ns              |
| RAM   | GBs            | ~60–100 ns          |

### 2.2 Localidade de Cache e Row-Major Order

A linguagem C armazena matrizes bidimensionais em **row-major order**: os elementos de uma mesma linha ficam em posições contíguas na memória. Assim, ao acessar `matriz[i][0]`, `matriz[i][1]`, `matriz[i][2]`... o processador carrega um bloco contíguo de memória para a cache de uma só vez — fenômeno chamado de **prefetching**.

Quando o acesso é feito por colunas (`matriz[0][j]`, `matriz[1][j]`, `matriz[2][j]`...), os elementos ficam distantes na memória, forçando o processador a buscar novos blocos a cada acesso, gerando **cache misses** frequentes e degradando o desempenho.

---

## 3. Implementação

Foram implementadas duas funções de multiplicação MxV em C:

**Versão por linhas** — loop externo sobre linhas, interno sobre colunas. Acesso sequencial à memória, alinhado com o layout row-major do C:

```c
void multiplicarPorLinha(Dados d, int *result) {
    for (int i = 0; i < d.linhas; i++) {
        result[i] = 0;
        for (int j = 0; j < d.colunas; j++) {
            result[i] += d.matriz[i][j] * d.vetor[j];
        }
    }
}
```

**Versão por colunas** — loop externo sobre colunas, interno sobre linhas. Acesso não sequencial à memória, causando cache misses:

```c
void multiplicarPorColunas(Dados d, int *result) {
    for (int i = 0; i < d.linhas; i++) result[i] = 0;

    for (int j = 0; j < d.colunas; j++) {
        for (int i = 0; i < d.linhas; i++) {
            result[i] += d.matriz[i][j] * d.vetor[j];
        }
    }
}
```

A medição de tempo foi feita com a função `clock()` da biblioteca `<time.h>`, calculando o tempo de CPU consumido por cada função individualmente. Os testes foram executados com matrizes quadradas de tamanhos 100, 500, 1000, 2000 e 5000.

---

## 4. Resultados

| Tamanho da Matriz | Tempo por Linhas (s) | Tempo por Colunas (s) | Razão (col/lin) |
|:-----------------:|:--------------------:|:---------------------:|:---------------:|
| 100 × 100         | 0,000000             | 0,000000              | —               |
| 500 × 500         | 0,001000             | 0,001000              | 1,0×            |
| 1000 × 1000       | 0,002000             | 0,006000              | 3,0×            |
| 2000 × 2000       | 0,012000             | 0,028000              | 2,3×            |
| 5000 × 5000       | 0,064000             | 0,191000              | 3,0×            |

---

## 5. Análise dos Resultados

### 5.1 A partir de que tamanho os tempos divergem?

Os resultados mostram que até a matriz **500×500** os tempos são idênticos. A divergência se torna clara a partir de **1000×1000**, onde a versão por colunas passa a ser **3× mais lenta**.

Isso tem uma explicação direta relacionada ao tamanho da cache. Uma matriz 500×500 de inteiros ocupa:

> 500 × 500 × 4 bytes = **1 MB**

Uma matriz 1000×1000 ocupa:

> 1000 × 1000 × 4 bytes = **4 MB**

Matrizes de até 1 MB cabem inteiramente na cache L2/L3 do processador. Nesse caso, mesmo o acesso por colunas não sofre penalidade severa, pois os dados estão todos na cache. A partir de 4 MB, a matriz ultrapassa a capacidade da cache L2, e o acesso por colunas começa a gerar cache misses frequentes — cada acesso a `matriz[i][j]` passa a buscar dados na RAM, que é dezenas de vezes mais lenta.

### 5.2 Por que o acesso por linhas é mais rápido?

Na versão por linhas, o loop interno percorre `matriz[i][0]`, `matriz[i][1]`, `matriz[i][2]`... que estão em endereços contíguos na memória. O processador carrega um bloco desses dados para a cache de uma vez (cache line), e as próximas leituras encontram os dados já disponíveis — **alta taxa de cache hit**.

Na versão por colunas, o loop interno percorre `matriz[0][j]`, `matriz[1][j]`, `matriz[2][j]`... que estão separados por `colunas × 4 bytes` na memória. Para matrizes grandes, cada um desses acessos aponta para um endereço distante, forçando uma nova busca na RAM — **alta taxa de cache miss**.

### 5.3 Crescimento do impacto com o tamanho

A razão entre os tempos se mantém em torno de 2,3× a 3,0× para matrizes grandes. Esse valor não é constante porque outros fatores também influenciam, como o nível de cache atingido e o comportamento do prefetcher do processador. Em geral, espera-se que a diferença continue crescendo até o limite em que toda a matriz precise ser buscada da RAM a cada iteração.

---

## 6. Conclusão

O experimento demonstra de forma clara que o desempenho de um programa não depende apenas da sua lógica algorítmica, mas também de como ele acessa a memória. Duas implementações matematicamente equivalentes da operação MxV apresentaram diferença de até **3×** no tempo de execução, simplesmente pela ordem em que percorrem os elementos da matriz.

A divergência de desempenho se torna significativa a partir da matriz **1000×1000** (4 MB), ponto em que os dados ultrapassam a capacidade da cache L2 e o acesso por colunas começa a gerar cache misses frequentes com idas à RAM.

Esse resultado reforça a importância de considerar a **localidade de memória** no desenvolvimento de software de alto desempenho: algoritmos que respeitam o layout de memória da linguagem utilizada tendem a ser substancialmente mais eficientes em matrizes de grande porte.

---

## Apêndice — Código Completo

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int *vetor;
    int **matriz;
    int tamanho_vetor;
    int linhas;
    int colunas;
} Dados;

Dados criarDados(int tamanho_vetor, int linhas, int colunas) {
    Dados d;
    d.tamanho_vetor = tamanho_vetor;
    d.linhas = linhas;
    d.colunas = colunas;

    d.vetor = malloc(tamanho_vetor * sizeof(int));
    if (d.vetor == NULL) { printf("Erro ao alocar vetor."); exit(1); }

    d.matriz = malloc(linhas * sizeof(int *));
    if (d.matriz == NULL) { printf("Erro ao alocar matriz."); exit(1); }

    for (int i = 0; i < linhas; i++) {
        d.matriz[i] = malloc(colunas * sizeof(int));
        if (d.matriz[i] == NULL) { printf("Erro ao alocar linha %d.", i); exit(1); }
    }
    return d;
}

void liberarMemoria(Dados d) {
    free(d.vetor);
    for (int i = 0; i < d.linhas; i++) free(d.matriz[i]);
    free(d.matriz);
}

void preencherDados(Dados d) {
    srand(time(NULL));
    for (int i = 0; i < d.tamanho_vetor; i++) d.vetor[i] = rand() % 100;
    for (int i = 0; i < d.linhas; i++)
        for (int j = 0; j < d.colunas; j++)
            d.matriz[i][j] = rand() % 100;
}

void multiplicarPorLinha(Dados d, int *result) {
    for (int i = 0; i < d.linhas; i++) {
        result[i] = 0;
        for (int j = 0; j < d.colunas; j++)
            result[i] += d.matriz[i][j] * d.vetor[j];
    }
}

void multiplicarPorColunas(Dados d, int *result) {
    for (int i = 0; i < d.linhas; i++) result[i] = 0;
    for (int j = 0; j < d.colunas; j++)
        for (int i = 0; i < d.linhas; i++)
            result[i] += d.matriz[i][j] * d.vetor[j];
}

int main() {
    int tamanhos[] = {100, 500, 1000, 2000, 5000};
    int n = sizeof(tamanhos) / sizeof(tamanhos[0]);

    printf("%-15s %-15s %-15s\n", "Tamanho", "Linhas (s)", "Colunas (s)");
    printf("----------------------------------------------\n");

    for (int t = 0; t < n; t++) {
        int tam = tamanhos[t];
        Dados d = criarDados(tam, tam, tam);
        preencherDados(d);
        int *result = malloc(tam * sizeof(int));

        clock_t inicio = clock();
        multiplicarPorLinha(d, result);
        double tempo_linhas = (double)(clock() - inicio) / CLOCKS_PER_SEC;

        inicio = clock();
        multiplicarPorColunas(d, result);
        double tempo_colunas = (double)(clock() - inicio) / CLOCKS_PER_SEC;

        printf("%-15d %-15.6f %-15.6f\n", tam, tempo_linhas, tempo_colunas);

        free(result);
        liberarMemoria(d);
    }
    return 0;
}
```
