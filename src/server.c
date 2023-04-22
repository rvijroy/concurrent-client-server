#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "shared_memory.h"
#include "utils.h"
#include "common_structs.h"
#include "worker.h"
#include "logger.h"
#include "conn_chanel.h"

int register_client(RequestOrResponse *conn_reqres)
{
    wait_until_stage(conn_reqres, 0);
    int comm_channel_exists = does_file_exist(conn_reqres->client_name);
    if (comm_channel_exists == 1)
    {
        logger("ERROR", "The client %s already exists. Please try a new client_name", conn_reqres->client_name);
        return -1;
    }
    else if (comm_channel_exists != 0)
    {
        logger("ERROR", "Something went wrong when verify client name (%s) validity. Please debug with other messages.", conn_reqres->client_name);
        return -1;
    }

    WorkerArgs *args = malloc(sizeof(WorkerArgs));
    strncpy(args->client_name, conn_reqres->client_name, MAX_CLIENT_NAME_LEN);

    args->comm_channel_block_id = create_comm_channel(args->client_name);
    if (args->comm_channel_block_id == IPC_RESULT_ERROR)
    {
        logger("ERROR", "Could not get shared block id for client %s.", args->client_name);
        return -1;
    }

    pthread_t client_tid;
    pthread_attr_t client_tattr;
    pthread_create(&client_tid, NULL, worker_function, args);

    conn_reqres->key = ftok(args->client_name, 0);
    printf("INFO: Client registered succesfully with key: %d\n", conn_reqres->key);

    set_stage(conn_reqres, 1);
    return 0;
}

int main(int argc, char **argv)
{
    if (init_logger("server") == EXIT_FAILURE)
        return EXIT_FAILURE;

    queue_t *conn_q = create_queue();
    if (conn_q == NULL)
    {
        logger("ERROR", "Could not create connection queue.");
        exit(EXIT_FAILURE);
    }

    while (true)
    {

        // ? Both the client and server have references to the particular request after this dequee.
        // ? Consequently, we don't need to keep the request on the queue.
        // ? Any updates required can be done directly on the shared memory buffer.
        RequestOrResponse *conn_reqres = dequeue(conn_q);
        // RequestOrResponse *conn_reqres = fetch(conn_q);
        if (conn_reqres)
        {
            printf("Waiting for registration...\n");

            printf("Request received to register new client: %s\n", conn_reqres->client_name);
            if (register_client(conn_reqres) < 0)
            {
                logger("ERROR", "Could not register client %s", conn_reqres->client_name);
            }
        }

        msleep(400);
    }

    close_logger();

    return 0;
}