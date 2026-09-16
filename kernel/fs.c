#include "fs.h"
#include "vga.h"

/*
 * Stage 4 - File System
 * Lecture L12
 *
 * Simple flat file system using the RAM disk.
 */

static fs_file_t open_files[FS_MAX_OPEN_FILES];

/*
 * Return directory entries stored in block 1.
 */
static directory_entry_t *get_directory(void)
{
    return (directory_entry_t *)
        ramdisk_get_block(RAMDISK_DIRECTORY_BLOCK);
}

/*
 * Return block bitmap.
 */
static uint8_t *get_block_bitmap(void)
{
    return ramdisk_get_block(RAMDISK_BLOCK_BITMAP);
}

/*
 * Return inode bitmap.
 */
static uint8_t *get_inode_bitmap(void)
{
    return ramdisk_get_block(RAMDISK_INODE_BITMAP);
}

/*
 * Find a directory entry by name.
 */
static int find_file(const char *name)
{
    directory_entry_t *dir;
    uint32_t i;

    dir = get_directory();

    for (i = 0; i < FS_MAX_FILES; i++) {
        if (dir[i].inode != 0) {
            uint32_t j = 0;
            int match = 1;

            while (name[j] != '\0' || dir[i].name[j] != '\0') {
                if (name[j] != dir[i].name[j]) {
                    match = 0;
                    break;
                }

                j++;

                if (j >= RAMDISK_NAME_LEN) {
                    break;
                }
            }

            if (match) {
                return (int)i;
            }
        }
    }

    return -1;
}

/*
 * Allocate an inode.
 */
static int allocate_inode(void)
{
    uint8_t *bitmap;
    uint32_t i;

    bitmap = get_inode_bitmap();

    for (i = 1; i < RAMDISK_MAX_INODES; i++) {
        uint32_t byte = i / 8;
        uint8_t bit = (uint8_t)(1 << (i % 8));

        if ((bitmap[byte] & bit) == 0) {
            bitmap[byte] |= bit;
            return (int)i;
        }
    }

    return -1;
}

/*
 * Free an inode.
 */
static void free_inode(uint32_t inode)
{
    uint8_t *bitmap;
    uint32_t byte;
    uint8_t bit;

    bitmap = get_inode_bitmap();

    byte = inode / 8;
    bit = (uint8_t)(1 << (inode % 8));

    bitmap[byte] &= (uint8_t)~bit;
}

/*
 * Allocate a data block.
 */
static int allocate_block(void)
{
    uint8_t *bitmap;
    uint32_t block;

    bitmap = get_block_bitmap();

    for (block = RAMDISK_DATA_START;
         block < RAMDISK_BLOCK_COUNT;
         block++) {

        uint32_t byte = block / 8;
        uint8_t bit = (uint8_t)(1 << (block % 8));

        if ((bitmap[byte] & bit) == 0) {
            bitmap[byte] |= bit;

            return (int)block;
        }
    }

    return -1;
}

/*
 * Free a data block.
 */
static void free_block(uint32_t block)
{
    uint8_t *bitmap;
    uint32_t byte;
    uint8_t bit;

    if (block < RAMDISK_DATA_START ||
        block >= RAMDISK_BLOCK_COUNT) {
        return;
    }

    bitmap = get_block_bitmap();

    byte = block / 8;
    bit = (uint8_t)(1 << (block % 8));

    bitmap[byte] &= (uint8_t)~bit;
}

/*
 * Copy a file name into a directory entry.
 */
static void copy_name(char *destination, const char *source)
{
    uint32_t i;

    for (i = 0; i < RAMDISK_NAME_LEN - 1; i++) {
        destination[i] = source[i];

        if (source[i] == '\0') {
            return;
        }
    }

    destination[RAMDISK_NAME_LEN - 1] = '\0';
}

/*
 * Initialise the file system.
 */
