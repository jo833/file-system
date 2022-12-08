#include <stdio.h>
#include "mfs.h"
#include <string.h>

#define BUFFER_SIZE (1000)

// takes a host name and port number and uses those to find the server exporting the file system.
int MFS_Init(char *hostname, int port){


}
// takes the parent inode number (which should be the inode number of a directory) and looks up the entry name in it. 
//The inode number of name is returned. 
//Success: return inode number of name; failure: return -1.
// Failure modes: invalid pinum, name does not exist in pinum.
int MFS_Lookup(int pinum, char *name){
 if(pinum < 0 || pinum >= MFS_BLOCK_SIZE){
        return -1;
    }
// first use pinum to find parent directory
// then loop through children and compare the names, searching for the name entered into the function
// return -1 if found, otherwise throw error for name does not exist in pinum

}
/*returns some information about the file specified by inum. Upon success, return 0, otherwise -1. 
The exact info returned is defined by MFS_Stat_t. 
Failure modes: inum does not exist. File and directory sizes are described below.*/
int MFS_Stat(int inum, MFS_Stat_t *m){
     if(inum < 0 || inum >= MFS_BLOCK_SIZE){
        return -1;
    }
// use inum to search for file
// throw error if inum does not exist
// once the file is located, use inode_t to change MFS_Stat_t to include the proper type/size
return 0;
}
/*writes a buffer of size nbytes (max size: 4096 bytes) at the byte offset specified by offset. 
Returns 0 on success, -1 on failure. 
Failure modes: invalid inum, invalid nbytes, invalid offset, not a regular file (because you can't write to directories).*/
int MFS_Write(int inum, char *buffer, int offset, int nbytes){
     if(inum < 0 || inum >= MFS_BLOCK_SIZE || nbytes < 0|| nbytes >= MFS_BLOCK_SIZE){
        return -1;
    }

// use inum to index inode bitmap and check if its allocated, if not throw error
// then use inum to find inode in inode table
// use offset to determine which inode pointer to use to access what block, then write buffer to that block
return 0;
}
/*reads nbytes of data (max size 4096 bytes) specified by the byte offset offset into the buffer from file specified by inum. 
The routine should work for either a file or directory; directories should return data in the format specified by MFS_DirEnt_t. 
Success: 0, failure: -1. Failure modes: invalid inum, invalid offset, invalid nbytes.*/
int MFS_Read(int inum, char *buffer, int offset, int nbytes){
     if(inum < 0 || inum >= MFS_BLOCK_SIZE || nbytes < 0|| nbytes > MFS_BLOCK_SIZE){
        return -1;
    }
   
// use inum to index inode bitmap and check if its allocated, if not throw error
// then use inum to find inode in inode table
// use offset to determine which inode pointer to use to access what block, then set buffer to that data
return 0;
}
//makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) in the parent directory specified by pinum of name name. 
//Returns 0 on success, -1 on failure. Failure modes: pinum does not exist, or name is too long. 
//If name already exists, return success.
int MFS_Creat(int pinum, int type, char *name){
      if(pinum < 0 || pinum >= MFS_BLOCK_SIZE){
        return -1;
    }
     if(MFS_Lookup(pinum, *name)!= -1){
        return 0;
    }
    if(strlen(name) > 28){
        return -1;
    }
// Use look up to check if the name already exists, throw error if it does
// find open spot in inode table, allocate it
// use that inum to create new inode in inode table
// find open spot in data table, allocate it
// use that to index data blocks, set name
return 0;
}
/*removes the file or directory name from the directory specified by pinum. 
0 on success, -1 on failure. Failure modes: pinum does not exist, directory is NOT empty. 
Note that the name not existing is NOT a failure by our definition (think about why this might be).*/
int MFS_Unlink(int pinum, char *name){

    if(pinum < 0 || pinum >= MFS_BLOCK_SIZE){
        return -1;
    }
    if(MFS_Lookup(pinum, *name)== -1){
        return -1;
    }


// use pinum to find parent directory
// loop through parent directory to find name
// if name doesn't exist, throw an error
// if name is a non-empty directory, throw an error
// otherwise remove the directory/file and update data accordingly

}
//just tells the server to force all of its data structures to disk and shutdown by calling exit(0). 
//This interface will mostly be used for testing purposes.
int MFS_Shutdown(){
    close(MFS_REGULAR_FILE); //
    exit(0);
}
// server code
int main(int argc, char *argv[]) {
    int sd = MFS_Init(10000);
    assert(sd > -1);
    while (1) {
	struct sockaddr_in addr;
	char message[BUFFER_SIZE];
	printf("server:: waiting...\n");
	int rc = MFS_Read(sd, &addr, message, BUFFER_SIZE);
	printf("server:: read message [size:%d contents:(%s)]\n", rc, message);
	if (rc > 0) {
            char reply[BUFFER_SIZE];
            sprintf(reply, "goodbye world");
            rc = MFS_Write(sd, &addr, reply, BUFFER_SIZE);
	    printf("server:: reply\n");
	} 
    }
    return 0; 
}
    


 