#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mfs.h"

#define BUFFER_SIZE (10000)

// client code
int main(int argc, char *argv[])
{
    printf("client:: attempting to init MFS\n");
    int rc = MFS_Init("localhost", 8080);
    if (rc < 0)
    {
        printf("client:: failed to send\n");
        exit(1);
    }
    printf("client:: succeeded to init MFS\n");

    printf("client:: attempting to creat MFS\n");
    rc = MFS_Creat(0, MFS_DIRECTORY, "folder1");
    rc = MFS_Lookup(0, "folder1");
    rc = MFS_Creat(1, MFS_REGULAR_FILE, "file1");
    // rc = MFS_Lookup(1, "file1");
    // rc = MFS_Unlink(1, "file1");
    // rc = MFS_Lookup(1, "file1");
    // char *buf1 = malloc(4096);
    // memcpy(buf1, "START BLOCK 1", 13);
    // int i;
    // for(i = 13; i < 4085; i++)
    //     buf1[i] = '\x00';
    // memcpy(buf1 + 4085, "1 END BLOCK", 11);
    // rc = MFS_Write(2, buf1, 0, 4096);
    // char *buf2 = malloc(4096);
    // rc = MFS_Read(2, buf2, 0, 4096);
    // rc = MFS_Write(2, buf1, 4096, 4096);
    // rc = MFS_Read(2, buf2, 4096, 4096);
    
    if (rc < 0)
    {
        printf("client:: failed to send\n");
        exit(1);
    }
    // printf("client:: succeeded to create MFS: %d\n", rc);
    return 0;
}
