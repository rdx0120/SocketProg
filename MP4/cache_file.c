#include "cache_file.h"
#include "string.h"
#include <stdio.h>

char *strDay[7]=
{
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

char *strMonth[12]=
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

int get_cache_idx(char *url)
{
    int i = 0;
    for (i = 0 ; i < MAX_CACHE_CNT; i++)
    {
        if (!strcmp (stCache_LRU[i].url, url))
        {
            return i;
        }
    }
    return -1;
}

void display_cache_LRU()
{
    int i = 0;
    printf("==============CACHE LIST==============\n");
    for (i = 0; i < MAX_CACHE_CNT; i++)
    {
        if (stCache_LRU[i].is_valid == 1)
        {
            printf("%d. ", i);
            printf("%s\n", stCache_LRU[i].url);
            printf("Expires: %s\n", stCache_LRU[i].expire);
            printf("Modified Time Stamp: %s\n", stCache_LRU[i].modified_timestamp);
            printf("File Name: %s\n", stCache_LRU[i].file_name);
        }
    }
    printf("===============END LIST===============\n");
}

int month_converter(char* month)
{
    int idx;
    int found = 0;

    for (idx = 0; idx < 12; idx++)
    {
        if (strcmp(month, strMonth[idx]) == 0)
        {
            found = 1;
            break;
        }
    }

    if (found == 0)
    {
        perror("ERROR: stored month value not found.");
        return -1;
    }

    idx += 1;
    return idx;
}

int compare_timestamp(char *old_timestamp, char *new_timestamp)
{
    int old_year, old_month, old_hour, old_minute, old_second,old_day;
    int new_year, new_month, new_hour, new_minute, new_second,new_day;

    char strOld_month[4];
    char strNew_month[4];

    memset(strOld_month, 0, 4);
    memset(strNew_month, 0, 4);

    sscanf(old_timestamp + 5, "%d %3s %d %d:%d:%d ", &old_day, strOld_month, &old_year, &old_hour, &old_minute, &old_second);
    sscanf(new_timestamp + 5, "%d %3s %d %d:%d:%d ", &new_day, strNew_month, &new_year, &new_hour, &new_minute, &new_second);
    old_month = month_converter(strOld_month);
    new_month = month_converter(strNew_month);

    if (old_year < new_year) return -1;
    if (old_year > new_year) return 1;
    if (old_month < new_month) return -1;
    if (old_month > new_month) return 1;
    if (old_day < new_day) return -1;
    if (old_day > new_day) return 1;
    if (old_hour < new_hour) return -1;
    if (old_hour > new_hour) return 1;
    if (old_minute < new_minute) return -1;
    if (old_minute > new_minute) return 1;
    if (old_second < new_second) return -1;
    if (old_second > new_second) return 1;
    return 0;
}

int check_expiration(char *url, struct tm *gmtTime)
{
    int i = 0;
    int rslt = 0;
    char updated_timestamp[MAX_TS_LEN];
    for (i = 0; i < MAX_CACHE_CNT; i++)
    {
        if (strcmp(stCache_LRU[i].url, url)==0)
        {
            break;
        }
    }
    memset(updated_timestamp, 0, MAX_TS_LEN);
    sprintf(updated_timestamp, "%s, %2d %s %4d %2d:%2d:%2d GMT", 
        strDay[gmtTime->tm_wday], gmtTime->tm_mday, strMonth[gmtTime->tm_mon], 
        gmtTime->tm_year+1900, gmtTime->tm_hour, gmtTime->tm_min, gmtTime->tm_sec);

    rslt = compare_timestamp(stCache_LRU[i].expire, updated_timestamp);
    if (rslt < 0)
    {
        return -1;
    }

    return 0;
}

void set_cache_entry_valid(int idx)
{
    stCache_LRU[idx].is_valid = 1;
}

int get_victim_idx(int cur_target_idx)
{
    int idx = 0;
    for (idx = 0; idx < MAX_CACHE_CNT; idx++)
    {
        if(stCache_LRU[idx].is_valid == 0)
        {
            cur_target_idx = idx;
            break;
        }
        else
        {
            if (compare_timestamp(stCache_LRU[idx].modified_timestamp, stCache_LRU[cur_target_idx].modified_timestamp) <= 0)
            {
                cur_target_idx = idx;
            }
        }
    }
    return cur_target_idx;
}
