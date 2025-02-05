#include "header.h"


double ring_broadcast(double local_temp, int num_nodes, int rank, int leader_id){
    double received_temp;
    double total_temp = local_temp;  // Initialize total_temp with the local temperature
    
    // Add each process's local temperature to the total temperature
    for (int i = 0; i < num_nodes; i++) {
        total_temp += local_temp;  // Add local temperature
    }

    // Perform ring broadcast
    for (int i = 0; i < num_nodes; i++) {
        int left_nei = (rank == 0) ? num_nodes - 1 : rank - 1;
        int right_nei = (rank == num_nodes - 1) ? 0 : rank + 1;
        
        MPI_Send(&local_temp, 1, MPI_DOUBLE, right_nei, TAG_BROADCAST, MPI_COMM_WORLD);
        MPI_Recv(&received_temp, 1, MPI_DOUBLE, left_nei, TAG_BROADCAST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        local_temp = (local_temp + received_temp) / 2.0;  // Update local temperature
    } 

    double avg_temp = total_temp / num_nodes;
    return avg_temp;
}
