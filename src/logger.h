#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>

#define LOG_FILENAME "main.log"
#define MAX_LOG_MSG_SIZE (1024)

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *log_file;

void logger(const char *code, const char *format, ...)
{
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", lt);

    va_list args;
    va_start(args, format);
    char log_msg[MAX_LOG_MSG_SIZE];
    vsnprintf(log_msg, MAX_LOG_MSG_SIZE, format, args);
    va_end(args);

    char final_msg[MAX_LOG_MSG_SIZE];
    snprintf(final_msg, MAX_LOG_MSG_SIZE, "%s %s %s\n", code, timestamp, log_msg);

    pthread_mutex_lock(&log_mutex);
    fputs(final_msg, log_file);
    fflush(log_file);
    pthread_mutex_unlock(&log_mutex);
}

int close_logger()
{
    return fclose(log_file);
}

int init_logger(const char *log_name)
{
    if (log_file != NULL)
    {
        close_logger();
    }

    char log_filename[MAX_LOG_MSG_SIZE];
    sprintf(log_filename, "%s.log", log_name);
    log_file = fopen(log_filename, "a");
    if (log_file == NULL)
    {
        fprintf(stderr, "ERROR: Failed to setup logger.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#endif