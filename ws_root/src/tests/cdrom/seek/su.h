/* @(#) su.h 95/09/18 1.1 */

unsigned char *AllocOrDie(int size);

int int2time(int value);
int time2int(int value);
int myatoi(char *string);

int track_to_sector(int track);
int sector_to_track(int sector);
int sectors_per_track(int track);

uint32 time_seek(int start, int end);
uint32 TimerOverhead(void);

void get_minmax(int start, int end , int *pmin, int *pmax);
void get_maxmax(int start, int end , int *pfor, int *prev);
void get_minmin(int start, int end , int *pfor, int *prev);
int get_min_time(int start, int end);
