/*
 * proxy.c - CS:APP Web proxy
 *
 * Student ID: 
 *         Name:
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"
//#include "echo.h"

/* The name of the proxy's log file */
#define PROXY_LOG "proxy.log"

/* Undefine this if you don't want debugging output */
#define DEBUG
#define MAX_MALLOC 10
/*
 * Functions to define
 */


 void proxy(int connfd, char *prefix);
 void proxy2(int connfd, char *prefix) ;
void *process_request(void* vargp);
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen_w(int fd, void *usrbuf, size_t n);
void *proxy_thread(void *vargp);
void *proxy_client();
/*
 * main - Main routine for the proxy program
 */

FILE *fp;

int main(int argc, char **argv)
{
	fp = fopen("proxy.log", "w+" );
	/* Check arguments */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
		exit(0);
	}

    int port = atoi(argv[1]);
    struct sockaddr_in clientaddr;
    int clientlen=sizeof(clientaddr);
    pthread_t tid; 

    int listenfd = Open_listenfd(port);
    while (1) {
	int *connfdp = Malloc(sizeof(int));
	struct hostent *hp;
	char *haddrp;
	int client_port;

	*connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
	/* determine the domain name and IP address of the client */
	hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			   sizeof(clientaddr.sin_addr.s_addr), AF_INET);
	haddrp = inet_ntoa(clientaddr.sin_addr);
	client_port = ntohs(clientaddr.sin_port);
	printf("Server connected to %s (%s), port %d\n",
	   hp->h_name, haddrp, client_port);
	Pthread_create(&tid, NULL, proxy_thread, connfdp);
    }
    fclose(fp);
	exit(0);
}


/* thread routine */
void *proxy_thread(void *vargp) 
{
    char prefix[40];
    pthread_t tid = pthread_self();
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self()); 
    Free(vargp);

    printf("Served by thread %u\n", (unsigned int) tid);
    sprintf(prefix, "Thread %u ", (unsigned int) tid);
    proxy2(connfd, prefix);
    Close(connfd);
    return NULL;
}


int bytecnt = 0;

void proxy(int connfd, char *prefix) 
{
    size_t n; 
    char buf[MAXLINE]; 
    rio_t rio;
    rio_t rio2;
    int fdProxyToServer;
    sem_t mutex;
    sem_init(&mutex, 0, 1);


    fdProxyToServer = Open_clientfd("localhost", 4190);
    Rio_readinitb(&rio2, fdProxyToServer);


    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
	sem_wait(&mutex);
	bytecnt += n;
	printf("%sreceived %d bytes (%d total)\n", prefix, (int) n, bytecnt);
	
	Rio_writen(fdProxyToServer, buf, strlen(buf));
	fwrite("[C] ", strlen("[C] "), 1, fp); 
	fwrite(buf, strlen(buf), 1, fp);
	fflush(fp);
	Rio_writen(connfd, buf, n);
	fwrite("[S] ", strlen("[S] "), 1, fp); 
	fwrite(buf, strlen(buf), 1, fp);
	fflush(fp);
	sem_post(&mutex);
    }
    Close(fdProxyToServer);
}


void proxy2(int connfd, char *prefix) 
{
    size_t n,n2; 
    char buf[MAXLINE], buf2[MAXLINE]; 
    rio_t rio;
    rio_t rio2;
    int fdProxyToServer;


    fdProxyToServer = Open_clientfd("localhost", 4190);
    Rio_readinitb(&rio2, fdProxyToServer);
    Rio_readinitb(&rio, connfd);

    while (1)
    {
	n = Rio_readlineb(&rio, buf, MAXLINE); // data -> read 
	if (n==0)
		break;
	fwrite("[C] ", strlen("[C] "), 1, fp); 
	fwrite(buf, strlen(buf), 1, fp);
	fflush(fp);
	Rio_writen(fdProxyToServer, buf, strlen(buf)); // write -> data

	n2 = Rio_readlineb(&rio2, buf2, MAXLINE); // read <- data
	if (n2==0)
		break;
	fwrite("[S] ", strlen("[S] "), 1, fp); 
	fwrite(buf2, strlen(buf2), 1, fp);
	fflush(fp);
	Rio_writen(connfd, buf2, strlen(buf2)); // data <- write 
    }
    Close(fdProxyToServer);
}
