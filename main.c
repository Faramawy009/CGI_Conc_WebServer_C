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
#define PORT_NUMBER "34567"



void handle_sigalrm(int signal) {
    if (signal == SIGINT) {
        exit(0);
    }
}

int main( int argc, char **argv ) {
    int sockfd, newsockfd, htmlFileFd, clilen, n;
    char *receiveBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);
    char *sendBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);
    char *msg = (char *) malloc(sizeof(char) * BUFF_SIZE);
    char *HTTP_Header = "HTTP/1.0 \n"
            "Content-type: text/html\n"
            "\n";
    struct sockaddr_in cli_addr;
    int fd = open("../index.html", O_RDONLY);
    read(fd, sendBuff, BUFF_SIZE - 1);
    sockfd = passiveTCP(PORT_NUMBER,BUFF_SIZE);
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    /* Accept actual connection from the client */
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        /* If connection is established then start communicating */
        bzero(receiveBuff, BUFF_SIZE - 1);
        n = recv(newsockfd, receiveBuff, BUFF_SIZE - 1, 0);

        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }

        printf("Here is the message: %s\n", receiveBuff);

        bzero(msg, BUFF_SIZE - 1);
        strcat(msg, HTTP_Header);
        strcat(msg, sendBuff);
        size_t len = strlen(msg);
        send(newsockfd, msg, len, 0);
        close(newsockfd);
    }
    close(sockfd);

    return 0;
}