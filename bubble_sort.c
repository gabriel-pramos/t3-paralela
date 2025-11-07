#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// Função para inicializar o vetor com números aleatórios
void Inicializa(int *vetor, int tam) {
    srand(314159);
    for (int i = 0; i < tam; i++) {
        vetor[i] = rand() % tam;
    }
}

// Função Bubble Sort
void BubbleSort(int *vetor, int tam) {
    int i, j, temp;
    for (i = 0; i < tam - 1; i++) {
        for (j = 0; j < tam - i - 1; j++) {
            if (vetor[j] > vetor[j + 1]) {
                temp = vetor[j];
                vetor[j] = vetor[j + 1];
                vetor[j + 1] = temp;
            }
        }
    }
}

// Função para intercalar dois vetores ordenados
void Intercala(int *vetor, int tam) {
    int meio = tam / 2;
    int *temp = (int *)malloc(tam * sizeof(int));
    int i = 0, j = meio, k = 0;
    
    // Intercala as duas metades ordenadas
    while (i < meio && j < tam) {
        if (vetor[i] <= vetor[j]) {
            temp[k++] = vetor[i++];
        } else {
            temp[k++] = vetor[j++];
        }
    }
    
    // Copia elementos restantes da primeira metade
    while (i < meio) {
        temp[k++] = vetor[i++];
    }
    
    // Copia elementos restantes da segunda metade
    while (j < tam) {
        temp[k++] = vetor[j++];
    }
    
    // Copia de volta para o vetor original
    for (i = 0; i < tam; i++) {
        vetor[i] = temp[i];
    }
    
    free(temp);
}

// Função para mostrar o vetor
void Mostra(int *vetor, int tam) {
    printf("Vetor ordenado: ");
    for (int i = 0; i < tam; i++) {
        printf("%d ", vetor[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    int my_rank, num_procs;
    int tam_vetor;
    int *vetor;
    int max_size = 0;
    int delta = 0;
    MPI_Status status;
    double start_time, end_time;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    
    // Root process: check arguments and get array size
    if (argc != 3) {
        printf("Usage: %s array-size delta\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    tam_vetor = atoi(argv[1]);
    if (tam_vetor <= 0) {
        printf("Error: array size must be positive\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    delta = atoi(argv[2]);
    if (delta <= 0) {
        printf("Error: delta must be positive\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    max_size = tam_vetor;
    if (my_rank == 0) {
        printf("Array size = %d\nProcesses = %d\nDelta = %d\n", tam_vetor, num_procs, delta);
    }
    
    // Start timing on rank 0
    if (my_rank == 0) {
        start_time = MPI_Wtime();
    }
    
    // Aloca espaço para o vetor (tamanho máximo)
    vetor = (int *)malloc(max_size * sizeof(int));
    if (vetor == NULL) {
        printf("Error: Could not allocate array of size %d\n", max_size);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Calcula o rank do pai (árvore binária)
    int pai = (my_rank - 1) / 2;
    
    // Recebe vetor
    if (my_rank != 0) {
        // Não sou a raiz, tenho pai
        MPI_Recv(vetor, max_size, MPI_INT, pai, 0, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &tam_vetor);
    } else {
        // Sou a raiz
        tam_vetor = max_size;
        Inicializa(vetor, tam_vetor);
    }
    
    // Dividir ou conquistar?
    int filho_esquerda = 2 * my_rank + 1;
    int filho_direita = 2 * my_rank + 2;
    
    if (tam_vetor <= delta || filho_direita >= num_procs) {
        // Conquisto (vetor pequeno ou não tenho filhos disponíveis)
        printf("Bubble sort called with size = %d\n on process %d\n", tam_vetor, my_rank);
        double start = MPI_Wtime();
        BubbleSort(vetor, tam_vetor);
        double end = MPI_Wtime();
        printf("Bubble sort time: %.6f seconds\n", end - start);
    } else {
        // Dividir
        int metade = tam_vetor / 2;
        
        // Manda metade inicial do vetor para filho esquerdo
        MPI_Send(&vetor[0], metade, MPI_INT, filho_esquerda, 0, MPI_COMM_WORLD);
        
        // Manda metade final do vetor para filho direito
        MPI_Send(&vetor[metade], tam_vetor - metade, MPI_INT, filho_direita, 0, MPI_COMM_WORLD);
        
        // Recebe dos filhos
        MPI_Recv(&vetor[0], metade, MPI_INT, filho_esquerda, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&vetor[metade], tam_vetor - metade, MPI_INT, filho_direita, 0, MPI_COMM_WORLD, &status);
        
        // Intercala vetor inteiro
        Intercala(vetor, tam_vetor);
    }
    
    // Manda para o pai
    if (my_rank != 0) {
        // Tenho pai, retorno vetor ordenado para ele
        MPI_Send(vetor, tam_vetor, MPI_INT, pai, 0, MPI_COMM_WORLD);
    } else {
        // Sou a raiz, mostro vetor
        end_time = MPI_Wtime();
        // Mostra(vetor, tam_vetor);
        printf("Execution time: %.6f seconds\n", end_time - start_time);
    }
    
    free(vetor);
    MPI_Finalize();
    
    return 0;
}
