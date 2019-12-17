#ifndef BEANCAFIINE_BUFFER_H
#define BEANCAFIINE_BUFFER_H

#include <stddef.h>

typedef struct {
    unsigned char *begin;
    unsigned char *data;
    unsigned char *end;
} buffer_t;

void buffer_init(buffer_t *buffer, unsigned char *begin, size_t length);

size_t buffer_size(buffer_t *buffer);
size_t buffer_position(buffer_t *buffer);
size_t buffer_available(buffer_t *buffer);

size_t buffer_write(buffer_t *buffer, void *source, size_t length);
void buffer_write_byte(buffer_t *buffer, unsigned char val);
void buffer_write_short(buffer_t *buffer, short val);
void buffer_write_int(buffer_t *buffer, int val);

size_t buffer_read(buffer_t *buffer, void *source, size_t length);
unsigned char buffer_read_byte(buffer_t *buffer);
short buffer_read_short(buffer_t *buffer);
int buffer_read_int(buffer_t *buffer);

#endif //BEANCAFIINE_BUFFER_H
