#include "mfs.h"
#include "udp.h"
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE (1000)

int MIN_PORT = 20000;
int MAX_PORT = 40000;
int sd;
struct sockaddr_in addrSnd, addrRcv;

// Use Ctrl-C first
// if it doesn't work, new terminal: ps aux | grep "/home/cs537-1/tests/p4/p4-test/project4.py"  | cut -d' '  -f 4 | xargs kill -9

int num_digits(int num)
{
    int count = 0;
    while (num != 0)
    {
        num = num / 10;
        count++;
    }
    return count;
}

// takes a host name and port number and uses those to find the server exporting the file system.
int MFS_Init(char *hostname, int port)
{
    printf("client:: beginning MFS_Init\n");
    sd = UDP_Open(51328);

    printf("client:: init sd to %d\n", sd);
    int rc = UDP_FillSockAddr(&addrSnd, hostname, port);
    //    printf("client:: init addrSnd to %s\n", addrSnd);

    if (rc < 0)
    {
        printf("client:: failed to send (UDP_FillSockAddr)\n");
        exit(1);
    }

    printf("client:: finished init\n");
    return 0;
}
// takes the parent inode number (which should be the inode number of a directory) and looks up the entry name in it.
// The inode number of name is returned.
// Success: return inode number of name; failure: return -1.
// Failure modes: invalid pinum, name does not exist in pinum.
int MFS_Lookup(int pinum, char *name)
{
    if (pinum < 0 || pinum >= MFS_BLOCK_SIZE)
    {
        return -1;
    }
    printf("client:: starting MFS_Lookup\n");

    printf("client:: sd is %d\n", sd);

    printf("client:: constructing message\n");
    char message[BUFFER_SIZE];
    char identifier = '0';
    char pinum_string[num_digits(pinum) + 1];
    sprintf(pinum_string, "%d", pinum);

    message[0] = identifier;
    strcat(message, pinum_string);
    strcat(message, name);

    printf("client:: identifier is %c\n", identifier);
    printf("client:: pinum_string is %s\n", pinum_string);
    printf("client:: message is %s\n", message);

    while (1)
    {
        int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send (UDP_Write)\n");
            exit(1);
        }
        // return - 1;

        printf("client:: wait for reply...\n");
        rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d message:(%s)]\n", rc, message);
            
        if (rc != -1)
        {
            if (message[0] == '1')
            {
                printf("client:: reply recieved was a failure\n");
                return -1;
            }
            printf("client:: reading reply...\n");
            int i = 1;
            int count = 1;
            while (message[count] != '\0')
            { 
                count++;
            }
            char inum_string[count];
            inum_string[count - 1] = '\0';
            count = 0;
            while (message[i] != '\0')
            {
                inum_string[count] = message[i];
                i++;
                count++;
            }

            printf("client:: inum_string is %s\n", inum_string);
            return atoi(inum_string);
        }

        sleep(1);
    }
    return -1;
}
/*returns some information about the file specified by inum. Upon success, return 0, otherwise -1.
The exact info returned is defined by MFS_Stat_t.
Failure modes: inum does not exist. File and directory sizes are described below.*/
int MFS_Stat(int inum, MFS_Stat_t *m)
{
    if (inum < 0 || inum >= MFS_BLOCK_SIZE)
    {
        return -1;
    }
    printf("client:: starting MFS_Stat\n");

    printf("client:: sd is %d\n", sd);

    printf("client:: constructing message\n");
    char message[BUFFER_SIZE];
    char identifier = '1';
    char inum_string[num_digits(inum) + 1];
    sprintf(inum_string, "%d", inum);

    message[0] = identifier;
    strcat(message, inum_string);

    printf("client:: identifier is %c\n", identifier);
    printf("client:: inum_string is %s\n", inum_string);
    printf("client:: message is %s\n", message);

    while (1)
    {
        int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send (UDP_Write)\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)]\n", rc, message);

        if (rc != -1)
        {
            if (message[0] == '1')
            {
                printf("client:: reply recieved was a failure\n");
                return -1;
            }
            printf("client:: reading reply...\n");
            int count = 1;
            while (message[count] != '\0')
            {
                count++;
            }
            char type_string[count];
            type_string[count - 1] = '\0';
            int i = count + 1;
            count = 1;
            while (message[i] != '\0')
            {
                count++;
                i++;
            }
            char size_string[count];
            size_string[count - 1] = '\0';
            i = 1;
            count = 0;
            while (message[i] != '\0')
            {
                type_string[count] = message[i];
                i++;
                count++;
            }
            i++;
            count = 0;
            while (message[i] != '\0')
            {
                size_string[count] = message[i];
                i++;
                count++;
            }
            printf("client:: type_string is %s\n", type_string);
            printf("client:: size_string is %s\n", type_string);

            m->type = atoi(type_string);
            m->size = atoi(size_string);

            return 0;
        }

        sleep(1);
    }
    return -1;
}
/*writes a buffer of size nbytes (max size: 4096 bytes) at the byte offset specified by offset.
Returns 0 on success, -1 on failure.
Failure modes: invalid inum, invalid nbytes, invalid offset, not a regular file (because you can't write to directories).*/
int MFS_Write(int inum, char *buffer, int offset, int nbytes)
{
    if (inum < 0 || inum >= MFS_BLOCK_SIZE || nbytes < 0 || nbytes >= MFS_BLOCK_SIZE)
    {
        return -1;
    }
    printf("client:: starting MFS_Write\n");

    printf("client:: sd is %d\n", sd);

    printf("client:: constructing message\n");
    char message[BUFFER_SIZE];
    char identifier = '2';
    char inum_string[num_digits(inum) + 1];
    sprintf(inum_string, "%d", inum);
    char offset_string[num_digits(offset) + 1];
    sprintf(offset_string, "%d", offset);
    char nbytes_string[num_digits(nbytes) + 1];
    sprintf(nbytes_string, "%d", nbytes);

    message[0] = identifier;
    strcat(message, inum_string);
    strcat(message, buffer);
    strcat(message, offset_string);
    strcat(message, nbytes_string);

    printf("client:: identifier is %c\n", identifier);
    printf("client:: inum_string is %s\n", inum_string);
    printf("client:: buffer is %s\n", buffer);
    printf("client:: offset_string is %s\n", offset_string);
    printf("client:: nbytes_string is %s\n", nbytes_string);
    printf("client:: message is %s\n", message);

    while (1)
    {
        int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send (UDP_Write)\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)]\n", rc, message);

        if (rc != -1)
        {
            if (message[0] == '1')
            {
                printf("client:: reply recieved was a failure\n");
                return -1;
            }
            printf("client:: reply recieved was a success\n");
            return 0;
        }

        sleep(1);
    }

    return -1;
}
/*reads nbytes of data (max size 4096 bytes) specified by the byte offset offset into the buffer from file specified by inum.
The routine should work for either a file or directory; directories should return data in the format specified by MFS_DirEnt_t.
Success: 0, failure: -1. Failure modes: invalid inum, invalid offset, invalid nbytes.*/
int MFS_Read(int inum, char *buffer, int offset, int nbytes)
{
    if (inum < 0 || inum >= MFS_BLOCK_SIZE || nbytes < 0 || nbytes > MFS_BLOCK_SIZE)
    {
        return -1;
    }
    printf("client:: starting MFS_Read\n");

    printf("client:: sd is %d\n", sd);

    printf("client:: constructing message\n");
    char message[BUFFER_SIZE];
    char identifier = '3';
    char inum_string[num_digits(inum) + 1];
    sprintf(inum_string, "%d", inum);
    char offset_string[num_digits(offset) + 1];
    sprintf(offset_string, "%d", offset);
    char nbytes_string[num_digits(nbytes) + 1];
    sprintf(nbytes_string, "%d", nbytes);

    message[0] = identifier;
    strcat(message, inum_string);
    strcat(message, offset_string);
    strcat(message, nbytes_string);

    printf("client:: identifier is %c\n", identifier);
    printf("client:: inum_string is %s\n", inum_string);
    printf("client:: offset_string is %s\n", offset_string);
    printf("client:: nbytes_string is %s\n", nbytes_string);
    printf("client:: message is %s\n", message);


    while (1)
    {
        int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send (UDP_Write)\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)]\n", rc, message);

        if (rc != -1)
        {
            if (message[0] == '1')
            {
                printf("client:: reply recieved was a failure\n");
                return -1;
            }
            printf("client:: reading reply...\n");
            int i = 1;
            while (message[i] != '\0')
            {
                buffer[i - 1] = message[i];
                i++;
            }
            printf("client:: buffer is %s\n", buffer);
            return 0;
        }

        sleep(1);
    }

    return -1;
}
// makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) in the parent directory specified by pinum of name name.
// Returns 0 on success, -1 on failure. Failure modes: pinum does not exist, or name is too long.
// If name already exists, return success.
int MFS_Creat(int pinum, int type, char *name)
{
    printf("client:: starting MFS_Creat\n");
    if (strlen(name) > 28)
    {
        printf("client:: MFS_Creat failed, name %s too long\n", name);
        return -1;
    }
    printf("client:: name %s not too long\n", name);

    printf("client:: sd is %d\n", sd);

    printf("client:: constructing message\n");
    char message[BUFFER_SIZE];
    char identifier = '4';
    char pinum_string[num_digits(pinum) + 1];
    sprintf(pinum_string, "%d", pinum);
    char type_string[num_digits(type) + 1];
    sprintf(type_string, "%d", type);

    message[0] = identifier;
    strcat(message, pinum_string);
    strcat(message, type_string);
    strcat(message, name);

    printf("client:: identifier is %c\n", identifier);
    printf("client:: pinum_string is %s\n", pinum_string);
    printf("client:: type_string is %s\n", type_string);
    printf("client:: message is %s\n", message);

    while (1)
    {
        int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send (UDP_Write)\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)]\n", rc, message);

        if (rc != -1)
        {
            if (message[0] == '1')
            {
                printf("client:: reply recieved was a failure\n");
                return -1;
            }
            return 0;
        }

        sleep(1);
    }

    return -1;
}
/*removes the file or directory name from the directory specified by pinum.
0 on success, -1 on failure. Failure modes: pinum does not exist, directory is NOT empty.
Note that the name not existing is NOT a failure by our definition (think about why this might be).*/
int MFS_Unlink(int pinum, char *name)
{
    printf("client:: starting MFS_Unlink\n");

    printf("client:: sd is %d\n", sd);

    printf("client:: constructing message\n");
    char message[BUFFER_SIZE];
    char identifier = '5';
    char pinum_string[num_digits(pinum) + 1];
    sprintf(pinum_string, "%d", pinum);

    message[0] = identifier;
    strcat(message, pinum_string);
    strcat(message, name);

    printf("client:: identifier is %c\n", identifier);
    printf("client:: pinum_string is %s\n", pinum_string);
    printf("client:: name is %s\n", name);
    printf("client:: message is %s\n", message);

    while (1)
    {
        int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send (UDP_Write)\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)]\n", rc, message);

        if (rc != -1)
        {
            if (message[0] == '1')
            {
                printf("client:: reply recieved was a failure\n");
                return -1;
            }
            printf("client:: reply recieved was a success\n");
            return 0;
        }

        sleep(1);
    }
    return -1;
}
// just tells the server to force all of its data structures to disk and shutdown by calling exit(0).
// This interface will mostly be used for testing purposes.
int MFS_Shutdown()
{
    printf("client:: starting MFS_Shutdown\n");

    printf("client:: sd is %d\n", sd);

    printf("client:: constructing message\n");
    char message[BUFFER_SIZE];
    char identifier = '6';

    message[0] = identifier;

    printf("client:: identifier is %c\n", identifier);
    printf("client:: message is %s\n", message);
    while (1)
    {
        int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0)
        {
            printf("client:: failed to send (UDP_Write)\n");
            exit(1);
        }
        // timer needed in order to send another write if reply is not recieved in time
        printf("client:: wait for reply...\n");
        rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
        printf("client:: got reply [size:%d contents:(%s)]\n", rc, message);

        if (rc != -1)
        {
            if (message[0] == '1')
            {
                printf("client:: reply recieved was a failure\n");
                return -1;
            }
                printf("client:: reply recieved was a success, finishing shutdown\n");
            UDP_Close(sd);
            return 0;
        }
        sleep(1);
    }
    return 0;
}
