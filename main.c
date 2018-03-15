#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>
#include "src/passiveTCP.c"
#define BUFF_SIZE 1024
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


void readConfig(int* nConnections, char* root, char* indexFile, char* port) {
    FILE * fstream = fopen("../conf/httpd.conf","r");
    if (fstream == NULL){
        perror("cannot open config file!");
        exit(1);
    }
    size_t len;
    char** lines = (char **) malloc(sizeof(char*) * 4);
    for(int i=0; i<4; i++) {
        lines[i] = (char *) malloc(sizeof(char) * 100);
        if(getline(&lines[i], &len, fstream) == -1){
            printf("%s", "error reading from config file!");
            exit(EXIT_FAILURE);
        }
        //lines[i][len] = '\0';
    }

    char* temp;
    char slash = '\\';
    temp = strtok(lines[0],"=");
    temp = strtok(NULL,"=");
    *nConnections = atoi(temp);
    temp = strtok(lines[1],"=");
    strcpy(root,strtok(NULL,"="));
    root[strlen(root)-1] = '\0';
    temp = strtok(lines[2],"=");
    strcpy(indexFile, strtok(NULL,"="));
    indexFile[strlen(indexFile)-1] = '\0';
    temp = strtok(lines[3],"=");
    strcpy(port,strtok(NULL,"="));
    port[strlen(port)-1] = '\0';
}

void printLog(char* logPath, char* ip, char* buffer) {
    FILE * access_log = fopen(logPath, "ab+");
    if (access_log == NULL){
        perror("cannot open config file!");
        exit(1);
    }
    char *logMsg = (char*) malloc(sizeof(char)*(BUFF_SIZE+15));
    sprintf(logMsg,"%s\n%s\n\n",ip,buffer);
    if (fputs(logMsg, access_log) < 0){
        perror("error witing to access_log");
        exit(1);
    }

    if (fclose(access_log) != 0) {
        perror("error closing access log file!\n");
        exit(1);
    }
    free(logMsg);
}
int main( int argc, char **argv ) {
    //redirecting stdout to error log


    int nConnections;
    char* root = (char *) malloc(sizeof(char) * 100);
    char* indexFile = (char *) malloc(sizeof(char) * 100);
    char* port = (char *) malloc(sizeof(char) * 6);
    readConfig(&nConnections, root, indexFile, port);
    chdir(root);
//    int error_log_fd = open("logs/error_log.txt", 	O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
//    if(error_log_fd < 0) {
//        perror("Can't open error log file");
//        exit(1);
//    }
//    dup2(error_log_fd,1);
    freopen("logs/error_log.txt","a+",stderr);
    perror("ZOBRYYYY KBEEEER");
    signal(SIGINT, handle_sig);
    int sockfd, newsockfd, clilen, fd;
    ssize_t n;
    size_t readSize;
    char *receiveBuff = (char *) malloc(sizeof(char) * BUFF_SIZE),
            *sendBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);


    sockfd = passiveTCP(port,BUFF_SIZE);
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
        char *client_ip = (char *) malloc(20 * sizeof(char));
        strcpy(client_ip, inet_ntoa(cli_addr.sin_addr));
        /* If connection is established then start communicating */
        n = recv(newsockfd, receiveBuff, BUFF_SIZE - 1, 0);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }
        if(n==0)
            continue;
        receiveBuff[n] = '\0';
        // writing to access log
        printLog("logs/access_log.txt",client_ip,receiveBuff);


        char * msgBody = (char *) malloc(strlen(receiveBuff)* sizeof(char));
        strcpy(msgBody,receiveBuff);
        char * method = strtok(receiveBuff," ");
        char * reqFile = strtok(NULL," ");
        reqFile=strtok(reqFile,"?");
        if (strcmp(reqFile,"/")==0) {
            printf("Iam here Html\n");
            fd = open(indexFile, O_RDONLY);
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
                printf("Req file currently is: %s\n", reqFile);
                if(strcmp(extension,"gif")==0)
                    send(newsockfd,HTTP_GIF_Header,STRLEN(HTTP_GIF_Header),0);
                else if(strcmp(extension,"jpg")==0)
                    send(newsockfd,HTTP_JPG_Header,STRLEN(HTTP_JPG_Header),0);
                else if(strcmp(extension,"html")==0)
                    send(newsockfd,HTTP_Text_Header,STRLEN(HTTP_Text_Header),0);
                else if(strcmp(extension,"class")==0) {
                    printf("I am in CGI\n\n");
                    int processID = fork();
                    printf("Pid is %d\n",processID);
                    if(processID==0) { //child
                        printf("I am in Child\n\n");
                        //close listening socket
                        close(sockfd);
                        //Move output to connected socket
                        dup2(newsockfd,1);
//                        close(1);
//                        execl("/bin/bash","bash", "cgi-bin/cgiTest.py");
//                        char *fileName = strtok(reqFile,".");
//                        printf("File name is: %s\n\n", fileName);
                        char * dirParsing = (char*) malloc(sizeof(char)*(strlen(reqFile)+1));
                        strcpy(dirParsing,reqFile);
                        char* dir = strtok(dirParsing+1,"/");
                        chdir(dir);
                        dir=strtok(NULL,"/");
                        char* execName = strtok(dir,".");
                        int val=execlp ("java","java", execName, msgBody, (char*)NULL);
                        if(val==-1) {
                            perror("ERROR on exec");
                            exit(1);
                        }
                    } else {
//                        printf("I am in Parent\n\n");
//                        waitpid(processID,0,0);
                        //Clear buffer and close connected socket
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
//        close(error_log_fd);
    }
    close(sockfd);

    return 0;
}
