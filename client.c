#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#include "shared_memory.h"
#include "file_utils.h"

#define CONNECT_CHANNEL_FNAME "srv_conn_channel"
#define CONNECT_CHANNEL_SIZE (1024)
#define CONNECT_CHANNEL_SEM_FNAME "srv_conn_channel_sem"

#define MAX_CLIENT_NAME_LEN (1024)
#define MAX_BUFFER_SIZE (1024)

#define UNREGISTER_STRING "unregister"

#define TEMP_CLIENT_SIZE (1024)

bool connect_to_server(const char *filename)
{
    char *shm_connect_channel = (char *)attach_memory_block(CONNECT_CHANNEL_FNAME, CONNECT_CHANNEL_SIZE);
    create_file_if_does_not_exist(CONNECT_CHANNEL_SEM_FNAME);

    sem_unlink(CONNECT_CHANNEL_SEM_FNAME);

    // Using mode 0 when creating semaphore, so that it only works if the semaphore already exists.
    // verify that this does actually work.
    sem_t *conn_channel_sem = sem_open(CONNECT_CHANNEL_SEM_FNAME, O_CREAT, 0, 1);
    if (conn_channel_sem == SEM_FAILED)
    {
        if (errno == ENOENT)
            fprintf(stderr, "Semaphore does not exist. The server is likely not running.\n");
        else
            fprintf(stderr, "Semaphore open failed due to unkown reasons.\n");

        exit(EXIT_FAILURE);
    }

    if (shm_connect_channel == NULL)
    {
        fprintf(stderr, "ERROR: Could not get block %s\n", CONNECT_CHANNEL_FNAME);
        exit(EXIT_FAILURE);
    }

    // Append the new filename to the connect channel list
    // printf("Starting sem wait...\n");
    sem_wait(conn_channel_sem);
    sprintf(shm_connect_channel, "%s %s", shm_connect_channel, filename);
    sem_post(conn_channel_sem);

    sem_close(conn_channel_sem);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: ./client <unique_name_for_client>\n");
        exit(EXIT_FAILURE);
    }

    char client_name[MAX_CLIENT_NAME_LEN];
    memcpy(client_name, argv[1], MAX_CLIENT_NAME_LEN);

    // If the connection file does not exist, then the server is probably not running.
    if (!does_file_exist(CONNECT_CHANNEL_FNAME))
    {
        fprintf(stderr, "Server not running. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    connect_to_server(client_name);

    return 0;
}