void fs_init(void)
{
    uint32_t i;
    inode_t *inodes;
    directory_entry_t *dir;

    ramdisk_init();

    inodes = ramdisk_get_inodes();
    dir = get_directory();

    /*
     * Clear inode table.
     */
    for (i = 0; i < RAMDISK_MAX_INODES; i++) {
        inodes[i].size = 0;
        inodes[i].used = 0;

        inodes[i].blocks[0] = 0;
        inodes[i].blocks[1] = 0;
        inodes[i].blocks[2] = 0;
        inodes[i].blocks[3] = 0;
        inodes[i].blocks[4] = 0;
        inodes[i].blocks[5] = 0;
        inodes[i].blocks[6] = 0;
        inodes[i].blocks[7] = 0;
    }

    /*
     * Clear directory.
     */
    for (i = 0; i < FS_MAX_FILES; i++) {
        dir[i].name[0] = '\0';
        dir[i].inode = 0;
    }

    /*
     * Clear inode bitmap.
     */
    for (i = 0; i < RAMDISK_BLOCK_SIZE; i++) {
        get_inode_bitmap()[i] = 0;
    }

    /*
     * Open-file table starts empty.
     */
    for (i = 0; i < FS_MAX_OPEN_FILES; i++) {
        open_files[i].used = 0;
        open_files[i].inode = 0;
        open_files[i].position = 0;
    }
}

/*
 * Create a new empty file.
 *
 * Stage 4 - Lecture L12
 */
int fs_create(const char *name)
{
    directory_entry_t *dir;
    inode_t *inodes;
    int inode_number;
    uint32_t i;

    if (name == (const char *)0 || name[0] == '\0') {
        return -1;
    }

    /*
     * Do not create a duplicate file.
     */
    if (find_file(name) >= 0) {
        return -1;
    }

    dir = get_directory();
    inodes = ramdisk_get_inodes();

    /*
     * Find a free directory entry.
     */
    for (i = 0; i < FS_MAX_FILES; i++) {
        if (dir[i].inode == 0) {
            break;
        }
    }

    if (i >= FS_MAX_FILES) {
        return -1;
    }

    /*
     * Allocate an inode.
     */
    inode_number = allocate_inode();

    if (inode_number < 0) {
        return -1;
    }

    /*
     * Initialise the inode.
     */
    inodes[inode_number].size = 0;
    inodes[inode_number].used = 1;

    for (uint32_t j = 0; j < 8; j++) {
        inodes[inode_number].blocks[j] = 0;
    }

    /*
     * Create directory entry.
     */
    copy_name(dir[i].name, name);
    dir[i].inode = (uint32_t)inode_number;

    return 0;
}

/*
 * Open an existing file.
 */
int fs_open(const char *name)
{
    int dir_index;
    uint32_t i;
    directory_entry_t *dir;

    dir_index = find_file(name);

    if (dir_index < 0) {
        return -1;
    }

    dir = get_directory();

    for (i = 0; i < FS_MAX_OPEN_FILES; i++) {
        if (!open_files[i].used) {
            open_files[i].used = 1;
            open_files[i].inode = dir[dir_index].inode;
            open_files[i].position = 0;

            return (int)i;
        }
    }

    return -1;
}

/*
 * Read data from an open file.
 */
int fs_read(int fd, char *buffer, uint32_t size)
{
    inode_t *inodes;
    inode_t *inode;
    uint32_t remaining;
    uint32_t total;
    uint32_t position;

    if (fd < 0 || fd >= FS_MAX_OPEN_FILES ||
        !open_files[fd].used || buffer == (char *)0) {
        return -1;
    }

    inodes = ramdisk_get_inodes();
    inode = &inodes[open_files[fd].inode];

    position = open_files[fd].position;

    if (position >= inode->size) {
        return 0;
    }

    remaining = inode->size - position;

    if (size > remaining) {
        size = remaining;
    }

    total = 0;

    while (total < size) {
        uint32_t block_index;
        uint32_t offset;
        uint32_t block_number;
        uint8_t *block;
        uint32_t count;

        block_index = position / RAMDISK_BLOCK_SIZE;
        offset = position % RAMDISK_BLOCK_SIZE;

        if (block_index >= 8) {
            break;
        }

        block_number = inode->blocks[block_index];

        if (block_number == 0) {
            break;
        }

        block = ramdisk_get_block(block_number);

        count = RAMDISK_BLOCK_SIZE - offset;

        if (count > size - total) {
            count = size - total;
        }

        for (uint32_t i = 0; i < count; i++) {
            buffer[total + i] = (char)block[offset + i];
        }

        total += count;
        position += count;
    }

    open_files[fd].position = position;

    return (int)total;
}

