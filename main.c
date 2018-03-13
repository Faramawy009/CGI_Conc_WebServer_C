#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include "src/passiveTCP.c"
#define BUFF_SIZE 1024
#define PORT_NUMBER "23456"
static const char HTTP_Text_Header[] =  "HTTP/1.0 200 OK\n"\
                                        "Content-type: text/html\n\n";

static const char HTTP_GIF_Header[] =   "HTTP/1.0 200 OK \n"\
                                        "Content-Type: image/gif \n"\
                                        "Last-Modified: Mon, 25 Apr 2005 21:06:18 GMT \n"\
                                        "Expires: Sun, 17 Jan 2038 19:14:07 GMT \n"\
                                        "Date: Thu, 09 Mar 2006 00:15:37 GMT\n\n";

#define STRLEN(s) (sizeof(s)/sizeof(s[0])-1)

void handle_sig(int signal) {
    if (signal == SIGINT) {
        exit(0);
    }
}

int main( int argc, char **argv ) {
    signal(SIGINT, handle_sig);
    int sockfd, newsockfd, clilen, fd;
    ssize_t n;
    size_t contentLen;
    char *receiveBuff = (char *) malloc(sizeof(char) * BUFF_SIZE),
         *sendBuff = (char *) malloc(sizeof(char) * BUFF_SIZE),
         *gifBuff = (char*) malloc(sizeof(char)*100);

    fd = open("../index.html", O_RDONLY);
    read(fd, sendBuff, BUFF_SIZE - 1);
    sockfd = passiveTCP(PORT_NUMBER,BUFF_SIZE);
    listen(sockfd, 5);

    /* Accept actual connection from the client */
    while (1) {
        struct sockaddr_in cli_addr;
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        /* If connection is established then start communicating */
        n = recv(newsockfd, receiveBuff, BUFF_SIZE - 1, 0);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }
        if(n==0)
            continue;
        receiveBuff[n] = '\0';
//        printf("Here is the message: %s\n", receiveBuff);
        char * method = strtok(receiveBuff," ");
        char * reqFile = strtok(NULL," ");
//        printf(reqFile);
        if (strcmp(reqFile,"/")==0) {
            printf("Iam here Html\n");
            send(newsockfd,HTTP_Text_Header,STRLEN(HTTP_Text_Header),0);
            contentLen = strlen(sendBuff);
            send(newsockfd,sendBuff,contentLen,0);
        } else{
            printf("Iam here gif\n");
            int gifFd = open("../images/duck.gif",O_RDONLY);
            bzero(gifBuff,100);
            ssize_t r;
//            printf("Read size:%d\n %s\n", r, gifBuff);
            send(newsockfd,HTTP_GIF_Header,STRLEN(HTTP_GIF_Header),0);
            while(r = read(gifFd,gifBuff,100)) {
//                printf("%d:%s\n",count++,gifBuff);
                send(newsockfd,gifBuff,r,0);
            }
        }
        bzero(receiveBuff, BUFF_SIZE - 1);
        close(newsockfd);
    }
    close(sockfd);

    return 0;
}