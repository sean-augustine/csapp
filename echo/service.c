#include"csapp.h"

void echo(int connfd)
{
    size_t n;
    char buff[MAXLINE];
    rio_t rio;
    rio_readinitb(&rio,connfd);
    while((n=rio_readlineb(&rio,buff,MAXLINE))!=0)
    {
        printf("server received %d bytes\n",(int)n);
        rio_writen(connfd,buff,n);
    }
}

int main(int argc, char **argv)
{
    int listenfd,connfd;
    socklen_t clientlen;
    struct sockaddr_storage clinetaddr;
    char clinet_hostname[MAXLINE],clinet_port[MAXLINE];
    if(argc!=2)
    {
        fprintf(stderr,"usage: %s <port>\n",argv[0]);
        exit(0);
    }

    listenfd=Open_listenfd(argv[1]);
    while(1)
    {
        clientlen=sizeof(struct sockaddr_storage);
        connfd=Accept(listenfd,(SA*)&clinetaddr,&clientlen);
        Getnameinfo((SA*)&clinetaddr,clientlen,clinet_hostname,MAXLINE,clinet_port,MAXLINE,0);
        printf("connected to (%s,%s)\n",clinet_hostname,clinet_port);
        echo(connfd);
        close(connfd);
    }
}