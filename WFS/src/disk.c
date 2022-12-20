/* disk.c: SimpleFS disk emulator */

#include "sfs/disk.h"
#include "sfs/logging.h"

#include <fcntl.h>
#include <unistd.h>

/* Internal Prototyes */

bool    disk_sanity_check(Disk *disk, size_t blocknum, const char *data);

/* External Functions */

/**
 *
 * Opens disk at specified path with the specified number of blocks by doing
 * the following:
 *
 *  1. Allocate Disk structure and sets appropriate attributes.
 *
 *  2. Open file descriptor to specified path.
 *
 *  3. Truncate file to desired file size (blocks * BLOCK_SIZE).
 *
 * @param       path        Path to disk image to create.
 * @param       blocks      Number of blocks to allocate for disk image.
 *
 * @return      Pointer to newly allocated and configured Disk structure (NULL
 *              on failure).
 **/
Disk *	disk_open(const char *path, size_t blocks) {

    // allocate disk structure
    Disk* d = calloc(1, sizeof(Disk));
    if (!(d)){
        return NULL;
    }

    d->blocks = blocks;
    d->reads = 0;
    d->writes = 0;

    // open file descriptor to specified path
    int fd = open(path, O_RDWR | O_CREAT, 0600);
    if (fd <= 0){
        free(d);
        return NULL;
    }
    d->fd = fd;

    // truncate file to desired file size
    if (ftruncate(fd, blocks * BLOCK_SIZE) < 0){
        close(fd);
        free(d);
        return NULL;
    }

    return d;

}

/**
 * Close disk structure by doing the following:
 *
 *  1. Close disk file descriptor.
 *
 *  2. Report number of disk reads and writes.
 *
 *  3. Release disk structure memory.
 *
 * @param       disk        Pointer to Disk structure.
 */
void	disk_close(Disk *disk) {

    // close disk file descriptor
    close(disk->fd);

    // report number of disk reads and writes
    printf("%lu disk block reads\n", disk->reads);
    printf("%lu disk block writes\n", disk->writes);

    // release disk structure memory
    free(disk);

}

/**
 * Read data from disk at specified block into data buffer by doing the
 * following:
 *
 *  1. Perform sanity check.
 *
 *  2. Seek to specified block.
 *
 *  3. Read from block to data buffer (must be BLOCK_SIZE).
 *
 * @param       disk        Pointer to Disk structure.
 * @param       block       Block number to perform operation on.
 * @param       data        Data buffer.
 *
 * @return      Number of bytes read.
 *              (BLOCK_SIZE on success, DISK_FAILURE on failure).
 **/
ssize_t disk_read(Disk *disk, size_t block, char *data) {

    // perform sanity check
    if (!disk_sanity_check(disk, block, data)) {

        return DISK_FAILURE;
    }

    int fd = disk->fd;
    
    // seek to specifed block
    lseek(fd, (block * BLOCK_SIZE), SEEK_SET);

    // read from block to data (must be BLOCK_SIZE)
    size_t count = read(fd, data, BLOCK_SIZE);

    disk->reads += 1;

    // return number of bytes read
    if (count == BLOCK_SIZE) {
        return BLOCK_SIZE;
    } else {

        return DISK_FAILURE;
    }
}

/**
 * Write data to disk at specified block from data buffer by doing the
 * following:
 *
 *  1. Perform sanity check.
 *
 *  2. Seek to specified block.
 *
 *  3. Write data buffer (must be BLOCK_SIZE) to disk block.
 *
 * @param       disk        Pointer to Disk structure.
 * @param       block       Block number to perform operation on.
 * @param       data        Data buffer.
 *
 * @return      Number of bytes written.
 *              (BLOCK_SIZE on success, DISK_FAILURE on failure).
 **/
ssize_t disk_write(Disk *disk, size_t block, char *data) {

    // perform sanity check
    if (!disk_sanity_check(disk, block, data))
        return DISK_FAILURE;

    int fd = disk->fd;

    // seek to specified block
    off_t off = lseek(fd, (block * BLOCK_SIZE), SEEK_SET);
    if (off < 0)
        return DISK_FAILURE;

    // write data buffer to disk block
    size_t count = write(fd, data, BLOCK_SIZE);

    disk->writes += 1;

    // return number of bytes read
    if (count == BLOCK_SIZE)
        return BLOCK_SIZE;
    else
        return DISK_FAILURE;

}

/* Internal Functions */

/**
 * Perform sanity check before read or write operation by doing the following:
 *
 *  1. Check for valid disk.
 *
 *  2. Check for valid block.
 *
 *  3. Check for valid data.
 *
 * @param       disk        Pointer to Disk structure.
 * @param       block       Block number to perform operation on.
 * @param       data        Data buffer.
 *
 * @return      Whether or not it is safe to perform a read/write operation
 *              (true for safe, false for unsafe).
 **/
bool    disk_sanity_check(Disk *disk, size_t block, const char *data) {
    
    // check for valid disk
    if (!disk)
        return false;

    // check for valid block
    if (block >= disk->blocks)
        return false;

    // check for valid data
    if (!data)
        return false;

    // now, its safe to perform a read/write operation!! <3
    return true;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
