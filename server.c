#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#include "shared_memory.h"
#include "file_utils.h"
#include "common_structs.h"

#define CONNECT_CHANNEL_FNAME "srv_conn_channel"
#define CONNECT_CHANNEL_SIZE (1024)
#define CONNECT_CHANNEL_SEM_FNAME "srv_conn_channel_sem"

#define MAX_CLIENT_NAME_LEN (1024)
#define MAX_BUFFER_SIZE (1024)

#define UNREGISTER_STRING "unregister"

#define TEMP_CLIENT_SIZE (1024)

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

    // TODO: Setup Semaphor for the buffer as well.
    char comm_channel_sem_fname[MAX_BUFFER_SIZE];
    sprintf(comm_channel_sem_fname, "%s_sem", client_name);

    create_file_if_does_not_exist(comm_channel_sem_fname);
    sem_unlink(comm_channel_sem_fname);
    sem_t *comm_channel_sem = sem_open(comm_channel_sem_fname, O_CREAT, 0644, 1);
    if (comm_channel_sem == SEM_FAILED)
    {
        fprintf(stderr, "Semaphore open failed due to unkown reasons.\n");
        exit(EXIT_FAILURE);
    }

    int sem_value = 0;

    // Setup New SHM, etc.
    sem_wait(comm_channel_sem);

    sem_getvalue(comm_channel_sem, &sem_value);
    printf("Creating chanel. Loaded semaphore. Initial value: %d\n", sem_value);

    void *shm_comm_channel = attach_memory_block(client_name, sizeof(RequestOrResponse));
    if (shm_comm_channel == NULL)
    {
        fprintf(stderr, "ERROR: Could not get comm. block: %s\n", client_name);
        return -1;
    }

    clear_memory_block(shm_comm_channel, sizeof(RequestOrResponse));
    int ret = sem_post(comm_channel_sem);
    printf("sem_post returned %d\n", ret);

    sem_getvalue(comm_channel_sem, &sem_value);
    printf("Creating chanel. Loaded semaphore. Final value: %d\n", sem_value);

    sem_close(comm_channel_sem);
    // TODO: Create a worker thread for this client.
    return ftok(client_name, 0);
}

int main(int argc, char **argv)
{
    // If the connection channel file does not exist, create it for SHM
    create_file_if_does_not_exist(CONNECT_CHANNEL_FNAME);
    create_file_if_does_not_exist(CONNECT_CHANNEL_SEM_FNAME);

    sem_unlink(CONNECT_CHANNEL_SEM_FNAME);
    sem_t *conn_channel_sem = sem_open(CONNECT_CHANNEL_SEM_FNAME, O_CREAT, 0644, 1);
    if (conn_channel_sem == SEM_FAILED)
    {
        fprintf(stderr, "Semaphore open failed due to unkown reasons.\n");
        exit(EXIT_FAILURE);
    }

    sem_wait(conn_channel_sem);
    char *shm_connect_channel = (char *)attach_memory_block(CONNECT_CHANNEL_FNAME, CONNECT_CHANNEL_SIZE);
    if (shm_connect_channel == NULL)
    {
        fprintf(stderr, "ERROR: Could not get connect channel block %s\n", CONNECT_CHANNEL_FNAME);
        exit(EXIT_FAILURE);
    }
    sem_post(conn_channel_sem);

    char client_name[MAX_CLIENT_NAME_LEN];

    for (int i = 0; i < 60; ++i)
    {
        printf("Starting sem wait...\n");
        sem_wait(conn_channel_sem);

        char *token = strtok(shm_connect_channel, " ");
        while (token != NULL)
        {
            printf("Request received to register new client: %s\n", token);
            int key = register_client(token);
            printf("Succesfully registered client %s\n", token);
            token = strtok(NULL, " ");
        }

        clear_memory_block(shm_connect_channel, CONNECT_CHANNEL_SIZE);

        sem_post(conn_channel_sem);
        sleep(1);
    }

    sem_close(conn_channel_sem);
    sem_unlink(CONNECT_CHANNEL_SEM_FNAME);
    return 0;
}