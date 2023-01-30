import random
import math
from mpi4py import MPI

comm = MPI.COMM_WORLD
rank = comm.Get_rank() #process rank
size = comm.Get_size() # n process

#function pour generer des points
def generate_point():
    x = random.uniform(-1, 1)
    y = random.uniform(-1, 1)
    return (x,y)

#function pour calculer le distances
def dist(p1, p2):
    return math.sqrt(pow((p1[0]-p2[0]), 2) + pow((p1[1]-p2[1]), 2))

#function pour savoir si c'est dans le circle ou pas
def in_circle(p):
    return dist(p, (0,0)) <= 1

#nombre de points dans le circle
n_in = 0
#nombre de iterations
reps = 10000
#calcule de plusieurs points
for i in range(reps):
    if(in_circle(generate_point())):
        n_in = n_in + 1

#le processus 0 va faire son calcul et recevoir tout le donnes et faire le calcule final
if rank == 0:
    all_in = n_in
    n_points = reps
    print(f"Hi, I am {rank} my pi is = {4 * all_in/n_points}")
    for i in range(1,3):
        end_process = comm.irecv(source=i,tag=1)
        data = end_process.wait()
        all_in = all_in + data[0]
        n_points = n_points + data[1]
    
    print(f"Hi, am I  {rank}, this is the result:  {4 * all_in/n_points}")

#les autres processus vont faire le calcul et envoier a le processus 0
else:
    print(f"Hi, I am {rank} my pi is = {4 * n_in/reps}")
    data = (n_in, reps)
    req = comm.isend(data, dest=0, tag=1)
