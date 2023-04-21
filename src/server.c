#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "shared_memory.h"
#include "file_utils.h"
#include "common_structs.h"
#include "worker.h"

int register_client(char *client_name)
{
    int client_name_fd = create_file_if_does_not_exist(client_name);
    if (client_name_fd < 0)
    {
        if (errno == EEXIST)
            fprintf(stderr, "The client %s already exists. Please try a new client_name\n", client_name);

        else
            fprintf(stderr, "Failed to create file to use as shared pointer.");

        return -1;
    }

    // Setup New SHM, etc.
    WorkerArgs *args = malloc(sizeof(WorkerArgs));
    memcpy(args->client_name, client_name, MAX_CLIENT_NAME_LEN);

    args->shm_comm_channel = create_req_or_res(client_name);
    if (args->shm_comm_channel == NULL)
        return -1;

    pthread_t client_tid;
    pthread_attr_t client_tattr;

    // Detach the thread - A detached thread can't be joined.
    pthread_attr_setdetachstate(&client_tattr, PTHREAD_CREATE_DETACHED);
    pthread_create(&client_tid, &client_tattr, worker_function, args);

    // TODO: Create a worker thread for this client.
    return ftok(client_name, 0);
}

int main(int argc, char **argv)
{
    // If the connection channel file does not exist, create it for SHM
    create_file_if_does_not_exist(CONNECT_CHANNEL_FNAME);

    char *shm_connect_channel = (char *)attach_memory_block(CONNECT_CHANNEL_FNAME, CONNECT_CHANNEL_SIZE);
    if (shm_connect_channel == NULL)
    {
        fprintf(stderr, "ERROR: Could not get block %s\n", CONNECT_CHANNEL_FNAME);
        exit(EXIT_FAILURE);
    }

    char client_name[MAX_CLIENT_NAME_LEN];

    while (true)
    {
        char *token = strtok(shm_connect_channel, " ");
        while (token != NULL)
        {
            printf("Request received to register new client: %s\n", token);
            int key = register_client(token);
            token = strtok(NULL, " ");
        }

        clear_memory_block(shm_connect_channel, CONNECT_CHANNEL_SIZE);

        sleep(1);
    }

    return 0;
}