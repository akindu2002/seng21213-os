#ifndef RAMDISK_H
#define RAMDISK_H

#include "../include/types.h"

/*
 * Stage 4 - RAM Disk
 * Lecture L12
 *
 * 1 MB RAM disk
 * Block size = 4 KB
 */

#define RAMDISK_SIZE        (1024 * 1024)
#define RAMDISK_BLOCK_SIZE  4096
#define RAMDISK_BLOCK_COUNT (RAMDISK_SIZE / RAMDISK_BLOCK_SIZE)

/*
 * RAM disk layout
 *
 * Block 0 : Superblock
 * Block 1 : Directory
 * Block 2 : Block bitmap
 * Block 3 : Inode bitmap
 * Block 4 : Inode table
 * Block 5+ : File data
 */
#define RAMDISK_SUPERBLOCK_BLOCK  0
#define RAMDISK_DIRECTORY_BLOCK   1
#define RAMDISK_BLOCK_BITMAP      2
#define RAMDISK_INODE_BITMAP      3
#define RAMDISK_INODE_TABLE       4
#define RAMDISK_DATA_START        5

#define RAMDISK_MAX_INODES 64
#define RAMDISK_NAME_LEN   28

/*
 * Superblock
 */
typedef struct {
    uint32_t magic;
    uint32_t block_count;
    uint32_t inode_count;
} ramdisk_superblock_t;

/*
 * Inode
 *
 * 8 direct pointers × 4 KB = 32 KB maximum file size.
 */
typedef struct {
    uint32_t size;
    uint32_t blocks[8];
    uint8_t used;
} inode_t;

/*
 * Flat directory entry.
 */
typedef struct {
    char name[RAMDISK_NAME_LEN];
    uint32_t inode;
} directory_entry_t;

/*
 * RAM disk functions.
 */
void ramdisk_init(void);

uint8_t *ramdisk_get_block(uint32_t block);

ramdisk_superblock_t *ramdisk_get_superblock(void);

inode_t *ramdisk_get_inodes(void);

#endif
