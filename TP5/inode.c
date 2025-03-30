#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"
#include <string.h>

#define INODES_PER_SECTOR (DISKIMG_SECTOR_SIZE / sizeof(struct inode)) //16 inodes por bloque
#define ADDRESSES_PER_SECTOR (DISKIMG_SECTOR_SIZE / sizeof(uint16_t)) //256 numeros de bloques

#define INODE_START_SECTOR 2
#define INODE_SIMPLE_BLOCKS 7
#define INODE_DOUBLE_BLOCK  1
#define INODE_DIRECT_BLOCKS 8

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    int totalInodes = fs->superblock.s_isize * INODES_PER_SECTOR; 

    if(!fs || !inp || inumber < 1 || inumber > totalInodes){return -1;}

    int inodeSector = INODE_START_SECTOR + ((inumber - 1) / INODES_PER_SECTOR); 
    int inodeOffset = (inumber - 1) % INODES_PER_SECTOR; 

    uint8_t buffer[DISKIMG_SECTOR_SIZE]; 
    int err = diskimg_readsector(fs->dfd, inodeSector, buffer); 
    if (err<0){return -1;}

    memcpy(inp, buffer + inodeOffset * sizeof(struct inode), sizeof(struct inode)); 

    return 0; 
}


int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {  
	if(!fs || !inp || blockNum < 0){return -1;}	

	int is_large_file = (inp->i_mode & ILARG);

	if(!is_large_file){
		// Caso de archivo pequeño: solo bloques directos
		if(blockNum >= (int)INODE_DIRECT_BLOCKS){return -1;}
		return inp->i_addr[blockNum];

	}else{
		// Caso de archivo grande: manejo de direccionamiento simple y doble
		if(blockNum < (int)(INODE_SIMPLE_BLOCKS * ADDRESSES_PER_SECTOR)){
			// Bloque de direccionamiento simple
			int addr_index = blockNum / ADDRESSES_PER_SECTOR; 
			int offset = blockNum % ADDRESSES_PER_SECTOR; 

			uint16_t indirectBuffer[ADDRESSES_PER_SECTOR];

			int err = diskimg_readsector(fs->dfd, inp->i_addr[addr_index], indirectBuffer); 
			if(err<0){return -1;}

			return indirectBuffer[offset]; 

		}else{
			// Bloque de direccionamiento doble
			blockNum -= INODE_SIMPLE_BLOCKS * ADDRESSES_PER_SECTOR;

			if(blockNum >= (int)(ADDRESSES_PER_SECTOR * ADDRESSES_PER_SECTOR)){return -1;}

			int first_level_index = blockNum /ADDRESSES_PER_SECTOR; 
			int second_level_index = blockNum % ADDRESSES_PER_SECTOR; 

			uint16_t firstLevelBuffer[ADDRESSES_PER_SECTOR]; 
			uint16_t secondLevelBuffer[ADDRESSES_PER_SECTOR];

			int err = diskimg_readsector(fs->dfd, inp->i_addr[INODE_SIMPLE_BLOCKS], firstLevelBuffer);
			if(err<0){return -1;}

			err = diskimg_readsector(fs->dfd, firstLevelBuffer[first_level_index], secondLevelBuffer); 
			if(err<0){return -1;}

			return secondLevelBuffer[second_level_index];
		}
	}        
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
