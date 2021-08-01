#ifndef WEBTINY_H
#define WEBTINY_H

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<errno.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/socket.h>
#include<netdb.h>
#include<sys/wait.h>

typedef struct sockaddr SA;
extern char **environ;

#define	MAXLINE	 8192  /* Max text line length */
#define MAXBUF   8192  /* Max I/O buffer size */
#define LISTENQ  1024  /* Second argument to listen() */

#define RIO_BUFSIZE 8192

typedef struct {
    int rio_fd;                /* Descriptor for this internal buf */
    int rio_cnt;               /* Unread bytes in internal buf */
    char *rio_bufptr;          /* Next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE]; /* Internal buffer */
} rio_t;

int Open_listenfd(char* port);
int Accept(int listenfd,SA* clientaddr,socklen_t* clilen);
void Getnameinfo(const SA* addr,socklen_t addrlen,char* hostname,size_t hostlen,char* serv,size_t servlen,int flag);
void Rio_readinitb(rio_t* rp,int fd);
ssize_t Rio_readlineb(rio_t* rp,void* userbuf,size_t maxlen);
void Rio_writen(int fd,void* userbuf,size_t n);
int Open(const char* filename,int flag,mode_t mode);
void Close(int fd);
void* Mmap(void* start,size_t length,int port,int flags,int fd,off_t offset);
void Munmap(void* start,size_t length);//inside:success returns 1,faile returns -1//
pid_t Fork();
int Dup(int oldfd);
void Execve(const char *filename, const char* argv[], const char* envp[]);
int Waitpid(pid_t pid,int* statloc,int options);

#endif