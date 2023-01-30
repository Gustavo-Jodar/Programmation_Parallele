from mpi4py import MPI

comm = MPI.COMM_WORLD
rank = comm.Get_rank() #Le nombre des processus
size = comm.Get_size() #Nombre total des processus dans le communicateur

if rank == 0:
    print("Hi, I am", rank)
    data = 1 #jeton
    req = comm.isend(data, dest=1, tag=1) #Envoie les données a l'autre processus
    req.wait()
    end_process = comm.irecv(source=size-1,tag=1) #Recevoir jetons finals
    data = end_process.wait()
    print(f"Hi, am I  {rank}, Result:  {data}")

elif(rank == size - 1): #Dernier processus
    print("Hi, I am", rank)

    req = comm.irecv(source=rank-1, tag=1)
    data = req.wait()
    data = data + 1
    req = comm.isend(data, dest=0, tag=1) #Envoie jusqu'à processus 0

else:
    print("Hi, I am", rank)
    req = comm.irecv(source=rank-1, tag=1)
    data = req.wait()
    data = data + 1
    req = comm.isend(data, dest=rank+1, tag=1)
