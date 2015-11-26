/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Jintao Hu,  5130379046 
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"

sem_t mutex1;
sem_t mutex2;

typedef struct 
{
  int connfd;
  struct sockaddr_in sockaddr;
} faddr;


int parse_uri(char *uri, char *target_addr, char *path, int *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
ssize_t Rio_readnb_w(rio_t *rp, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *ptr, size_t maxlen);
void Rio_writen_w(int fd, void *ptr, size_t n);
int Open_clientfd_ts(char *hostname, int port);
void *thread(void *arg);
void doit(int connfd, struct sockaddr_in *sockaddr);
void clienterror(int fd, char *cause, int errnum, char *shortmsg, char *longmsg);

int Open_clientfd_ts(char *hostname, int port)
{
	int clientfd;
	struct hostent *hp;
	struct sockaddr_in serveraddr;

	if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "%s: %s\n", "Open_clientfd_ts error", strerror(errno));
		exit(0);
	}
	
	P(&mutex2);
	if ((hp = gethostbyname(hostname)) == NULL)
	{ 
		fprintf(stderr, "%s: DNS error %d\n", "Open_clientfd_ts error", h_errno);
		exit(0);
	}
	V(&mutex2);

	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
	serveraddr.sin_port = htons(port);

	if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		fprintf(stderr, "%s: %s\n", "Open_clientfd_ts error", strerror(errno));
		exit(0);
	}
	return clientfd;
}

void clienterror(int fd, char *cause, int errnum, char *shortmsg, char *longmsg)
{
  	char buf[MAXLINE], body[MAXBUF];
	int l = strlen(longmsg);

	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%n: %s\r\n", body, &errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  	sprintf(buf, "HTTP/1.1 %d %s\r\n", errnum, shortmsg);
  	Rio_writen_w(fd, buf, strlen(buf));
  	sprintf(buf, "Content-type: text/html\r\n");
  	Rio_writen_w(fd, buf, strlen(buf));
  	sprintf(buf, "Content-length: %d\r\n\r\n", l);
  	Rio_writen_w(fd, buf, strlen(buf));
  	Rio_writen_w(fd, longmsg, strlen(longmsg));
}

void doit(int connfd, struct sockaddr_in *sockaddr)
{
  	rio_t rio;
	FILE *log_file;
  	char buf[MAXLINE], req[MAXLINE];
  	char host[MAXLINE], path[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  	int  port;
	Rio_readinitb(&rio, connfd);  	
	Rio_readlineb_w(&rio, req, MAXLINE);
  
	sscanf(req, "%s %s %s", method, uri, version);
  	if(strcmp(method, "GET"))
  	{
    		clienterror(connfd, method, 501, "Not Implemented", "Tiny does not implement this method!");
  	  	return;
  	}
	parse_uri(uri, host, path, &port);
	sprintf(req,"%s %s %s\r\n",method, path, version);  	

  	int sclientfd = Open_clientfd_ts(host, port);
  	rio_t srio;
  	Rio_readinitb(&srio, sclientfd);
  	Rio_writen_w(sclientfd ,req, strlen(req)); 
  	while (strcmp(buf,"\r\n"))
  	{
		Rio_readlineb_w(&rio, buf, MAXLINE);
    		Rio_writen_w(sclientfd ,buf, strlen(buf));
  	}
  	
  	int sum = 0;
  	int size;
  	while((size = Rio_readnb_w(&srio, buf, MAXLINE)) != 0)
  	{
    		sum = sum + size;
    		Rio_writen_w(connfd, buf, size);
  	}

  	format_log_entry(buf, sockaddr, uri, sum);
  	P(&mutex1);
	log_file = fopen("proxy.log", "a");
  	fprintf(log_file, "%s\n", buf);
  	fflush(log_file);
  	V(&mutex1);
  	Close(sclientfd);
}

void *thread(void *vargp)
{
	static int count = 1;
	Pthread_detach(pthread_self());
	printf("thread%d\n", count++);
	faddr *p = (faddr *)vargp;
  	int connfd = p->connfd;
  	struct sockaddr_in sockaddr = p->sockaddr;
  	free(vargp);
  	doit(connfd, &sockaddr);
  	Close(connfd);
  	return 0;
}

int main(int argc, char **argv)
{
	int listenfd, connfd, port;
	socklen_t clientlen;
	struct sockaddr_in clientaddr;
	pthread_t tid;

	if (argc != 2) {
  		fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
      		exit(0);
    	}

	signal(SIGPIPE, SIG_IGN);
	Sem_init(&mutex1, 0, 1);
    	Sem_init(&mutex2, 0, 1);
    	port = atoi(argv[1]);
   	listenfd = Open_listenfd(port);

    	while(1)
    	{
		clientlen = sizeof(clientaddr);
      		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
      		faddr *client = (faddr *)malloc(sizeof(faddr));
      		client->connfd = connfd;
      		client->sockaddr = clientaddr;
      		Pthread_create(&tid, 0, thread, (void *)client);
    	}
    	exit(0);
}

ssize_t Rio_readnb_w(rio_t *rp, void *ptr, size_t nbytes)
{
	ssize_t n;
	if ((n = rio_readnb(rp, ptr, nbytes)) < 0)
	{
		fprintf(stderr, "Rio_readnb error: %s\n", strerror(errno));
		return 0;
	}
	return n;
}
	

ssize_t Rio_readlineb_w(rio_t *rp, void *ptr, size_t maxlen)
{
	ssize_t n;
	if ((n = rio_readlineb(rp, ptr, maxlen)) < 0)
	{
		fprintf(stderr, "Rio_readlineb error: %s\n", strerror(errno));
		return 0;
	}
	return n;
}

void Rio_writen_w(int fd, void *ptr, size_t n)
{
	if (rio_writen(fd, ptr, n) != n)
		fprintf(stderr, "Rio_writen error: %s\n", strerror(errno));
}

/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return 0 if there are any problems.
 */

int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
	char *hostbegin;
    	char *hostend;
    	char *pathbegin;
    	int len;
	
    	if (strncasecmp(uri, "http://", 7) != 0) {
        	hostname[0] = '\0';
        	return -1;
    	}
	
    	/* Extract the host name */
    	hostbegin = uri + 7;
    	hostend = strpbrk(hostbegin, " :/\r\n\0");
    	len = hostend - hostbegin;
    	strncpy(hostname, hostbegin, len);
    	hostname[len] = '\0';

    	/* Extract the port number */
    	*port = 80; /* default */
    	if (*hostend == ':')   
        	*port = atoi(hostend + 1);

    	/* Extract the path */
    	pathbegin = strchr(hostbegin, '/');
    	if (pathbegin == NULL) {
        	pathname[0] = '\0';
   	 }
    	else {
        	strcpy(pathname, pathbegin);
   	}

    	return 1;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %d", time_str, a, b, c, d, uri, size);
}



