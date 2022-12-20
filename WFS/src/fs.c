/* fs.c: SimpleFS file system */

#include "sfs/fs.h"
#include "sfs/logging.h"
#include "sfs/utils.h"

#include <stdio.h>
#include <string.h>

size_t gimme_block(FileSystem *fs);

/* External Functions */

/**
 * Debug FileSystem by doing the following:
 *
 *  1. Read SuperBlock and report its information.
 *
 *  2. Read Inode Table and report information about each Inode.
 *
 * @param       disk        Pointer to Disk structure.
 **/
void    fs_debug(Disk *disk) {
    Block block;

    /* Read SuperBlock */
    if (disk_read(disk, 0, block.data) == DISK_FAILURE) {
        return;
    }

    printf("SuperBlock:\n");
    printf("    magic number is %s\n",
        (block.super.magic_number == MAGIC_NUMBER) ? "valid" : "invalid");
    printf("    %u blocks\n"         , block.super.blocks);
    printf("    %u inode blocks\n"   , block.super.inode_blocks);
    printf("    %u inodes\n"         , block.super.inodes);

    /* Read Inodes */
    for (int i = 1; i <= block.super.inode_blocks; i++) {
        Block B;
        if (disk_read(disk, i, B.data) == DISK_FAILURE) {
            return;
        }

        for (int j = 0; j < INODES_PER_BLOCK; j++) {

            /* read inode */
            if (B.inodes[j].valid) {
                printf("Inode %d:\n", j);
                printf("    size: %d bytes\n", B.inodes[j].size);
                printf("    direct blocks:");

                /* read direct ptrs */
                for (int k = 0; k < POINTERS_PER_INODE; k++) {
                    int block_num = B.inodes[j].direct[k];
                    if (block_num) {
                        printf(" %d", block_num);
                    }              
                }
                printf("\n");

                /* read indirect ptr */
                int iblock_num = B.inodes[j].indirect;
                if (iblock_num) {
                    printf("    indirect block: %d\n", iblock_num);

                    /* read indirect block */
                    Block iblock;
                    if(disk_read(disk, iblock_num, iblock.data) == DISK_FAILURE){
                       return;
                    }
                    
                    /* read indirect block ptrs */
                    printf("    indirect data blocks:");
                    for (int p = 0; p < POINTERS_PER_BLOCK; p++) {
                        int ipblock_num = iblock.pointers[p];
                        if(ipblock_num){
                            printf(" %d", ipblock_num);
                        }
                    }
                printf("\n");
                }

            }
        }

    }
}

/**
 * Format Disk by doing the following:
 *
 *  1. Write SuperBlock (with appropriate magic number, number of blocks,
 *  number of inode blocks, and number of inodes).
 *
 *  2. Clear all remaining blocks.
 *
 * Note: Do not format a mounted Disk!
 *
 * @param       fs      Pointer to FileSystem structure.
 * @param       disk    Pointer to Disk structure.
 * @return      Whether or not all disk operations were successful.
 **/
bool    fs_format(FileSystem *fs, Disk *disk) {

    /* check valid ptrs and if mounted fs*/
    if (!fs)
        return false;
    if (!disk)
        return false;
    if (fs->disk)
        return false;

    /* calc 10% */
    uint32_t ceil;
    if (disk->blocks % 10 == 0)
        ceil = disk->blocks * 0.10;
    else
        ceil = disk->blocks * 0.10 + 1;

    /* write superblock */
    Block block = {{0}};
    block.super.magic_number = MAGIC_NUMBER;
    block.super.blocks = disk->blocks;
    block.super.inode_blocks = ceil;
    block.super.inodes = ceil*INODES_PER_BLOCK;

    if(disk_write(disk, 0, block.data) == DISK_FAILURE)
        return false;
   
    /* clear remaining blocks */
    char buff[BLOCK_SIZE] = {0}; 
    for (size_t i = 1; i < disk->blocks; i++){
        if(disk_write(disk, i, buff) == DISK_FAILURE)
            return false;
    }

    return true;
}



/**
 * Mount specified FileSystem to given Disk by doing the following:
 *
 *  1. Read and check SuperBlock (verify attributes).
 *
 *  2. Verify and record FileSystem disk attribute. 
 *
 *  3. Copy SuperBlock to FileSystem meta data attribute
 *
 *  4. Initialize FileSystem free blocks bitmap.
 *
 * Note: Do not mount a Disk that has already been mounted!
 *
 * @param       fs      Pointer to FileSystem structure.
 * @param       disk    Pointer to Disk structure.
 * @return      Whether or not the mount operation was successful.
 **/
