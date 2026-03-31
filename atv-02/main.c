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
