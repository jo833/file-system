#include "mfs.h"
#include "udp.h"
#include <stdlib.h>

#define BUFFER_SIZE (1000)

int fd;
struct sockaddr_in addrSnd, addrRcv;

// takes a host name and port number and uses those to find the server exporting the file system.
int MFS_Init(char *hostname, int port)
{
    fd = UDP_Open(port);
    int rc = UDP_FillSockAddr(&addrSnd, hostname, 10000);

    if (rc < 0)
    {
        printf("client:: failed to send\n");
        exit(1);
    }
}
// takes the parent inode number (which should be the inode number of a directory) and looks up the entry name in it.
// The inode number of name is returned.
// Success: return inode number of name; failure: return -1.
// Failure modes: invalid pinum, name does not exist in pinum.
int MFS_Lookup(int pinum, char *name)
{

    char message[BUFFER_SIZE];
    char indentifier[2] = "0\0";
    char *binary;
    itoa(pinum, binary, 2);
    int num_zeros = 16 - strlen(binary);

    char zeros[num_zeros + 1];
    for (int i = 0; i < num_zeros; i++)
    {
        zeros[i] = '0';
    }
    zeros[num_zeros] = '\0';

    strcat(message, indentifier);
    strcat(message, zeros);
    strcat(message, binary);
    strcat(message, name);

    while (1)
    {
        int rc = UDP_Write(fd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(fd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    }
    return 0;
}
/*returns some information about the file specified by inum. Upon success, return 0, otherwise -1.
The exact info returned is defined by MFS_Stat_t.
Failure modes: inum does not exist. File and directory sizes are described below.*/
int MFS_Stat(int inum, MFS_Stat_t *m)
{
    char message[BUFFER_SIZE];
    char indentifier[2] = "1\0";
    char *binary;
    itoa(inum, binary, 2);
    int num_zeros = 16 - strlen(binary);

    char zeros[num_zeros + 1];
    for (int i = 0; i < num_zeros; i++)
    {
        zeros[i] = '0';
    }
    zeros[num_zeros] = '\0';

    strcat(message, indentifier);
    strcat(message, zeros);
    strcat(message, binary);

    while (1)
    {
        int rc = UDP_Write(fd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(fd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    }
    return 0;
}
/*writes a buffer of size nbytes (max size: 4096 bytes) at the byte offset specified by offset.
Returns 0 on success, -1 on failure.
Failure modes: invalid inum, invalid nbytes, invalid offset, not a regular file (because you can't write to directories).*/
int MFS_Write(int inum, char *buffer, int offset, int nbytes)
{
    char message[BUFFER_SIZE];
    char indentifier[2] = "2\0";
    char *binary1;
    itoa(inum, binary1, 2);
    int num_zeros1 = 16 - strlen(binary1);

    char zeros1[num_zeros1 + 1];
    for (int i = 0; i < num_zeros1; i++)
    {
        zeros1[i] = '0';
    }
    zeros1[num_zeros1] = '\0';

    char *binary2;
    itoa(offset, binary2, 2);
    int num_zeros2 = 16 - strlen(binary2);

    char zeros2[num_zeros2 + 1];
    for (int i = 0; i < num_zeros2; i++)
    {
        zeros2[i] = '0';
    }
    zeros2[num_zeros2] = '\0';

    char *binary3;
    itoa(nbytes, binary3, 2);
    int num_zeros3 = 16 - strlen(binary3);

    char zeros3[num_zeros3 + 1];
    for (int i = 0; i < num_zeros3; i++)
    {
        zeros3[i] = '0';
    }
    zeros3[num_zeros3] = '\0';

    strcat(message, indentifier);
    strcat(message, zeros1);
    strcat(message, binary1);
    strcat(message, buffer);
    strcat(message, zeros2);
    strcat(message, binary2);
    strcat(message, zeros3);
    strcat(message, binary3);

    while (1)
    {
        int rc = UDP_Write(fd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(fd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    }
    return 0;
}
/*reads nbytes of data (max size 4096 bytes) specified by the byte offset offset into the buffer from file specified by inum.
The routine should work for either a file or directory; directories should return data in the format specified by MFS_DirEnt_t.
Success: 0, failure: -1. Failure modes: invalid inum, invalid offset, invalid nbytes.*/
int MFS_Read(int inum, char *buffer, int offset, int nbytes)
{
    char message[BUFFER_SIZE];
    char indentifier[2] = "3\0";
    char *binary1;
    itoa(inum, binary1, 2);
    int num_zeros1 = 16 - strlen(binary1);

    char zeros1[num_zeros1 + 1];
    for (int i = 0; i < num_zeros1; i++)
    {
        zeros1[i] = '0';
    }
    zeros1[num_zeros1] = '\0';

    char *binary2;
    itoa(offset, binary2, 2);
    int num_zeros2 = 16 - strlen(binary2);

    char zeros2[num_zeros2 + 1];
    for (int i = 0; i < num_zeros2; i++)
    {
        zeros2[i] = '0';
    }
    zeros2[num_zeros2] = '\0';

    char *binary3;
    itoa(nbytes, binary3, 2);
    int num_zeros3 = 16 - strlen(binary3);

    char zeros3[num_zeros3 + 1];
    for (int i = 0; i < num_zeros3; i++)
    {
        zeros3[i] = '0';
    }
    zeros3[num_zeros3] = '\0';

    strcat(message, indentifier);
    strcat(message, zeros1);
    strcat(message, binary1);
    strcat(message, zeros2);
    strcat(message, binary2);
    strcat(message, zeros3);
    strcat(message, binary3);

    while (1)
    {
        int rc = UDP_Write(fd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(fd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    }
    return 0;
}
// makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) in the parent directory specified by pinum of name name.
// Returns 0 on success, -1 on failure. Failure modes: pinum does not exist, or name is too long.
// If name already exists, return success.
int MFS_Creat(int pinum, int type, char *name)
{
    char message[BUFFER_SIZE];
    char indentifier[2] = "4\0";
    char *binary1;
    itoa(pinum, binary1, 2);
    int num_zeros1 = 16 - strlen(binary1);

    char zeros1[num_zeros1 + 1];
    for (int i = 0; i < num_zeros1; i++)
    {
        zeros1[i] = '0';
    }
    zeros1[num_zeros1] = '\0';

    char *binary2;
    itoa(type, binary2, 2);
    int num_zeros2 = 16 - strlen(binary2);

    char zeros2[num_zeros2 + 1];
    for (int i = 0; i < num_zeros2; i++)
    {
        zeros2[i] = '0';
    }
    zeros2[num_zeros2] = '\0';

    strcat(message, indentifier);
    strcat(message, zeros1);
    strcat(message, binary1);
    strcat(message, zeros2);
    strcat(message, binary2);
    strcat(message, name);

    while (1)
    {
        int rc = UDP_Write(fd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(fd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    }
    return 0;
}
/*removes the file or directory name from the directory specified by pinum.
0 on success, -1 on failure. Failure modes: pinum does not exist, directory is NOT empty.
Note that the name not existing is NOT a failure by our definition (think about why this might be).*/
int MFS_Unlink(int pinum, char *name)
{
    char message[BUFFER_SIZE];
    char indentifier[2] = "5\0";
    char *binary1;
    itoa(pinum, binary1, 2);
    int num_zeros1 = 16 - strlen(binary1);

    char zeros1[num_zeros1 + 1];
    for (int i = 0; i < num_zeros1; i++)
    {
        zeros1[i] = '0';
    }
    zeros1[num_zeros1] = '\0';

    strcat(message, indentifier);
    strcat(message, zeros1);
    strcat(message, binary1);
    strcat(message, name);

    while (1)
    {
        int rc = UDP_Write(fd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(fd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    }
    return 0;
}
// just tells the server to force all of its data structures to disk and shutdown by calling exit(0).
// This interface will mostly be used for testing purposes.
int MFS_Shutdown()
{
    exit(0);
}
