#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>
#include <netdb.h>       /* for close() */
#include <sys/time.h>
#include <sys/resource.h>

#define RCVBUFSIZE 10000 /* Size of receive buffer */
void DieWithError(const char *errorMessage)
{
    printf(errorMessage);
    exit(1);
} /* Error handling function */

struct addrinfo *getHostInfo(char *host, char *port)
{
    int r;
    struct addrinfo hints, *getaddrinfo_res;

    // Setup hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((r = getaddrinfo(host, port, &hints, &getaddrinfo_res)))
    {
        DieWithError("getaddrinfo error");
    }

    return getaddrinfo_res;
}

long gettimeinms() {

	long total_time;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	total_time = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

	return total_time;
}

int main(int argc, char *argv[])
{
    int sock;                      /* Socket descriptor */
    struct addrinfo *servAddrInfo; /* Echo server address */
    unsigned short urlServPort;    /* server port */
    char *servUrl;                 /* Server url */
    char *servUrlPath;
    char *urlPortString;
    bool getrtt;                      /* if printing rtt for accessing the url */
    char echoBuffer[RCVBUFSIZE];   /* Buffer for echo string */
    unsigned int echoStringLen;    /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd; /* Bytes read in single recv()and total bytes read */
    long start,end,rtt;

    if ((argc < 3) || (argc > 4)) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s [-options] <server_url> <port_number> \n",
                argv[0]);
        exit(1);
    }

    if (argc == 3)
    {
        servUrl = argv[1]; /* First arg: server url */
        servUrlPath = strchr(servUrl, '/');
        if (servUrlPath != nullptr)
        {
            *servUrlPath++ = '\0';
        }

        urlPortString = argv[2]; /* Second Arg: given port */
    }
    else
    {

        if (argc == 4 && (strcmp(argv[1], "-p") == 0))
        {
            getrtt = true;
            servUrl = argv[2]; /* First arg: server url */
            servUrlPath = strchr(servUrl, '/');
            if (servUrlPath != nullptr)
            {
                *servUrlPath++ = '\0';
            }

            urlPortString = argv[3];
        }
        else
        {
            fprintf(stderr, "Usage: %s [-options] <server_url> <port_number> \n",
                    argv[0]);
            exit(1);
        }
    }

    urlServPort = atoi(urlPortString);

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        DieWithError("socket() failed");
    }

    servAddrInfo = getHostInfo(servUrl, urlPortString);

    start = gettimeinms();
    /* Establish the connection to the echo server */
    if (connect(sock, servAddrInfo->ai_addr, servAddrInfo->ai_addrlen) < 0)
    {
        DieWithError("connect() failed");
    }
    end = gettimeinms();
    rtt = end - start;
    if(getrtt) /* print rtt if -p option is enabled */
    {
        printf("RTT in milliseconds: %d\n", rtt);
    }

    /* Create raw GET Request */
    char getReq[1000] = {0};
    if (servUrlPath == nullptr)
    {
        sprintf(getReq, "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", servUrl);
    }
    else
    {
        sprintf(getReq, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n", servUrlPath, servUrl);
    }

    echoStringLen = strlen(getReq); /* Determine input length */
    /* Send the string to the server */
    if (send(sock, getReq, echoStringLen, 0) != echoStringLen)
    {
        DieWithError("send() sent a different number of bytes than expected");
    }

    /* Receive the same string back from the server */
    totalBytesRcvd = 0;
    bool hashtml = false;
    char *htmlstart;
    FILE *f;
    while ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) > 0)
    {
        totalBytesRcvd += bytesRcvd;  /* Keep tally of total bytes */
        echoBuffer[bytesRcvd] = '\0'; /* Terminate the string! */
        printf("%s", echoBuffer);     /* Print the echo buffer */
        
        if(!hashtml)
        { 
            if( (htmlstart = strstr(echoBuffer,"\r\n\r\n")) != nullptr)
            {
                hashtml = true;
                htmlstart += 4;

                f = fopen("file","w");
                fprintf(f,htmlstart);
                htmlstart = nullptr;
            }
        } 
        else
        {
            fprintf(f,echoBuffer);
        }  
        
        

        memset(echoBuffer, 0, RCVBUFSIZE);
    }

    if(f != nullptr)
    {
        fclose(f);
    } else
    {
        printf("No html found\n");
    }
    

    printf("\n"); /* Print a final linefeed */
    close(sock);
    exit(0);
}