
#include"webtiny.h"


void doit(int fd); //hander the http issue(by reading and analyzing the request line)
void read_requesthdrs(rio_t *rp);
int parse_uri(char* uri, char* filename, char* cgiargs);
void serve_static(int fd,char* filename,int filesize);
void get_filetype(const char* filename,char* filetype);
void serve_dynamic(int fd,char* filename,char* cgiargs);
void clienterror(int fd,char* cause,char* errnum,char* shortmsg,char* longmsg);

int main(int argc, char **argv)
{
    int listenfd,connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    if(argc!=2)
    {
        fprintf(stderr,"usage: %s <port>\n",argv[0]);
        exit(0);
    }
    listenfd=Open_listenfd(argv[1]);
    while(1)
    {
        clientlen=sizeof(clientaddr);
        connfd=Accept(listenfd,(SA*)&clientaddr,&clientlen);
        Getnameinfo((SA*)&clientaddr,clientlen,hostname,MAXLINE,port,MAXLINE,0);
        printf("Accept connection from (%s %s)\n",hostname,port);
        doit(connfd);
        close(connfd);
    }
}

void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
    char filename[MAXLINE],cgiargs[MAXLINE];
    rio_t rio;
    //read request line and headers//
    Rio_readinitb(&rio,fd);
    Rio_readlineb(&rio,buf,MAXLINE);
    printf("request headers\n");
    printf("%s",buf);
    sscanf(buf,"%s %s %s",method,uri,version);
    if(strcasecmp(method,"GET"))
    {
        clienterror(fd,method,"501","not implemented","tiny does not inplment this method");
        return;
    }
    read_requesthdrs(&rio); //忽略请求报头
    is_static=parse_uri(uri,filename,cgiargs);
    if(stat(filename,&sbuf)<0)//stat根据pathname获取文件信息
    {
        clienterror(fd,filename,"404","not found","tiny could not find this file");
        return;
    }
    if(is_static)
    {   /*S_ISREG宏定义在sys/stat.h中*/
        if(!S_ISREG(sbuf.st_mode)||!(S_IRUSR&sbuf.st_mode)) //判断是否为可读的普通文件
        {
            clienterror(fd,filename,"403","fobidden","tiny could not read this file");
            return;
        }
        serve_static(fd,filename,sbuf.st_size);
    }
    else{ //处理动态内容
        if(!S_ISREG(sbuf.st_mode)||!(S_IXUSR&sbuf.st_mode)) //判断是否为可执行的文件
        {
            clienterror(fd,filename,"403","fobidden","tiny could not run the CGI program");
            return;
        }
        serve_dynamic(fd,filename,cgiargs);
    }
}

void clienterror(int fd,char* cause,char* errnum,char* shortmsg,char* longmsg)
{
    char buf[MAXLINE],body[MAXBUF];//http响应行以及相应头部，http响应主体

    //build the http response body//
    sprintf(body,"<html><title>TINY Error</title>");
    sprintf(body,"%s<body bgcolor =""ffffff"">\r\n",body);
    sprintf(body,"%s%s: %s\r\n",body,errnum,shortmsg);
    sprintf(body,"%s<p>%s: %s\r\n",body,longmsg,cause);
    sprintf(body,"%s<hr><em>The Tiny web server</em>\r\n",body);

    //print the http respones//
    sprintf(buf,"HTTP/1.0 %s %s \r\n",errnum,shortmsg);
    Rio_writen(fd,buf,strlen(buf));
    sprintf(buf,"HTTP/1.0 %s %s \r\n",errnum,shortmsg);
    Rio_writen(fd,buf,strlen(buf));
    sprintf(buf,"HTTP/1.0 %s %s \r\n",errnum,shortmsg);
    Rio_writen(fd,body,strlen(buf));
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    Rio_readlineb(rp,buf,MAXLINE);
    while(strcmp(buf,"\r\n")){
        Rio_readlineb(rp,buf,MAXLINE);
        printf("%s",buf);
    }
    return;
}

int parse_uri(char* uri,char* filename,char* cgiargs)
{
    char *ptr;
    if(!strstr(uri,"cgi-bin"))//static_content
    {
        strcpy(filename,".");
        strcpy(cgiargs," ");
        strcat(filename,uri);
        if(uri[strlen(uri)-1]=='/')
        {
            strcat(filename,"host.html");
        }
        return 1;
    }
    else//dynamic_content
    {
        ptr=strchr(uri,'?');
        if(ptr)
        {
            *ptr='\0';
            strcpy(cgiargs,++ptr);
        }
        else
            strcpy(cgiargs," ");
        strcpy(filename,".");
        strcat(filename,uri);
        return 0;
        
    }
}

void serve_static(int fd,char* filename,int filesize)
{
    char buf[MAXLINE];
    char* filetype[MAXLINE];
    char* srcp;//used for mmap,save the beginning address
    get_filetype(filename,filetype);
    //build headers//
    sprintf(buf,"HTTP/1.1 200 OK\r\n");
    sprintf(buf,"%sServer: TINY WEB SERVER\r\n",buf);
    sprintf(buf,"%sConnection: close\r\n",buf);
    sprintf(buf,"%sContent_type: %s\r\n",buf,filetype);
    sprintf(buf,"%sContent_length: %d\r\n\r\n",buf,filesize);
    Rio_writen(fd,buf,strlen(buf));
    printf("Response headers:\n");
    printf("%s",buf);
    //build body//
    int srcfd=Open(filename,O_RDONLY,0);
    srcp=Mmap(0,filesize,PROT_READ,MAP_PRIVATE,srcfd,0);
    Close(srcfd);
    Rio_writen(fd,srcp,filesize);
    Munmap(srcp,filesize);
}

void get_filetype(const char* filename,char* filetype)
{
    if(strstr(filename,".html"))
    {
        strcpy(filetype,"text/html");
    }
    else if(strstr(filename,".gif"))
    {
        strcpy(filetype,"image/gif");
    }
    else if(strstr(filename,".png"))
    {
        strcpy(filetype,"image/png");
    }
    else if(strstr(filename,".jpg"))
    {
        strcpy(filetype,"image/jpg");
    }
    else
    {
        strcpy(filetype,"text/plain");
    }
    return;
}

void serve_dynamic(int fd,char* filename,char* cgiargs)
{
    int cpid;
    char* emptylist[]={NULL};
    char* buf[MAXLINE];
    //response headers(patial),remains to print in cgi program//
    sprintf(buf,"HTTP/1.1 200 OK\r\n");
    sprintf(buf,"%sServer: TINY WEB SERVER\r\n",buf);
    Rio_writen(fd,buf,strlen(buf));

    if((cpid=Fork())==0)
    {
        setenv("QUERY_STRING",cgiargs,1);//添加环境变量，stdlib include，1代表如果已经存在该环境变量则先删除
        Close(STDOUT_FILENO);
        Dup(fd);
        Execve(filename,emptylist,environ);
    }
    Waitpid(cpid,NULL,0);
}
