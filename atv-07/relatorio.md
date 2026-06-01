# Relatório — Processamento Paralelo de Lista Encadeada com OpenMP Task

**Disciplina:** Computação de Alto Desempenho
**Curso:** Tecnologia da Informação — 8º Período

---

## 1. Introdução

O paralelismo com OpenMP é geralmente associado a loops sobre arrays, onde o número de elementos é conhecido e os dados têm acesso indexado. Estruturas dinâmicas como listas encadeadas apresentam um desafio diferente: não há índice, o tamanho pode não ser conhecido com antecedência, e a navegação é feita por ponteiros.

Este relatório explora o uso de `#pragma omp task` para processar uma lista encadeada em paralelo, investigando o comportamento do escalonamento de tarefas, o compartilhamento de variáveis entre threads e os riscos de condição de corrida.

---

## 2. Fundamentação Teórica

### 2.1 Limitações do `parallel for` em listas encadeadas

O `#pragma omp parallel for` divide iterações de um loop entre threads. Para isso, o OpenMP precisa saber quantas iterações existem e poder acessar qualquer elemento diretamente pelo índice. Em listas encadeadas, nenhuma das duas condições é satisfeita — para chegar ao nó `i`, é preciso percorrer todos os anteriores.

### 2.2 OpenMP Task

O `#pragma omp task` resolve esse problema com um modelo diferente: em vez de dividir o loop, **cria uma tarefa** para cada elemento encontrado. As tarefas vão para uma fila e são executadas pelas threads disponíveis.

A estrutura padrão para percorrer uma lista encadeada com tarefas é:

```c
#pragma omp parallel
{
    #pragma omp single
    {
        while (atual != NULL) {
            #pragma omp task firstprivate(atual)
            {
                // processa atual
            }
            atual = atual->proximo;
        }
    }
}
```

- `parallel` — cria as threads
- `single` — garante que apenas uma thread percorre a lista e cria as tarefas
- `task` — cria uma tarefa para cada nó
- As demais threads ficam disponíveis para executar as tarefas da fila

### 2.3 Compartilhamento de variáveis em tarefas

Por padrão, variáveis declaradas fora de uma task e dentro do bloco `single` são tratadas como `firstprivate` — cada tarefa recebe uma cópia do valor no momento da criação. Quando a variável é declarada fora da região `parallel`, ela se torna `shared` por padrão, e todas as tarefas apontam para a mesma posição de memória.

Em um loop sobre lista encadeada, isso é crítico: se `atual` for compartilhada, quando a tarefa finalmente executar, `atual` pode já ter avançado para outro nó — causando processamento incorreto ou acesso a `NULL`.

### 2.4 Condição de corrida

Uma condição de corrida ocorre quando múltiplas threads acessam ou escrevem um recurso compartilhado sem sincronização. O resultado depende do timing de execução e pode variar entre execuções do mesmo programa.

---

## 3. Implementação

Foi criada uma lista encadeada de 20 nós, cada um contendo o nome de um arquivo fictício (`arquivo01.txt` a `arquivo20.txt`). A lista é construída por uma função auxiliar `criar_lista`, e percorrida em uma região paralela com `#pragma omp task`.

```c
#pragma omp parallel
{
    #pragma omp single
    {
        struct No* atual = cabeca;
        while (atual != NULL) {
            #pragma omp task firstprivate(atual)
            {
                printf("Nome do arquivo: %s\n", atual->nome);
                printf("Numero da thread executora: %d\n", omp_get_thread_num());
                printf("-----------------------\n");
            }
            atual = atual->proximo;
        }
    }
}
```

Para investigar o risco de condição de corrida, foram testadas duas variações:

1. **Versão correta:** `atual` declarada dentro do `single`, tratada como `firstprivate` automaticamente
2. **Versão com risco:** `atual` declarada fora do `parallel` com `shared(atual)` explícito, e `Sleep(10ms)` dentro da tarefa para forçar concorrência

---

## 4. Resultados

### 4.1 Versão correta (firstprivate)

