#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>
#include "src/passiveTCP.c"
#define BUFF_SIZE 1024
#define PORT_NUMBER "23456"
static const char HTTP_Text_Header[] =  "HTTP/1.0 200 OK\n"\
                                        "Content-type: text/html\n\n";

static const char HTTP_404_Header[] =   "HTTP/1.0 404 NOT FOUND\n"\
                                        "Content-type: text/html\n\n";

static const char HTTP_GIF_Header[] =   "HTTP/1.0 200 OK \n"\
                                        "Content-Type: image/gif \n"\
                                        "Last-Modified: Mon, 25 Apr 2005 21:06:18 GMT \n"\
                                        "Expires: Sun, 17 Jan 2038 19:14:07 GMT \n"\
                                        "Date: Thu, 09 Mar 2006 00:15:37 GMT\n\n";

static const char HTTP_JPG_Header[] =   "HTTP/1.0 200 OK \n"\
                                        "Content-Type: image/jpg \n"\
                                        "Last-Modified: Mon, 25 Apr 2005 21:06:18 GMT \n"\
                                        "Expires: Sun, 17 Jan 2038 19:14:07 GMT \n"\
                                        "Date: Thu, 09 Mar 2006 00:15:37 GMT\n\n";

#define STRLEN(s) (sizeof(s)/sizeof(s[0])-1)

void handle_sig(int signal) {
    if (signal == SIGINT) {
        exit(0);
    }
}

//int parse(const char* line)
//{
//    /* Find out where everything is */
//    const char *start_of_path = strchr(line, ' ') + 1;
//    const char *start_of_query = strchr(start_of_path, '?');
//    const char *end_of_query = strchr(start_of_query, ' ');
//
//    /* Get the right amount of memory */
//    char path[start_of_query - start_of_path];
//    char query[end_of_query - start_of_query];
//
//    /* Copy the strings into our memory */
//    strncpy(path, start_of_path,  start_of_query - start_of_path);
//    strncpy(query, start_of_query, end_of_query - start_of_query);
//
//    /* Null terminators (because strncpy does not provide them) */
//    path[sizeof(path)] = 0;
//    query[sizeof(query)] = 0;
//
//    /*Print */
//    printf("%s\n", query, sizeof(query));
//    printf("%s\n", path, sizeof(path));
//}


int main( int argc, char **argv ) {
    chdir("../");
    signal(SIGINT, handle_sig);
    int sockfd, newsockfd, clilen, fd;
    ssize_t n;
    size_t readSize;
    char *receiveBuff = (char *) malloc(sizeof(char) * BUFF_SIZE),
         *sendBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);

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
//        parse(receiveBuff);
        char * method = strtok(receiveBuff," ");
        char * reqFile = strtok(NULL," ");
//        printf(reqFile);
        if (strcmp(reqFile,"/")==0) {
            printf("Iam here Html\n");
            fd = open("index.html", O_RDONLY);
            send(newsockfd,HTTP_Text_Header,STRLEN(HTTP_Text_Header),0);
            while(readSize = read(fd,sendBuff,BUFF_SIZE-1)) {
                send(newsockfd,sendBuff,readSize,0);
            }
        } else{
            printf("Iam here not blank and this is req file:%s\n",reqFile);
            fd = open(reqFile+1,O_RDONLY);
            if(fd==-1) {
                printf("Yess fd is -1\n");
                send(newsockfd,HTTP_404_Header,STRLEN(HTTP_404_Header),0);
                fd = open("404.html",O_RDONLY);
            } else {
                char * fileExtParser = (char *) malloc(sizeof(char) *strlen(reqFile));
                strcpy(fileExtParser,reqFile);
                char*extension = strtok(fileExtParser,".");
                extension=strtok(NULL,".");
                printf("Extension is: %s\n\n",extension);
                if(strcmp(extension,"gif")==0)
                    send(newsockfd,HTTP_GIF_Header,STRLEN(HTTP_GIF_Header),0);
                else if(strcmp(extension,"jpg")==0)
                    send(newsockfd,HTTP_JPG_Header,STRLEN(HTTP_JPG_Header),0);
                else if(strcmp(extension,"html")==0)
                    send(newsockfd,HTTP_Text_Header,STRLEN(HTTP_Text_Header),0);
                else if(strcmp(extension,"cgi")==0) {
                    printf("I am in CGI\n\n");
//                    printf("HTTP/1.0 200 OK\nContent-type: text/html\n\n<html><body><h1>MatzzRokzz</h1><h1> Hello world</h1><body>");
//                    printf("Testttt\n");
//                    printf("<html><body><h1>MatzzRokzz</h1><h1> Hello world</h1><body>");
//                    char* headerTest = "HTTP/1.0 200 OK\nContent-type: text/html\n\n<html><body><h1>MatzzRokzz</h1><h1> Hello world</h1><body>";
//                    char* contentTest = "<html><body><h1>MatzzRokzz</h1><h1> Hello world</h1><body>";
//                    int headerLen = strlen(headerTest);
//                    int contentLen = strlen(contentTest);
//                    send(newsockfd,headerTest,headerLen,0);
//                    send(newsockfd,contentTest,contentLen,0);
                    int processID = fork();
                    printf("Pid is %d\n",processID);
                    if(processID==0) { //child
                        printf("I am in Child\n\n");
                        dup2(newsockfd,1);
//                        close(1);
//                        execl("/bin/bash","bash", "cgi-bin/cgiTest.py");
                        int val=execl (reqFile+1, reqFile+1, (char*)NULL);
                        if(val==-1) {
                            perror("ERROR on exec");
                            exit(1);
                        }
                    } else {
                        printf("I am in Parent\n\n");
                        waitpid(processID,0,0);
                        bzero(receiveBuff, BUFF_SIZE);
                        bzero(sendBuff, BUFF_SIZE);
                        close(newsockfd);
                        printf("Closed socket!\n\n");
                        continue;
                    }
                }

            }
//            printf("Read size:%d\n %s\n", r, gifBuff);
            while(readSize = read(fd,sendBuff,BUFF_SIZE)) {
//                printf("%d:%s\n",count++,gifBuff);
                send(newsockfd,sendBuff,readSize,0);
            }
        }
        bzero(receiveBuff, BUFF_SIZE);
        bzero(sendBuff, BUFF_SIZE);
        close(newsockfd);
    }
    close(sockfd);

    return 0;
}