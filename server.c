#include <stdio.h>
#include "mfs.h"
#include "ufs.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
 #include <sys/types.h>
 #include <unistd.h>
 #include "udp.h"


#define BUFFER_SIZE (1000)
int sd;
char *pointer;
void intHandler(int dummy)
{
    UDP_Close(sd);
    exit(130);
}

// server code
int main(int argc, char *argv[])
{
    
    signal(SIGINT, intHandler);
    int port = argv[1];
    FILE *fd = open(argv[2], "r+");
    fseek(fd, 0L, SEEK_END);
    size_t filesize = ftell(fd);
    if (filesize == 0) {
        exit(0);
    }
    fseek(fd, 0L, SEEK_SET);
    pointer = mmap(NULL, filesize, PROT_WRITE, MAP_PRIVATE, fileno(fd), 0);
    super_t* super = (super_t*)pointer;
    
    sd = UDP_Open(port);
    assert(sd > -1);
    while (1)
    {
        struct sockaddr_in addr;
        char message[BUFFER_SIZE];
        char reply[BUFFER_SIZE];
        printf("server:: waiting...\n");
        int rc = UDP_Read(sd, &addr, message, BUFFER_SIZE);
        printf("server:: read message [size:%d contents:(%s)]\n", rc, message);
        char identifier = message[0];
        switch (identifier)
        {
        case '0':
            char *pinum_string;
            char *name;
            int i, pinum;
            i = 0;
            while (message[i] != '\0')
            {
                i++;
                strcat(pinum_string, message[i]);
            }
            pinum = atoi(pinum_string);
            while (message[i] != '\0')
            {
                i++;
                strcat(name, message[i]);
            }

            inode_t* pinode = (inode_t*)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (pinum * (sizeof(inode_t))));

            if (pinode->type != MFS_DIRECTORY){
                exit(0);
            }

            for (i = 0; i < DIRECT_PTRS; i++){
                MFS_DirEnt_t* dir_entry = (MFS_DirEnt_t*)pinode->direct[i];
                if (strcmp(dir_entry->name, name) == 0){
                    break;
                }
                if (i == DIRECT_PTRS - 1){
                    i++;
                }
            }

            if (i == DIRECT_PTRS){
                char *pinum_string;
                itoa(pinum, pinum_string, 10);

                strcat(message, );
                
            } else {

            }
            break;
        case '1':
            char *inum_string;
            int i, inum;
            i = 0;
            while (message[i] != '\0')
            {
                i++;
                strc(inum_string, message[i]);
            }
            inum = atoi(inum_string);
            break;
        case '2':
            char *inum_string;
            char *buffer;
            char *offset_string;
            char *nbytes_string;
            int i, inum, offset, nbytes;
            i = 0;
            while (message[i] != '\0')
            {
                i++;
                strcat(inum_string, message[i]);
            }
            inum = atoi(inum_string);
            while (message[i] != '\0')
            {
                i++;
                strcat(buffer, message[i]);
            }
            while (message[i] != '\0')
            {
                i++;
                strcat(offset_string, message[i]);
            }
            offset = atoi(inum_string);
            while (message[i] != '\0')
            {
                i++;
                strcat(nbytes_string, message[i]);
            }
            nbytes = atoi(inum_string);
            break;
        case '3':
            char *inum_string;
            char *offset_string;
            char *nbytes_string;
            int i, inum, offset, nbytes;
            i = 0;
            while (message[i] != '\0')
            {
                i++;
                strcat(inum_string, message[i]);
            }
            inum = atoi(inum_string);
            while (message[i] != '\0')
            {
                i++;
                strcat(offset_string, message[i]);
            }
            offset = atoi(inum_string);
            while (message[i] != '\0')
            {
                i++;
                strcat(nbytes_string, message[i]);
            }
            nbytes = atoi(inum_string);
            break;
        case '4':
            char *pinum_string;
            char *type_string;
            char *name;
            int i, pinum, type;
            i = 0;
            while (message[i] != '\0')
            {
                i++;
                strcat(pinum_string, message[i]);
            }
            pinum = atoi(pinum_string);
            while (message[i] != '\0')
            {
                i++;
                strcat(type_string, message[i]);
            }
            type = atoi(type_string);
            while (message[i] != '\0')
            {
                i++;
                strcat(name, message[i]);
            }
            break;
        case '5':
            char *pinum_string;
            char *name;
            int i, pinum;
            i = 0;
            while (message[i] != '\0')
            {
                i++;
                strcat(pinum_string, message[i]);
            }
            int pinum = atoi(pinum_string);
            while (message[i] != '\0')
            {
                i++;
                strcat(name, message[i]);
            }
            break;
        default:
            printf("invalid identifier");
            break;
        }
        if (rc > 0)
        {
            char reply[BUFFER_SIZE];
            sprintf(reply, "goodbye world");
            rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
            printf("server:: reply\n");
        }
    }
    return 0;
}

unsigned int get_bit(unsigned int *bitmap, int position)
{
    int index = position / 32;
    int offset = 31 - (position % 32);
    return (bitmap[index] >> offset) & 0x1;
}

void set_bit(unsigned int *bitmap, int position)
{
    int index = position / 32;
    int offset = 31 - (position % 32);
    bitmap[index] |= 0x1 << offset;
}
