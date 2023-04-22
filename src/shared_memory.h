#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#define IPC_RESULT_ERROR (-1)

int create_key(const char *filename)
{
    return ftok(filename, 0);
}

static int get_shared_block(const char *filename, size_t size)
{
    // Request a key
    // key is linked to a filename so that other programs can access it
    key_t key = ftok(filename, 0);

    if (key == IPC_RESULT_ERROR)
    {
        fprintf(stderr, "ERROR: File %s does not exist, or can not be accesses. Verify its existence and permissions.\n", filename);
        return IPC_RESULT_ERROR;
    }

    // get shared block -- create if it does not exist
    int shared_block_id = shmget(key, size, 0644 | IPC_CREAT);
    if (shared_block_id == IPC_RESULT_ERROR)
    {
        if (errno == EACCES)
            fprintf(stderr, "ERROR: shmget failed with code EACCESS.\n");
        else if (errno == EEXIST)
            fprintf(stderr, "ERROR: shmget failed with code EEXIST.\n");
        else if (errno == EINVAL)
            fprintf(stderr, "ERROR: shmget failed with code EINVAL.\n");
        else if (errno == ENOENT)
            fprintf(stderr, "ERROR: shmget failed with code ENOENT.\n");
        else if (errno == ENOMEM)
            fprintf(stderr, "ERROR: shmget failed with code ENOMEM.\n");
        else if (errno == ENOSPC)
            fprintf(stderr, "ERROR: shmget failed with code ENOSPC.\n");
        else
            fprintf(stderr, "ERROR: wtf.\n");

        return IPC_RESULT_ERROR;
    }

    return shared_block_id;
}

// TODO: Add parameter of preferred memory address?
void *attach_with_shared_block_id(int shared_block_id)
{
    // map the shared block into this process's memory
    // and return a pointer to it
    void *block = shmat(shared_block_id, NULL, 0);
    if (block == (void*)IPC_RESULT_ERROR)
    {
        fprintf(stderr, "ERROR: Could not attached to shared memory block with block_id: %d.", shared_block_id);
        return NULL;
    }

    return block;
}

void *attach_memory_block(const char *filename, size_t size)
{
    int shared_block_id = get_shared_block(filename, size);
    if (shared_block_id == IPC_RESULT_ERROR)
    {
        fprintf(stderr, "ERROR: get_shared_block failed. Could not get shared_block_id.\n");
        return NULL;
    }

    return attach_with_shared_block_id(shared_block_id);
}

bool detach_memory_block(const void *block)
{
    return (shmdt(block) != IPC_RESULT_ERROR);
}

bool destroy_memory_block(const char *filename)
{
    int shared_block_id = get_shared_block(filename, 0);

    if (shared_block_id == IPC_RESULT_ERROR)
        return false;

    return shmctl(shared_block_id, IPC_RMID, NULL) != IPC_RESULT_ERROR;
}

void clear_memory_block(void *block, size_t size)
{
    memset(block, '\0', size);
}

#endif