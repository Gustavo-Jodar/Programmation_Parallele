#include <iostream>
#include <vector>
#include <mpi.h>
#include <vector>
#include <stdio.h>      /* printf */
#include <stdlib.h>     /* qsort */
#include <chrono>

using namespace std;

//function pour calculer la some d'un vector
int sum(int *arr, int size){
    int s = 0;
    for (int i = 0; i < size; i++)
        s = s + arr[i];
    return s;
}

//function comparaison pour le sort()
int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

//function que donne quel bucket un nombre doit aller
int get_bucket(int *limits, int sz_limits ,int number){
    for(int i=0;i<sz_limits;i++){
        if(number <= limits[i])
            return i;
    }
    return sz_limits;
}

//function pour montrer un array
void print(int *arr, int lim, int num_proc){
    std::cout<< "Process (" << num_proc <<") = ";
    for(int i =0; i < lim; i++)
        std::cout << arr[i] << " ";
    
    std::cout<<std::endl;
}

//function pour montrer les buckets qui étaient former
void print_buckets(vector<vector<int> >arr, int num_proc){
    std::cout<< num_proc <<" = ";
    for (int i = 0; i < arr.size(); i++){
        cout << "[";
        for(int j =0; j < arr[i].size(); j++)
            std::cout << arr[i][j] << " ";
        cout << "]";
    }
    
    std::cout<<std::endl;
}


int main(int argc, char **argv){
    int nb_process, num_proc;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_process);
    MPI_Comm_rank(MPI_COMM_WORLD, &num_proc);
    
    int sz = 2048;
    int sz_loc = sz/nb_process;
    int arr[sz];
    int *arr_loc = new int[sz_loc];

    if(num_proc == 0){
        //initializing array
        for(int i =0; i < sz; i++){
            arr[i] = std::rand()%10000;
        }
        printf("Array before: ");
        //print(arr, sz, num_proc);
    }
    
    auto start = std::chrono::high_resolution_clock::now();

    //division to processes
    MPI_Scatter(arr, sz_loc, MPI_INT, arr_loc, sz_loc, MPI_INT, 0, MPI_COMM_WORLD);
    
    //first sort to learn limite
    qsort(arr_loc, sz_loc, sizeof(int), compare);
    
    //finding medianes of each subvector and sendind to process 0
    int medianes[nb_process-1];
    if(num_proc != 0){
        for (int i = 1; i < nb_process; i++){
            medianes[i-1] = arr_loc[int(sz_loc/nb_process)*i];
        }
        //seding his medianes to process 0
        MPI_Send(medianes, nb_process-1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        //receiving medianes from process 0
        MPI_Recv(medianes, nb_process-1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
        
    }
    //calculationg medianes of process 0 and receiving the others medianes
    else{
        //calculating process 0 medianes
        int all_medianes[(nb_process - 1)*nb_process];
        
        int count = 0;
        for (int i = 1; i < nb_process; i++){
            all_medianes[i-1] = arr_loc[sz_loc/nb_process*i];
            count++;
        }
        //receiving others medians
        for(int i=1; i < nb_process; i++){
            MPI_Recv(medianes,nb_process-1,MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
            for (int j = 0; j < nb_process - 1; j++){
                all_medianes[count] = medianes[j];
                count++;
            }
        }
        //sorting medianes
        qsort(all_medianes, (nb_process - 1)*nb_process, sizeof(int), compare);
        
        //choosing medianes
        count = 0;
        for(int i=(nb_process-1)/2;i<(nb_process - 1)*nb_process; i = i + nb_process - 1){
            medianes[count] = all_medianes[i];
            count++;
        }

        for(int proc_i = 1; proc_i < nb_process; proc_i++)
            MPI_Send(medianes, nb_process-1, MPI_INT, proc_i, 0, MPI_COMM_WORLD);
    }

    if(num_proc == 0){
        cout << "Medianes = ";
        print(medianes, nb_process-1, num_proc);
    }

    //moving elements for respective bucket
    vector<vector<int>> buckets(nb_process);
    int bucket_dest = get_bucket(medianes, nb_process-1, arr_loc[0]); 
    int curr_bucket = bucket_dest;   
    for(int i = 0; i < sz_loc ; i++)
    {
        buckets[bucket_dest].push_back(arr_loc[i]);
        if(i+1 < sz_loc){                     
            bucket_dest = get_bucket(medianes, nb_process-1, arr_loc[i+1]);    
            if(curr_bucket != bucket_dest){
                curr_bucket = get_bucket(medianes, nb_process-1, arr_loc[i+1]);
            }
        }
    }
    
    //print_buckets(buckets, num_proc);
    
    //calculating buckets size
    int bucket_sizes[nb_process][nb_process];  
    for (int i = 0; i < nb_process; i++)
        bucket_sizes[num_proc][i] = buckets[i].size();
    
    //seding to other processes the number os buckets that eah one has to receive
    for (int i = 0; i < nb_process; i++)
    {
        MPI_Bcast(bucket_sizes[i], nb_process, MPI_INT, i, MPI_COMM_WORLD);
    }
    int bucket_sizes_to_use[nb_process][nb_process];
    for (int i = 0; i < nb_process; i++)
        for (int j = 0; j < nb_process; j++)
            bucket_sizes_to_use[j][i] = bucket_sizes[i][j];

    //calculating the displacement of each bucket to send to each process
    int dspls[nb_process][nb_process];
    for (int i = 0; i < nb_process; i++){
        for (int j = 0; j < nb_process; j++)
        {
        if(j == 0)
            dspls[i][j] = 0;
        else
            dspls[i][j] = sum(bucket_sizes_to_use[i], j);        
        }
    }
    
    //calculating the new local array size
    int new_v_loc_size = sum(bucket_sizes_to_use[num_proc], nb_process);
    int *new_v_loc = new int[new_v_loc_size];

    //gathering the respective buckets of each process
    for (int i = 0; i < nb_process; i++)
    {
        MPI_Gatherv(&buckets[i][0], buckets[i].size(), MPI_INT, new_v_loc, bucket_sizes_to_use[i], dspls[i], MPI_INT, i, MPI_COMM_WORLD);
    }

    //sorting medianes
    qsort(new_v_loc, new_v_loc_size, sizeof(int), compare);

    int all_bucket_sizes[nb_process];
    for (int i = 0; i < nb_process; i++){
        all_bucket_sizes[i] = sum(bucket_sizes_to_use[i], nb_process);        
    }

    int all_dspls[nb_process];
    for (int i = 0; i < nb_process; i++){
        if(i == 0)
            all_dspls[i] = 0;
        else
            all_dspls[i] = sum(all_bucket_sizes, i);        
        
    }

    MPI_Gatherv(new_v_loc, new_v_loc_size, MPI_INT, arr, all_bucket_sizes, all_dspls, MPI_INT, 0, MPI_COMM_WORLD);

    if(num_proc == 0){
        //print(arr, sz, num_proc);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        
        cout << duration.count() << endl;
    }

    MPI_Finalize();
    return 0;
}