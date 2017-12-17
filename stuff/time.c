// https://www.epochconverter.com/programming/c

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 101

char time_buffer[BUFFER_SIZE];
unsigned long long int seconds;

// Convert time FROM seconds TO buffer
void time_convert() {
    time_t tmp = seconds;
    struct tm ts = *localtime(&tmp);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &ts);
}

// Convert time FROM buffer TO seconds
bool time_load() {
    int retval;
    struct tm t;
    retval = sscanf(time_buffer, "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon , &t.tm_mday, &t.tm_hour, &t.tm_min , &t.tm_sec);
    if (retval != 6) return false;
    t.tm_year = t.tm_year - 1900;
    seconds = (long)mktime(&t);
    return true;
}

int main(void) {

    seconds = (unsigned long long int)time(NULL);

    fprintf(stdout, "BUFFER     ==>%s<==\n", time_buffer);
    fprintf(stdout, "SECONDS    ==>%lld<==\n", seconds);
    fprintf(stdout, "time_convert();\n");
    time_convert();
    fprintf(stdout, "BUFFER     ==>%s<==\n", time_buffer);
    fprintf(stdout, "SECONDS    ==>%lld<==\n", seconds);

    fprintf(stdout, "\n");

    fprintf(stdout, "BUFFER     ==>%s<==\n", time_buffer);
    fprintf(stdout, "SECONDS    ==>%lld<==\n", seconds);
    fprintf(stdout, "time_load();\n");
    time_load();
    fprintf(stdout, "BUFFER     ==>%s<==\n", time_buffer);
    fprintf(stdout, "SECONDS    ==>%lld<==\n", seconds);

    return 0;
}
