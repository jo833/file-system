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

void mfs_lookup(char *message, super_t *super, struct sockaddr_in addr)
{
    printf("server:: starting mfs_lookup\n");
    char reply[BUFFER_SIZE];
    int i, pinum, inum, count;
    char failure = '0';

    printf("server:: beginning to read message\n");
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
    count = 0;
    while (message[i] != '\0')
    {
        pinum_string[count] = message[i];
        i++;
        count++;
    }
    pinum = atoi(pinum_string);
    i++;
    count = 0;
    while (message[i] != '\0')
    {
        name[count] = message[i];
        i++;
        count++;
    }

    printf("server:: finished reading message\n");

    printf("server:: pinum is %d\n", pinum);
    printf("server:: name is %s\n", name);

    printf("server:: about to call get_bit to check if pinum is valid\n");
    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), pinum) != 1)
    {
        printf("server:: pinum is not valid, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: pinum is valid, finding pinum in inode table\n");

    inode_t *pinode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (pinum * (sizeof(inode_t))));

    printf("server:: checking if pinode is a directory\n");
    if (pinode->type != MFS_DIRECTORY)
    {
        printf("server:: pinum is not a directory, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: pinode is a directory\n");

    printf("server:: looping through direct pointers to find name\n");
    for (i = 0; i < DIRECT_PTRS; i++)
    {
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        {
            MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j);
            if (dir_entry->inum != -1 && strcmp(dir_entry->name, name) == 0)
            {
                printf("server:: found directory name, storing inum\n");
                inum = dir_entry->inum;
                break;
            }
        }
        if (i == DIRECT_PTRS - 1)
        {
            printf("server:: unable to find directory name, replying with failure\n");
            char failure = '1';

            reply[0] = failure;
            UDP_Write(sd, &addr, reply, BUFFER_SIZE);
            return;
        }
    }

    if (failure == '0')
    {
        printf("server:: replying with success, and send inum to client\n");
        char inum_string[num_digits(inum) + 1];
        sprintf(inum_string, "%d", inum);
        reply[0] = failure;
        strcat(reply, inum_string);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
}

void mfs_stat(char *message, super_t *super, struct sockaddr_in addr)
{
    printf("server:: starting mfs_stat\n");
    char reply[BUFFER_SIZE];
    int i, inum, count;
    char failure = '0';

    printf("server:: beginning to read message\n");
    count = 1;
    while (message[count] != '\0')
    {
        count++;
    }
    char inum_string[count];
    inum_string[count - 1] = '\0';

    i = 1;
    count = 0;
    while (message[i] != '\0')
    {
        inum_string[count] = message[i];
        i++;
        count++;
    }
    inum = atoi(inum_string);
    printf("server:: finished reading message\n");

    printf("server:: inum is %d\n", inum);

    printf("server:: about to call get_bit to check if inum is valid\n");
    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), inum) != 1)
    {
        printf("server:: inum is not valid, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: inum is valid, finding pinum in inode table\n");

    inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (inum * (sizeof(inode_t))));

    if (failure == '0')
    {
        printf("server:: replying with success, and sending inode type and size to client\n");
        char inode_type_string[num_digits(inode->type) + 1];
        char inode_size_string[num_digits(inode->size) + 1];
        sprintf(inode_type_string, "%d", inode->type);
        sprintf(inode_size_string, "%d", inode->size);
        reply[0] = failure;
        strcat(reply, inode_type_string);
        strcat(reply, inode_size_string);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
    }
}

