# Relatório — Cálculo de π Paralelo com Monte Carlo e OpenMP

**Disciplina:** Computação de Alto Desempenho
**Curso:** Tecnologia da Informação — 8º Período

---

## 1. Introdução

A estimativa estocástica de π pelo método de Monte Carlo consiste em gerar pontos aleatórios e verificar quantos caem dentro de um círculo inscrito em um quadrado. A proporção entre pontos dentro do círculo e o total se aproxima de π/4. Este relatório paraleliza esse cálculo com OpenMP, demonstra a condição de corrida resultante e aplica as cláusulas de compartilhamento de dados para corrigi-la e controlá-la.

*(Fonte: [Academo.org - Monte Carlo Pi](https://academo.org/demos/estimating-pi-monte-carlo/))*

---

## 2. Fundamentação Teórica

### 2.1 Método de Monte Carlo para π

Um ponto `(x, y)` gerado aleatoriamente no intervalo `[0,1]` está dentro do quarto de círculo de raio 1 se:

```
x² + y² ≤ 1
```

A estimativa de π é calculada por:

```
π ≈ 4 × (pontos dentro do círculo) / (total de pontos)
```

Quanto maior o número de pontos, mais precisa a estimativa.

### 2.2 Condição de corrida

Uma condição de corrida ocorre quando múltiplas threads acessam e modificam uma variável compartilhada sem sincronização. O incremento `contagem++` não é uma operação atômica — envolve três passos: ler o valor, incrementar, escrever. Quando duas threads executam esses passos simultaneamente, uma pode sobrescrever o resultado da outra, gerando valores incorretos e não determinísticos.

### 2.3 Cláusulas de compartilhamento de dados do OpenMP

*(Fonte: [OpenMP 5.0 Specification](https://www.openmp.org/spec-html/5.0/openmpsu106.html))*

| Cláusula | Comportamento |
|----------|--------------|
| `shared(x)` | Todas as threads compartilham a mesma variável `x` |
| `private(x)` | Cada thread tem sua própria cópia sem valor inicial |
| `firstprivate(x)` | Cada thread tem sua própria cópia inicializada com o valor atual |
| `lastprivate(x)` | Cada thread tem sua própria cópia; o valor da última iteração é copiado de volta |
| `default(none)` | Força declaração explícita de todas as variáveis — sem compartilhamento implícito |

### 2.4 `#pragma omp critical`

Garante que apenas uma thread por vez executa o bloco protegido, eliminando a condição de corrida no incremento de `contagem`.

---

## 3. Implementação

### 3.1 Versão sequencial

Loop simples gerando 100.000 pontos aleatórios e contando os que caem dentro do círculo.

### 3.2 Versão com condição de corrida

Adição de `#pragma omp parallel for` sem proteção à variável `contagem`:

```c
#pragma omp parallel for
for(int i = 0; i < 100000; i++) {
    pontos[i].x = (double)rand() / RAND_MAX;
    pontos[i].y = (double)rand() / RAND_MAX;
    if(pontos[i].x*pontos[i].x + pontos[i].y*pontos[i].y <= 1.0)
        contagem++;  // condição de corrida
}
```

### 3.3 Versão corrigida com `critical` e cláusulas

Reestruturação com `parallel` separado do `for`, aplicando todas as cláusulas:

```c
#pragma omp parallel shared(contagem, pontos)
{
    srand(omp_get_thread_num() + 1);

    #pragma omp for lastprivate(ultimo_x) firstprivate(base)
    for(int i = 0; i < 100000; i++) {
        pontos[i].x = (double)rand() / RAND_MAX;
        pontos[i].y = (double)rand() / RAND_MAX;

        ultimo_x = pontos[i].x;
        base++;

        if(pontos[i].x*pontos[i].x + pontos[i].y*pontos[i].y <= 1.0) {
            #pragma omp critical
            {
                contagem++;
            }
        }
    }
}
```

---

## 4. Resultados

### 4.1 Versão com condição de corrida

Resultados incorretos e não determinísticos a cada execução:

| Execução | π estimado |
|----------|-----------|
| 1 | 1.3246 |
| 2 | 1.6454 |
| 3 | 2.0633 |
| 4 | 1.5828 |

### 4.2 Versão corrigida

Resultados consistentes e próximos de π:

| Execução | π estimado | ultimo_x | base |
|----------|-----------|----------|------|
| 1 | 3.1355 | 0.813593 | 10 |
| 2 | 3.1355 | 0.813593 | 10 |
| 3 | 3.1355 | 0.813593 | 10 |

---

## 5. Análise

### 5.1 Por que a versão sem proteção falha

O `contagem++` envolve três operações não atômicas. Quando múltiplas threads executam simultaneamente, uma lê o valor antes que outra termine de escrever — o incremento é perdido. Com 6 threads competindo, a maioria dos incrementos se perde, resultando em valores muito abaixo do correto.

### 5.2 Efeito do `#pragma omp critical`

O `critical` serializa o acesso ao incremento — apenas uma thread por vez executa `contagem++`. O resultado se torna correto e determinístico. O custo é desempenho: threads ficam esperando umas pelas outras nesse ponto.

### 5.3 Efeito das cláusulas

**`shared(contagem, pontos)`** — garante que todas as threads incrementam a mesma `contagem` e escrevem no mesmo array. Correto desde que o acesso seja protegido pelo `critical`.

**`firstprivate(base)`** — cada thread recebe uma cópia de `base` inicializada com `10`. Cada thread incrementa sua cópia independentemente. Após o loop, `base` permanece `10` — as modificações das threads não afetam a variável original.

**`lastprivate(ultimo_x)`** — cada thread tem sua cópia de `ultimo_x`. Ao final, o valor da última iteração do loop (i=99999) é copiado de volta para a variável original, independente de qual thread executou essa iteração.

**`default(none)`** — força o programador a declarar explicitamente o comportamento de cada variável. Em programas complexos, variáveis esquecidas tornam-se `shared` silenciosamente, podendo causar condições de corrida sem aviso. O `default(none)` elimina esse risco ao tornar o erro de compilação.

### 5.4 `parallel` separado do `for`

Separar `#pragma omp parallel` de `#pragma omp for` permite executar código fora do loop mas ainda dentro da região paralela — como o `srand()` por thread. Com `parallel for` combinado, isso não seria possível.

---

## 6. Conclusão

O método de Monte Carlo paralelizado demonstra de forma clara os riscos de condição de corrida em programas paralelos. A simples adição de `parallel for` sem sincronização produziu resultados completamente incorretos e não determinísticos.

A correção com `critical` garantiu consistência, ao custo de serializar o ponto crítico do loop. As cláusulas de compartilhamento de dados — `shared`, `private`, `firstprivate`, `lastprivate` — oferecem controle preciso sobre como cada variável é tratada pelas threads, e `default(none)` é uma boa prática em programas complexos para tornar esse controle explícito e seguro.

---

## Apêndice — Código Completo

```c
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <math.h>

typedef struct ponto {
    double x;
    double y;
} Ponto;

int main() {
    Ponto* pontos = malloc(sizeof(Ponto) * 100000);

    int contagem = 0;
    int base = 10;
    double ultimo_x = 0.0;

    #pragma omp parallel shared(contagem, pontos)
    {
        srand(omp_get_thread_num() + 1);

        #pragma omp for lastprivate(ultimo_x) firstprivate(base)
        for (int i = 0; i < 100000; i++) {
            pontos[i].x = (double)rand() / RAND_MAX;
            pontos[i].y = (double)rand() / RAND_MAX;

            ultimo_x = pontos[i].x;
            base++;

            if (pontos[i].x*pontos[i].x + pontos[i].y*pontos[i].y <= 1.0) {
                #pragma omp critical
                {
                    contagem++;
                }
            }
        }
    }

    double pi = 4.0 * contagem / 100000;

    printf("valor de pi: %f\n", pi);
    printf("Ultimo x: %f\n", ultimo_x);
    printf("Valor de base: %d\n", base);

    return 0;
}
```
