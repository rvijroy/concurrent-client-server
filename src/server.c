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
#include "client_tree.h"

static queue_t *conn_q;

void cleanup()
{
    logger("INFO", "Starting cleanup.");
    destroy_queue(conn_q);

    logger("INFO", "Closing logger");
    close_logger();
}

void handle_sigint(int sig)
{
    cleanup();
    exit(sig);
}

int register_client(RequestOrResponse *conn_reqres)
{
    wait_until_stage(conn_reqres, 0);
    int comm_channel_exists = does_file_exist(conn_reqres->client_name);
    if (comm_channel_exists == 1)
    {
        logger("ERROR", "The client %s already exists. Please try a new client_name", conn_reqres->client_name);
        conn_reqres->res.response_code = RESPONSE_FAILURE;
        set_stage(conn_reqres, 1);

        return -1;
    }
    else if (comm_channel_exists != 0)
    {
        logger("ERROR", "Something went wrong when verify client name (%s) validity. Please debug with other messages.", conn_reqres->client_name);
        conn_reqres->res.response_code = RESPONSE_FAILURE;
        set_stage(conn_reqres, 1);

        return -1;
    }

    WorkerArgs *args = malloc(sizeof(WorkerArgs));
    strncpy(args->client_name, conn_reqres->client_name, MAX_CLIENT_NAME_LEN);

    args->comm_channel_block_id = create_comm_channel(args->client_name);
    if (args->comm_channel_block_id == IPC_RESULT_ERROR)
    {
        logger("ERROR", "Could not get shared block id for client %s.", args->client_name);
        conn_reqres->res.response_code = RESPONSE_FAILURE;
        set_stage(conn_reqres, 1);
        return -1;
    }

    pthread_t client_tid;
    pthread_create(&client_tid, NULL, worker_function, args);

    int key = ftok(args->client_name, 0);

    // TODO: Error handling
    insert_to_client_tree(key, conn_reqres->client_name); 

    conn_reqres->res.response_code = RESPONSE_SUCCESS;
    conn_reqres->res.result = key;
    logger("INFO", "Client registered succesfully with key: %d", conn_reqres->res.result);

    set_stage(conn_reqres, 1);
    return 0;
}

int main(int _argc, char **_argv)
{
    signal(SIGINT, handle_sigint);

    if (init_logger("server") == EXIT_FAILURE)
        return EXIT_FAILURE;

    conn_q = create_queue();
    if (conn_q == NULL)
    {
        logger("ERROR", "Could not create connection queue.");
        exit(EXIT_FAILURE);
    }

    init_client_tree();

    printf("Started server. Waiting for requests...\n");
    fflush(stdout);

    while (true)
    {

        // ? Both the client and server have references to the particular request after this dequee.
        // ? Consequently, we don't need to keep the request on the queue.
        // ? Any updates required can be done directly on the shared memory buffer.
        RequestOrResponse *conn_reqres = dequeue(conn_q);
        if (conn_reqres == (void *)-1)
        {
            logger("ERROR", "Failed to dequeue. System has probably run out of resources to create more shared memory blocks. Closing server...");
            printf("System might have run out of resources. Closing the server...");
            break;
        }

        // RequestOrResponse *conn_reqres = fetch(conn_q);
        if (conn_reqres)
        {

            logger("INFO", "Request received to register new client: %s", conn_reqres->client_name);
            if (register_client(conn_reqres) < 0)
            {
                logger("ERROR", "Could not register client %s", conn_reqres->client_name);
            }
        }

        logger("INFO", "Number of connected clients: %d", get_num_connected_clients());
        msleep(400);
    }

    cleanup();
    return 0;
}