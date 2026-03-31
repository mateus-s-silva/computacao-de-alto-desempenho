#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define VECTOR_SIZE 100000000

#define THREADS_NUMBER 6

double compute_bound(double* a, int n, int threads){
     omp_set_num_threads(threads);

     double resultado = 0;

     double inicio = omp_get_wtime();

     #pragma omp parallel for reduction(+:resultado)
     for(int i = 0; i < n; i++) {
        resultado += sqrt(a[i]) + sin(a[i]) + cos(a[i]);
     }

     double fim = omp_get_wtime();

     printf("Threads: %d | Tempo: %.4f s\n", threads, fim - inicio);

     return fim - inicio;
}

double memory_bound(double* a, double* b, double* c, int n, int threads){
    omp_set_num_threads(threads);

    double inicio = omp_get_wtime();

    #pragma omp parallel for
    for (int i = 0; i < n; i++)
    {
        c[i] = a[i] + b[i];
    }

    double fim = omp_get_wtime();
    
    printf("Threads: %d | Tempo: %.4f s\n", threads, fim - inicio);

    return fim - inicio;
}

int main(){
    double* vector_A = malloc(VECTOR_SIZE * sizeof(double));

    double* vector_B = malloc(VECTOR_SIZE * sizeof(double));

    double* vector_C = malloc(VECTOR_SIZE * sizeof(double));

    double one_thread_time;

    for(int i = 0; i < VECTOR_SIZE; i++) {
        vector_A[i] = (double)i;
        vector_B[i] = (double)i;
    }

    printf("\n--- MEMORY BOUND ---\n");
    for(int i = 1; i <= THREADS_NUMBER; i++){
        double speedup = 0;
        for (int t = 0; t < 3; t++)
        {
            speedup += memory_bound(vector_A, vector_B, vector_C, VECTOR_SIZE, i);
        }

        if(i == 1) one_thread_time = speedup / 3;
        else {
            printf("speedup(%d): %.4fx\n", i, one_thread_time / (speedup / 3));
        }
    }

    printf("\n--- COMPUTE BOUND ---\n");
    for(int i = 1; i <= THREADS_NUMBER; i++){
        double speedup = 0;
        for (int t = 0; t < 3; t++)
        {
            speedup += compute_bound(vector_A,  VECTOR_SIZE, i);
        }

        if(i == 1) one_thread_time = speedup / 3;
        else {
            printf("speedup(%d): %.4fx\n", i, one_thread_time / (speedup / 3));
        }
    }

    return 0;
}