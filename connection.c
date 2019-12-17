#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <setjmp.h>
#include "connection.h"
#include "cafiine.h"
#include "beandef.h"
#include "files.h"
#include "stream.h"

#define READ_stream_SIZE 2048
#define WRITE_stream_SIZE 2048

#define FD_MASK 0x0FFF00FF
#define FD_MASKIFY(handle) (FD_MASK | ((handle) << 8))
#define FD_UNMASK(handle) ((handle >> 8) & 0xFF)
#define FD_ISRIGHTMASK(handle) (((handle) & FD_MASK) == FD_MASK)

extern bool running;

typedef struct {
    int fd;
} threaddata_t;

typedef struct {
    threaddata_t *threaddata;

    stream_t stream;

    fshandle_t **handles;
    size_t handles_size;

    titleid_t titleid;
} connection_t;

static void *connection_run(void *userdata);
static int connection_acquire_handle(connection_t *conn);

__thread jmp_buf error_jmp;
__thread bool thread_error = false;

void connection_handle(int fd) {
    threaddata_t *data = malloc(sizeof(threaddata_t));
    data->fd = fd;

    pthread_t thread;
    int r = pthread_create(&thread, NULL, connection_run, data);
    if (r != 0) {
        printf("Failed to create thread for client %d: pthread_create error %d\n", fd, r);
    }
}

