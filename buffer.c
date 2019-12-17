#include <string.h>
#include "buffer.h"
#include "byteorder.h"

void buffer_init(buffer_t *buffer, unsigned char *begin, size_t length) {
    buffer->begin = begin;
    buffer->data = begin;
    buffer->end = begin + length;
}

size_t buffer_size(buffer_t *buffer) {
    return buffer->end - buffer->begin;
}

size_t buffer_position(buffer_t *buffer) {
    return buffer->data - buffer->begin;
}

size_t buffer_available(buffer_t *buffer) {
    return buffer->end - buffer->data;
}

size_t buffer_write(buffer_t *buffer, void *source, size_t length) {
    size_t avail = buffer_available(buffer);

    if (avail < length) {
        length = avail;
    }

    if (length < 0) {
        return 0;
    }

    memcpy(buffer->data, source, length);
    buffer->data += length;

    return length;
}

void buffer_write_byte(buffer_t *buffer, unsigned char val) {
    buffer->data[0] = val;
    buffer->data++;
}

void buffer_write_short(buffer_t *buffer, short val) {
    val = (val);
    buffer->data[1] = val & 0xFF;
    buffer->data[0] = (val >> 8) & 0xFF;
    buffer->data += 2;
}

void buffer_write_int(buffer_t *buffer, int val) {
    val = (val);
    buffer->data[3] = val & 0xFF;
    buffer->data[2] = (val >> 8) & 0xFF;
    buffer->data[1] = (val >> 16) & 0xFF;
    buffer->data[0] = (val >> 24) & 0xFF;
    buffer->data += 4;
}

size_t buffer_read(buffer_t *buffer, void *dest, size_t length) {
    size_t avail = buffer_available(buffer);

    if (avail < length) {
        length = avail;
    }

    if (length < 0) {
        return 0;
    }

    memcpy(dest, buffer->data, length);
    buffer->data += length;

    return length;
}

unsigned char buffer_read_byte(buffer_t *buffer) {
    unsigned char val = *buffer->data;
    buffer->data++;
    return val;
}

short buffer_read_short(buffer_t *buffer) {
    short val;
    val = buffer->data[1] + (buffer->data[0] << 8);

    buffer->data += 2;

    return val;
}

int buffer_read_int(buffer_t *buffer) {
    int val;
    val = buffer->data[3] + (buffer->data[2] << 8) + (buffer->data[1] << 16) + (buffer->data[0] << 24);

    buffer->data += 4;

    return val;
}