#include <iostream>
#include <stdio.h>      /* printf */
#include <stdlib.h>     /* qsort */
#include <chrono>

//function comparaison pour le sort()
int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

using namespace std;

int main(){
    
    int sz = 2048;
    int arr[sz];

    //initializing array
    for(int i =0; i < sz; i++){
        arr[i] = std::rand()%10000;
    }
    printf("Array before: ");
    //print(arr, sz, num_proc);

    
    auto start = std::chrono::high_resolution_clock::now();
    
    //first sort to learn limite
    qsort(arr, sz, sizeof(int), compare);
    
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    
    cout << duration.count() << endl;
}