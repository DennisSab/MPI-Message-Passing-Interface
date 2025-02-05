#include "header.h"

void readEvent(const char* filename, double (*event_data)[3],int *parents,double (*check_event)[3]) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int myparent;
    char event_type[20];
    int process_id;
    double status_value;
    double time[3];
    double temp;
    int received_PRO;
    double status[20][3];
    repeats=0;
    int check_index[N/2];
    double tempD[4];



    while (fscanf(file, "%s",event_type )!= EOF){
        if (strcmp(event_type, "ADD_GS_COORDINATES") == 0) {
            if (fscanf(file, "%d %lf %lf %lf",&process_id,&tempD[0], &tempD[1],&tempD[2]) != 4) {
                printf("Error reading event data for ADD_GS_COORDINATES\n");
                exit(EXIT_FAILURE);
            }
            event_data[process_id][0]=tempD[0];
            event_data[process_id][1]=tempD[1];
            event_data[process_id][2]=tempD[2];
            MPI_Send(event_data[process_id], 3, MPI_DOUBLE,process_id, TAG_COORDINATES, MPI_COMM_WORLD);
        } else if (strcmp(event_type, "ADD_ST_COORDINATES") == 0) {
            if (fscanf(file, "%d %lf %lf %lf",&process_id,&tempD[0], &tempD[1],&tempD[2]) != 4) {
                printf("Error reading event data for ADD_GS_COORDINATES\n");
                exit(EXIT_FAILURE);
            }
            event_data[process_id][0]=tempD[0];
            event_data[process_id][1]=tempD[1];
            event_data[process_id][2]=tempD[2];  
            MPI_Send(event_data[process_id], 3, MPI_DOUBLE,process_id, TAG_COORDINATES, MPI_COMM_WORLD);
        }else if(strcmp(event_type,"ADD_STATUS") == 0){
            if(fscanf(file,"%d %lf",&process_id,&status_value)!=2){
                printf("Error reading event data for ADD_STATUS\n");
                exit(EXIT_FAILURE);
            }
            MPI_Send(&status_value,1,MPI_DOUBLE,process_id,TAG_STATUS,MPI_COMM_WORLD);
        }else if(strcmp(event_type,"ADD_METRIC")==0){
            struct tm tm_result;
            if (fscanf(file, "%d %lf %d:%d:%d %d/%d/%d",&process_id,&temp, &tm_result.tm_hour, &tm_result.tm_min, &tm_result.tm_sec, 
               &tm_result.tm_mday, &tm_result.tm_mon, &tm_result.tm_year) != 8) {
                fprintf(stderr, "Error parsing timestamp\n");
                exit(EXIT_FAILURE);
            }

            Measurement measuremnt={temp,tm_result};
            MPI_Send(&measuremnt,1,MPI_MEASUREMENT,process_id,TAG_MEASURMENTS,MPI_COMM_WORLD);
        }else if(strcmp(event_type,"CONNECT")==0){
            if(fscanf(file,"%d %d",&process_id,&myparent) !=2){
                fprintf(stderr,"Error parsing timestamp\n");
                exit(EXIT_FAILURE);
            }
            parents[process_id]=myparent;
        }else if(strcmp(event_type,"START_LELECT_ST")==0){
            int message=1;
            for(int i=0;i<=(N/2)-1;i++){
                MPI_Send(&message,1,MPI_INT,i,TAG_ELECTION,MPI_COMM_WORLD);
            }
        }else if(strcmp(event_type,"START_LELECT_GS")==0){
            int message=1;
            for(int i=N/2;i<N-1;i++){
                MPI_Send(&message,1,MPI_INT,i,TAG_ELECTION,MPI_COMM_WORLD);
            }
        }else if(strcmp(event_type,"STATUS_CHECK")==0){ 
            if(fscanf(file, "%d %lf %lf %lf", &process_id , &tempD[1], &tempD[2], &tempD[3]) != 4) {
                fprintf(stderr, "Error parsing STATUS_CHECK\n");
                exit(EXIT_FAILURE);
            }
            tempD[0]=(double)process_id;
            MPI_Send(tempD,4,MPI_DOUBLE,0,TAG_CHECK,MPI_COMM_WORLD);
            repeats++;
        }else if(strcmp(event_type,"AVG_EARTH_TEMP")==0){
            struct tm tm_result;
            if(fscanf(file,"%d:%d:%d %d/%d/%d",&tm_result.tm_hour, &tm_result.tm_min, &tm_result.tm_sec, 
               &tm_result.tm_mday, &tm_result.tm_mon, &tm_result.tm_year)!=6){
                fprintf(stderr, "ERROR PASSING AVG_EARTH_TEMP\n");
                exit(EXIT_FAILURE);
            }
            Measurement measurement={(double)0,tm_result};
            MPI_Send(&measurement,1,MPI_MEASUREMENT,0,TAG_MEASURMENTS,MPI_COMM_WORLD);
        }else if(strcmp(event_type,"SYNC")==0){
            for(int i=N/2;i<N-1;i++){
                MPI_Send(&i,1,MPI_INT,i,TAG_SYNC,MPI_COMM_WORLD);
            }
        }else if(strcmp(event_type,"PRINT")==0){
            for(int i=N/2;i<N-1;i++){
                MPI_Send(&i,1,MPI_INT,i,TAG_PRINT,MPI_COMM_WORLD);
            }
        }else{
            printf("Unknown event type: %s\n", event_type);
        }
        fscanf(file, "\n"); // Skip newline character
    }
    fclose(file);
}