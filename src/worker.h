#ifndef WORKER_H
#define WORKER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "logger.h"
#include "common_structs.h"

#define CONNECT_CHANNEL_FNAME "srv_conn_channel"
#define CONNECT_CHANNEL_SIZE (1024)

#define MAX_CLIENT_NAME_LEN (1024)
#define MAX_BUFFER_SIZE (1024)

#define UNREGISTER_STRING "unregister"

#define TEMP_CLIENT_SIZE (1024)

typedef struct WorkerArgs
{
    char client_name[MAX_CLIENT_NAME_LEN];
    int comm_channel_block_id;
} WorkerArgs;

Response handle_arithmetic(Request req)
{
    Response res;
    switch (req.op)
    {
    case '+':
        res.result = req.n1 + req.n2;
        res.response_code = RESPONSE_SUCCESS;
        break;
    case '-':
        res.result = req.n1 - req.n2;
        res.response_code = RESPONSE_SUCCESS;
        break;
    case '*':
        res.result = req.n1 * req.n2;
        res.response_code = RESPONSE_SUCCESS;
        break;
    case '/':
        if (req.n2 == 0)
            res.response_code = RESPONSE_UNSUPPORTED;
        else
        {
            res.result = req.n1 / req.n2;
            res.response_code = RESPONSE_SUCCESS;
        }
        break;

    default:
        res.response_code = RESPONSE_UNSUPPORTED;
        break;
    }
    return res;
}

Response handle_even_or_odd(Request req)
{
    Response res;
    res.result = req.n1 % 2;
    res.response_code = RESPONSE_SUCCESS;
    return res;
}

Response handle_is_prime(Request req)
{
    Response res;
    if (req.n1 < 0)
    {
        res.response_code = RESPONSE_UNKNOWN_FAILURE;
        return res;
    }

    // 0 and 1 are not prime numbers.
    if (req.n1 == 0 || req.n1 == 1)
    {
        res.result = 0;
        res.response_code = RESPONSE_SUCCESS;
        return res;
    }

    res.result = 1;
    for (int i = 2; i * i <= req.n1; ++i)
    {
        if (req.n1 % i == 0)
        {
            res.result = 0;
            break;
        }
    }

    res.response_code = RESPONSE_SUCCESS;
    return res;
}

Response handle_is_negative(Request req)
{
    Response res;
    res.response_code = RESPONSE_UNSUPPORTED;
    return res;
}

void *worker_function(void *args)
{
    printf("Starting work!!!\n");
    int comm_channel_block_id = ((WorkerArgs *)args)->comm_channel_block_id;
    RequestOrResponse *comm_reqres = get_comm_channel(comm_channel_block_id);

    printf("Party starting for client: %s. Woohoooo!!!\n", comm_reqres->client_name);

    if (comm_reqres == NULL)
    {
        logger("ERROR", "Invalid comm channel block id provided. Could not get communication channel block." );

        free(args);
        args = NULL;

        pthread_exit(NULL);
    }

    while (true)
    {
        printf("Waiting to reach stage 1...\n");
        wait_until_stage(comm_reqres, 1);
        printf("%d", comm_reqres->req.request_type);
        if (comm_reqres->req.request_type == ARITHMETIC)
        {
            Response res = handle_arithmetic(comm_reqres->req);
            comm_reqres->res = res;
        }
        else if (comm_reqres->req.request_type == EVEN_OR_ODD)
        {
            Response res = handle_even_or_odd(comm_reqres->req);
            comm_reqres->res = res;
        }
        else if (comm_reqres->req.request_type == IS_PRIME)
        {
            Response res = handle_is_prime(comm_reqres->req);
            comm_reqres->res = res;
        }
        else if (comm_reqres->req.request_type == IS_NEGATIVE)
        {
            Response res = handle_is_negative(comm_reqres->req);
            comm_reqres->res = res;
        }
        else if (comm_reqres->req.request_type == UNREGISTER)
        {
            // TODO: Cleanup if deregister
            printf("Deregistering client %s\n", ((WorkerArgs *)args)->client_name);
            comm_reqres->res.response_code = RESPONSE_SUCCESS;

            break;
        }
        next_stage(comm_reqres);
        msleep(600);
    }

    free(args);
    args = NULL;

    return NULL;
}

#endif
