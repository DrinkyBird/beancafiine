#ifndef BEANCAFIINE_FILES_H
#define BEANCAFIINE_FILES_H

#include <stdbool.h>
#include <stdio.h>
#include "beandef.h"

typedef struct {
    titleid_t title;
    FILE *file;

    char *path;
} fshandle_t;

#define FS_STAT_ATTRIBUTES_SIZE 48
typedef struct
{
    uint32_t flag;
    uint32_t permission;
    uint32_t owner_id;
    uint32_t group_id;
    uint32_t size;
    uint32_t alloc_size;
    uint64_t quota_size;
    uint32_t ent_id;
    uint64_t ctime;
    uint64_t mtime;
    uint8_t  attributes[FS_STAT_ATTRIBUTES_SIZE];
} fsstat_t;

void fs_init(const char *root);
void fs_shutdown();

bool fs_file_exists(titleid_t title, const char *path);

fshandle_t *fs_open(titleid_t title, const char *path, const char *mode);
void fs_close(fshandle_t *handle);
int fs_read(fshandle_t *handle, void *buffer, size_t count);
int fs_seek(fshandle_t *handle, int pos);
int fs_tell(fshandle_t *handle);
bool fs_eof(fshandle_t *handle);
int fs_size(fshandle_t *handle);
int fs_stat(fshandle_t *handle, fsstat_t *st);

#endif //BEANCAFIINE_FILES_H
