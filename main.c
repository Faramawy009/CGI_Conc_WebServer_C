#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include "src/connectTCP.c"
#define BUFF_SIZE 1024
#define PORT_NUMBER 45678


void handle_sigalrm(int signal) {
    if (signal == SIGINT) {
        exit(0);
    }
}

int main( int argc, char **argv ) {
    chdir("../../Lab1Repo/src/");
    int sockfd, newsockfd, htmlFileFd, clilen;
    char *receiveBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);
    char *sendBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);
    char *HTTP_Header = "HTTP/1.0 \n"
            "Content-type: text/html\n"
            "\n";
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    int fd = open("../index.html", O_RDONLY);
    read(fd, sendBuff, BUFF_SIZE - 1);
    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT_NUMBER);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Now start listening for the clients, here process will
       * go in sleep mode and will wait for the incoming connection
    */

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

        char *msg = (char *) malloc(sizeof(char) * BUFF_SIZE);
        strcat(msg, HTTP_Header);
        strcat(msg, sendBuff);
        int len = strlen(msg);
        int c = send(newsockfd, msg, len, 0);
        close(newsockfd);
    }
    /* Write a response to the client */
//    if (c < 0) {
//        perror("ERROR writing to socket");
//        exit(1);
//    }
    // close(newsockfd);
    close(sockfd);

    return 0;
}