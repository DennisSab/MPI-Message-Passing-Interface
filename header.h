#ifndef PROJECT2_H
#define PROJECT2_H


#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <math.h>

#define Max_children 30
#define MAX_MEASUREMENTS 10
#define __USE_XOPEN
#define TAG_COORDINATES 0
#define TAG_STATUS 1
#define TAG_MEASURMENTS 2
#define TAG_ACK 4
#define TAG_CONNECT 5
#define TAG_ELECTION 6
#define TAG_PROBE 7
#define TAG_REPLY 8
#define LELECT_ST_DONE 9
#define TAG_ELECT 12
#define LELECT_GS_DONE 13
#define TAG_CHECK 14 
#define TAG_NEW_CHECK 15
#define TAG_DISTANCE 16
#define TAG_STATUS_EXCHANGE 17
#define TAG_BROADCAST 18
#define TAG_SYNC 19
#define TAG_PRINT 20



extern MPI_Datatype MPI_MEASUREMENT;

extern int N;
extern int *parents;
extern int repeats;


typedef struct {
    double temperature;
    struct tm timestamp;
}Measurement;

typedef struct {
    int id;
    double rec_distance;
}Distance_CH;

typedef struct {
    int id;
    double coordinates[3]; // Συντεταγμένες (πλάτος, μήκος, υψόμετρο)
    double status; // Κατάσταση ορθής λειτουργίας (μόνο για Satellites)
    Measurement measurements[MAX_MEASUREMENTS]; // Πίνακας μετρήσεων θερμοκρασίας (μόνο για Satellites και Ground Stations)
    int parent[Max_children];
    int leader;
    int leaderST;
    int leaderGS;
    double check_status[3];
} ProcessInfo;
extern ProcessInfo process_info;

void readEvent(const char* filename, double (*event_data)[3],int *parents,double (*check_event)[3]);
double haversine(double lat1,double lon1,double alt1,double lat2,double lon2,double alt2);

#endif