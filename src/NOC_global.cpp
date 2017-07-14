#include "time.h"
#include "stdio.h"
//#include "NOC_global.h"
bool MANUAL_PIC=true;

long pic_x=8;//320,192
long pic_y=4;//240,144
long mpix=pic_x*pic_y;
//long mpack=(mpix+mpix/2+2)/6+1  + 10;
long mpack=10;
FILE *InFile;
FILE *OutFile;

int packet_size_g=6;


int IP_CORE_DELAY=2*packet_size_g;

int frame_num_max_run=100;

char *res_file_name ;//;="./result.txt";



clock_t my_start_time, my_estimated_time;
clock_t StartTime,EndTime;



