# Calcul de l'ensemble de Mandelbrot en python
import numpy as np
from time import time
from mpi4py import MPI

n = int(input("Array and vector dimention: "))

mat = np.zeros((n,n), dtype=int)
vec = np.zeros((n), dtype=int)

for i in range(0, len(mat)):
    for j in range(0, len(mat[i])):
        mat[i][j] = (i+j) % n 

for i in range(0, len(vec)):
    vec[i] = i+1



print(mat)
print(vec)
print(vec.T)