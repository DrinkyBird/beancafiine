#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "files.h"
#include "byteorder.h"

struct {
    char *root;
} fs;

static void fs_resolve(titleid_t title, const char *path, char *out, size_t maxlen);

void fs_init(const char *root) {
    fs.root = strdup(root);
    printf("Using %s as root directory\n", fs.root);
}

void fs_shutdown() {
    free(fs.root);
}

void fs_resolve(titleid_t title, const char *path, char *out, size_t maxlen) {
    snprintf(out, maxlen, "%s/%08X-%08X/%s", fs.root, title.type, title.id, path);
}

bool fs_file_exists(titleid_t title, const char *path) {
    char real[512];
    fs_resolve(title, path, real, sizeof real);

    // just check if we can read
    FILE *f = fopen(real, "r");
    if (!f) {
        return false;
    }

    fclose(f);
    return true;
}

fshandle_t *fs_open(titleid_t title, const char *path, const char *mode) {
    fshandle_t *handle = malloc(sizeof(fshandle_t));
    memset(handle, 0, sizeof(fshandle_t));

    char full[512];
    fs_resolve(title, path, full, sizeof(full));

    handle->title = title;
    handle->path = strdup(path);
    handle->file = fopen(full, mode);
    if (!handle->file) {
        goto failure;
    }

    return handle;

failure:
    if (handle) {
        if (handle->file) {
            fclose(handle->file);
        }

        free(handle);
    }

    return NULL;
}

void fs_close(fshandle_t *handle) {
    fclose(handle->file);
    free(handle->path);
    free(handle);
}

int fs_read(fshandle_t *handle, void *buffer, size_t count) {
    int r = fread(buffer, 1, count, handle->file);
    return r;
}

int fs_seek(fshandle_t *handle, int pos) {
    fseek(handle->file, pos, SEEK_SET);
}

int fs_tell(fshandle_t *handle) {
    return ftell(handle->file);
}

bool fs_eof(fshandle_t *handle) {
    return feof(handle->file) != 0;
}

int fs_size(fshandle_t *handle) {
    int orig = fs_tell(handle);
    fseek(handle->file, 0, SEEK_END);
    int size = fs_tell(handle);
    fs_seek(handle, orig);
    return size;
}

int fs_stat(fshandle_t *handle, fsstat_t *st) {
    st->flag = 0;
    st->permission = endian_big32(0x400);
    st->owner_id = endian_big32(handle->title.id);
    st->group_id = endian_big32(0x101e);
    st->size = endian_big32(fs_size(handle));
}