bool    fs_mount(FileSystem *fs, Disk *disk) {

    Block block;
    if(disk_read(disk, 0, block.data) == DISK_FAILURE)
        return false;

    /* read and check superblock (verify attributes) */
    if (block.super.magic_number != MAGIC_NUMBER)
        return false;
    if (block.super.blocks != disk->blocks)
        return false;
    if (block.super.inode_blocks < (disk->blocks / 10)) 
        return false;
    if (block.super.inodes != (block.super.inode_blocks * INODES_PER_BLOCK))
        return false;

    /* Verify and record disk attb */
    if (fs->disk) {
        return false;
    } else {
        fs->disk = disk;
    }

    /* copy superblock to metadata */
    fs->meta_data.magic_number = block.super.magic_number;
    fs->meta_data.blocks = block.super.blocks;
    fs->meta_data.inode_blocks = block.super.inode_blocks;
    fs->meta_data.inodes = block.super.inodes;

    /* initialize bitmap */
    fs->free_blocks = calloc(disk->blocks, sizeof(bool));
    memset(fs->free_blocks, true, disk->blocks);

    fs->free_blocks[0] = false;

    for (int i = 1; i <= fs->meta_data.inode_blocks; i++) {

        /* read inode block */
        Block B;
        if (disk_read(disk, i, B.data) == DISK_FAILURE)
            return false;

        /* mark inode block */
        fs->free_blocks[i] = false;

        /* check inodes in block */
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            
            /* if valid inode, check ptrs */
            if (B.inodes[j].valid) {

                /* check direct */
                for (int k = 0; k < POINTERS_PER_INODE; k++) {

                    /* if ptr being used, mark block */
                    if (B.inodes[j].direct[k]) 
                        fs->free_blocks[B.inodes[j].direct[k]] = false;
                }

                /* check indirect */
                if (B.inodes[j].indirect) {
                    Block iblock;
                    if (disk_read(disk, B.inodes[j].indirect, iblock.data) == DISK_FAILURE)
                        return false;

                    /* mark ptr block */
                    fs->free_blocks[B.inodes[j].indirect] = false;

                    /* check ptr block */
                    for (int p = 0; p < POINTERS_PER_BLOCK; p++) {

                        /* mark block */
                        if (iblock.pointers[p])
                            fs->free_blocks[iblock.pointers[p]] = false;
                    }
                }
            }
        }
    }
    return true;
}

/**
 * Unmount FileSystem from internal Disk by doing the following:
 *
 *  1. Set FileSystem disk attribute.
 *
 *  2. Release free blocks bitmap.
 *
 * @param       fs      Pointer to FileSystem structure.
 **/
void    fs_unmount(FileSystem *fs) {
    fs->disk = NULL;
    free(fs->free_blocks);
    fs->free_blocks = NULL;
}

/**
 * Allocate an Inode in the FileSystem Inode table by doing the following:
 *
 *  1. Search Inode table for free inode.
 *
 *  2. Reserve free inode in Inode table.
 *
 * Note: Be sure to record updates to Inode table to Disk.
 *
 * @param       fs      Pointer to FileSystem structure.
 * @return      Inode number of allocated Inode.
 **/
ssize_t fs_create(FileSystem *fs) {

    Block B;

    /* search free node list */
    for (int block = 0; block < fs->meta_data.inode_blocks; block++) {


        /* read inode block */
        if(disk_read(fs->disk, block + 1, B.data) == DISK_FAILURE)
            return -1;

        /* find free inode in table */
        for (int i = 0; i < INODES_PER_BLOCK; i++) {

            if (!B.inodes[i].valid) {

                /* mark as in use */
                B.inodes[i].valid = true;
               
                /* write back to disk */
                if(disk_write(fs->disk, block / INODES_PER_BLOCK + 1, B.data) == DISK_FAILURE)
                    return -1;

                /* return inode number */
                return i + (block * INODES_PER_BLOCK);
            }
        }
    }

    return -1;
}

/**
 * Remove Inode and associated data from FileSystem by doing the following:
 *
 *  1. Load and check status of Inode.
 *
 *  2. Release any direct blocks.
 *
 *  3. Release any indirect blocks.
 *
 *  4. Mark Inode as free in Inode table.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to remove.
 * @return      Whether or not removing the specified Inode was successful.
 **/
