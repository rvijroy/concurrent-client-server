#ifndef CONN_CHANNEL_H
#define CONN_CHANNEL_H

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "common_structs.h"
#include "shared_memory.h"

#define MAX_QUEUE_LEN 100

typedef struct node_t
{
    RequestOrResponse *req_or_res;
} node_t;

typedef struct queue_t
{
    pthread_mutex_t lock;
    int head;
    int tail;
    int size;
    node_t nodes[MAX_QUEUE_LEN];
} queue_t;

queue_t *create_queue()
{
    create_file_if_does_not_exist(CONNECT_CHANNEL_FNAME);
    queue_t *q = (queue_t *)attach_memory_block(CONNECT_CHANNEL_FNAME, sizeof(queue_t));

    if (q == NULL)
    {
        fprintf(stderr, "Error: Could not create shared memory block for queue.\n");
        return NULL;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&q->lock, &attr);
    pthread_mutexattr_destroy(&attr);
    q->head = 0;
    q->tail = 0;
    q->size = 0;

    return q;
}

queue_t *get_queue()
{
    queue_t *q = (queue_t *)attach_memory_block(CONNECT_CHANNEL_FNAME, sizeof(queue_t));
    if (q == NULL)
    {
        fprintf(stderr, "Error: Could not create shared memory block for queue.\n");
        return NULL;
    }

    return q;
}

RequestOrResponse *post(queue_t *q, const char *client_name)
{
    pthread_mutex_lock(&q->lock);
    if (q->size == MAX_QUEUE_LEN)
    {
        fprintf(stderr, "Error: Could not enqueue to connection queue. Connection queue is full.\n");
        pthread_mutex_unlock(&q->lock);
    }

    char shm_req_or_res_fname[MAX_CLIENT_NAME_LEN];
    sprintf(shm_req_or_res_fname, "queue_%d", q->tail);

    RequestOrResponse *shm_req_or_res = (RequestOrResponse *)attach_memory_block(shm_req_or_res_fname, sizeof(RequestOrResponse));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm_req_or_res->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    memcpy(shm_req_or_res->req.client_name, client_name, MAX_CLIENT_NAME_LEN);
    shm_req_or_res->stage = 0;

    q->nodes[q->tail].req_or_res = shm_req_or_res;
    q->tail = (q->tail + 1) % MAX_QUEUE_LEN;
    pthread_mutex_unlock(&q->lock);

    return shm_req_or_res;
}

RequestOrResponse *dequeue(queue_t *q)
{
    pthread_mutex_lock(&q->lock);

    if (q->size == 0)
    {
        fprintf(stderr, "Error: Connection queue empty. Could not fetch new request.\n");
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    RequestOrResponse *req_or_res = q->nodes[q->head].req_or_res;
    q->head = (q->head + 1) % MAX_QUEUE_LEN;

    pthread_mutex_unlock(&q->lock);

    return req_or_res;
}

RequestOrResponse *fetch(queue_t *q)
{
    pthread_mutex_lock(&q->lock);

    if (q->size == 0)
    {
        fprintf(stderr, "Error: Connection queue empty. Could not fetch new request.\n");
        pthread_mutex_unlock(&q->lock);

        return NULL;
    }

    RequestOrResponse *req_or_res = q->nodes[q->head].req_or_res;
    pthread_mutex_unlock(&q->lock);

    return req_or_res;
}

int empty(queue_t *q)
{
    pthread_mutex_lock(&q->lock);

    q->size = 0;
    pthread_mutex_unlock(&q->lock);

    return 0;
}

#endif