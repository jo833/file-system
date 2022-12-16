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

    // TODO: print all these variables.

    // if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), pinum) != 1)
    // {
    //     char failure = '1';

    //     reply[0] = failure;
    //     UDP_Write(sd, &addr, reply, BUFFER_SIZE);
    //     printf("server:: reply\n");
    //     return;
    // }

    inode_t *pinode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (pinum * (sizeof(inode_t))));

    if (pinode->type != MFS_DIRECTORY)
    {
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
    printf("Checked that directory");

    for (i = 0; i < DIRECT_PTRS; i++)
    {
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        {
            MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j);
            
            if (dir_entry->inum != -1 && strcmp(dir_entry->name, name) == 0)
            {
                inum = dir_entry->inum;
            }
        }
        if (i == DIRECT_PTRS - 1)
        {
            char failure = '1';

            reply[0] = failure;
            UDP_Write(sd, &addr, reply, BUFFER_SIZE);
            printf("server:: reply\n");
            return;
        }
    }

    if (failure == '0')
    {
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
    count = 0;
    while (message[i] != '\0')
    {
        inum_string[count] = message[i];
        i++;
        count++;
    }
    inum = atoi(inum_string);

    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), inum) != 1)
    {
        char failure = '1';

        reply[0] = failure;
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
        reply[0] = failure;
        strcat(reply, inode_type_string);
        strcat(reply, inode_size_string);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
    }
}

void mfs_write(char *message, super_t *super, struct sockaddr_in addr, int fd)
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

    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), inum) != 1)
    {
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (inum * (sizeof(inode_t))));

    if (inode->type == MFS_DIRECTORY)
    {
        char failure = '1';

        reply[0] = failure;
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
            *file_pointer = buffer[i];
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

                        reply[0] = failure;
                        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                        printf("server:: reply\n");
                        return;
                    }
                    char *data_bitmap_pointer = (pointer + (super->data_bitmap_addr * (UFS_BLOCK_SIZE)));
                    i = 0;
                    while (get_bit((unsigned int *)data_bitmap_pointer, i) == 1)
                    {
                        i++;
                    }
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
        if (offset + nbytes > UFS_BLOCK_SIZE)
        {
            char failure = '1';

            reply[0] = failure;
            UDP_Write(sd, &addr, reply, BUFFER_SIZE);
            printf("server:: reply\n");
            return;
        }
        char *data_bitmap_pointer = (pointer + (super->data_bitmap_addr * (UFS_BLOCK_SIZE)));
        i = 0;
        while (*data_bitmap_pointer == '1')
        {
            i++;
            data_bitmap_pointer++;
        }
        *data_bitmap_pointer = '1';
        char *file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (i * (UFS_BLOCK_SIZE)) + offset;
        inode->direct[0] = i;
        i = 0;
        while (i < nbytes)
        {
            *file_pointer = buffer[i];
            i++;
            file_pointer++;
        }
    }

    if (failure == '0')
    {
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

    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), inum) != 1)
    {
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (inum * (sizeof(inode_t))));

    if (inode->type == MFS_DIRECTORY)
    {
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    if (inode->size < offset + nbytes)
    {
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    int direct_pointer_index = offset / UFS_BLOCK_SIZE;
    int block_pointer_offset = offset % UFS_BLOCK_SIZE;
    char *file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (inode->direct[direct_pointer_index] * (UFS_BLOCK_SIZE)) + block_pointer_offset;
    i = 0;
    while (i < nbytes)
    {
        reply[i + 1] = *file_pointer;
        i++;
        if (block_pointer_offset + i == UFS_BLOCK_SIZE)
        {
            if (inode->size > direct_pointer_index * UFS_BLOCK_SIZE)
            {
                file_pointer = (pointer + (super->data_region_addr * (UFS_BLOCK_SIZE))) + (inode->direct[direct_pointer_index + 1] * (UFS_BLOCK_SIZE));
            }
            else
            {
                char failure = '1';

                reply[0] = failure;
                UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                printf("server:: reply\n");
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
        reply[0] = failure;

        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
    return;
}

void mfs_create(char *message, super_t *super, struct sockaddr_in addr, int fd)
{
    char reply[BUFFER_SIZE];
    int i, pinum, type, count;
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

    printf("LOG: About to call get_bit\n");
    if (get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), pinum) != 1)
    {
        printf("LOG: Called get_bit\n");
        char failure = '1';

        reply[0] = failure;
        // reply[1] = get_bit((unsigned int *)(pointer + super->inode_bitmap_addr * (UFS_BLOCK_SIZE)), pinum);
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
    printf("LOG: Called get_bit\n");


    inode_t *pinode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (pinum * (sizeof(inode_t))));

    if (pinode->type != MFS_DIRECTORY)
    {
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    for (i = 0; i < DIRECT_PTRS; i++)
    {
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        {
            MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j);
            if (dir_entry->inum != -1 && strcmp(dir_entry->name, name) == 0)
            {
                reply[0] = failure;
                UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                printf("server:: reply\n");
                return;
            }
        }
    }

    for (i = 0; i < DIRECT_PTRS; i++)
    {
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        {
            MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j);
            if (dir_entry->inum == -1)
            {
                char *inode_bitmap_pointer = (pointer + (super->inode_bitmap_addr * (UFS_BLOCK_SIZE)));
                i = 0;
                while (get_bit((unsigned int *)inode_bitmap_pointer, i) == 1)
                {
                    i++;
                }
                set_bit((unsigned int *)inode_bitmap_pointer, i);
                dir_entry->inum = i;
                strcpy(dir_entry->name, name);
            }
        }
    }

    inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (i * (sizeof(inode_t))));
    inode->type = type;

    if (type == MFS_REGULAR_FILE)
    {
        inode->size = 0;
    }
    else
    {
        inode->size = 2;

        char *data_bitmap_pointer = (pointer + (super->data_bitmap_addr * (UFS_BLOCK_SIZE)));
        i = 0;
        while (get_bit((unsigned int *)data_bitmap_pointer, i) == 1)
        {
            i++;
        }
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
        fsync(fd);
        reply[0] = failure;

        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }
    return;
}