void mfs_write(char *message, super_t *super, struct sockaddr_in addr, int fd)
{
    printf("server:: starting mfs_write\n");
    char reply[BUFFER_SIZE];
    int i, inum, offset, nbytes, count;
    char failure = '0';

    printf("server:: beginning to read message\n");
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
    count = 0;
    while (message[i] != '\0')
    {
        inum_string[count] = message[i];
        i++;
        count++;
    }
    inum = atoi(inum_string);
    i++;
    count = 0;
    while (message[i] != '\0')
    {
        buffer[count] = message[i];
        i++;
        count++;
    }
    i++;
    count = 0;
    while (message[i] != '\0')
    {
        offset_string[count] = message[i];
        i++;
        count++;
    }
    offset = atoi(offset_string);
    i++;
    count = 0;
    while (message[i] != '\0')
    {
        nbytes_string[count] = message[i];
        i++;
        count++;
    }
    nbytes = atoi(nbytes_string);
    printf("server:: finished reading message\n");

    printf("server:: inum is %d\n", inum);
    printf("server:: offset is %d\n", offset);
    printf("server:: nbytes is %d\n", nbytes);
    printf("server:: buffer is %s\n", buffer);

    printf("server:: about to call get_bit to check if inum is valid\n");
    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), inum) != 1)
    {
        printf("server:: inum is not valid, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: inum is valid, finding pinum in inode table\n");

    inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (inum * (sizeof(inode_t))));

    printf("server:: checking if inode is a regular file\n");
    if (inode->type == MFS_DIRECTORY)
    {
        printf("server:: inum is not a regular file, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: inode is a regular file\n");

    if (inode->size != 0)
    {
        printf("server:: inode is not empty, calculating where in file to write\n");
        int direct_pointer_index = offset / UFS_BLOCK_SIZE;
        int block_pointer_offset = offset % UFS_BLOCK_SIZE;
        char *file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (inode->direct[direct_pointer_index] * (UFS_BLOCK_SIZE)) + block_pointer_offset;
        i = 0;
        printf("server:: writing to file one character at a time\n");
        while (i < nbytes)
        {
            *file_pointer = buffer[i];
            i++;
            if (block_pointer_offset + i == UFS_BLOCK_SIZE)
            {
                printf("server:: reached end of data block before finished writing\n");
                if (inode->size > direct_pointer_index * UFS_BLOCK_SIZE)
                {
                    printf("server:: next direct pointer is allocated, so moving pointer to new data block and continue write\n");
                    file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (inode->direct[direct_pointer_index + 1] * (UFS_BLOCK_SIZE));
                }
                else
                {
                    if (direct_pointer_index == DIRECT_PTRS)
                    {
                        printf("server:: no more space left in the inode, replying with failure\n");
                        char failure = '1';

                        reply[0] = failure;
                        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                        return;
                    }
                    printf("server:: next direct pointer is not allocated, so finding unallocated data block in data bitmap\n");
                    char *data_bitmap_pointer = (pointer + (super->data_bitmap_addr * (UFS_BLOCK_SIZE)));
                    i = 0;
                    while (get_bit((unsigned int *)data_bitmap_pointer, i) == 1)
                    {
                        i++;
                    }
                    printf("server:: found unallocated data block, setting direct pointer to new data block and setting bit to allocated\n");
                    set_bit((unsigned int *)data_bitmap_pointer, i);
                    file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (i * (UFS_BLOCK_SIZE));
                    inode->direct[direct_pointer_index + 1] = i;
                }
                block_pointer_offset = 0;
            }
            else
            {
                file_pointer++;
            }
        }
    }
    else
    {
        printf("server:: inode is empty, must point direct pointer to unallocated data block\n");
        if (offset + nbytes > UFS_BLOCK_SIZE)
        {
            printf("server:: invalid offset/nbytes for single data block, replying with failure\n");
            char failure = '1';

            reply[0] = failure;
            UDP_Write(sd, &addr, reply, BUFFER_SIZE);
            return;
        }
        char *data_bitmap_pointer = (pointer + (super->data_bitmap_addr * (UFS_BLOCK_SIZE)));
        i = 0;
        while (get_bit((unsigned int *)data_bitmap_pointer, i) == 1)
        {
            i++;
        }
        printf("server:: found unallocated data block, setting direct pointer to new data block and setting bit to allocated\n");
        set_bit((unsigned int *)data_bitmap_pointer, i);
        char *file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (i * (UFS_BLOCK_SIZE)) + offset;
        inode->direct[0] = i;
        i = 0;
        printf("server:: writing to file one character at a time\n");
        while (i < nbytes)
        {
            *file_pointer = buffer[i];
            i++;
            file_pointer++;
        }
    }

    if (failure == '0')
    {
        printf("server:: replying with succes and calling fsync\n");
        fsync(fd);

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
    return;
}

void mfs_read(char *message, super_t *super, struct sockaddr_in addr)
{
    printf("server:: starting mfs_read\n");
    char reply[BUFFER_SIZE];
    int i, inum, offset, nbytes, count;
    char failure = '0';

    printf("server:: beginning to read message\n");
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
    count = 0;
    while (message[i] != '\0')
    {
        inum_string[count] = message[i];
        i++;
        count++;
    }
    inum = atoi(inum_string);
    i++;
    count = 0;
    while (message[i] != '\0')
    {
        offset_string[count] = message[i];
        i++;
        count++;
    }
    offset = atoi(offset_string);
    i++;
    count = 0;
    while (message[i] != '\0')
    {
        nbytes_string[count] = message[i];
        i++;
        count++;
    }
    nbytes = atoi(nbytes_string);
    printf("server:: finished reading message\n");

    printf("server:: inum is %d\n", inum);
    printf("server:: offset is %d\n", offset);
    printf("server:: nbytes is %d\n", nbytes);

    printf("server:: about to call get_bit to check if inum is valid\n");
    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), inum) != 1)
    {
        printf("server:: inum is not valid, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: inum is valid, finding pinum in inode table\n");

    inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (inum * (sizeof(inode_t))));

    printf("server:: checking if inode is a regular file\n");
    if (inode->type == MFS_DIRECTORY)
    {
        printf("server:: inum is not a regular file, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: inode is a regular file\n");

    if (inode->size < offset + nbytes)
    {
        printf("server:: invalid offset/nbytes, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }

    printf("server:: calculating where in file to read\n");
    int direct_pointer_index = offset / UFS_BLOCK_SIZE;
    int block_pointer_offset = offset % UFS_BLOCK_SIZE;
    char *file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (inode->direct[direct_pointer_index] * (UFS_BLOCK_SIZE)) + block_pointer_offset;
    i = 0;
    printf("server:: reading one character at a time\n");
    while (i < nbytes)
    {
        reply[i + 1] = *file_pointer;
        i++;
        printf("server:: reached end of data block before finished reading\n");
        if (block_pointer_offset + i == UFS_BLOCK_SIZE)
        {
            if (inode->size > direct_pointer_index * UFS_BLOCK_SIZE)
            {
                printf("server:: next direct pointer is allocated, update file pointer adn continue reading\n");
                file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (inode->direct[direct_pointer_index + 1] * (UFS_BLOCK_SIZE));
            }
            else
            {
                printf("server:: next direct pointer is not allocated, replying with failure\n");
                char failure = '1';

                reply[0] = failure;
                UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                return;
            }
            block_pointer_offset = 0;
        }
        else
        {
            file_pointer++;
        }
    }

    if (failure == '0')
    {
        printf("server:: replying with success and sending read to client\n");
        reply[0] = failure;

        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
    return;
}

void mfs_create(char *message, super_t *super, struct sockaddr_in addr, int fd)
{
    printf("server:: starting mfs_create\n");
    char reply[BUFFER_SIZE];
    int i, pinum, type, count;
    char failure = '0';

    printf("server:: beginning to read message\n");
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
    char type_string[count];
    type_string[count - 1] = '\0';
    i++;
    count = 1;
    while (message[i] != '\0')
    {
        count++;
        i++;
    }
    char name[count];
    name[count - 1] = '\0';
    i = 1;
    count = 0;
    while (message[i] != '\0')
    {
        pinum_string[count] = message[i];
        i++;
        count++;
    }
    pinum = atoi(pinum_string);
    i++;
    count = 0;
    while (message[i] != '\0')
    {
        type_string[count] = message[i];
        i++;
        count++;
    }
    type = atoi(type_string);
    i++;
    count = 0;
    while (message[i] != '\0')
    {
        name[count] = message[i];
        i++;
        count++;
    }

    printf("server:: finished reading message\n");

    printf("server:: pinum is %d\n", pinum);
    printf("server:: type is %d\n", type);
    printf("server:: name is %s\n", name);

    printf("server:: about to call get_bit to check if pinum is valid\n");
    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), pinum) != 1)
    {
        printf("server:: pinum is not valid, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: pinum is valid, finding pinum in inode table\n");

    inode_t *pinode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (pinum * (sizeof(inode_t))));

    printf("server:: checking if pinode is a directory\n");
    if (pinode->type != MFS_DIRECTORY)
    {
        printf("server:: pinum is not a directory, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: pinode is a directory\n");

    printf("server:: looping through direct pointers to see if name already exists\n");
    for (i = 0; i < DIRECT_PTRS; i++)
    {
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        {
            MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j);
            if (dir_entry->inum != -1 && strcmp(dir_entry->name, name) == 0)
            {
                printf("server:: found that name already exists, replying with failure\n");
                reply[0] = failure;
                UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                return;
            }
        }
    }

    printf("server:: found that name does not yet exist, looping through direct pointers to find unallocated directory entry\n");
    for (i = 0; i < DIRECT_PTRS; i++)
    {
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        {
            MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j);
            if (dir_entry->inum == -1)
            {
                printf("server:: found unallocated directory entry, now finding free inode in inode bitmap\n");
                char *inode_bitmap_pointer = (pointer + (super->inode_bitmap_addr * (UFS_BLOCK_SIZE)));
                i = 0;
                while (get_bit((unsigned int *)inode_bitmap_pointer, i) == 1)
                {
                    i++;
                }
                printf("server:: found unallocated inode, now setting directory entry inum and name, as well as setting the bit to allocated\n");
                set_bit((unsigned int *)inode_bitmap_pointer, i);
                dir_entry->inum = i;
                strcpy(dir_entry->name, name);
            }
        }
    }

    inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (i * (sizeof(inode_t))));
    printf("server:: setting inode type to %d\n", type);
    inode->type = type;

    if (type == MFS_REGULAR_FILE)
    {
        printf("server:: type was regular, so inode size set to 0\n");
        inode->size = 0;
    }
    else
    {
        printf("server:: type was directory, so inode size set to 2, now finding unallocated data block in data bitmap to add . and ..\n");
        inode->size = 2;

        char *data_bitmap_pointer = (pointer + (super->data_bitmap_addr * (UFS_BLOCK_SIZE)));
        i = 0;
        while (get_bit((unsigned int *)data_bitmap_pointer, i) == 1)
        {
            i++;
        }
        printf("server:: found unallocated data block, now setting directory entries for . and .., as well as setting the bit in the data bitmap\n");
        set_bit((unsigned int *)data_bitmap_pointer, i);
        inode->direct[0] = i;
        MFS_DirEnt_t *current_directory_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[0] * UFS_BLOCK_SIZE));
        MFS_DirEnt_t *parent_directory_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[0] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t));

        strcpy(current_directory_entry->name, ".");
        strcpy(parent_directory_entry->name, "..");

        current_directory_entry->inum = i;
        parent_directory_entry->inum = pinum;
    }
    if (failure == '0')
    {
        printf("server:: replying with success and calling fsync\n");
        fsync(fd);
        reply[0] = failure;

        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    return;
}

