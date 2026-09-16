#include "ramdisk.h"

/*
 * Stage 4 - RAM Disk
 * Lecture L12
 *
 * Fixed 1 MB RAM disk stored in BSS.
 */
static uint8_t ramdisk[RAMDISK_SIZE];

/*
 * Initialise the RAM disk.
 */
void ramdisk_init(void)
{
    uint32_t i;
    uint8_t *block;

    /*
     * Clear the complete 1 MB RAM disk.
     */
    for (i = 0; i < RAMDISK_SIZE; i++) {
        ramdisk[i] = 0;
    }

    /*
     * Initialise superblock in block 0.
     */
    block = ramdisk_get_block(RAMDISK_SUPERBLOCK_BLOCK);

    ramdisk_superblock_t *superblock =
        (ramdisk_superblock_t *)block;

    superblock->magic = 0x53454E47; /* "SENG" */
    superblock->block_count = RAMDISK_BLOCK_COUNT;
    superblock->inode_count = RAMDISK_MAX_INODES;

    /*
     * Mark metadata blocks as used.
     *
     * Block bitmap:
     * bit 0 = block 0
     * bit 1 = block 1
     * ...
     */
    block = ramdisk_get_block(RAMDISK_BLOCK_BITMAP);

    block[0] |= (1 << RAMDISK_SUPERBLOCK_BLOCK);
    block[0] |= (1 << RAMDISK_DIRECTORY_BLOCK);
    block[0] |= (1 << RAMDISK_BLOCK_BITMAP);
    block[0] |= (1 << RAMDISK_INODE_BITMAP);
    block[0] |= (1 << RAMDISK_INODE_TABLE);
}

/*
 * Return a pointer to a RAM disk block.
 */
uint8_t *ramdisk_get_block(uint32_t block)
{
    if (block >= RAMDISK_BLOCK_COUNT) {
        return (uint8_t *)0;
    }

    return &ramdisk[block * RAMDISK_BLOCK_SIZE];
}

/*
 * Return the RAM disk superblock.
 */
ramdisk_superblock_t *ramdisk_get_superblock(void)
{
    return (ramdisk_superblock_t *)
        ramdisk_get_block(RAMDISK_SUPERBLOCK_BLOCK);
}

/*
 * Return the inode table.
 *
 * The inode table occupies block 4.
 */
inode_t *ramdisk_get_inodes(void)
{
    return (inode_t *)
        ramdisk_get_block(RAMDISK_INODE_TABLE);
}
