#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

int create_file_if_does_not_exist(const char * filename) {
    return open(filename, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
}

bool does_file_exist(const char* filename) {
    return access(filename, F_OK) == 0;
}

#endif