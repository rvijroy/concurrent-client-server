#ifndef CONN_CHANNEL_H
#define CONN_CHANNEL_H

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "common_structs.h"
#include "shared_memory.h"
#include "logger.h"

#define MAX_QUEUE_LEN 100

typedef struct node_t
{
    int req_or_res_block_id;
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
    logger("DEBUG", "Initialising connection channel queue");
    create_file_if_does_not_exist(CONNECT_CHANNEL_FNAME);
    queue_t *q = (queue_t *)attach_memory_block(CONNECT_CHANNEL_FNAME, sizeof(queue_t));

    if (q == NULL)
    {
        logger("ERROR", "Could not create shared memory block for queue.");
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

    logger("INFO", "Connection channel queue creation succesful");
    return q;
}

queue_t *get_queue()
{
    queue_t *q = (queue_t *)attach_memory_block(CONNECT_CHANNEL_FNAME, sizeof(queue_t));
    if (q == NULL)
    {
        logger("ERROR", "Could not create shared memory block for queue.");
        return NULL;
    }

    return q;
}

RequestOrResponse *post(queue_t *q, const char *client_name)
{
    pthread_mutex_lock(&q->lock);
    if (q->size == MAX_QUEUE_LEN)
    {
        logger("ERROR", "Could not enqueue to connection queue. Connection queue is full.");
        pthread_mutex_unlock(&q->lock);
    }

    char shm_reqres_fname[MAX_CLIENT_NAME_LEN];
    sprintf(shm_reqres_fname, "queue_%d", q->tail);
    create_file_if_does_not_exist(shm_reqres_fname);

    int req_or_res_block_id = get_shared_block(shm_reqres_fname, sizeof(RequestOrResponse));
    RequestOrResponse *shm_req_or_res = (RequestOrResponse *)attach_with_shared_block_id(req_or_res_block_id);
    if (shm_req_or_res == NULL)
    {
        logger("ERROR", "Could not create shared memory block %s for the queue.", shm_reqres_fname);
        return NULL;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm_req_or_res->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    strncpy(shm_req_or_res->client_name, client_name, MAX_CLIENT_NAME_LEN);
    shm_req_or_res->stage = 0;

    // ? Does the nodes req_or_res pointer point to the same shared memory object
    // ? across proceeses ?

    // ? It is the same value for the pointer. Which means, the memcpy of the client_name did not occur properly.
    // q->nodes[q->tail].req_or_res = shm_req_or_res;
    q->nodes[q->tail].req_or_res_block_id = req_or_res_block_id;

    q->tail = (q->tail + 1) % MAX_QUEUE_LEN;
    q->size += 1;
    pthread_mutex_unlock(&q->lock);

    return shm_req_or_res;
}

RequestOrResponse *dequeue(queue_t *q)
{
    pthread_mutex_lock(&q->lock);

    if (q->size == 0)
    {
        logger("INFO", "Connection queue empty. Nothing to dequeue.");
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    int req_or_res_block_id = q->nodes[q->head].req_or_res_block_id;
    RequestOrResponse *req_or_res = attach_with_shared_block_id(req_or_res_block_id);

    q->head = (q->head + 1) % MAX_QUEUE_LEN;
    q->size -= 1;

    pthread_mutex_unlock(&q->lock);

    return req_or_res;
}

RequestOrResponse *fetch(queue_t *q)
{
    pthread_mutex_lock(&q->lock);

    if (q->size == 0)
    {
        logger("INFO", "Connection queue empty. Nothing to fetch.");
        pthread_mutex_unlock(&q->lock);

        return NULL;
    }

    int req_or_res_block_id = q->nodes[q->head].req_or_res_block_id;

    // TODO: Preferably attach memory block at the original location we had specified. If not, that is fine as well.
    // RequestOrResponse *req_or_res = q->nodes[q->head].req_or_res;
    // req_or_res = attach_with_shared_block_id(req_or_res_block_id, req_or_res);

    RequestOrResponse *req_or_res = attach_with_shared_block_id(req_or_res_block_id);
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

int destroy_queue(queue_t *q)
{
    logger("INFO", "Starting queue cleanup");
    int index;
    char shm_reqres_fname[MAX_CLIENT_NAME_LEN];
    while (q->size)
    {
        index = q->head;
        sprintf(shm_reqres_fname, "queue_%d", index);

        logger("DEBUG", "Cleaning node at index %d: %s", index, shm_reqres_fname);

        RequestOrResponse *reqres = dequeue(q);

        detach_memory_block(reqres);
        destroy_memory_block(shm_reqres_fname);

        logger("INFO", "Deleting file %s", shm_reqres_fname);
        remove_file(shm_reqres_fname);
    }

    logger("DEBUG", "Detaching memory block for queue.");
    detach_memory_block(q);

    logger("DEBUG", "Destroying file of memory block for queue.");
    destroy_memory_block(CONNECT_CHANNEL_FNAME);

    logger("INFO", "Deleting file %s", CONNECT_CHANNEL_FNAME);
    remove_file(CONNECT_CHANNEL_FNAME);

    logger("INFO", "Completed queue cleanup");
}

#endif