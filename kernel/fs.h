#ifndef FS_H
#define FS_H

#include "../include/types.h"
#include "ramdisk.h"

/*
 * Stage 4 - RAM Disk File System
 * Lecture L12
 */

#define FS_MAX_FILES       32
#define FS_MAX_OPEN_FILES  16
#define FS_MAX_FILE_SIZE   (8 * RAMDISK_BLOCK_SIZE)

/*
 * File handle.
 */
typedef struct {
    uint32_t inode;
    uint32_t position;
    uint8_t used;
} fs_file_t;

/*
 * Initialise the file system.
 */
void fs_init(void);

/*
 * File operations.
 */
int fs_open(const char *name);
int fs_create(const char *name);
int fs_read(int fd, char *buffer, uint32_t size);
int fs_write(int fd, const char *buffer, uint32_t size);
int fs_append(int fd, const char *buffer, uint32_t size);
void fs_close(int fd);
int fs_unlink(const char *name);

/*
 * Directory listing.
 */
void fs_list(void);

#endif
