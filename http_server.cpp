#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define MAXPENDING 5 /* Maximum outstanding connection reqruests */

void DieWithError(const char *errorMessage); /* Error handling function */
void HandleTCPClient(int clntSocket);        /* TCP client handling function */
void send_header(int clntSocket, int status);

int main(int argc, char *argv[])
{
    int servSock;                    /*Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */
    if (argc != 2)                   /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }
    echoServPort = atoi(argv[1]); /* First arg: local port */
    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        DieWithError("socket() failed");
    }
    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */
    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
    {
        close(servSock);
        DieWithError("bind() failed");
    }
    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
    {
        DieWithError("listen() failed");
    }

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr); /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen)) < 0)
        {
            DieWithError("accept() failed");
        }
        /* clntSock is connected to a client! */
        printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
        HandleTCPClient(clntSock);
    }
    /* NOT REACHED */
}

void DieWithError(const char *errorMessage)
{
    printf(errorMessage);
    exit(1);
}

void HandleTCPClient(int clntSocket)
{
    char request_header[500];
    char *http_method;
    char *filename;
    int bytesRead;

    bzero(request_header, 500);

    if (!((bytesRead = recv(clntSocket, request_header, sizeof(request_header) - 1, 0)) > 0))
    {
        return;
    }
    request_header[bytesRead] = '\0';

    http_method = strtok(request_header, " ");
    if (strcmp(http_method, "GET") != 0)
    {
        return;
    }

        

    filename = strtok(NULL, " ");

    if (strcmp(filename, "/") == 0)
    {
        send_header(clntSocket, 0);
    }
    else if (filename[0] == '/')
    {
        filename++;
        if (access(filename, F_OK) != 0)
        {
            send_header(clntSocket, 2);
            return;
        }
        else if (access(filename, R_OK) != 0)
        {
            send_header(clntSocket, 1);
            return;
        }
        else
        {
            send_header(clntSocket, 0);
        }
    }

    FILE *file;
    if(strcmp(filename, "/") == 0)
    {
        file = fopen("index.html", "r");
    } else
    {
        file = fopen(filename, "r");
    }
    
    char response_buff[100];

    while (fgets(response_buff, 100, file))
    {
        send(clntSocket, response_buff, strlen(response_buff), 0);
        memset(response_buff, 0, 100);
    }
    memset(request_header,0,500);
    close(clntSocket);
}

void send_header(int clntSocket, int status)
{
    char header[100] = {0};

    if (status == 0)
    {
        sprintf(header, "HTTP/1.1 200 OK\r\n\r\n");
    }
    else if (status == 1)
    {
        sprintf(header, "HTTP/1.1 403 Forbidden\r\n\r\n");
    }
    else
    {
        sprintf(header, "HTTP/1.1 404 Not Found\r\n\r\n");
    }

    send(clntSocket, header, strlen(header), 0);
    if(status > 0)
    {
        close(clntSocket);
    }
}