void *connection_run(void *userdata) {
    threaddata_t *data = (threaddata_t *)userdata;
    connection_t *connection = malloc(sizeof(connection_t));

    connection->threaddata = data;
    stream_init(&connection->stream, data->fd);

    connection->handles = NULL;
    connection->handles_size = 0;

    {
        // read initial state info
        unsigned int ids[4];
        for (unsigned i = 0; i < 4; i++) {
            ids[i] = stream_read_int(&connection->stream);
        }

        connection->titleid.type = ids[0];
        connection->titleid.id = ids[1];
        printf("%d: Title ID %08X-%08X\n", data->fd, ids[0], ids[1]);

        stream_write_byte(&connection->stream, BYTE_SPECIAL);
    }

    setjmp(error_jmp);
    if (thread_error) {
        printf("%d: Lost connection to client\n", data->fd);
        goto end_thread;
    }

    while (running) {
        unsigned char command = stream_read_byte(&connection->stream);

        switch (command) {
            case BYTE_OPEN: {
                int pathLen = stream_read_int(&connection->stream);
                int modeLen = stream_read_int(&connection->stream);

                char *path = malloc(pathLen);
                char *mode = malloc(modeLen);

                stream_read(&connection->stream, path, pathLen);
                stream_read(&connection->stream, mode, modeLen);

                printf("%d: Open %s\n", data->fd, path);

                if (fs_file_exists(connection->titleid, path)) {
                    int handle = connection_acquire_handle(connection);
                    connection->handles[handle] = fs_open(connection->titleid, path, mode);

                    printf("%d: Intercepting %s (handle %d)\n", data->fd, path, handle);

                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, 0);
                    stream_write_int(&connection->stream, FD_MASKIFY(handle));
                } else {
                    stream_write_byte(&connection->stream, BYTE_NORMAL);
                }

                break;
            }

            case BYTE_CLOSE: {
                int fd = stream_read_int(&connection->stream);

                if (!FD_ISRIGHTMASK(fd)) {
                    stream_write_byte(&connection->stream, BYTE_NORMAL);
                    break;
                }

                fd = FD_UNMASK(fd);

                printf("%d: Closing file %s (handle %d)\n", data->fd, connection->handles[fd]->path, fd);
                if (connection->handles[fd] == NULL) {
                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, -38);
                } else {
                    fs_close(connection->handles[fd]);
                    connection->handles[fd] = NULL;

                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, 0);
                }

                break;
            }

            case BYTE_READ: {
                int size = stream_read_int(&connection->stream);
                int count = stream_read_int(&connection->stream);
                int fd = stream_read_int(&connection->stream);

                if (!FD_ISRIGHTMASK(fd)) {
                    stream_write_byte(&connection->stream, BYTE_NORMAL);
                    break;
                }

                fd = FD_UNMASK(fd);

                if (connection->handles[fd] == NULL) {
                    printf("Don't have a handle %d\n", fd);
                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, -19);
                } else {
                    char *buf = malloc(size * count);
                    int r = fs_read(connection->handles[fd], buf, size * count);

                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, r / size);
                    stream_write_int(&connection->stream, r);
                    stream_write(&connection->stream, buf, r);

                    free(buf);

                    if (stream_read_byte(&connection->stream) != BYTE_OK) {
                        printf("%d: Error reading fd %d\n", data->fd, fd);
                        break;
                    }
                }

                break;
            }

            case BYTE_SETPOS: {
                int fd = stream_read_int(&connection->stream);
                int pos = stream_read_int(&connection->stream);

                if (!FD_ISRIGHTMASK(fd)) {
                    stream_write_byte(&connection->stream, BYTE_NORMAL);
                    break;
                }

                fd = FD_UNMASK(fd);

                if (connection->handles[fd] == NULL) {
                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, -38);
                } else {
                    fs_seek(connection->handles[fd], pos);

                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, 0);
                }

                break;
            }

            case BYTE_GETPOS: {
                int fd = stream_read_int(&connection->stream);

                if (!FD_ISRIGHTMASK(fd)) {
                    stream_write_byte(&connection->stream, BYTE_NORMAL);
                    break;
                }

                fd = FD_UNMASK(fd);

                if (connection->handles[fd] == NULL) {
                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, -38);
                } else {
                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, 0);
                    stream_write_int(&connection->stream, fs_tell(connection->handles[fd]));
                }

                break;
            }

            case BYTE_STATFILE: {
                int fd = stream_read_int(&connection->stream);

                if (!FD_ISRIGHTMASK(fd)) {
                    stream_write_byte(&connection->stream, BYTE_NORMAL);
                    break;
                }

                fd = FD_UNMASK(fd);

                if (connection->handles[fd] == NULL) {
                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, -38);
                    stream_write_int(&connection->stream, 0);
                } else {
                    fsstat_t stat;
                    fs_stat(connection->handles[fd], &stat);

                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, 0);
                    stream_write_int(&connection->stream, sizeof(fsstat_t));
                    stream_write(&connection->stream, &stat, sizeof(fsstat_t));
                }

                break;
            }

            case BYTE_EOF: {
                int fd = stream_read_int(&connection->stream);

                if (!FD_ISRIGHTMASK(fd)) {
                    stream_write_byte(&connection->stream, BYTE_NORMAL);
                    break;
                }

                fd = FD_UNMASK(fd);

                if (connection->handles[fd] == NULL) {
                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, -38);
                } else {
                    fsstat_t stat;
                    fs_stat(connection->handles[fd], &stat);

                    stream_write_byte(&connection->stream, BYTE_SPECIAL);
                    stream_write_int(&connection->stream, fs_eof(connection->handles[fd]) ? -5 : 0);
                }

                break;
            }

            default: {
                printf("%d: Unknown command %d\n", data->fd, command);

                stream_write_byte(&connection->stream, BYTE_NORMAL);

                break;
            }
        }
    }

end_thread:
    free(connection->handles);
    free(connection);
    free(data);

    return NULL;
}

int connection_acquire_handle(connection_t *conn) {
    // try and find a NULL one
    for (int i = 0; i < conn->handles_size; i++) {
        if (conn->handles[i] == NULL) {
            return i;
        }
    }

    // no free handle indices - expand and try again
    size_t prevsize = conn->handles_size;
    conn->handles_size += 8;
    conn->handles = realloc(conn->handles, conn->handles_size * sizeof(fshandle_t *));
    memset(conn->handles + prevsize, 0, 8 * sizeof(fshandle_t *));

    return connection_acquire_handle(conn);
}