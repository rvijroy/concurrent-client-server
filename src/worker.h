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

static unsigned long total_serviced_requests = 0;
static pthread_mutex_t tsr_mutex = PTHREAD_MUTEX_INITIALIZER;

int increment_service_requests()
{
    pthread_mutex_lock(&tsr_mutex);
    total_serviced_requests++;
    pthread_mutex_unlock(&tsr_mutex);

    return total_serviced_requests;
}

int get_total_serviced_requests()
{
    return total_serviced_requests;
}

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

Response handle_is_negative(Request _req)
{
    Response res;
    res.response_code = RESPONSE_UNSUPPORTED;
    return res;
}

void *worker_function(void *args)
{
    logger("DEBUG", "[%08x] Setting up communication channel for client %s", pthread_self(), ((WorkerArgs *)args)->client_name);
    int comm_channel_block_id = ((WorkerArgs *)args)->comm_channel_block_id;
    RequestOrResponse *comm_reqres = get_comm_channel(comm_channel_block_id);
    logger("INFO", "[%08x] Communication channel setup succesful for client %s", pthread_self(), ((WorkerArgs *)args)->client_name);

    unsigned long thread_tsr = 0;

    if (comm_reqres == NULL)
    {
        logger("ERROR", "[%08x] Invalid comm channel block id provided. Could not get communication channel block.", pthread_self());

        free(args);
        args = NULL;

        pthread_exit(NULL);
    }

    while (true)
    {
        wait_until_stage(comm_reqres, 1);
        logger("INFO", "[%08x] Received request of type %d", pthread_self(), comm_reqres->req.request_type);

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
            logger("INFO", "[%08x] Initiating deregister of client %s", pthread_self(), ((WorkerArgs *)args)->client_name);
            printf("Deregistering client %s\n", ((WorkerArgs *)args)->client_name);

            detach_memory_block(comm_reqres);
            destroy_memory_block(((WorkerArgs *)args)->client_name); // TODO: Refactor this out.

            logger("DEBUG", "[%08x] Removing file %s as a part of deregistration", pthread_self(), ((WorkerArgs *)args)->client_name);
            remove_file(((WorkerArgs *)args)->client_name);

            set_stage(comm_reqres, 0); // ? Is this required?
            logger("INFO", "[%08x] Deregistration of client %s succesful", pthread_self(), ((WorkerArgs *)args)->client_name);

            break;
        }

        logger("INFO", "[%08x] Thread Total serviced requests: %d", pthread_self(), ++thread_tsr);
        logger("INFO", "[%08x] Total serviced requests: %d", pthread_self(), increment_service_requests());
        next_stage(comm_reqres);

        logger("INFO", "[%08x] Response sent to client for request with response code %d", pthread_self(), comm_reqres->res.response_code);
        msleep(600);
    }

    free(args);
    args = NULL;

    return NULL;
}

#endif