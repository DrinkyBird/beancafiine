#ifndef BEANCAFIINE_STREAM_H
#define BEANCAFIINE_STREAM_H

#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

typedef struct {
    int fd;
} stream_t;

void stream_init(stream_t *stream, int fd);

ssize_t stream_write(stream_t *stream, void *source, size_t length);
void stream_write_byte(stream_t *stream, unsigned char val);
void stream_write_short(stream_t *stream, short val);
void stream_write_int(stream_t *stream, int val);

ssize_t stream_read(stream_t *stream, void *buffer, size_t length);
unsigned char stream_read_byte(stream_t *stream);
short stream_read_short(stream_t *stream);
int stream_read_int(stream_t *stream);

#endif //BEANCAFIINE_STREAM_H