/*
 * Write data to an open file.
 */
int fs_write(int fd, const char *buffer, uint32_t size)
{
    inode_t *inodes;
    inode_t *inode;
    uint32_t position;
    uint32_t total;

    if (fd < 0 || fd >= FS_MAX_OPEN_FILES ||
        !open_files[fd].used || buffer == (const char *)0) {
        return -1;
    }

    inodes = ramdisk_get_inodes();
    inode = &inodes[open_files[fd].inode];

    position = open_files[fd].position;

    if (position >= FS_MAX_FILE_SIZE) {
        return 0;
    }

    if (size > FS_MAX_FILE_SIZE - position) {
        size = FS_MAX_FILE_SIZE - position;
    }

    total = 0;

    while (total < size) {
        uint32_t block_index;
        uint32_t offset;
        uint32_t block_number;
        uint8_t *block;
        uint32_t count;

        block_index = position / RAMDISK_BLOCK_SIZE;
        offset = position % RAMDISK_BLOCK_SIZE;

        if (block_index >= 8) {
            break;
        }

        /*
         * Allocate a new data block when needed.
         */
        if (inode->blocks[block_index] == 0) {
            int new_block = allocate_block();

            if (new_block < 0) {
                break;
            }

            inode->blocks[block_index] = (uint32_t)new_block;
        }

        block_number = inode->blocks[block_index];
        block = ramdisk_get_block(block_number);

        count = RAMDISK_BLOCK_SIZE - offset;

        if (count > size - total) {
            count = size - total;
        }

        for (uint32_t i = 0; i < count; i++) {
            block[offset + i] = (uint8_t)buffer[total + i];
        }

        total += count;
        position += count;
    }

    if (position > inode->size) {
        inode->size = position;
    }

    open_files[fd].position = position;

    return (int)total;
}

/*
 * Append data to the end of an open file.
 * Stage 4 - Lecture L12
 */
int fs_append(int fd, const char *buffer, uint32_t size)
{
    inode_t *inodes;
    inode_t *inode;

    if (fd < 0 || fd >= FS_MAX_OPEN_FILES ||
        !open_files[fd].used || buffer == (const char *)0) {
        return -1;
    }

    inodes = ramdisk_get_inodes();
    inode = &inodes[open_files[fd].inode];

    open_files[fd].position = inode->size;

    return fs_write(fd, buffer, size);
}

/*
 * Close an open file.
 */
void fs_close(int fd)
{
    if (fd < 0 || fd >= FS_MAX_OPEN_FILES) {
        return;
    }

    open_files[fd].used = 0;
    open_files[fd].inode = 0;
    open_files[fd].position = 0;
}

/*
 * Delete a file.
 */
int fs_unlink(const char *name)
{
    int dir_index;
    uint32_t inode_number;
    uint32_t i;
    inode_t *inodes;
    directory_entry_t *dir;

    dir_index = find_file(name);

    if (dir_index < 0) {
        return -1;
    }

    dir = get_directory();
    inode_number = dir[dir_index].inode;

    inodes = ramdisk_get_inodes();

    /*
     * Free all data blocks.
     */
    for (i = 0; i < 8; i++) {
        if (inodes[inode_number].blocks[i] != 0) {
            free_block(inodes[inode_number].blocks[i]);
            inodes[inode_number].blocks[i] = 0;
        }
    }

    inodes[inode_number].size = 0;
    inodes[inode_number].used = 0;

    free_inode(inode_number);

    /*
     * Remove directory entry.
     */
    dir[dir_index].name[0] = '\0';
    dir[dir_index].inode = 0;

    return 0;
}

/*
 * List all files in the directory.
 */
void fs_list(void)
{
    directory_entry_t *dir;
    inode_t *inodes;
    uint32_t i;

    dir = get_directory();
    inodes = ramdisk_get_inodes();

    vga_printf("\nFile System\n");
    vga_printf("-------------------------\n");

    for (i = 0; i < FS_MAX_FILES; i++) {
        if (dir[i].inode != 0) {
            vga_printf("%s  %u bytes\n",
                    dir[i].name,
                    inodes[dir[i].inode].size);
        }
    }

    vga_printf("-------------------------\n");
}
