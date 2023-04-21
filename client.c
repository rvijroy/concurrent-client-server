#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#include "shared_memory.h"
#include "common_structs.h"
#include "file_utils.h"

#define CONNECT_CHANNEL_FNAME "srv_conn_channel"
#define CONNECT_CHANNEL_SIZE (1024)
#define CONNECT_CHANNEL_SEM_FNAME "srv_conn_channel_sem"

#define MAX_CLIENT_NAME_LEN (1024)
#define MAX_BUFFER_SIZE (1024)

#define UNREGISTER_STRING "unregister"

#define TEMP_CLIENT_SIZE (1024)

bool connect_to_server(const char *client_name)
{
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

    sem_wait(conn_channel_sem);
    char *shm_connect_channel = (char *)attach_memory_block(CONNECT_CHANNEL_FNAME, CONNECT_CHANNEL_SIZE);
    if (shm_connect_channel == NULL)
    {
        fprintf(stderr, "ERROR: Could not get connect channel block %s\n", CONNECT_CHANNEL_FNAME);
        exit(EXIT_FAILURE);
    }
    sem_post(conn_channel_sem);

    // Append the new client_name to the connect channel list
    // printf("Starting sem wait...\n");
    sem_wait(conn_channel_sem);
    printf("Sending request to create client: %s\n", client_name);
    sprintf(shm_connect_channel, "%s %s", shm_connect_channel, client_name);
    sem_post(conn_channel_sem);

    sem_close(conn_channel_sem);
    return 0;
}

void communicate(const char *client_name)
{

    char req_or_res_sem_fname[MAX_BUFFER_SIZE];
    sprintf(req_or_res_sem_fname, "%s_sem", client_name);

    create_file_if_does_not_exist(req_or_res_sem_fname);
    sem_t *req_or_res_sem = sem_open(req_or_res_sem_fname, 0);
    if (req_or_res_sem == SEM_FAILED)
    {
        fprintf(stderr, "Semaphore open failed due to unkown reasons.\n");
        exit(EXIT_FAILURE);
    }

    // int sem_value = 0;
    // while (!sem_value)
    // {
    //     sem_getvalue(req_or_res_sem, &sem_value);
    //     printf("Sem value: %d\n", sem_value);
    // }

    // sem_wait(req_or_res_sem);
    // It is assumed, that at the point the file with name client_name has already been created.
    RequestOrResponse *req_or_res = attach_memory_block(client_name, sizeof(RequestOrResponse));
    if (req_or_res == NULL)
    {
        fprintf(stderr, "ERROR: Could not get comm channel block: %s\n", client_name);
        return;
    }
    sem_post(req_or_res_sem);

    int sem_value = 0;
    while (!sem_value)
    {
        sem_getvalue(req_or_res_sem, &sem_value);
        printf("Sem value: %d\n", sem_value);
    }

    int n1, n2;
    char op;

    // TODO: Fix synchronization.
    /*
    semaphore_value set_by  used_by  operation
         1          client  client    read from user, and write request.
         2          client  server    read request, process and write response.
         3          server  client    read response and print.
    */

    // TODO: Handle req seq numbers.
    // TODO: Handle res seq numbers.

    int current_choice = 0;
    while (true)
    {
        // Busy waiting:
        // While semaphore value is not 1, keep waiting.
        scanf("%d", &current_choice);

        if (current_choice == 0) // ARITHMETIC
        {
            printf("Enter two numbers: ");
            scanf("%d %d %c", &n1, &n2, &op);

            req_or_res->req.request_type = ARITHMETIC;
            req_or_res->req.n1 = n1;
            req_or_res->req.n2 = n2;
            req_or_res->req.op = op;
        }

        else if (current_choice == 1) // EVEN_OR_ODD
        {
            printf("Enter number to check evenness: ");
            scanf("%d", &n1);

            req_or_res->req.request_type = EVEN_OR_ODD;
            req_or_res->req.n1 = n1;
        }

        else if (current_choice == 2) // IS_PRIME
        {
            printf("Enter number to check primality: ");
            scanf("%d", &n1);

            req_or_res->req.request_type = IS_PRIME;
            req_or_res->req.n1 = n1;
        }

        else if (current_choice == 3) // IS_NEGATIVE
        {
            printf("Enter number to check sign: ");
            scanf("%d", &n1);

            req_or_res->req.request_type = IS_NEGATIVE;
            req_or_res->req.n1 = n1;
        }

        else if (current_choice == 4) // UNREGISTER
        {
            req_or_res->req.request_type = UNREGISTER;

            break;
        }

        else if (current_choice == 5) // DON'T DO ANYTHING AND EXIT
        {
            break;
        }

        // Set semaphore value to 2
        // Wait till semaphore value is 3
        //      Print Result
        // Set semaphore value to 1
    }
}

int main(int argc, char **argv)
{
    // if (argc < 2)
    // {
    //     printf("Usage: ./client <unique_name_for_client>\n");
    //     exit(EXIT_FAILURE);
    // }

    char client_name[MAX_CLIENT_NAME_LEN] = "xyz_";
    // memcpy(client_name, argv[1], MAX_CLIENT_NAME_LEN);

    // If the connection file does not exist, then the server is probably not running.
    if (!does_file_exist(CONNECT_CHANNEL_FNAME))
    {
        fprintf(stderr, "Server not running. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    connect_to_server(client_name);
    printf("It worked!");
    communicate(client_name);

    return 0;
}