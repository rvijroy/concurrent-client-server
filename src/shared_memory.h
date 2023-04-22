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

#include "logger.h"

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
        logger("ERROR", "File %s does not exist, or can not be accesses. Verify its existence and permissions.", filename);
        return IPC_RESULT_ERROR;
    }

    // get shared block -- create if it does not exist
    int shared_block_id = shmget(key, size, 0644 | IPC_CREAT);
    if (shared_block_id == IPC_RESULT_ERROR)
    {
        if (errno == EACCES)
            logger("ERROR", "shmget failed with code EACCESS.");
        else if (errno == EEXIST)
            logger("ERROR", "shmget failed with code EEXIST.");
        else if (errno == EINVAL)
            logger("ERROR", "shmget failed with code EINVAL.");
        else if (errno == ENOENT)
            logger("ERROR", "shmget failed with code ENOENT.");
        else if (errno == ENOMEM)
            logger("ERROR", "shmget failed with code ENOMEM.");
        else if (errno == ENOSPC)
            logger("ERROR", "shmget failed with code ENOSPC.");
        else
            logger("ERROR", "wtf.");

        return IPC_RESULT_ERROR;
    }

    return shared_block_id;
}

void *attach_with_shared_block_id(int shared_block_id)
{
    // map the shared block into this process's memory
    // and return a pointer to it
    void *block = shmat(shared_block_id, NULL, 0);
    if (block == (void *)IPC_RESULT_ERROR)
    {
        logger("ERROR", "Could not attached to shared memory block with block_id: %d", shared_block_id);
        return NULL;
    }

    return block;
}

void *attach_memory_block(const char *filename, size_t size)
{
    int shared_block_id = get_shared_block(filename, size);
    if (shared_block_id == IPC_RESULT_ERROR)
    {
        logger("ERROR", "get_shared_block failed. Could not get shared_block_id.");
        return NULL;
    }

    return attach_with_shared_block_id(shared_block_id);
}

int detach_memory_block(const void *block)
{
    int result = shmdt(block);
    if (result == IPC_RESULT_ERROR)
    {
        if (errno == EINVAL)
            logger("ERROR", "Failed to detach connection memory block. The address passed is not the start address of a mapped shared memory segment.");
        else
            logger("ERROR", "Failed to detach connection memory block. Unknown error occured.");
        return IPC_RESULT_ERROR;
    }

    return result;
}

int destroy_memory_block(const char *filename)
{
    int shared_block_id = get_shared_block(filename, 0);
    if (shared_block_id == IPC_RESULT_ERROR)
        return IPC_RESULT_ERROR;

    int result = shmctl(shared_block_id, IPC_RMID, NULL);
    if (result == IPC_RESULT_ERROR)
    {
        if (errno == EACCES)
            logger("ERROR", "Failed to destroy connection memory block %s. The command is IPC_STAT and the caller has no read permission for this shared memory segment.", filename);
        if (errno == EFAULT)
            logger("ERROR", "Failed to destroy connection memory block %s. `buf` specifies an invalid address", filename);
        if (errno == EINVAL)
            logger("ERROR", "Failed to destroy connection memory block %s. `shmid` is not a valid shared memory segment identifier or `cmd` is not a valid command.", filename);
        if (errno == EPERM)
            logger("ERROR", "Failed to destroy connection memory block %s. User does not have the permissions required to carry out the specified action.", filename);
        else
            logger("ERROR", "Failed to destroy connection memory block %s. Unknown error occured.", filename);
        return IPC_RESULT_ERROR;
    }

    return result;
}

void clear_memory_block(void *block, size_t size)
{
    memset(block, '\0', size);
}

#endif