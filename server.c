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

void lookup(char *message, super_t *super, struct sockaddr_in addr)
{
    char reply[BUFFER_SIZE];
    int i, pinum, inum, count;
    char failure = '0';
    count = 1;
    while (message[count] != '\0')
    {
        count++;
    }
    char pinum_string[count];
    pinum_string[count - 1] = '\0';
    i = count + 1;
    count = 1;
    while (message[i] != '\0')
    {
        count++;
        i++;
    }
    char name[count];
    name[count - 1] = '\0';
    i = 1;
    while (message[i] != '\0')
    {
        strcat(pinum_string, &message[i]);
        i++;
    }
    pinum = atoi(pinum_string);
    i++;
    while (message[i] != '\0')
    {
        strcat(name, &message[i]);
        i++;
    }

    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), pinum) != 1)
    {
        char failure = '1';

        strcat(reply, &failure);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    inode_t *pinode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (pinum * (sizeof(inode_t))));

    if (pinode->type != MFS_DIRECTORY)
    {
        char failure = '1';

        strcat(reply, &failure);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    for (i = 0; i < DIRECT_PTRS; i++)
    {
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        {
            MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)((pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j);
            if (strcmp(dir_entry->name, name) == 0)
            {
                inum = dir_entry->inum;
            }
        }
        if (i == DIRECT_PTRS - 1)
        {
            char failure = '1';

            strcat(reply, &failure);
            UDP_Write(sd, &addr, reply, BUFFER_SIZE);
            printf("server:: reply\n");
            return;
        }
    }

    if (failure == '0')
    {
        char inum_string[num_digits(inum) + 1];
        sprintf(inum_string, "%d", inum);
        strcat(reply, &failure);
        strcat(reply, inum_string);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
}

void stat(char *message, super_t *super, struct sockaddr_in addr)
{
    char reply[BUFFER_SIZE];
    int i, inum, count;
    char failure = '0';

    count = 1;
    while (message[count] != '\0')
    {
        count++;
    }
    char inum_string[count];
    inum_string[count - 1] = '\0';

    i = 1;
    while (message[i] != '\0')
    {
        strcat(inum_string, &message[i]);
        i++;
    }
    inum = atoi(inum_string);

    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), inum) != 1)
    {
        char failure = '1';

        strcat(reply, &failure);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (inum * (sizeof(inode_t))));

    if (failure == '0')
    {
        char inode_type_string[num_digits(inode->type) + 1];
        char inode_size_string[num_digits(inode->size) + 1];
        sprintf(inode_type_string, "%d", inode->type);
        sprintf(inode_size_string, "%d", inode->size);
        strcat(reply, &failure);
        strcat(reply, inode_type_string);
        strcat(reply, inode_size_string);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
    }
}

