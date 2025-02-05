#include "header.h"

MPI_Datatype MPI_MEASUREMENT;
int N;
int *parents;
int repeats;
ProcessInfo process_info;


int main(int argc, char** argv) {
    int rank, size;
    MPI_Status status;
    int my_parent;
    int id, phase, hop_counter;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    N=size;
    double event_data[N][3];
    double check_event[N][3];
    int status_exchange[N][1];

    for(int i=0;i<N;i++){
        for(int j=0;j<3;j++){
            check_event[i][j]=0;
            status_exchange[i][0]=-1;
        }
    }

    // Initialize the array with -1
    parents=(int*)malloc(N*sizeof(int));
    for (int i = 0; i < N; i++) {
            parents[i]= -1;
    }



    // Create MPI datatype for the struct
    MPI_Datatype MPI_DISTANCE_CH;
    MPI_Datatype type[2] = {MPI_INT, MPI_DOUBLE};
    int blocklen[2] = {1, 1};
    MPI_Aint disp[2];
    disp[0] = offsetof(Distance_CH, id);
    disp[1] = offsetof(Distance_CH, rec_distance);
    MPI_Type_create_struct(2, blocklen, disp, type, &MPI_DISTANCE_CH);
    MPI_Type_commit(&MPI_DISTANCE_CH);

    // You also need to define a matching MPI datatype for the Measurement struct
    int block_lengths[2] = {1, 7}; // Number of elements in each block
    MPI_Aint displacements[2] = {offsetof(Measurement, temperature), offsetof(Measurement, timestamp)}; // Byte offsets of each element
    MPI_Datatype types[2] = {MPI_DOUBLE, MPI_INT}; // MPI data types of each element
    MPI_Type_create_struct(2, block_lengths, displacements, types, &MPI_MEASUREMENT);
    MPI_Type_commit(&MPI_MEASUREMENT);

    if (size != N) {
        printf("Το πλήθος των διεργασιών πρέπει να είναι %d\n", N);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // initialization of contents
    process_info.id = rank;
    process_info.leader=rank;
    for(int i=0;i<Max_children;i++){
        process_info.parent[i]=-1;
    }

    //connect the satellites
    if(rank>=1 && rank<N/2){
        process_info.parent[rank-1]=rank-1;
        process_info.parent[(rank+1)%(N/2)]=(rank+1)%(N/2);
    }
    if(rank==0){
        process_info.parent[1]=1;
        process_info.parent[(N/2)-1]=(N/2)-1;
    }   

    //start reading from the txt file
    if(rank==N-1){
        readEvent("N30.txt",event_data,parents,check_event);       
    }



    MPI_Barrier(MPI_COMM_WORLD);

    //inform all proccesses about their parents(left and right)
    int sum=0;
    if (rank==N-1){
        for(int i=0;i<N-1;i++){
            MPI_Send(&parents[i],1,MPI_INT,i,TAG_CONNECT,MPI_COMM_WORLD);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //receive from the coordinator the info about the parents
    if(parents!=NULL && rank<N-1){
        MPI_Recv(&my_parent,1,MPI_INT,N-1,TAG_CONNECT,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        if(my_parent!=-1){
            printf("Rank %d Connect to %d\n",rank,my_parent);
            process_info.parent[my_parent]=my_parent;
        }
        //send ack
        MPI_Send(&rank,1,MPI_INT,N-1,TAG_ACK,MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);


    //update array of connections of GS
    int res;
    if (rank>(N/2)-1 && rank<N-1){
        for(int i=N/2;i<N-1;i++){
            if(rank==i){
                continue;
            }else{
                MPI_Send(&process_info.parent[i],1,MPI_INT,i,11,MPI_COMM_WORLD);

                MPI_Recv(&res,1,MPI_INT,i,11,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                if(res!=-1){
                    process_info.parent[i]=i;
                }
            }

        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Receive acknowledgment from neighbors and send to coordinator if this process is the coordinator
    if (rank == N - 1 && parents != NULL) {
        for (int i = 0; i < N - 1; i++) {
            if (parents[i] != -1) { // Check if the process has a child
                int ack;
                MPI_Recv(&ack, 1, MPI_INT, i, TAG_ACK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("Coordinator received acknowledgment from process %d\n", i);
            }
        }
    }


    MPI_Barrier(MPI_COMM_WORLD);

    //ititialize elements
    if (rank<N-1){
        process_info.coordinates[0] = 0.0; // Πλάτος
        process_info.coordinates[1] = 0.0; // Μήκος
        process_info.coordinates[2] = 0.0; // Υψόμετρο
        process_info.status = 0.0;
            // Initialize measurements array
        for (int i = 0; i < MAX_MEASUREMENTS; i++) {
            process_info.measurements[i].temperature = 0.0;      
            // Initialize timestamp components to zero
            process_info.measurements[i].timestamp.tm_sec = 0;
            process_info.measurements[i].timestamp.tm_min = 0;
            process_info.measurements[i].timestamp.tm_hour = 0;
            process_info.measurements[i].timestamp.tm_mday = 0;
            process_info.measurements[i].timestamp.tm_mon = 0;
            process_info.measurements[i].timestamp.tm_year = 0;
            process_info.measurements[i].timestamp.tm_wday = 0;
            process_info.measurements[i].timestamp.tm_yday = 0;
            process_info.measurements[i].timestamp.tm_isdst = 0;
        }
  
        MPI_Recv(process_info.coordinates, 3, MPI_DOUBLE, N-1, TAG_COORDINATES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    }

    Measurement temp;
    //receive info for satellites and update
    if(rank<=(N/2)-1){
        MPI_Recv(&process_info.status,1,MPI_DOUBLE,N-1,TAG_STATUS,MPI_COMM_WORLD,MPI_STATUS_IGNORE); 
        //MPI_Recv(&process_info.measurements[0], 1, MPI_MEASUREMENT, N-1, TAG_MEASURMENTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        
        for(int i=0;i<3;i++){
            MPI_Recv(&temp, 1, MPI_MEASUREMENT, N-1, TAG_MEASURMENTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            process_info.measurements[i].temperature=temp.temperature;
            process_info.measurements[i].timestamp=temp.timestamp;
        }

    }

    
    //print the updated info about satellites and ground stations
    if (rank<N-1 && rank>(N/2)-1){
        printf("Process %d updated its coordinates to: (%f, %f, %f) and status (%lf)\n", rank, process_info.coordinates[0], process_info.coordinates[1], process_info.coordinates[2],process_info.status);
    }else if(rank<=(N/2)-1){
        printf("Process %d updated its coordinates to: (%f, %f, %f) and status (%lf)\n", rank, process_info.coordinates[0], process_info.coordinates[1], process_info.coordinates[2],process_info.status);
        for(int i=0;i<3;i++){
                printf("   Proccess:%d -> Received temperature: %lf, Timestamp: %d/%d/%d %d:%d:%d\n",rank,
                process_info.measurements[i].temperature,
                process_info.measurements[i].timestamp.tm_mday, process_info.measurements[i].timestamp.tm_mon, process_info.measurements[i].timestamp.tm_year,
                process_info.measurements[i].timestamp.tm_hour, process_info.measurements[i].timestamp.tm_min, process_info.measurements[i].timestamp.tm_sec);
            }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    /*
    if (rank<N-1){
        for(int i=0;i<Max_children;i++){
            if (process_info.parent[i]!=-1){
                printf("Rank %d Has Parent Rank %d\n",rank,i);
            }
        }
    }
    */

    MPI_Barrier(MPI_COMM_WORLD);

    //Satellites Election
    int SATelection;
    if (rank<=(N/2)-1){
        MPI_Recv(&SATelection,1,MPI_INT,N-1,TAG_ELECTION,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }   

    MPI_Barrier(MPI_COMM_WORLD);

    
    if(rank<=(N/2)-1){

        //find left and right neighbor
        int left_nei=(rank == 0) ? (size / 2) - 1 : rank - 1;
        int right_nei=(rank == (size / 2) - 1) ? 0 : rank + 1;

        //send to right and left neighbor
        MPI_Send(&rank,1,MPI_INT,right_nei,TAG_PROBE,MPI_COMM_WORLD);
        MPI_Send(&rank,1,MPI_INT,left_nei,TAG_PROBE,MPI_COMM_WORLD);

        int left_child_id, right_child_id;

        //receive from lefft and right neighbor
        MPI_Recv(&left_child_id,1,MPI_INT,left_nei,TAG_PROBE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(&right_child_id,1,MPI_INT,right_nei,TAG_PROBE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

        //update info about the leader
        if(process_info.leader<left_child_id || process_info.leader<right_child_id){
            int max;
            if(left_child_id>right_child_id){
                max=left_child_id;
            }else{
                max=right_child_id;
            }
            process_info.leader=max;
        }


        int my_id = rank;
        int left_id, right_id;
        int neighborhood_size = 2; // Neighborhood size (including self)
        int round = 0;
        int received_id_left;
        int received_id_right;

        // Calculate left and right neighbors based on neighborhood size
        

        //for each proccess now move in the next neighborhood
        for(int i=0;i<(N/2)-1;i++){


            MPI_Send(&rank,1,MPI_INT,right_nei,TAG_PROBE,MPI_COMM_WORLD);
            MPI_Send(&rank,1,MPI_INT,left_nei,TAG_PROBE,MPI_COMM_WORLD);


            int left_child_id, right_child_id;

            MPI_Recv(&left_child_id,1,MPI_INT,left_nei,TAG_PROBE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&right_child_id,1,MPI_INT,right_nei,TAG_PROBE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);


            if(left_child_id>process_info.leader || right_child_id>process_info.leader){
                if(left_child_id>right_child_id){
                    process_info.leader=left_child_id;
                }else{
                    process_info.leader=right_child_id;
                }
            }
            
            left_nei=(left_nei == 0) ? (size / 2) - 1 : left_nei - 1;
            right_nei=(right_nei == (size / 2) - 1) ? 0 : right_nei + 1;

            round++;

        }

    } 



    MPI_Barrier(MPI_COMM_WORLD);


    //inform coordinator about Satellite leader
    if(rank==process_info.leader && rank<=(N/2)-1){
        MPI_Send(&rank,1,MPI_INT,N-1,LELECT_ST_DONE,MPI_COMM_WORLD);
    }
    if(rank==N-1){
       MPI_Recv(&process_info.leaderST,1,MPI_INT,MPI_ANY_SOURCE,LELECT_ST_DONE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        printf("SATELLITE LEADER IS %d\n",process_info.leaderST);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    //Ground Stations Election 
    int GSelection;

    if (rank<N-1 && rank>(N/2)-1){
        MPI_Recv(&GSelection,1,MPI_INT,N-1,TAG_ELECTION,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
    int num_neighbors=0;

    //find how many parents 
    if(rank<N-1 && rank>(N/2)-1){
        for(int i=N/2;i<Max_children;i++){
            if(process_info.parent[i]!=-1){
                //printf("RANK %d HAS NEIGHBOR %d\n",rank,i);
                num_neighbors++;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //elect GS 
    if(N>11){
        process_info.leaderGS=N-2;
        if(rank>=N/2 && rank<N-1){
            process_info.leader=N-2;
        }
        if(rank==N-2){
            MPI_Send(&process_info.leader,1,MPI_INT,N-1,LELECT_GS_DONE,MPI_COMM_WORLD);
        }
    }else if(num_neighbors==1){
        for(int i=N/2;i<N-1;i++){
            if(process_info.parent[i]!=-1){
                MPI_Send(&rank,1,MPI_INT,process_info.parent[i],TAG_ELECT,MPI_COMM_WORLD);
                //printf("SEND FROM %d TO %d\n",rank,process_info.parent[i]);
            }
        }
    }else{
        int received_msgs=0;
        int sent_to_only = 0; // Flag to track whether message has been sent to the only other process
        if (num_neighbors > 1) {
            int received_from[N];
            for(int i=0;i<N-1;i++){
                received_from[i]=0;
            }
            for(int i=0;i<num_neighbors-1;i++){
                MPI_Recv(&res,1,MPI_INT,MPI_ANY_SOURCE,TAG_ELECT,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                //printf("Rank %d RECEIVED FROM %d\n",rank,res);
                received_msgs++;
                received_from[res]=1;
            }

            int missing_neibhor=-1;
            if(num_neighbors-received_msgs==1){
                //printf("RANK %d should sent message to the only\n",rank);
                for(int i=N/2;i<N-1;i++){
                    if(process_info.parent[i]!=-1 && received_from[process_info.parent[i]] == 0){
                        missing_neibhor=process_info.parent[i];
                        break;
                    }
                }
            }

            if(missing_neibhor!=-1){
                MPI_Send(&rank,1,MPI_INT,missing_neibhor,TAG_REPLY,MPI_COMM_WORLD);
                sent_to_only=1;
                
                MPI_Recv(&res,1,MPI_INT,missing_neibhor,TAG_REPLY,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                //printf("RANK %d RECEIVED FROM %d\n",rank,missing_neibhor);
                if(res>rank){
                    process_info.leader=res;
                }else{
                    MPI_Send(&process_info.leader,1,MPI_INT,N-1,LELECT_GS_DONE,MPI_COMM_WORLD);
                }
            }     
        }

    }


    MPI_Barrier(MPI_COMM_WORLD);

    //receive the Ground Station leader and inform all the GS
    if(rank==N-1){
        MPI_Recv(&process_info.leaderGS,1,MPI_INT,MPI_ANY_SOURCE,LELECT_GS_DONE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        printf("GROUND STATION LEADER IS %d\n",process_info.leaderGS);
        for(int i=N/2;i<N-1;i++){
            MPI_Send(&process_info.leaderGS,1,MPI_INT,i,LELECT_GS_DONE,MPI_COMM_WORLD);
        }
    }

    if(rank>(N/2)-1 && rank<N-1){
        MPI_Recv(&process_info.leader,1,MPI_INT,N-1,LELECT_GS_DONE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    //inform everybody about all leaders
    if(rank<N/2 && rank==process_info.leader){
        for(int i=0;i<N;i++){
            if(rank==i){
                continue;
            }else{
                MPI_Send(&process_info.leader,1,MPI_INT,i,0,MPI_COMM_WORLD);
            }
        }
    }

    if(rank>=N/2){
        MPI_Recv(&process_info.leaderST,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }
    //inform everybody about all leaders
    if(rank>=N/2 && rank==process_info.leader){
        for (int i=0;i<N;i++){
            if(rank==i){
                continue;
            }else{
                MPI_Send(&process_info.leader,1,MPI_INT,i,1,MPI_COMM_WORLD);
            }
        }
    }

    if(rank<N/2){
        MPI_Recv(&process_info.leaderGS,1,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //inform every gs about the number of repeats 
    if(rank==N-1){
        MPI_Send(&repeats,1,MPI_INT,0,TAG_REPLY,MPI_COMM_WORLD);
        for(int i=N/2;i<N-1;i++){
            MPI_Send(&repeats,1,MPI_INT,i,TAG_REPLY,MPI_COMM_WORLD);
        }
    }else if(rank==0){
        MPI_Recv(&repeats,1,MPI_INT,N-1,TAG_REPLY,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }else if(rank>=N/2 && rank<N-1){
        MPI_Recv(&repeats,1,MPI_INT,N-1,TAG_REPLY,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //inform coordinator through proccess 0 for the status check values
    //then send to the GS leader
    double REC[4];
    if(rank==0){
        for(int i=0;i<repeats;i++){
            MPI_Recv(REC,4,MPI_DOUBLE,N-1,TAG_CHECK,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Send(REC,4,MPI_DOUBLE,N-1,TAG_CHECK,MPI_COMM_WORLD);
        }
    }else if(rank==N-1){
        for(int i=0;i<repeats;i++){
            MPI_Recv(REC,4,MPI_DOUBLE,0,TAG_CHECK,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Send(REC,4,MPI_DOUBLE,process_info.leaderGS,TAG_REPLY,MPI_COMM_WORLD);
        }   
    }else if(rank>N/2 && rank==process_info.leader){
        for(int i=0;i<repeats;i++){
            MPI_Recv(REC,4,MPI_DOUBLE,N-1,TAG_REPLY,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            for(int i=N/2;i<N-1;i++){
                if(i==rank){
                    continue;
                }else{
                    MPI_Send(REC,4,MPI_DOUBLE,i,TAG_NEW_CHECK,MPI_COMM_WORLD);
                }
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    double local_distance = 0.0; // Initialize local distance variable
    double min_distance = -1.0;  // Initialize min distance variable
    int min_process_id = -1;      // Initialize min process ID variable
    Distance_CH newOBJ;
    int array[N];
    for(int i=0;i<N;i++){
        array[i]=0;
    }

    if (rank>(N/2)-1 && rank<N-1 && process_info.leader!=rank){    
        for(int i=0;i<repeats;i++){  
            MPI_Recv(REC,4,MPI_DOUBLE,process_info.leader,TAG_NEW_CHECK,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        
            double distance=haversine(REC[1],REC[2],REC[3],process_info.coordinates[0],process_info.coordinates[1],process_info.coordinates[2]);
            newOBJ.id=(int)REC[0];
            newOBJ.rec_distance=distance;

            MPI_Send(&newOBJ,1,MPI_DISTANCE_CH,process_info.leader,TAG_DISTANCE,MPI_COMM_WORLD);
        }
    }else if(rank>(N/2)-1 && process_info.leader==rank && rank<N-1){
            
            // Initialize minimum distance and process IDs
            int min_process_id = -1;
            int connect_W = -1;

            for(int i = N/2; i < N-1; i++) {
                double min_distance = -1.0;
                if (i == rank) {
                    continue; // Skip the current process
                } else {
                    for (int j = 0; j < repeats; j++) {
                        Distance_CH newOBJ;
                        MPI_Recv(&newOBJ, 1, MPI_DISTANCE_CH, i, TAG_DISTANCE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        // Update minimum distance and process IDs if a new minimum is found
                        if (min_distance == -1.0 || newOBJ.rec_distance < min_distance) {
                            min_distance = newOBJ.rec_distance;
                            min_process_id = i;
                            connect_W = newOBJ.id;
                        }
                    }
                    // Print the process with minimum distance and its connection
                    if (min_process_id != -1) {
                        printf("Process with minimum distance: %d and connect with %d\n", min_process_id, connect_W);
                        status_exchange[min_process_id][0]=connect_W;
                    }
                }
            }
        for (int i = 0; i < N-1; i++) {
            if(i>=N/2 && i!=rank){
                MPI_Send(&status_exchange[i][0],1,MPI_INT,i,TAG_NEW_CHECK,MPI_COMM_WORLD);
                MPI_Send(&i,1,MPI_INT,status_exchange[i][0],TAG_CONNECT,MPI_COMM_WORLD);
                array[status_exchange[i][0]]++;
            }
        }
        for (int i = 0; i < N/2; i++) {
            MPI_Send(&array[i],1,MPI_INT,i,TAG_REPLY,MPI_COMM_WORLD);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    int saved=0;

    if(rank<N/2){
        MPI_Recv(&saved,1,MPI_INT,MPI_ANY_SOURCE,TAG_REPLY,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

    //update the correct status
    int oke;
    if(rank<N/2){
        for(int i=0;i<saved;i++){
            MPI_Recv(&oke,1,MPI_INT,MPI_ANY_SOURCE,TAG_CONNECT,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Send(&process_info.status,1,MPI_DOUBLE,oke,TAG_STATUS_EXCHANGE,MPI_COMM_WORLD);
        }
    }
    


    
    MPI_Barrier(MPI_COMM_WORLD);
    int ok;
    if(rank>=N/2 && rank!=process_info.leader && rank<N-1){
        MPI_Recv(&ok,1,MPI_INT,process_info.leader,TAG_NEW_CHECK,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        //printf("connect rank %d with %d\n",rank,ok);

        MPI_Recv(&process_info.status,1,MPI_DOUBLE,ok,TAG_STATUS_EXCHANGE,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        printf("PROCESS %d STATUS UPDATED TO %lf\n",process_info.id,process_info.status);
    
    }


    MPI_Barrier(MPI_COMM_WORLD);

    //take measuremnt for avg_earth_temp
    Measurement mea;
    if(rank==0){
        MPI_Recv(&mea,1,MPI_MEASUREMENT,N-1,TAG_MEASURMENTS,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        printf("ST LEADER %d AND HOUR IS %d\n",process_info.leader,mea.timestamp.tm_hour);

        MPI_Send(&mea,1,MPI_MEASUREMENT,process_info.leader,TAG_MEASURMENTS,MPI_COMM_WORLD);
    }

    //arrive at SS leader
    int indexT;
    if (rank==process_info.leader && rank<N/2){
       MPI_Recv(&mea,1,MPI_MEASUREMENT,0,TAG_MEASURMENTS,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
       for (int i = 0; i < MAX_MEASUREMENTS; i++) {
            if (mea.timestamp.tm_hour == process_info.measurements[i].timestamp.tm_hour) {
            indexT = i;  // Set indexT to the index where matching hour is found
            break;  // Break out of the loop since a match is found
            }
        }
        for(int i=0;i<N/2;i++){
            if(rank==i){
                continue;
            }else{
                MPI_Send(&indexT,1,MPI_INT,i,TAG_CHECK,MPI_COMM_WORLD);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double avg_temp;
    if(rank<N/2){
        if(rank!=process_info.leader){
            MPI_Recv(&indexT,1,MPI_INT,process_info.leader,TAG_CHECK,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Send(&process_info.measurements[indexT].temperature,1,MPI_DOUBLE,process_info.leader,TAG_BROADCAST,MPI_COMM_WORLD);
        }
    }
    double tot;
    if(rank<N/2 && rank==process_info.leader){
        double count;
        double total=0;
        for(int i=0;i<N/2;i++){
            if(i==rank){
                continue;
            }else{
                MPI_Recv(&count,1,MPI_DOUBLE,i,TAG_BROADCAST,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            }
            total=total+count;
        }
        total=total+process_info.measurements[indexT].temperature;
        tot=total/(N/2);
        printf("AVG TEMPERATURE IS %lf\n",tot);
        MPI_Send(&tot,1,MPI_DOUBLE,process_info.leaderGS,TAG_NEW_CHECK,MPI_COMM_WORLD);
    }


    MPI_Barrier(MPI_COMM_WORLD);

    //take sync messages
    if(rank>=N/2 && rank<N-1){
        int rec;
        MPI_Recv(&rec,1,MPI_INT,N-1,TAG_SYNC,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        int changed;
        if(process_info.status!=0.0){
            changed=1;
        }else{
            changed=0;
        }

        
        if(rank!=process_info.leader){
            MPI_Send(&changed,1,MPI_INT,process_info.leader,TAG_SYNC,MPI_COMM_WORLD);
        }
    }

    int total_CH=0;
    if(rank>=N/2 && rank<N-1 && rank==process_info.leader){
        int rec;
        for(int i=N/2;i<N-1;i++){
            if(i==rank){
                continue;
            }else{
                MPI_Recv(&rec,1,MPI_INT,i,TAG_SYNC,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                total_CH+=rec;
            }
        }
        if(process_info.status!=0.0){
            total_CH++;
        }

        printf("TOTAL STATUS CHECKS WERE %d\n",total_CH);
        MPI_Send(&total_CH,1,MPI_INT,N-1,TAG_SYNC,MPI_COMM_WORLD);
    }

    //coordinator receive the number of checks and print it

    if(rank==N-1){
        int rec;
        MPI_Recv(&rec,1,MPI_INT,process_info.leaderGS,TAG_SYNC,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        printf("COORDINATOR REPORT:TOTAL STATUS CHECKS WERE %d\n",rec);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //take print messages
    if(rank>=N/2 && rank<N-1){
        int rec;
        MPI_Recv(&rec,1,MPI_INT,N-1,TAG_PRINT,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank<N-1 && rank>(N/2)-1){
        printf("Process %d updated its coordinates to: (%f, %f, %f) and status (%lf)\n", rank, process_info.coordinates[0], process_info.coordinates[1], process_info.coordinates[2],process_info.status);
    }else if(rank<=(N/2)-1){
        printf("Process %d updated its coordinates to: (%f, %f, %f) and status (%lf)\n", rank, process_info.coordinates[0], process_info.coordinates[1], process_info.coordinates[2],process_info.status);
        for(int i=0;i<3;i++){
                printf("   Proccess:%d -> Received temperature: %lf, Timestamp: %d/%d/%d %d:%d:%d\n",rank,
                process_info.measurements[i].temperature,
                process_info.measurements[i].timestamp.tm_mday, process_info.measurements[i].timestamp.tm_mon, process_info.measurements[i].timestamp.tm_year,
                process_info.measurements[i].timestamp.tm_hour, process_info.measurements[i].timestamp.tm_min, process_info.measurements[i].timestamp.tm_sec);
            }
    }

    if(rank<N/2 && rank==process_info.leader){
        printf("AVG TEMP %lf\n",tot);
    }


    MPI_Finalize();
    return EXIT_SUCCESS;
}

