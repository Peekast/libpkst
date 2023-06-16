#ifndef _PKST_STATS_H
#define _PKST_STATS_H

#include <unistd.h>
#include <time.h>

typedef struct {
    int input_pkts;
    int output_pkts;
    time_t start_time;
    time_t current_time;
} PKSTStats;


extern void pkts_print_stats(const PKSTStats *stats);

extern char *pkst_stats_to_json(const PKSTStats *stats);
#endif