void mfs_unlink(char *message, super_t *super, struct sockaddr_in addr, int fd)
{
    printf("server:: starting mfs_unlink\n");
    char reply[BUFFER_SIZE];
    int i, pinum, count;
    char failure = '0';

    printf("server:: beginning to read message\n");
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

    printf("server:: finished reading message\n");

    printf("server:: pinum is %d\n", pinum);
    printf("server:: name is %s\n", name);

    printf("server:: about to call get_bit to check if pinum is valid\n");
    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), pinum) != 1)
    {
        printf("server:: pinum is not valid, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: pinum is valid, finding pinum in inode table\n");

    inode_t *pinode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (pinum * (sizeof(inode_t))));

    printf("server:: checking if pinode is a directory\n");
    if (pinode->type != MFS_DIRECTORY)
    {
        printf("server:: pinum is not a directory, replying with failure\n");
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        return;
    }
    printf("server:: pinode is a directory\n");

    printf("server:: looping through direct pointers to find name to unlink\n");
    for (i = 0; i < DIRECT_PTRS; i++)
    {
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        {
            MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j);
            if (dir_entry->inum != -1 && strcmp(dir_entry->name, name) == 0)
            {
                printf("server:: found name to unlink\n");
                inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (dir_entry->inum * (sizeof(inode_t))));
                if (inode->type == MFS_DIRECTORY && inode->size != 2)
                {
                    printf("server:: name is a nonempty directory, replying with failure\n");
                    char failure = '1';

                    reply[0] = failure;
                    UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                    return;
                }
                printf("server:: deallocating name\n");
                char *inode_bitmap_pointer = (pointer + (super->inode_bitmap_addr * (UFS_BLOCK_SIZE)));
                set_bit((unsigned int *)inode_bitmap_pointer, dir_entry->inum);
                dir_entry->inum = -1;
            }
            if (i == DIRECT_PTRS - 1)
            {
                printf("server:: name not found, replying with failure\n");
                char failure = '1';

                reply[0] = failure;
                UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                return;
            }
        }
    }

    if (failure == '0')
    {
        printf("server:: replying with success and calling fsync\n");
        fsync(fd);

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
    return;
}

void mfs_shutdown(struct sockaddr_in addr, char *pointer, size_t filesize, int fd)
{
    printf("server:: starting mfs_shutdown\n");
    char reply[BUFFER_SIZE];
    printf("server:: calling msync and fsync\n");
    msync(pointer, filesize, MS_SYNC);
    fsync(fd);

    printf("server:: replying with success\n");
    reply[0] = '0';
    UDP_Write(sd, &addr, reply, BUFFER_SIZE);
    exit(0);
    return;
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
    pointer = mmap(NULL, filesize, PROT_WRITE, MAP_SHARED, fileno(fd), 0);
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
            mfs_lookup(message, super, addr);
            break;
        case '1':
            mfs_stat(message, super, addr);
            break;
        case '2':
            mfs_write(message, super, addr, sd);
            break;
        case '3':
            mfs_read(message, super, addr);
            break;
        case '4':
            mfs_create(message, super, addr, sd);
            break;
        case '5':
            mfs_unlink(message, super, addr, sd);
            break;
        case '6':
            mfs_shutdown(addr, pointer, filesize, sd);
            break;
        default:
            printf("invalid identifier");
            break;
        }
    }
    return 0;
}
