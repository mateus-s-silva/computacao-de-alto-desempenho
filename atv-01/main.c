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

Dados criarDados(int tamanho_vetor, int linhas, int colunas){
    Dados d;

    d.tamanho_vetor = tamanho_vetor;
    d.linhas = linhas;
    d.colunas = colunas;

    d.vetor = malloc(tamanho_vetor * sizeof(int));

    if(d.vetor == NULL){
        printf("Não foi possivel alocar memoria para o vetor.");
        exit(1);
    }

    d.matriz = malloc(linhas * sizeof(int*));

    if(d.matriz == NULL) {
        printf("Não foi possivel alocar memoria para a matriz.");
        exit(1);
    }
    
    for (int i = 0; i < linhas; i++)
    {
        d.matriz[i] = malloc(colunas * sizeof(int));
        if(d.matriz[i] == NULL){
            printf("Erro ao alocar memoria para a linha %d da matriz.", i);
            exit(1);
        }
    }

    return d;
}

void liberarMemoria(Dados d){
    free(d.vetor);

    for(int i = 0; i < d.linhas; i++) {
        free(d.matriz[i]);
    }

    free(d.matriz);
} 

void multiplicarPorLinha(Dados d, int *result) {
    for(int i = 0; i < d.linhas; i++){
        result[i] = 0;
        for(int j = 0; j < d.colunas; j++){
            result[i] += d.matriz[i][j] * d.vetor[j];
        }
    }
}

void multiplicarPorColunas(Dados d, int *result) {
    for(int i = 0; i < d.linhas; i++) result[i] = 0;

    for(int j = 0; j < d.colunas; j++){
        for (int i = 0; i < d.linhas; i++)
        {
            result[i] += d.matriz[i][j] * d.vetor[j];
        }
    }
}

void preencherDados(Dados d){
    srand(time(NULL));
    for(int i = 0; i < d.tamanho_vetor; i++) {
        d.vetor[i] = rand() % 100;
    }

    for(int i = 0; i < d.linhas; i++){
        for(int j = 0; j < d.colunas; j++){
            d.matriz[i][j] = rand() % 100;
        }
    }
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