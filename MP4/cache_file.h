#ifndef CACHE_FILE_H
#define CACHE_FILE_H

#include <time.h>
#define MAX_STR_LEN (1 << 12)   // 2K
#define MAX_CACHE_CNT 10
#define MAX_TS_LEN 50

extern char *strDay[7];
extern char *strMonth[12];

// cache structure definition
typedef struct
{
    char url[MAX_STR_LEN];
    char modified_timestamp[MAX_TS_LEN];
    char expire[MAX_TS_LEN];
    char file_name[MAX_STR_LEN];
    int  is_valid;
}stCache;

stCache stCache_LRU[MAX_CACHE_CNT];

int get_cache_idx(char *url);
void display_cache_LRU(void);
int month_converter(char* month);
int compare_timestamp(char *old_timestamp, char *new_timestamp);
int check_expiration(char *url, struct tm *gmtTime);
void set_cache_entry_valid(int idx);
int get_victim_idx(int cur_target_idx);
#endif // CACHE_FILE_H