bool    fs_remove(FileSystem *fs, size_t inode_number) {

    size_t iblock = inode_number / INODES_PER_BLOCK;
    size_t inum   = inode_number % INODES_PER_BLOCK;
    Block block;

    /* read in inode block */
    if (disk_read(fs->disk, iblock + 1, block.data) == DISK_FAILURE)
        return false;

    /* check if valid first */
    if (block.inodes[inum].valid) {

        /* free direct blocks */
        for (int d = 0; d < POINTERS_PER_INODE; d++) {
            int db = block.inodes[inum].direct[d];

            /* mark db as free */
            if (db) {
                fs->free_blocks[db] = true;
                block.inodes[inum].direct[d] = 0;
            }
        }

        /* free indirect blocks */
        int ib = block.inodes[inum].indirect;
        if (ib) {
            Block ipblock;

            /* read indirect */
            if (disk_read(fs->disk, ib, ipblock.data) == DISK_FAILURE)
                return false;

            /* free ptrs */
            for (int p = 0; p < POINTERS_PER_BLOCK; p++) {
                if (ipblock.pointers[p]) {

                    /* mark data as free */
                    fs->free_blocks[ipblock.pointers[p]] = true;
                    ipblock.pointers[p] = 0;
                }
                    
            }

            /* mark ip as free */
            fs->free_blocks[ib] = true;
            block.inodes[inum].indirect = 0;
        }

        /* mark free in table */
        block.inodes[inum].valid = false;
        block.inodes[inum].size = 0;

        /* write back to disk */
        if (disk_write(fs->disk, iblock + 1, block.data) == DISK_FAILURE)
            return false;

        return true;
    }

    return false;
}

/**
 * Return size of specified Inode.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to remove.
 * @return      Size of specified Inode (-1 if does not exist).
 **/
ssize_t fs_stat(FileSystem *fs, size_t inode_number) {

    size_t iblock = inode_number / INODES_PER_BLOCK;
    size_t inum = inode_number % INODES_PER_BLOCK;

    Block block;

    if (disk_read(fs->disk, iblock + 1, block.data) == DISK_FAILURE)
        return -1;

    if (block.inodes[inum].valid)
        return block.inodes[inum].size;

    return -1;
}

/**
 * Read from the specified Inode into the data buffer exactly length bytes
 * beginning from the specified offset by doing the following:
 *
 *  1. Load Inode information.
 *
 *  2. Continuously read blocks and copy data to buffer.
 *
 *  Note: Data is read from direct blocks first, and then from indirect blocks.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to read data from.
 * @param       data            Buffer to copy data to.
 * @param       length          Number of bytes to read.
 * @param       offset          Byte offset from which to begin reading.
 * @return      Number of bytes read (-1 on error).
 **/
ssize_t fs_read(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset) {

	/* calc nums */
    size_t iblock = inode_number / INODES_PER_BLOCK;
    size_t inum = inode_number % INODES_PER_BLOCK;

    size_t data_b = offset / BLOCK_SIZE;
    size_t data_o = offset % BLOCK_SIZE;

    /* read inode */
    Block block;
    if (disk_read(fs->disk, iblock + 1, block.data) == DISK_FAILURE) {
        return -1;
    }

	/* adjust size */
    size_t size = block.inodes[inum].size;

    if (length + offset > size)
        length = size - offset;

    ssize_t ncopy;
    ssize_t nread = 0;

	/* addjuct ncopy */
    if (length + data_o < BLOCK_SIZE) {
       ncopy = length;
    } else {
        ncopy = BLOCK_SIZE - data_o;
    }
    while (nread < length) {

        /* read data block */
        Block Dblock;
        if (data_b < POINTERS_PER_INODE) {
			/* direct ptr */
            if(disk_read(fs->disk, block.inodes[inum].direct[data_b], Dblock.data) == DISK_FAILURE)
                return -1;

        } else {
			
			/* indirect ptr */
            size_t idata_o = data_b - POINTERS_PER_INODE;
            Block Iblock;
            if (disk_read(fs->disk, block.inodes[inum].indirect, Iblock.data) == DISK_FAILURE) 
                return -1;

            if (disk_read(fs->disk, Iblock.pointers[idata_o], Dblock.data) == DISK_FAILURE) 
                return -1;

        }
        
        /* copy chunk */
        memcpy(data + nread, Dblock.data + data_o, ncopy);
        nread += ncopy;

		/* update vals */
        if (length - nread > BLOCK_SIZE) {
            ncopy = BLOCK_SIZE;
        } else {
            ncopy = length - nread;
        }
        data_o = 0;
        data_b += 1;
    }

    return nread;
}



