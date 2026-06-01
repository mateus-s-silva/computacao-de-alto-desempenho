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

    // --- Parte 1: duas listas com critical nomeado ---
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

    // --- Parte 2: N listas com locks explicitos ---
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
