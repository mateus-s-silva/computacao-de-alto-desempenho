#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <windows.h>

struct No {
    char nome[50];
    struct No* proximo;
};

struct No* criar_no(char* nome){
    struct No* no = malloc(sizeof(struct No));
    strcpy(no->nome, nome);
    no->proximo = NULL;

    return no;
};

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


int main(){



    struct No* cabeca = criar_lista(20);

    

    #pragma omp parallel 
    {
        #pragma omp single 
        {
            struct No* atual = cabeca;
            while(atual != NULL){
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