# Relatório — Aproximação de π por Série Matemática

**Disciplina:** Computação de Alto Desempenho
**Curso:** Tecnologia da Informação — 8º Período

---

## 1. Introdução

O número π é uma constante matemática irracional — sua representação decimal é infinita e não periódica. Por isso, qualquer cálculo computacional de π é, por definição, uma aproximação. A precisão dessa aproximação depende diretamente do esforço computacional empregado.

Este relatório implementa o cálculo de π por meio da **Série de Leibniz**, variando o número de iterações de 100 a 1 bilhão. São analisados a acurácia obtida em cada caso, o tempo de execução correspondente e a relação entre esses dois fatores. Ao final, o comportamento observado é relacionado a aplicações reais que enfrentam o mesmo compromisso entre precisão e custo computacional.

---

## 2. Fundamentação Teórica

### 2.1 A Série de Leibniz

A Série de Leibniz (também chamada de Série de Gregory-Leibniz) é uma das formas mais simples de aproximar π:

$$\frac{\pi}{4} = 1 - \frac{1}{3} + \frac{1}{5} - \frac{1}{7} + \frac{1}{9} - \cdots = \sum_{i=0}^{n} \frac{(-1)^i}{2i+1}$$

Multiplicando por 4, obtemos π. Cada iteração adiciona um termo à soma, e quanto mais termos, mais próxima a aproximação do valor real.

### 2.2 Taxa de Convergência

A Série de Leibniz converge na taxa **O(1/n)**: o erro é inversamente proporcional ao número de iterações. Na prática, isso significa que:

- Para ganhar **1 casa decimal** a mais de precisão, são necessárias **10× mais iterações**
- O tempo de execução cresce **linearmente** com as iterações

Essa é uma convergência lenta comparada a outras séries (como a de Machin ou algoritmos como BBP), mas sua simplicidade a torna ideal para fins didáticos.

---

## 3. Implementação

O programa calcula π iterativamente pela Série de Leibniz, medindo o tempo de CPU com `clock()` para cada quantidade de iterações testada. O erro absoluto é calculado em relação ao valor de referência `3.14159265358979323846`.

```c
double calcularPi(long long iteracoes) {
    double soma = 0.0;
    for (long long i = 0; i < iteracoes; i++) {
        double termo = 1.0 / (2.0 * i + 1.0);
        if (i % 2 == 0)
            soma += termo;
        else
            soma -= termo;
    }
    return 4.0 * soma;
}
```

Os testes foram executados com 8 tamanhos distintos, variando de 10² a 10⁹ iterações.

---

## 4. Resultados

| Iterações     | π aproximado          | Erro absoluto | Tempo (s) |
|:-------------:|:---------------------:|:-------------:|:---------:|
| 100           | 3,131592903558554     | 1,00 × 10⁻²   | < 0,001   |
| 1.000         | 3,140592653839794     | 1,00 × 10⁻³   | < 0,001   |
| 10.000        | 3,141492653590034     | 1,00 × 10⁻⁴   | < 0,001   |
| 100.000       | 3,141582653589720     | 1,00 × 10⁻⁵   | < 0,001   |
| 1.000.000     | 3,141591653589774     | 1,00 × 10⁻⁶   | 0,003     |
| 10.000.000    | 3,141592553589792     | 1,00 × 10⁻⁷   | 0,032     |
| 100.000.000   | 3,141592643589326     | 1,00 × 10⁻⁸   | 0,302     |
| 1.000.000.000 | 3,141592652588050     | 1,00 × 10⁻⁹   | 3,032     |

---

## 5. Análise dos Resultados

### 5.1 Convergência

Os resultados confirmam com precisão a taxa teórica O(1/n) da Série de Leibniz. O padrão é exato: cada vez que o número de iterações é multiplicado por 10, o erro cai exatamente 10×, adicionando uma nova casa decimal correta ao resultado.

| Multiplicação de iterações | Ganho de precisão |
|:--------------------------:|:-----------------:|
| 100 → 1.000                | +1 casa decimal   |
| 1.000 → 10.000             | +1 casa decimal   |
| 10.000 → 100.000           | +1 casa decimal   |
| ... (padrão se repete)     | ...               |

Isso demonstra empiricamente que a relação entre iterações e precisão é **logarítmica**: dobrar a precisão em dígitos decimais exige uma ordem de magnitude a mais de iterações.

### 5.2 Tempo de Execução

