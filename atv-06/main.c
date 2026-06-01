#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <math.h>

typedef struct ponto
{
    double x;
    double y;
} Ponto;


int main() {

    srand(42);
    
    Ponto* pontos = malloc(sizeof(Ponto) * 100000);

    
    int contagem = 0;

    int base = 10;

    double ultimo_x = 0.0;

    #pragma omp parallel shared(contagem, pontos) 
    {

        srand(omp_get_thread_num() + 1);

        #pragma omp for lastprivate(ultimo_x) firstprivate(base)
        for(int i = 0; i < 100000; i++) {
                pontos[i].x = (double)rand() / RAND_MAX;
                pontos[i].y = (double)rand() / RAND_MAX;

                ultimo_x = pontos[i].x;
                base++;

                if(pow(pontos[i].x, 2) + pow(pontos[i].y, 2) <= 1.0) {
                
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
    printf("Valor de base: %d", base);

    return 1;
}