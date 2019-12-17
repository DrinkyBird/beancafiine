#include <sys/socket.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "stream.h"
#include "byteorder.h"

extern __thread jmp_buf error_jmp;
extern __thread bool thread_error;

void stream_init(stream_t *stream, int fd) {
    stream->fd = fd;
}

ssize_t stream_write(stream_t *stream, void *source, size_t length) {
    int r = send(stream->fd, source, length, MSG_NOSIGNAL);
    if (r == -1) {
        return -1;
    } else if (r == 0) {
        thread_error = true;
        longjmp(error_jmp, 0);
    }

    return r;
}

void stream_write_byte(stream_t *stream, unsigned char val) {
    stream_write(stream, &val, 1);
}

void stream_write_short(stream_t *stream, short val) {
    val = endian_big16(val);
    stream_write(stream, &val, 2);
}

void stream_write_int(stream_t *stream, int val) {
    val = endian_big32(val);
    stream_write(stream, &val, 4);
}

ssize_t stream_read(stream_t *stream, void *buffer, size_t length) {
    int r = recv(stream->fd, buffer, length, 0);
    if (r == -1) {
        return -1;
    } else if (r == 0) {
        thread_error = true;
        longjmp(error_jmp, 0);
    }

    return r;
}

unsigned char stream_read_byte(stream_t *stream) {
    unsigned char val;
    stream_read(stream, &val, 1);
    return val;
}

short stream_read_short(stream_t *stream) {
    short val;
    stream_read(stream, &val, 2);
    return endian_big16(val);
}

int stream_read_int(stream_t *stream) {
    int val;
    stream_read(stream, &val, 4);
    return endian_big32(val);
}