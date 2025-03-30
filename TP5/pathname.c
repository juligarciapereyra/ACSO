#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (!fs || !pathname || pathname[0] != '/') {return -1;}

    int currentInumber = ROOT_INUMBER; 
    char *pathCopy = strdup(pathname); 
    if (!pathCopy) {return -1;}

    char *token = strtok(pathCopy, "/");
    while (token != NULL) {
        struct direntv6 dirEntry;

        int ret = directory_findname(fs, token, currentInumber, &dirEntry);
        if (ret == -1) {
            free(pathCopy);
            return -1;
        }

        currentInumber = dirEntry.d_inumber;
        token = strtok(NULL, "/");
    }

    free(pathCopy);
    return currentInumber; 
}
