# Relatório — Regiões Críticas Nomeadas e Travas Explícitas com OpenMP

**Disciplina:** Computação de Alto Desempenho
**Curso:** Tecnologia da Informação — 8º Período

---

## 1. Introdução

Em programas paralelos com múltiplos recursos compartilhados, a granularidade da sincronização é tão importante quanto a sua presença. Proteger recursos independentes com a mesma trava serializa operações desnecessariamente. Este relatório implementa inserções paralelas em listas encadeadas usando regiões críticas nomeadas para dois recursos e locks explícitos para um número arbitrário de listas definido em tempo de execução.

---

## 2. Fundamentação Teórica

### 2.1 Regiões críticas sem nome

O `#pragma omp critical` sem nome usa uma trava global única. Qualquer bloco `critical` sem nome bloqueia todos os outros, independente de protegerem recursos diferentes:

```c
#pragma omp critical
{ inserir(&lista_A, i); }  // bloqueia...

#pragma omp critical
{ inserir(&lista_B, i); }  // ...este também
```

Mesmo que lista_A e lista_B sejam independentes, as inserções são serializada globalmente.

### 2.2 Regiões críticas nomeadas

Nomes diferentes criam travas independentes. Inserções em listas distintas não se bloqueiam mutuamente:

```c
#pragma omp critical(lista_A)
{ inserir(&lista_A, i); }  // trava só lista_A

#pragma omp critical(lista_B)
{ inserir(&lista_B, i); }  // trava independente
```

Os nomes são resolvidos em **tempo de compilação** — precisam estar escritos no código fonte.

### 2.3 Locks explícitos (`omp_lock_t`)

Quando o número de recursos é definido em tempo de execução, nomes de `critical` não podem ser gerados dinamicamente. O `omp_lock_t` é uma variável que representa uma trava, criada e gerenciada em tempo de execução:

| Operação | Função |
|----------|--------|
| Criar | `omp_init_lock(&trava)` |
| Travar | `omp_set_lock(&trava)` |
| Destravar | `omp_unset_lock(&trava)` |
| Destruir | `omp_destroy_lock(&trava)` |

Um array de `omp_lock_t` permite proteger N recursos independentes com N travas distintas, onde N é conhecido apenas em tempo de execução.

---

## 3. Implementação

### 3.1 Estrutura da lista encadeada

```c
typedef struct No {
    int valor;
    struct No* proximo;
} No;

void inserir(No** lista, int valor) {
    No* no = malloc(sizeof(No));
    no->valor = valor;
    no->proximo = *lista;
    *lista = no;
}
```

A inserção é feita no início da lista. O parâmetro `No**` permite modificar o ponteiro da cabeça dentro da função.

### 3.2 Parte 1 — Duas listas com critical nomeado

20 tarefas são criadas. Cada uma escolhe aleatoriamente entre lista_A e lista_B e insere usando a trava correspondente:

```c
#pragma omp parallel
{
    #pragma omp single
    {
        for (int i = 0; i < N; i++) {
            #pragma omp task firstprivate(i)
            {
                int escolha = rand() % 2;
                if (escolha == 0) {
                    #pragma omp critical(lista_A)
                    { inserir(&lista_A, i); }
                } else {
                    #pragma omp critical(lista_B)
                    { inserir(&lista_B, i); }
                }
            }
        }
    }
}
```

### 3.3 Parte 2 — N listas com locks explícitos

O usuário define o número de listas em tempo de execução. Um array de travas é criado dinamicamente:

```c
No** listas = calloc(num_listas, sizeof(No*));
omp_lock_t* travas = malloc(num_listas * sizeof(omp_lock_t));
for (int i = 0; i < num_listas; i++)
    omp_init_lock(&travas[i]);

#pragma omp parallel
{
    #pragma omp single
    {
        for (int i = 0; i < N; i++) {
            #pragma omp task firstprivate(i)
            {
                int escolha = rand() % num_listas;
                omp_set_lock(&travas[escolha]);
                inserir(&listas[escolha], i);
                omp_unset_lock(&travas[escolha]);
            }
        }
    }
}
```

