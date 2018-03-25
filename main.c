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
#include <pthread.h>
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

int nConnections;
char* root;
char* indexFile;
char* port;
char * dummy;

struct RequestData{
    int clientDescriptor;
    char request [BUFF_SIZE];
};

void printLog(char* logPath, char* ip, char* buffer) {
    FILE * access_log = fopen(logPath, "ab+");
    if (access_log == NULL){
        perror("cannot open access log file!");
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

void* handleClientCGI(void * reqData){
    struct RequestData* requestData = (struct RequestData *) reqData;
    char* receiveBuff = requestData->request;
    int clientSocketD = requestData->clientDescriptor;

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(clientSocketD, (struct sockaddr *)&addr, &addr_size);
    char clientip [20];
    strcpy(clientip, inet_ntoa(addr.sin_addr));

    printLog("logs/access_log.txt",clientip,receiveBuff);

    char msgBody [strlen(receiveBuff)];
    strcpy(msgBody,receiveBuff);

    strtok_r(receiveBuff," ", &receiveBuff);
    char * reqFile = strtok_r(receiveBuff," ", &dummy);

    reqFile=strtok_r(reqFile,"?", &dummy);
    printf("I am in CGI\n\n");
    int processID = fork();
    printf("Pid is %d\n",processID);
    if(processID==0) { //child
        printf("I am in Child\n\n");
        //Move output to connected socket
        dup2(clientSocketD,1);
        char dirParsing[strlen(reqFile)+1];
        strcpy(dirParsing,reqFile);
        char* savingPointer;
        char* dir = strtok_r(dirParsing+1,"/", &savingPointer);
        chdir(dir);
        dir=strtok_r(savingPointer,"/",&dummy);
        char* execName = strtok(dir,".");
        int val=execlp ("java","java", execName, msgBody, (char*)NULL);
        if(val==-1) {
            perror("ERROR on exec");
            exit(1);
        }
    } else {
        close(clientSocketD);
        free(reqData);
//        printf("Closed socket!\n\n");
    }
}

void* handleClient(void * reqData) {
    char *sendBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);
    struct RequestData* requestData = (struct RequestData *) reqData;
    char* receiveBuff = requestData->request;
    int clientSocketD = requestData->clientDescriptor;
    int fd = 0;
    int readSize = 0;

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(clientSocketD, (struct sockaddr *)&addr, &addr_size);
    char clientip [20];
    strcpy(clientip, inet_ntoa(addr.sin_addr));

    printLog("logs/access_log.txt",clientip,receiveBuff);

    char msgBody [strlen(receiveBuff)];
    strcpy(msgBody, receiveBuff);
    strtok_r(receiveBuff, " ", &receiveBuff);
    char *reqFile = strtok_r(receiveBuff, " ", &receiveBuff);
    reqFile = strtok_r(reqFile, "?", &dummy);
    if (strcmp(reqFile, "/") == 0) {
        printf("Iam here Html\n");
        fd = open(indexFile, O_RDONLY);
        send(clientSocketD, HTTP_Text_Header, STRLEN(HTTP_Text_Header), 0);
        while (readSize = read(fd, sendBuff, BUFF_SIZE - 1)) {
            send(clientSocketD, sendBuff, readSize, 0);
        }
    } else {
        printf("Iam here not blank and this is req file:%s\n", reqFile);
        fd = open(reqFile + 1, O_RDONLY);
        if (fd == -1) {
            printf("Yess fd is -1\n");
            send(clientSocketD, HTTP_404_Header, STRLEN(HTTP_404_Header), 0);
            fd = open("404.html", O_RDONLY);
        } else {
            char fileExtParser[strlen(reqFile)];
            strcpy(fileExtParser, reqFile);
            char *fileName;
            strtok_r(fileExtParser, ".", &fileName);
            char *extension = strtok_r(fileName, ".", &dummy);
            printf("Extension is: %s\n\n", extension);
            printf("Req file currently is: %s\n", reqFile);
            if (strcmp(extension, "gif") == 0)
                send(clientSocketD, HTTP_GIF_Header, STRLEN(HTTP_GIF_Header), 0);
            else if (strcmp(extension, "jpg") == 0)
                send(clientSocketD, HTTP_JPG_Header, STRLEN(HTTP_JPG_Header), 0);
            else if (strcmp(extension, "html") == 0)
                send(clientSocketD, HTTP_Text_Header, STRLEN(HTTP_Text_Header), 0);
        }
        while(readSize = read(fd,sendBuff,BUFF_SIZE))
            send(clientSocketD,sendBuff,readSize,0);
    }
    close(clientSocketD);
    free(sendBuff);
    free(reqData);
}


void handle_sig(int signal) {
    if (signal == SIGINT) {
        exit(0);
    }
}


void readConfig(int* nConnections, char* root, char* indexFile, char* port) {
    FILE * fstream = fopen("./conf/httpd.conf","r");
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

int main( int argc, char **argv ) {
    //redirecting stdout to error log


    nConnections = 0;
    root = (char *) malloc(sizeof(char) * 100);
    indexFile = (char *) malloc(sizeof(char) * 100);
    port = (char *) malloc(sizeof(char) * 6);
    readConfig(&nConnections, root, indexFile, port);
    chdir(root);
//    int error_log_fd = open("logs/error_log.txt", 	O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
//    if(error_log_fd < 0) {
//        perror("Can't open error log file");
//        exit(1);
//    }
//    dup2(error_log_fd,1);
    freopen("logs/error_log.txt","a+",stderr);
    signal(SIGINT, handle_sig);
    int sockfd, newsockfd, clilen, fd;
    ssize_t n;
    size_t readSize;
    char *receiveBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);

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
//        printLog("logs/access_log.txt",client_ip,receiveBuff);
        receiveBuff[n] = '\0';

        struct RequestData* requestData = (struct RequestData*) malloc(sizeof(struct RequestData));
        requestData->clientDescriptor = newsockfd;
        strcpy(requestData->request, receiveBuff);
        bzero(receiveBuff, BUFF_SIZE+1);
        if (strstr(requestData->request, ".class") != NULL) {
            handleClientCGI(requestData);
        } else{
            pthread_t tid;
            if (pthread_create(&tid, NULL, handleClient, (void *) requestData) < 0) {
                perror("could not create thread");
                exit(1);
            }
            int err = pthread_detach(tid);
            if (err) {
                perror("could not make thread detachable");
            }
        }

    }
    close(sockfd);

    return 0;
}
