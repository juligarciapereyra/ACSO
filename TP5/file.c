#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"


int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    if(!fs || inumber < 0 || blockNum < 0 || !buf){return -1;}

    struct inode inp;
    int ret = inode_iget(fs, inumber, &inp); 
    if(ret == -1){return -1;}

    int fileSize = inode_getsize(&inp); 
    if(fileSize<0){return -1;}

    int blockSize = DISKIMG_SECTOR_SIZE;
    if(blockNum > fileSize/blockSize){return -1;}

    int bytesToRead = blockSize; 
    if(blockNum == fileSize/blockSize){
    	// El último bloque puede contener menos bytes que un bloque completo
    	bytesToRead = fileSize %blockSize; 
    } 

    int blockAddr = inode_indexlookup(fs, &inp, blockNum);
    if(blockAddr == -1){return -1;}

    int err = diskimg_readsector(fs->dfd, blockAddr, buf); 
    if(err == -1){return -1;}

    return bytesToRead; 
   
}