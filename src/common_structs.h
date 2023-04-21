#ifndef COMMON_STRUCTS_H
#define COMMON_STRUCTS_H

#include <pthread.h>
#include <semaphore.h>

#include "file_utils.h"

#define CONNECT_CHANNEL_FNAME "srv_conn_channel"
#define CONNECT_CHANNEL_SIZE (1024)

#define MAX_CLIENT_NAME_LEN (1024)
#define MAX_BUFFER_SIZE (1024)

typedef enum RequestType
{
    ARITHMETIC,
    EVEN_OR_ODD,
    IS_PRIME,
    IS_NEGATIVE,
    UNREGISTER
} RequestType;

typedef enum ResponseCode
{
    RESPONSE_SUCCESS = 200,
    RESPONSE_UNSUPPORTED = 422,
    RESPONSE_UNKNOWN_FAILURE = 500
} ResponseCode;

typedef struct Request
{
    RequestType request_type;
    char client_name[MAX_CLIENT_NAME_LEN];
    int n1, n2;
    char op;
    int client_seq_num, server_seq_num;
} Request;

typedef struct Response
{
    int key;
    ResponseCode response_code;
    int client_seq_num, server_seq_num;
    int result;
} Response;

typedef struct RequestOrResponse
{
    pthread_mutex_t lock;
    int stage;
    Request req;
    Response res;
} RequestOrResponse;

RequestOrResponse *create_req_or_res(const char *client_name)
{
    create_file_if_does_not_exist(client_name);
    RequestOrResponse *req_or_res =
        (RequestOrResponse *)attach_memory_block(client_name, sizeof(req_or_res));

    if (req_or_res == NULL)
    {
        fprintf(stderr, "Error: Could not create shared RequestOrResponse object for client %s\n", client_name);
        return NULL;
    }

    req_or_res->stage = 0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&req_or_res->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    return req_or_res;
}

void wait_until_stage(RequestOrResponse *req_or_res, int stage)
{
    bool reached_stage = false;
    while (!reached_stage)
    {
        pthread_mutex_lock(&req_or_res->lock);
        reached_stage = (req_or_res->stage == stage);
        pthread_mutex_unlock(&req_or_res->lock);
    }
}

void next_stage(RequestOrResponse *req_or_res)
{
    pthread_mutex_lock(&req_or_res->lock);
    req_or_res->stage = (req_or_res->stage + 1) % 3;
    printf("Set stage to: %d", req_or_res->stage);
    pthread_mutex_unlock(&req_or_res->lock);
}

void set_stage(RequestOrResponse *req_or_res, int stage)
{
    pthread_mutex_lock(&req_or_res->lock);
    req_or_res->stage = stage;
    printf("Set stage to: %d", req_or_res->stage);
    pthread_mutex_unlock(&req_or_res->lock);
}

RequestOrResponse *get_req_or_res(const char *client_name)
{
    create_file_if_does_not_exist(client_name);
    RequestOrResponse *req_or_res =
        (RequestOrResponse *)attach_memory_block(client_name, sizeof(req_or_res));

    return req_or_res;
}

void destroy_req_or_res(RequestOrResponse *req_or_res)
{
    pthread_mutex_destroy(&req_or_res->lock);

    detach_memory_block(req_or_res);
    req_or_res = NULL;
    // destroy_memory_block(req_or_res);
}

#endif