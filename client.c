#include <stdio.h>
#include <stdlib.h>
#include "mfs.h"

#define BUFFER_SIZE (1000)

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
    if (rc < 0)
    {
        printf("client:: failed to send\n");
        exit(1);
    }
    printf("client:: succeeded to create MFS: %d\n", rc);
    return 0;
}
