#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <netdb.h>
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


int nConnections;
char* root;
char* indexFile;
char* port;

struct my_counter{
	int n;
	pthread_mutex_t lock;
};

static struct my_counter connections;

void counter_init(struct my_counter *counter) {
  counter->n = 0;
  pthread_mutex_init(&counter->lock, NULL);
}

void counter_inc(struct my_counter *counter) {
  pthread_mutex_lock(&counter->lock);
  counter->n++;
  pthread_mutex_unlock(&counter->lock);
}

void counter_decr(struct my_counter *counter) {
  pthread_mutex_lock(&counter->lock);
  counter->n--;
  pthread_mutex_unlock(&counter->lock);
}

int get_counter(struct my_counter *counter) {
  pthread_mutex_lock(&counter->lock);
  int val = counter->n;
  pthread_mutex_unlock(&counter->lock);
  return val;
}

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

void* clientHandler(void*);


int main( int argc, char **argv ) {
    root = (char *) malloc(sizeof(char) * 100);
    indexFile = (char *) malloc(sizeof(char) * 100);
    port = (char *) malloc(sizeof(char) * 6);
    readConfig(&nConnections, root, indexFile, port);
	counter_init(&connections);
	
    chdir(root);
    signal(SIGINT, handle_sig);
    int sockfd, newsockfd, clilen, fd;
    ssize_t n;
	/*
    size_t readSize;
    char *receiveBuff = (char *) malloc(sizeof(char) * BUFF_SIZE),
         *sendBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);
	*/
    sockfd = passiveTCP(port,BUFF_SIZE);
    listen(sockfd, 5);

    /* Accept actual connection from the client */
    while (1) {
        struct sockaddr_in cli_addr;
        clilen = sizeof(cli_addr);
		while(1) {
			if (get_counter(&connections) < nConnections)
				break;
		}
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }
		
		pthread_t tid;
		if( pthread_create( &tid , NULL ,  clientHandler , (void*) &newsockfd) < 0)
        {
            perror("could not create thread");
            exit(1);
        }
		pthread_detach(tid);
		/*
        // If connection is established then start communicating
        n = recv(newsockfd, receiveBuff, BUFF_SIZE - 1, 0);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }
        if(n==0)
            continue;
        receiveBuff[n] = '\0';
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
		*/
    }
    close(sockfd);

    return 0;
}




void* clientHandler(void* sfd) {
		int newsockfd = *(int *) sfd;
		counter_inc(&connections);
		size_t readSize;
		char *receiveBuff = (char *) malloc(sizeof(char) * BUFF_SIZE),
         *sendBuff = (char *) malloc(sizeof(char) * BUFF_SIZE);
	/* If connection is established then start communicating */
        int n = recv(newsockfd, receiveBuff, BUFF_SIZE - 1, 0);
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }
        if(n==0) {
			bzero(receiveBuff, BUFF_SIZE);
			bzero(sendBuff, BUFF_SIZE);
			close(newsockfd);
			counter_decr(&connections);
			pthread_exit(NULL);
		}
        receiveBuff[n] = '\0';
        char * msgBody = (char *) malloc(strlen(receiveBuff)* sizeof(char));
        strcpy(msgBody,receiveBuff);
        char * method = strtok(receiveBuff," ");
        char * reqFile = strtok(NULL," ");
        reqFile=strtok(reqFile,"?");
        if (strcmp(reqFile,"/")==0) {
            printf("Iam here Html\n");
            int fd = open(indexFile, O_RDONLY);
            send(newsockfd,HTTP_Text_Header,STRLEN(HTTP_Text_Header),0);
            while(readSize = read(fd,sendBuff,BUFF_SIZE-1)) {
                send(newsockfd,sendBuff,readSize,0);
            }
        } else{
            printf("Iam here not blank and this is req file:%s\n",reqFile);
            int fd = open(reqFile+1,O_RDONLY);
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
						//
						//
						//
						//REVISIT NEXT LINE
						//
						//
						//
                        //close(sockfd);
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
                        counter_decr(&connections);
						pthread_exit(NULL);
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
		counter_decr(&connections);
		pthread_exit(NULL);
}