Todos os 20 arquivos foram processados em todas as execuções. A ordem variou entre execuções — esperado, pois as threads pegam tarefas da fila sem ordem garantida. Nenhum arquivo foi repetido ou ignorado.

```
Nome do arquivo: arquivo01.txt  | Thread: 1
Nome do arquivo: arquivo04.txt  | Thread: 5
Nome do arquivo: arquivo08.txt  | Thread: 5
Nome do arquivo: arquivo06.txt  | Thread: 0
...
```

### 4.2 Versão com condição de corrida (shared + Sleep)

Com `atual` compartilhada e `Sleep(10ms)` dentro das tarefas, foi possível observar condição de corrida no `printf` — múltiplas threads escrevendo simultaneamente, causando saídas misturadas:

```
Nome do arquivo: arquivo11.txt
Nome do arquivo: arquivo09.txt
Nome do arquivo: arquivo10.txt
Numero da thread executora: 0
-----------------------
```

O problema no acesso ao `atual` em si não foi reproduzido de forma consistente no hardware utilizado, pois a thread principal percorre a lista inteira em microssegundos — antes que qualquer tarefa comece a executar. O risco, no entanto, é real e se manifestaria em sistemas com mais carga ou listas maiores.

---

## 5. Análise

### Todos os nós foram processados?

Sim, na versão correta com `firstprivate`. Cada nó gerou exatamente uma tarefa, e cada tarefa foi executada por exatamente uma thread.

### O comportamento mudou entre execuções?

A **ordem** de processamento variou entre execuções, pois as threads pegam tarefas da fila de forma não determinística. O **conteúdo** processado foi sempre o mesmo — todos os 20 arquivos, sem repetição.

### Como garantir que cada nó seja processado uma única vez?

Dois mecanismos combinados garantem isso:

1. **`#pragma omp single`** — apenas uma thread percorre a lista e cria as tarefas, garantindo que cada nó gere exatamente uma tarefa
2. **`firstprivate(atual)`** — cada tarefa recebe sua própria cópia do ponteiro `atual` no momento da criação, garantindo que ela sempre processe o nó correto independente do timing de execução

Sem `firstprivate`, o comportamento dependeria do timing: se a tarefa executasse antes de `atual` avançar, processaria o nó correto; caso contrário, poderia processar um nó errado ou causar acesso inválido a `NULL`.

---

## 6. Conclusão

O `#pragma omp task` é a abordagem correta para paralelizar estruturas dinâmicas como listas encadeadas, onde `parallel for` não se aplica. A combinação de `single` para criação das tarefas e `firstprivate` para isolamento do ponteiro garante processamento correto e sem condições de corrida.

O experimento demonstrou que o comportamento não determinístico em programas paralelos pode ser difícil de reproduzir — o bug existe no design do código mesmo quando não aparece na execução. Isso reforça a importância de entender os mecanismos de compartilhamento de variáveis do OpenMP, e não apenas confiar nos resultados observados.

---

## Apêndice — Código Completo

```c
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>

struct No {
    char nome[50];
    struct No* proximo;
};

struct No* criar_no(char* nome) {
    struct No* no = malloc(sizeof(struct No));
    strcpy(no->nome, nome);
    no->proximo = NULL;
    return no;
}

struct No* criar_lista(int n) {
    struct No* cabeca = NULL;
    struct No* anterior = NULL;

    for (int i = 1; i <= n; i++) {
        char nome[50];
        sprintf(nome, "arquivo%02d.txt", i);
        struct No* novo = criar_no(nome);

        if (i == 1) cabeca = novo;
        else anterior->proximo = novo;

        anterior = novo;
    }

    return cabeca;
}

int main() {
    struct No* cabeca = criar_lista(20);

    #pragma omp parallel
    {
        #pragma omp single
        {
            struct No* atual = cabeca;
            while (atual != NULL) {
                #pragma omp task firstprivate(atual)
                {
                    printf("Nome do arquivo: %s\n", atual->nome);
                    printf("Numero da thread executora: %d\n", omp_get_thread_num());
                    printf("-----------------------\n");
                }
                atual = atual->proximo;
            }
        }
    }

    return 0;
}
```
