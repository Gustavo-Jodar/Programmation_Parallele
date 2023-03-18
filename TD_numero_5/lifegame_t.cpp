#include <iostream>
#include <cstdlib>
#include <mpi.h>

#define ROWS 1000
#define COLS 1000
#define GENERATIONS 100

using namespace std;

int main(int argc, char** argv) {
    int num_procs, rank, i, j, k, offset;
    int **current_grid, **next_grid, *sendbuf, *recvbuf;
    int rows_per_proc, start_row, end_row, num_elements;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Divide a matriz em submatrizes para cada processo
    rows_per_proc = ROWS / num_procs;
    start_row = rank * rows_per_proc;
    end_row = (rank + 1) * rows_per_proc;
    if (rank == num_procs - 1) {
        end_row = ROWS;
    }

    // Aloca memória para as matrizes
    current_grid = new int*[rows_per_proc];
    next_grid = new int*[rows_per_proc];
    for (i = 0; i < rows_per_proc; i++) {
        current_grid[i] = new int[COLS];
        next_grid[i] = new int[COLS];
    }

    // Preenche a matriz inicial com valores aleatórios
    for (i = 0; i < rows_per_proc; i++) {
        for (j = 0; j < COLS; j++) {
            current_grid[i][j] = rand() % 2;
        }
    }

    // Aloca memória para os buffers de envio e recebimento
    num_elements = COLS;
    sendbuf = new int[num_elements];
    recvbuf = new int[num_elements];

    // Executa a simulação do jogo da vida
    for (k = 0; k < GENERATIONS; k++) {
        // Envia as linhas de borda aos processos adjacentes
        if (rank > 0) {
            MPI_Send(current_grid[0], num_elements, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
            MPI_Recv(recvbuf, num_elements, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &status);
            for (j = 0; j < COLS; j++) {
                current_grid[0][j] = recvbuf[j];
            }
        }

        if (rank < num_procs - 1) {
            MPI_Send(current_grid[rows_per_proc - 1], num_elements, MPI_INT, rank + 1, 1, MPI_COMM_WORLD);
            MPI_Recv(recvbuf, num_elements, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &status);
            for (j = 0; j < COLS; j++) {
                current_grid[rows_per_proc - 1][j] = recvbuf[j];
            }
        }

        // Calcula o estado das células na submatriz atribuída
        for (i = 1; i < rows_per_proc - 1; i++) {
            for (j = 1; j < COLS - 1; j++) {
                int neighbors = 0;
                for (int x = i - 1; x <= i + 1; x++) {
                    for (int y = j - 1; y <= j + 1; y++) {
                        neighbors += current_grid[x][y];
                    }
                }
            }
        }
    }
}