void write(char *message, super_t *super, struct sockaddr_in addr)
{
    char reply[BUFFER_SIZE];
    int i, inum, offset, nbytes, count;
    char failure = '0';
    count = 1;
    while (message[count] != '\0')
    {
        count++;
    }
    char inum_string[count];
    inum_string[count - 1] = '\0';
    i = count + 1;
    count = 1;
    while (message[i] != '\0')
    {
        count++;
        i++;
    }
    char buffer[count];
    buffer[count - 1] = '\0';
    i++;
    count = 1;
    while (message[i] != '\0')
    {
        count++;
        i++;
    }
    char offset_string[count];
    offset_string[count - 1] = '\0';
    i++;
    count = 1;
    while (message[i] != '\0')
    {
        count++;
        i++;
    }
    char nbytes_string[count];
    nbytes_string[count - 1] = '\0';
    i = 1;
    while (message[i] != '\0')
    {
        strcat(inum_string, &message[i]);
        i++;
    }
    inum = atoi(inum_string);
    i++;
    while (message[i] != '\0')
    {
        strcat(buffer, &message[i]);
        i++;
    }
    i++;
    while (message[i] != '\0')
    {
        strcat(offset_string, &message[i]);
        i++;
    }
    offset = atoi(offset_string);
    i++;
    while (message[i] != '\0')
    {
        strcat(nbytes_string, &message[i]);
        i++;
    }
    nbytes = atoi(nbytes_string);

    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), inum) != 1)
    {
        char failure = '1';

        strcat(reply, &failure);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (inum * (sizeof(inode_t))));

    if (inode->type == MFS_DIRECTORY)
    {
        char failure = '1';

        strcat(reply, &failure);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    if (inode->size != 0)
    {
        int direct_pointer_index = offset / UFS_BLOCK_SIZE;
        int block_pointer_offset = offset % UFS_BLOCK_SIZE;
        char *file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (inode->direct[direct_pointer_index] * (UFS_BLOCK_SIZE)) + block_pointer_offset;
        i = 0;
        while (i < nbytes)
        {
            file_pointer = buffer[i];
            i++;
            if (block_pointer_offset + i == UFS_BLOCK_SIZE)
            {
                if (inode->size > direct_pointer_index * UFS_BLOCK_SIZE)
                {
                    file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (inode->direct[direct_pointer_index + 1] * (UFS_BLOCK_SIZE));
                }
                else
                {
                    if (direct_pointer_index == DIRECT_PTRS)
                    {
                        char failure = '1';

                        strcat(reply, &failure);
                        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                        printf("server:: reply\n");
                        return;
                    }
                    char *data_bitmap_pointer = (pointer + (super->data_bitmap_addr * (UFS_BLOCK_SIZE)));
                    i = 0;
                    while (data_bitmap_pointer == '1')
                    {
                        i++;
                        data_bitmap_pointer++;
                    }
                    data_bitmap_pointer = '1';
                    file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (i * (UFS_BLOCK_SIZE));
                    inode->direct[direct_pointer_index + 1] = i;
                }
            }
            else
            {
                file_pointer++;
            }
        }
    }
    else
    {
        char *data_bitmap_pointer = (pointer + (super->data_bitmap_addr * (UFS_BLOCK_SIZE)));
        i = 0;
        while (data_bitmap_pointer == '1')
        {
            i++;
            data_bitmap_pointer++;
        }
        data_bitmap_pointer = '1';
        char *file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (i * (UFS_BLOCK_SIZE));
        inode->direct[0] = i;
        i = 0;
        while (i < nbytes)
        {
            file_pointer = buffer[i];
            i++;
            file_pointer++;
        }
    }

    if (failure == '0')
    {
        strcat(reply, &failure);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
    return -1;
}

void intHandler(int dummy)
{
    UDP_Close(sd);
    exit(130);
}

// server code
int main(int argc, char *argv[])
{

    signal(SIGINT, intHandler);
    int port;
    port = strtol(argv[1], NULL, 10);
    FILE *fd;
    fd = fopen(argv[2], "r+");
    fseek(fd, 0L, SEEK_END);
    size_t filesize = ftell(fd);
    if (filesize == 0)
    {
        exit(0);
    }
    fseek(fd, 0L, SEEK_SET);
    pointer = mmap(NULL, filesize, PROT_WRITE, MAP_PRIVATE, fileno(fd), 0);
    super_t *super = (super_t *)pointer;

    sd = UDP_Open(port);
    assert(sd > -1);
    while (1)
    {
        struct sockaddr_in addr;
        char message[BUFFER_SIZE];

        printf("server:: waiting...\n");
        int rc = UDP_Read(sd, &addr, message, BUFFER_SIZE);
        printf("server:: read message [size:%d contents:(%s)]\n", rc, message);
        char identifier = message[0];
        switch (identifier)
        {
        case '0':
            lookup(message, super, addr);
            break;
        case '1':
            stat(message, super, addr);
            break;
        // case '2':
        //     i = 0;
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(inum_string, &message[i]);
        //     }
        //     inum = atoi(inum_string);

        //     inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (inum * (sizeof(inode_t))));

        //     if (inode->type == MFS_DIRECTORY)
        //     {
        //         char failure = '1';

        //         strcat(reply, &failure);
        //         rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        //         printf("server:: reply\n");
        //         break;
        //     }

        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(buffer, &message[i]);
        //     }
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(offset_string, &message[i]);
        //     }
        //     offset = atoi(inum_string);
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(nbytes_string, &message[i]);
        //     }
        //     nbytes = atoi(inum_string);
        //     offset = nbytes;
        //     nbytes = offset;
        //     break;
        // case '3':
        //     i = 0;
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(inum_string, &message[i]);
        //     }
        //     inum = atoi(inum_string);
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(offset_string, &message[i]);
        //     }
        //     offset = atoi(inum_string);
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(nbytes_string, &message[i]);
        //     }
        //     nbytes = atoi(inum_string);
        //     offset = nbytes;
        //     nbytes = offset;
        //     break;
        // case '4':
        //     i = 0;
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(pinum_string, &message[i]);
        //     }
        //     pinum = atoi(pinum_string);

        //     if (get_bit((pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), pinum) != 1)
        //     {
        //         char failure = '1';

        //         strcat(reply, failure);
        //         rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        //         printf("server:: reply\n");
        //         break;
        //     }
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(type_string, &message[i]);
        //     }
        //     type = atoi(type_string);
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(name, &message[i]);
        //     }

        //     inode_t *pinode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (pinum * (sizeof(inode_t))));

        //     if (pinode->type != MFS_DIRECTORY)
        //     {
        //         char failure = '1';

        //         strcat(reply, &failure);
        //         rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        //         printf("server:: reply\n");
        //         break;
        //     }

        //     for (i = 0; i < DIRECT_PTRS; i++)
        //     {
        //         for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        //         {
        //             MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)(pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j;
        //             if (get_bit((pointer + super->data_bitmap_addr * (UFS_BLOCK_SIZE)), '?') != 1)
        //             {
        //                 set_bit((pointer + super->data_bitmap_addr * (UFS_BLOCK_SIZE)), '?');
        //                 dir_entry->name = buffer;
        //                 dir_entry->inum = '?';
        //             }
        //         }
        //         if (i == DIRECT_PTRS - 1)
        //         {
        //             char failure = '1';

        //             strcat(reply, &failure);
        //             rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        //             printf("server:: reply\n");
        //         }
        //     }
        //     break;
        // case '5':
        //     i = 0;
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(pinum_string, &message[i]);
        //     }
        //     pinum = atoi(pinum_string);
        //     while (message[i] != '\0')
        //     {
        //         i++;
        //         strcat(name, &message[i]);
        //     }
        //     break;
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