/**
 * Write to the specified Inode from the data buffer exactly length bytes
 * beginning from the specified offset by doing the following:
 *
 *  1. Load Inode information.
 *
 *  2. Continuously copy data from buffer to blocks.
 *
 *  Note: Data is read from direct blocks first, and then from indirect blocks.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to write data to.
 * @param       data            Buffer with data to copy
 * @param       length          Number of bytes to write.
 * @param       offset          Byte offset from which to begin writing.
 * @return      Number of bytes read (-1 on error).
 **/
ssize_t fs_write(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset) {


    /* inode info */
    size_t iblock = inode_number / INODES_PER_BLOCK + 1;
    size_t inum = inode_number % INODES_PER_BLOCK;

    /* ptr info */
    size_t data_b = offset / BLOCK_SIZE;
    size_t data_o = offset % BLOCK_SIZE;

    /* read in inode */
    Block block;
    if (disk_read(fs->disk, iblock, block.data) == DISK_FAILURE)
        return -1;


    size_t ncopy, nwrite = 0, blk = -1;
    while(nwrite < length) {

		Block Dblock = {{0}};
		if (data_b < POINTERS_PER_INODE) {
        	
			/* find new block */
        	if (block.inodes[inum].direct[data_b] == 0) {
				size_t b = gimme_block(fs);

				if (b == -1)
					return nwrite;

            	block.inodes[inum].direct[data_b] = b;   
			} else {	

				/* read in data block */
        		if (disk_read(fs->disk, block.inodes[inum].direct[data_b], Dblock.data) == DISK_FAILURE)
            		return -1;
			}
			blk = block.inodes[inum].direct[data_b];
		} else {
            size_t idata_o = data_b - POINTERS_PER_INODE;

			Block pblock = {{0}};
			/* check and find indirect block */
			if (block.inodes[inum].indirect == 0) {
				size_t i = gimme_block(fs);
				if (i == -1)
					return nwrite;
				block.inodes[inum].indirect = i;
			} else {	

				/* read in ptr block */
             	if (disk_read(fs->disk, block.inodes[inum].indirect, pblock.data) == DISK_FAILURE) 
                	return -2;
			}


            /* check ptr */
			if (pblock.pointers[idata_o] == 0) {
				size_t p = gimme_block(fs);
				pblock.pointers[idata_o] = p;
				if (p == -1)
					return nwrite;
			} else {

				/* read data */
				if (disk_read(fs->disk, pblock.pointers[idata_o], Dblock.data) == DISK_FAILURE) 
                	return -3;
			}

			blk = pblock.pointers[idata_o];
			
			/* write back ptr block */
			if(disk_write(fs->disk, block.inodes[inum].indirect, pblock.data) == DISK_FAILURE)
				return -4;
		}

		/* update ncopy */
        if ((length - nwrite) + data_o < BLOCK_SIZE) {
            ncopy = length - nwrite;
        } else {
            ncopy = BLOCK_SIZE - data_o;
        }

        /* copy the data */
        memcpy(Dblock.data + data_o, data + nwrite, ncopy);
        nwrite += ncopy;

        /* update inode info */
        block.inodes[inum].size = nwrite + offset;


        /* write block back */
        if (disk_write(fs->disk, blk, Dblock.data) == DISK_FAILURE)
            return -5;

        /* reset stuff */
        data_o = 0;
        data_b++;

        /* write back inode */
        if (disk_write(fs->disk, iblock, block.data) == DISK_FAILURE)
            return -1;

    }
    return nwrite;
     
}

/**
 *
 * the GIMME_BLOCK function loops through the free block list and find the next
 * avaliable block and GIMMES to the caller!! :) happi pandaA
 *
 * @param       fs              Pointer to FileSystem structure.
 * @return      first avaliable Block to write to (-1 on nothing found).
 *
 * */

size_t gimme_block(FileSystem *fs){

    for (size_t i = 1; i < fs->disk->blocks; i++){

        if (fs->free_blocks[i] == true){

            fs->free_blocks[i] = false;
            return i;
        }
    }

    return -1;

}
 
