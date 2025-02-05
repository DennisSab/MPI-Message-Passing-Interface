#include "header.h"

double haversine(double lat1,double lon1,double alt1,double lat2,double lon2,double alt2){
    double r=6371.0;

    lat1=lat1* M_PI / 180.0;
    lon1=lon1* M_PI / 180.0;
    lat2=lat2* M_PI / 180.0;
    lon2=lon2* M_PI / 180.0;

    double dlat = lat2 -lat1;
    double dlon = lon2 -lon1;

    double a=sin(dlat/2) * sin(dlat/2)+ cos(lat1) * cos(lat2) * sin(dlon/2) * sin(dlon/2);
    double c=2*atan2(sqrt(a),sqrt(1-a));

    double distance=r*c;

    distance +=abs(alt2 - alt1);
    return distance;
}
