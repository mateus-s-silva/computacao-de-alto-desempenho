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
    long long iteracoes[] = {100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
    int n = sizeof(iteracoes) / sizeof(iteracoes[0]);

    printf("%-15s %-20s %-15s %-15s\n", "Iteracoes", "Pi aprox.", "Erro abs.", "Tempo (s)");
    printf("------------------------------------------------------------------------\n");

    for (int t = 0; t < n; t++) {
        clock_t inicio = clock();
        double pi = calcularPi(iteracoes[t]);
        double tempo = (double)(clock() - inicio) / CLOCKS_PER_SEC;

        double erro = fabs(pi - PI_REAL);

        printf("%-15lld %-20.15f %-15.2e %.6f\n", iteracoes[t], pi, erro, tempo);
    }

    return 0;
}
