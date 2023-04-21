#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "shared_memory.h"
#include "file_utils.h"
#include "common_structs.h"
#include "worker.h"
#include "conn_chanel.h"

int register_client(RequestOrResponse *req_or_res)
{

    wait_until_stage(req_or_res, 0);
    if (!does_file_exist(req_or_res->req.client_name))
    {
        fprintf(stderr, "Error: The client %s already exists. Please try a new client_name\n", req_or_res->req.client_name);
        return -1;
    }

    WorkerArgs *args = malloc(sizeof(WorkerArgs));
    memcpy(args->client_name, req_or_res->req.client_name, MAX_CLIENT_NAME_LEN);

    args->shm_comm_channel = create_req_or_res(req_or_res->req.client_name);
    if (args->shm_comm_channel == NULL)
        return -1;

    pthread_t client_tid;
    pthread_attr_t client_tattr;

    // Detach the thread - A detached thread can't be joined.
    pthread_attr_setdetachstate(&client_tattr, PTHREAD_CREATE_DETACHED);
    pthread_create(&client_tid, &client_tattr, worker_function, args);

    req_or_res->res.key = ftok(req_or_res->req.client_name, 0);
    set_stage(req_or_res, 1);

    return 0;
}

int main(int argc, char **argv)
{
    queue_t *conn_q = create_queue();
    if (conn_q == NULL)
    {
        fprintf(stderr, "Error: Could not create connection queue.\n");
        exit(EXIT_FAILURE);
    }

    char client_name[MAX_CLIENT_NAME_LEN];

    while (true)
    {

        RequestOrResponse *req_or_res = fetch(conn_q);
        if (req_or_res)
        {
            printf("Waiting for registration...\n");
            printf("Request received to register new client: %s\n", req_or_res->req.client_name);
            if (register_client(req_or_res) < 0)
            {
                fprintf(stderr, "Error: Could not register client %s\n", req_or_res->req.client_name);
            }
        }

        sleep(1);
    }

    return 0;
}