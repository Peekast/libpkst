
#include <stdio.h>
#include <jansson.h>
#include "pkst_stats.h"


void pkts_print_stats(const PKSTStats *stats) {
    if (!stats)
        return;

    double elapsed_time = difftime(stats->current_time, stats->start_time);

    double input_speed = stats->input_pkts / elapsed_time;
    double output_speed = stats->output_pkts / elapsed_time;

    printf("Input packets: %8d, Speed: %8.2f packets/s, Output packets: %8d, Speed: %8.2f packets/s, Elapsed time: %8.2f s\n", 
            stats->input_pkts, input_speed, stats->output_pkts, output_speed, elapsed_time);
    fflush(stdout);
}
    

char *pkst_stats_to_json(const PKSTStats *stats) {
    json_t *jobj = json_object();

    // Asegúrate de comprobar que stats no es NULL antes de acceder a sus miembros
    if (stats) {
        json_object_set_new(jobj, "input_pkts", json_integer(stats->input_pkts));
        json_object_set_new(jobj, "output_pkts", json_integer(stats->output_pkts));
        json_object_set_new(jobj, "start_time", json_integer(stats->start_time));
        json_object_set_new(jobj, "current_time", json_integer(stats->current_time));
    }

    char *result = json_dumps(jobj, JSON_ENCODE_ANY);

    // Libera la memoria del objeto JSON
    json_decref(jobj);

    return result;
}