O tempo escala de forma estritamente linear com o número de iterações, o que era esperado para um laço simples sem estruturas de dados auxiliares:

| Iterações     | Tempo (s) | Razão com anterior |
|:-------------:|:---------:|:-----------------:|
| 10.000.000    | 0,032     | —                 |
| 100.000.000   | 0,302     | ~9,4×             |
| 1.000.000.000 | 3,032     | ~10,0×            |

Para os primeiros casos (até 100.000 iterações), o tempo é inferior à resolução do `clock()` — menor que 1 ms. O primeiro tempo mensurável aparece em 1.000.000 iterações (3 ms).

### 5.3 Custo por Dígito de Precisão

Combinando as duas observações anteriores, obtemos o custo real de aumentar a precisão:

> **Cada dígito decimal adicional de precisão custa 10× mais tempo de processamento.**

Com 1 bilhão de iterações e 3 segundos de execução, obtemos apenas 9 dígitos corretos de π. Para obter 15 dígitos, seriam necessárias aproximadamente 10¹⁵ iterações — inviável computacionalmente com essa série.

Esse é o compromisso central da computação de alto desempenho: **precisão tem custo exponencial em tempo**.

### 5.4 Relação com Aplicações Reais

O comportamento observado neste experimento é recorrente em diversas áreas:

**Simulações físicas:** Métodos numéricos como diferenças finitas e elementos finitos refinam gradualmente a solução de equações diferenciais. Dobrar a resolução da malha tipicamente quadruplica o tempo de cálculo (O(n²) em 2D). Em simulações climáticas ou de fluidos, cada décimo de grau a mais de precisão pode exigir dias adicionais de processamento em supercomputadores.

**Inteligência Artificial:** O treinamento de redes neurais profundas segue uma lógica similar. Reduzir o erro pela metade frequentemente exige mais que o dobro de iterações de treinamento — a função de perda converge cada vez mais lentamente conforme o modelo se aproxima do ótimo. Isso motiva o uso de técnicas como *learning rate scheduling* e *early stopping*, que interrompem o treinamento quando o custo adicional não justifica o ganho marginal de precisão.

Em ambos os casos, o engenheiro ou cientista precisa decidir qual precisão é suficiente para o problema em questão — e aceitar que obter uma precisão "perfeita" é computacionalmente inviável.

---

## 6. Conclusão

A Série de Leibniz demonstra de forma clara o compromisso entre precisão e custo computacional. Com convergência O(1/n), cada dígito adicional de π exige 10× mais iterações e, consequentemente, 10× mais tempo de processamento.

Com 1 bilhão de iterações e aproximadamente 3 segundos de execução, obteve-se π com 9 dígitos corretos — resultado satisfatório para a maioria das aplicações práticas, mas ainda distante da precisão total de um `double` (15-16 dígitos), que exigiria uma série mais eficiente ou um algoritmo diferente.

A lição central do experimento é que em computação de alto desempenho, **a eficiência do algoritmo importa tanto quanto o poder do hardware**. Trocar a Série de Leibniz por um método de convergência mais rápida (como a fórmula de Machin) permitiria obter a mesma precisão com uma fração das iterações — sem qualquer melhoria de hardware.

---

## Apêndice — Código Completo

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define PI_REAL 3.14159265358979323846

// Série de Leibniz: π/4 = 1 - 1/3 + 1/5 - 1/7 + ...
double calcularPi(long long iteracoes) {
    double soma = 0.0;
    for (long long i = 0; i < iteracoes; i++) {
        double termo = 1.0 / (2.0 * i + 1.0);
        if (i % 2 == 0)
            soma += termo;
        else
            soma -= termo;
    }
    return 4.0 * soma;
}

int main() {
    long long iteracoes[] = {100, 1000, 10000, 100000, 1000000,
                             10000000, 100000000, 1000000000};
    int n = sizeof(iteracoes) / sizeof(iteracoes[0]);

    printf("%-15s %-20s %-15s %-15s\n",
           "Iteracoes", "Pi aprox.", "Erro abs.", "Tempo (s)");
    printf("------------------------------------------------------------------------\n");

    for (int t = 0; t < n; t++) {
        clock_t inicio = clock();
        double pi = calcularPi(iteracoes[t]);
        double tempo = (double)(clock() - inicio) / CLOCKS_PER_SEC;

        double erro = fabs(pi - PI_REAL);

        printf("%-15lld %-20.15f %-15.2e %.6f\n",
               iteracoes[t], pi, erro, tempo);
    }

    return 0;
}
```