---

## 4. Por que critical nomeado não é suficiente para N listas

Regiões críticas nomeadas são construções de **tempo de compilação**. O compilador resolve os nomes antes do programa executar — é impossível gerar nomes dinamicamente:

```c
// IMPOSSÍVEL em C:
for (int i = 0; i < N; i++) {
    #pragma omp critical("lista_" + i)  // nomes não podem ser gerados em runtime
    { inserir(&listas[i], valor); }
}
```

Mesmo que fosse possível gerar os nomes, o compilador não saberia quantas travas distintas criar, pois `N` só é conhecido durante a execução.

O `omp_lock_t` resolve isso porque é uma variável comum — pode ser alocada em array, inicializada em loop, e indexada dinamicamente:

```c
omp_lock_t travas[N];  // N conhecido em runtime
omp_set_lock(&travas[i]);  // i calculado em runtime
```

Portanto, para qualquer programa onde o número de recursos protegidos depende de entrada do usuário, configuração ou dados, locks explícitos são a única solução viável.

---

## 5. Conclusão

Regiões críticas nomeadas oferecem sincronização de granularidade fina para um número fixo e conhecido de recursos — evitando que a proteção de um recurso bloqueie outro independente. Para um número variável de recursos definido em tempo de execução, locks explícitos (`omp_lock_t`) são necessários, pois permitem criar e gerenciar travas como variáveis comuns, com total flexibilidade em tempo de execução.

A combinação de `#pragma omp task` com locks explícitos oferece paralelismo flexível e sincronização precisa — cada tarefa protege apenas o recurso que de fato vai modificar, sem bloqueios desnecessários.

---

## Apêndice — Código Completo

```c
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

typedef struct No {
    int valor;
    struct No* proximo;
} No;

void inserir(No** lista, int valor) {
    No* no = malloc(sizeof(No));
    no->valor = valor;
    no->proximo = *lista;
    *lista = no;
}

void imprimir(No* lista, const char* nome) {
    printf("%s: ", nome);
    while (lista != NULL) {
        printf("%d ", lista->valor);
        lista = lista->proximo;
    }
    printf("\n");
}

int main() {
    int N = 20;

    No* lista_A = NULL;
    No* lista_B = NULL;

    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 0; i < N; i++) {
                #pragma omp task firstprivate(i)
                {
                    int escolha = rand() % 2;
                    if (escolha == 0) {
                        #pragma omp critical(lista_A)
                        { inserir(&lista_A, i); }
                    } else {
                        #pragma omp critical(lista_B)
                        { inserir(&lista_B, i); }
                    }
                }
            }
        }
    }

    printf("--- Parte 1: critical nomeado ---\n");
    imprimir(lista_A, "Lista A");
    imprimir(lista_B, "Lista B");

    int num_listas = 0;
    printf("\nQuantas listas? ");
    scanf("%d", &num_listas);

    No** listas = calloc(num_listas, sizeof(No*));
    omp_lock_t* travas = malloc(num_listas * sizeof(omp_lock_t));
    for (int i = 0; i < num_listas; i++)
        omp_init_lock(&travas[i]);

    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 0; i < N; i++) {
                #pragma omp task firstprivate(i)
                {
                    int escolha = rand() % num_listas;
                    omp_set_lock(&travas[escolha]);
                    inserir(&listas[escolha], i);
                    omp_unset_lock(&travas[escolha]);
                }
            }
        }
    }

    printf("\n--- Parte 2: locks explicitos (%d listas) ---\n", num_listas);
    for (int i = 0; i < num_listas; i++) {
        char nome[20];
        sprintf(nome, "Lista %d", i);
        imprimir(listas[i], nome);
        omp_destroy_lock(&travas[i]);
    }

    free(listas);
    free(travas);
    return 0;
}
```
