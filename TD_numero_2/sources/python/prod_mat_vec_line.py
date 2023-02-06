# Calcul de l'ensemble de Mandelbrot en python
import numpy as np
from time import time
import time
from mpi4py import MPI

comm = MPI.COMM_WORLD
rank = comm.Get_rank() #process rank
size = comm.Get_size() # n process


def mult_mat_vec(mat, vec, from_line, to_line, from_col, to_col):
    result = [0] * len(vec)
    for i in range(from_line, to_line):
        for j in range(from_col, to_col):
            result[i] = result[i] + mat[i][j]*vec[j]
    return np.array(result)

#n = int(input("Array and vector dimention: "))
n = 100

mat = np.zeros((n,n), dtype=int)
vec = np.zeros((n), dtype=int)

offset = int(n/size)*rank
for i in range(int(n/size)*rank, int(n/size)*(rank+1)):
    for j in range(0,n):
        mat[i][j] = (i+j) % n 

for i in range(0, len(vec)):
    vec[i] = i+1

tempos = []

for i in range (10):
    start = time.time()

    result = comm.reduce( mult_mat_vec(mat, vec, int(n/size)*rank, int(n/size)*(rank+1),0, n) , MPI.SUM, 0)

    end = time.time()

    print(f"Calcule intermediaire: {end-start}")

    if(rank == 0):
        print(result)

    print(end-start)
    tempos.append(end-start)

if(rank == 0):
    print(f"Final Result => {np.array(tempos).mean()}")
