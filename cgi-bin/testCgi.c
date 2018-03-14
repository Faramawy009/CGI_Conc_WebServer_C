#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char** argv) {
	printf("HTTP/1.0 200 OK\nContent-type: text/html\n\n<html><body><h1>MatzzRokzz</h1><h1> Hello world</h1><body>");
	mkdir("I ran", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}
