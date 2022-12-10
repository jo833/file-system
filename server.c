#include <stdio.h>
#include "mfs.h"
#include "ufs.h"

#define BUFFER_SIZE (1000)
int sd;
void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

// server code
int main(int argc, char *argv[])
{
    signal(SIGINT, intHandler);
    int port = argv[1];
    int f = open(argv[2]);
    sd = UDP_Open(port);
    assert(f > -1);
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
            char *pinum_string;
            char *name;
            int i = 0;
            while (message[i] != '\0'){
                i++;
                strcat(pinum_string, message[i]);
            }
            int pinum = atoi(pinum_string);
            while (message[i] != '\0'){
                i++;
                strcat(name, message[i]);
            }
            break;
        case '1':
            char *pinum_string;
            char *name;
            int i = 0;
            while (message[i] != '\0'){
                i++;
                strcat(pinum_string, message[i]);
            }
            int pinum = atoi(pinum_string);
            break;
        case '2':

            break;
        case '3':

            break;
        case '4':

            break;
        case '5':

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
    bitmap[index] |=  0x1 << offset;
}


