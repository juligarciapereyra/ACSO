#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber, struct direntv6 *dirEnt) {
    if (!fs || !name || !dirEnt) {return -1;}

    struct inode dirInode; 
    int ret = inode_iget(fs, dirinumber, &dirInode); 
    if (ret == -1) {return -1;}

    if ((dirInode.i_mode & IFMT) != IFDIR) {return -1;}

    int fileSize = inode_getsize(&dirInode); 

    int bytesRead = 0; 
    uint8_t buffer[DISKIMG_SECTOR_SIZE]; 

    for (int blockNum = 0; bytesRead < fileSize; blockNum++) {
        int bytesInBlock = file_getblock(fs, dirinumber, blockNum, buffer);
        if (bytesInBlock == -1) {return -1;}

        for (int i = 0; i < bytesInBlock; i += sizeof(struct direntv6)) {
            struct direntv6 entry;
            memcpy(&entry, buffer + i, sizeof(struct direntv6)); 

            if (strncmp(entry.d_name, name, sizeof(entry.d_name)) == 0) {
                memcpy(dirEnt, &entry, sizeof(struct direntv6));
                return 0; 
            }
        }

        bytesRead += bytesInBlock;
    }
    return -1; 
}