void mfs_unlink(char *message, super_t *super, struct sockaddr_in addr, int fd)
{
    char reply[BUFFER_SIZE];
    int i, pinum, count;
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

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    inode_t *pinode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (pinum * (sizeof(inode_t))));

    if (pinode->type != MFS_DIRECTORY)
    {
        char failure = '1';

        reply[0] = failure;
        UDP_Write(sd, &addr, reply, BUFFER_SIZE);
        printf("server:: reply\n");
        return;
    }

    for (i = 0; i < DIRECT_PTRS; i++)
    {
        for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); j++)
        {
            MFS_DirEnt_t *dir_entry = (MFS_DirEnt_t *)((pointer + (super->data_region_addr * (UFS_BLOCK_SIZE)) + pinode->direct[i] * UFS_BLOCK_SIZE) + sizeof(dir_ent_t) * j);
            if (dir_entry->inum != -1 && strcmp(dir_entry->name, name) == 0)
            {
                inode_t *inode = (inode_t *)(pointer + (super->inode_region_addr * (UFS_BLOCK_SIZE)) + (dir_entry->inum * (sizeof(inode_t))));
                if (inode->type == MFS_DIRECTORY && inode->size != 2)
                {
                    char failure = '1';

                    reply[0] = failure;
                    UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                    printf("server:: reply\n");
                    return;
                }
                char *inode_bitmap_pointer = (pointer + (super->inode_bitmap_addr * (UFS_BLOCK_SIZE)));
                set_bit((unsigned int *)inode_bitmap_pointer, dir_entry->inum);
                dir_entry->inum = -1;
            }
            if (i == DIRECT_PTRS - 1)
            {
                char failure = '1';

                reply[0] = failure;
                UDP_Write(sd, &addr, reply, BUFFER_SIZE);
                printf("server:: reply\n");
                return;
            }
        }
    }

    if (failure == '0')
    {
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
    char reply[BUFFER_SIZE];
    msync(pointer, filesize, MS_SYNC);
    fsync(fd);

    reply[0] = '0';
    UDP_Write(sd, &addr, reply, BUFFER_SIZE);
    printf("server:: reply\n");
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
