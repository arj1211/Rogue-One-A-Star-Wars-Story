#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT "2520"
#define PLANS_FILE "deathstarplans.dat"

typedef struct {
    char * data;
    int length;
} buffer;

extern int errno;

/* This function loads the file of the Death Star
   plans so that they can be transmitted to the
   awaiting Rebel Fleet. It takes no arguments, but
   returns a buffer structure with the data. It is the
   responsibility of the caller to deallocate the 
   data element inside that structure.
   */ 
buffer load_plans( );
int sendall(int socket, char* buf, int* len);

int main( int argc, char** argv ) {
    if ( argc != 2 ) {
        printf( "Usage: %s IP-Address\n", argv[0] );
        return -1;
    }
    printf("Planning to connect to %s.\n", argv[1]);
    buffer buf = load_plans();
    struct addrinfo hints; 
    struct addrinfo *res; 
    /* Set up hints to restrict connection type */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    /* Get address info of remote server, error check */
    int result = getaddrinfo(argv[1],PORT,&hints,&res);
    if (result==-1)
    {
        printf("Couldn't get address info for %s:%s\n", argv[1],PORT);
        free(buf.data);
        freeaddrinfo(res);
        return -1;
    }
    /* Grab internet address in struct */
    struct sockaddr_in *sain = (struct sockaddr_in*)res->ai_addr;
    /* make a socket for client, error check */
    int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s==-1)
    {
        printf("Couldn't create socket\n");
        close(s);
        free(buf.data);
        freeaddrinfo(res);
        return -1;
    }
    /* connect up, error check */
    int connection_status = connect(s, res->ai_addr, res->ai_addrlen);
    if(connection_status==-1)
    {
        printf("Couldn't connect to socket\n");
        close(s);
        free(buf.data);
        freeaddrinfo(res);
        return -1;
    }
    /* send the deathstar plans, error check */
    int send_status = sendall(s, buf.data, &buf.length);
    if (send_status==-1)
    {
        printf("Error sending deathstar\n");
        close(s);
        free(buf.data);
        freeaddrinfo(res);
        return -1;
    }
    /* recv feedback, error check */
    char* feedback = (char*)malloc(64);
    int buff_bytes = recv(s, feedback, 64, 0);
    if (buff_bytes==-1)
    {
        printf("Error getting response\n");
        free(feedback);
        close(s);
        free(buf.data);
        freeaddrinfo(res);
        return -1;
    }
    if (buff_bytes==0)
    {
        printf("%s closed the connection\n", argv[1]);
        free(feedback);
        close(s);
        free(buf.data);
        freeaddrinfo(res);
        return -1;
    }
    for (int i = 0; i < buff_bytes; i++)
        printf("%c",feedback[i]);
    printf("\n");
    free(feedback);
    close(s);
    free(buf.data);
    freeaddrinfo(res);
    return 0;
}

buffer load_plans( ) {
    struct stat st;
    stat( PLANS_FILE, &st );
    ssize_t filesize = st.st_size;
    char* plansdata = malloc( filesize );
    int fd = open( PLANS_FILE, O_RDONLY );
    memset( plansdata, 0, filesize );
    read( fd, plansdata, filesize );
    close( fd );

    buffer buf;
    buf.data = plansdata;
    buf.length = filesize;

    return buf;
}

int sendall(int socket, char* buf, int* len)
{
    int total = 0; /* total # of bytes sent */
    int bytesleft = *len; /* bytes left to send */
    int n;
    while (total<*len)
    {
        n = send(socket, buf+total, bytesleft, 0);
        if (n==-1) break;
        total+=n;
        bytesleft-=n;
    }
    *len = total; /* *len is total bytes actually sent in the end */
    return n==-1?-1:0; /* return failure -1 or success 0 */
}
