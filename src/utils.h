#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>

int create_file_if_does_not_exist(const char *filename)
{
    return open(filename, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
}

int does_file_exist(const char *filename)
{
    int res = access(filename, F_OK);
    if (res == -1)
    {
        if (errno == ENOENT)
            return 0;

        switch (errno)
        {
        case EINVAL:
            fprintf(stderr, "ERROR: Call to access failed. The value of mode argument is invalid.\n");
            break;

        case ENOTDIR:
            fprintf(stderr, "ERROR: Call to access failed. A component of the path prefix is not a directory.\n");
            break;

        case ENAMETOOLONG:
            fprintf(stderr, "ERROR: Call to access failed. A component of a pathname exceeded {NAME_MAX} characters, or an entire path name exceeded {PATH_MAX} characters.\n");
            break;

        case ELOOP:
            fprintf(stderr, "ERROR: Call to access failed. Too many symbolic links were encountered in translating the pathname or AT_SYMLINK_NOFOLLOW_ANY was passed and a symbolic link was encountered in translating the pathname.\n");
            break;

        case EROFS:
            fprintf(stderr, "ERROR: Call to access failed. Write access is requested for a file on a read-only file system.\n");
            break;

        case ETXTBSY:
            fprintf(stderr, "ERROR: Call to access failed. Write access is requested for a pure procedure (shared text) file presently being executed.\n");
            break;

        case EACCES:
            fprintf(stderr, "ERROR: Call to access failed. Permission bits of the file mode do not permit the requested access, or search permission is denied on a component of the path prefix.\n");
            break;
        case EFAULT:
            fprintf(stderr, "ERROR: Call to access failed. The path argument points outside the process's allocated address space.\n");
            break;
        case EIO:
            fprintf(stderr, "ERROR: Call to access failed. An I/O error occurred while reading from or writing to the file system.\n");
            break;

        default:
            fprintf(stderr, "ERROR: Call to access failed. Unknown error occured.\n");
            break;
        }
        return -1;
    }
    return 1;
}

int msleep(long msec)
{
    struct timespec ts;
    int res = 0;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * (long)(1e6);

    nanosleep(&ts, NULL);

    // ? Use this if you need to sleep during interrupts as well.
    // do {
    //     res = nanosleep(&ts, &ts);
    // } while (res && errno == EINTR);

    return res;
}

#endif