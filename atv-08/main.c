#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <limits.h>

unsigned int meu_rand(unsigned int* semente) {
      *semente ^= *semente << 13;
      *semente ^= *semente >> 17;
      *semente ^= *semente << 5;
      return *semente;
}

int main() {

    int N = 100000000;
    int contagem = 0;   
    int acertos[6] = {0};
    double start = omp_get_wtime();

    #pragma omp parallel 
    {
       
        unsigned int semente = omp_get_thread_num() + 1;
        int acertos_local = 0;

        #pragma omp for
        for (int i = 0; i < N; i++)
        { 
            double x = (double)meu_rand(&semente) / UINT_MAX;
            double y = (double)meu_rand(&semente) / UINT_MAX;

            if(pow(x, 2) + pow(y, 2) <= 1.0) {
                acertos[omp_get_thread_num()]++;
            }

        }

        // #pragma omp critical 
        // {
        //     contagem += acertos_local;    
        // }
    }    

    for (int i = 0; i < 6; i++)
    {
        contagem += acertos[i];
    }
    

    double end = omp_get_wtime();

    double pi = 4.0 * contagem / N;

    printf("Tempo total %f\n", end - start);
    printf("pi: %f", pi);
    
    return 0;
}