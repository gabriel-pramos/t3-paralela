/* MPI parallel bubble sort

   Derived from mpi_mergesort.c
   Date: 2025-11-04

   Copyright (C) 2011  Atanas Radenski

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation; either version 2 of
 the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public
 License along with this program; if not, write to the Free
 Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.

*/

/* IMPORTANT: Compile with -lm:
   mpicc mpi_bubblesort.c -lm -o mpi_bubblesort */

#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DELTA 32

void merge(int *vetor, int tam);
void bubble_sort(int a[], int size);
void bubblesort_parallel_mpi(int a[], int size, int level, int my_rank,
                             int max_rank, int tag, MPI_Comm comm);
int my_topmost_level_mpi(int my_rank);
void run_root_mpi(int a[], int size, int max_rank, int tag, MPI_Comm comm);
void run_helper_mpi(int my_rank, int max_rank, int tag, MPI_Comm comm);
int main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
  double start, end;

  // All processes
  MPI_Init(&argc, &argv);
  // Check processes and their ranks
  // number of processes == communicator size
  int comm_size;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  int my_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  int max_rank = comm_size - 1;
  int tag = 123;

  if (my_rank == 0) {
    start = MPI_Wtime();
  }

  // Set test data
  if (my_rank == 0) { // Only root process sets test data
    puts("-MPI Parallel Bubble Sort-\t");
    // Check arguments
    if (argc != 2) /* argc must be 2 for proper execution! */
    {
      printf("Usage: %s array-size\n", argv[0]);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    // Get argument
    int size = atoi(argv[1]); // Array size
    printf("Array size = %d\nProcesses = %d\n", size, comm_size);
    // Array allocation
    int *a = malloc(sizeof(int) * size);
    if (a == NULL) {
      printf("Error: Could not allocate array of size %d\n", size);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
    // Random array initialization
    srand(314159);
    int i;
    for (i = 0; i < size; i++) {
      a[i] = rand() % size;
    }
    // Sort with root process
    run_root_mpi(a, size, max_rank, tag, MPI_COMM_WORLD);
    // Result check
    for (i = 1; i < size; i++) {
      if (!(a[i - 1] <= a[i])) {
        printf("Implementation error: a[%d]=%d > a[%d]=%d\n", i - 1, a[i - 1],
               i, a[i]);
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }
  } // Root process end
  else { // Helper processes
    run_helper_mpi(my_rank, max_rank, tag, MPI_COMM_WORLD);
  }

  if (my_rank == 0) {
    end = MPI_Wtime();
    printf("Start = %.2f\nEnd = %.2f\nElapsed = %.2f\n", start, end,
           end - start);
  }

  fflush(stdout);
  MPI_Finalize();
  return 0;
}

// Root process code
void run_root_mpi(int a[], int size, int max_rank, int tag, MPI_Comm comm) {
  int my_rank;
  MPI_Comm_rank(comm, &my_rank);
  if (my_rank != 0) {
    printf("Error: run_root_mpi called from process %d; must be called from "
           "process 0 only\n",
           my_rank);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  bubblesort_parallel_mpi(a, size, 0, my_rank, max_rank, tag, comm);
  /* level=0; my_rank=root_rank=0; */
  return;
}

// Helper process code
void run_helper_mpi(int my_rank, int max_rank, int tag, MPI_Comm comm) {
  int level = my_topmost_level_mpi(my_rank);
  // probe for a message and determine its size and sender
  MPI_Status status;
  int size;
  MPI_Probe(MPI_ANY_SOURCE, tag, comm, &status);
  MPI_Get_count(&status, MPI_INT, &size);
  int parent_rank = status.MPI_SOURCE;
  // allocate int a[size], temp[size]
  int *a = malloc(sizeof(int) * size);
  MPI_Recv(a, size, MPI_INT, parent_rank, tag, comm, &status);
  bubblesort_parallel_mpi(a, size, level, my_rank, max_rank, tag, comm);
  // Send sorted array to parent process
  MPI_Send(a, size, MPI_INT, parent_rank, tag, comm);
  return;
}

// Given a process rank, calculate the top level of the process tree in which
// the process participates Root assumed to always have rank 0 and to
// participate at level 0 of the process tree
int my_topmost_level_mpi(int my_rank) {
  int level = 0;
  while (pow(2, level) <= my_rank)
    level++;
  return level;
}

// MPI parallel bubble sort
void bubblesort_parallel_mpi(int a[], int size, int level, int my_rank,
                             int max_rank, int tag, MPI_Comm comm) {
  int helper_rank = my_rank + pow(2, level);
  if (helper_rank > max_rank ||
      size <= DELTA) { // no more processes available or size too small
    printf("Bubble sort called with size = %d\n", size);
    double start = MPI_Wtime();
    bubble_sort(a, size);
    double end = MPI_Wtime();
    printf("Bubble sort time: %.6f seconds\n", end - start);
  } else {
    MPI_Request request;
    MPI_Status status;
    // Send second half, asynchronous
    // MPI_Isend(a + size / 2, size - size / 2, MPI_INT, helper_rank, tag, comm,
    // &request);
    MPI_Send(a + size / 2, size - size / 2, MPI_INT, helper_rank, tag, comm);
    // Sort first half
    bubblesort_parallel_mpi(a, size / 2, level + 1, my_rank, max_rank, tag,
                            comm);
    // Free the async request (matching receive will complete the transfer).
    MPI_Request_free(&request);
    // Receive second half sorted
    MPI_Recv(a + size / 2, size - size / 2, MPI_INT, helper_rank, tag, comm,
             &status);
    // Merge the two sorted sub-arrays through temp
    merge(a, size);
  }
  return;
}

void bubble_sort(int *vetor, int tam) {
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

void merge(int *vetor, int tam) {
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
