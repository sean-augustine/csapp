#include"webtiny.h"
//function declare//
int open_listenfd(char* port);
void rio_readinitb(rio_t* rp,int fd);
ssize_t rio_readlineb(rio_t* rp,void* userbuf,size_t maxlen);
ssize_t rio_writen(int fd,void* userbuf,size_t n);

//error functions//
void unix_err(char *msg)
{
    fprintf(stderr,"%s: %s\n",msg,strerror(errno));
    exit(0);
}

void app_err(char *msg)
{
    fprintf(stderr,"%s\n",msg);
    exit(0);
}

void gai_err(int code,char* msg)//getaddrinfo error
{
    fprintf(stderr,"%s: %s\n",msg,gai_strerror(code));
    exit(0);
}

//wrapper functions//
int Open_listenfd(char* port)
{
    int fd;
    fd=open_listenfd(port);
    return fd;
}

int Accept(int listenfd,SA* clientaddr,socklen_t* clilen)
{
    int connfd;
    if((connfd=accept(listenfd,clientaddr,clilen))<0)
    unix_err("accept error");
    return connfd;
}

void Getnameinfo(const SA* addr,socklen_t addrlen,char* hostname,size_t hostlen,char* serv,size_t servlen,int flag)
{
    int code;
    if((code=getnameinfo(addr,addrlen,hostname,hostlen,serv,servlen,flag))!=0)
    {
        gai_err(code,"getnameinfo error");
    }
}

void Rio_readinitb(rio_t* rp,int fd)
{
    rio_readinitb(rp,fd);
}

ssize_t Rio_readlineb(rio_t* rp,void* userbuf,size_t maxlen)
{
    size_t rc;
    if((rc=rio_readlineb(rp,userbuf,maxlen))<0)
    {
        unix_err("rio_readlineb error");
    }
    return rc;
}

void Rio_writen(int fd,void* userbuf,size_t n)
{
    if(rio_writen(fd,userbuf,n)!=n)
    {
        unix_err("rio_writen error");
    }
}

int Open(const char* filename,int flag,mode_t mode)
{
    int rc;
    if((rc=open(filename,flag,mode))<0)
    unix_err("open error");
    return rc;
}

void Close(int fd)
{
    if(close(fd)<0)
    unix_err("close error");
}

void* Mmap(void* start,size_t length,int port,int flags,int fd,off_t offset)
{
    void* ptr;
    if((ptr=mmap(start,length,port,flags,fd,offset))==MAP_FAILED)
    unix_err("mmap error");
    return ptr;
}
void Munmap(void* start,size_t length)//inside:success returns 1,faile returns -1//
{
    if(munmap(start,length)<0)
    unix_err("munmap error");
}

pid_t Fork()
{
    int rc;
    if((rc=fork())<0)
    unix_err("fork error");
    return rc;
}

int Dup(int oldfd)
{
    int rc;
    if((rc=dup(oldfd))<0)
    unix_err("dup error");
    return rc;
}

void Execve(const char *filename, const char* argv[], const char* envp[])
{
    if(execve(filename,argv,envp)<0)
    unix_err("execve error");
}

int Waitpid(pid_t pid,int* statloc,int options)
{
    int cpid;
    if((cpid=waitpid(pid,statloc,options))<0)
    unix_err("waitpid error");
    return cpid;
}

//selfdefine functions//

int open_listenfd(char* port)
{
    int n,listenfd;
    const int on=1;
    struct addrinfo hints,*res,*resave;
    bzero(&hints,sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_NUMERICSERV|AI_PASSIVE|AI_ADDRCONFIG;
    if(n=getaddrinfo(NULL,port,&hints,&res)!=0)
        gai_err(n,"getaddrinfo error");
    resave=res;
    for(;res!=NULL;res=res->ai_next)
    {
        listenfd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
        if(listenfd<0)
        continue;
        setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
        if(bind(listenfd,res->ai_addr,res->ai_addrlen)==0)
        break;
        Close(listenfd);
    }
    if(res==NULL)
    app_err("open_listenfd error");
    if(listen(listenfd,LISTENQ)<0)
    {
        Close(listenfd);
        unix_err("listen error");
    }
    freeaddrinfo(resave);
    return listenfd;
}
static ssize_t rio_read(rio_t *rp,char* userbuf,size_t n)
{
    int cnt;
    while(rp->rio_cnt<=0){
        rp->rio_cnt=read(rp->rio_fd,rp->rio_buf,sizeof(rp->rio_buf));
        if(rp->rio_cnt<0)
        {
            if(errno!=EINTR)
            return -1;
        }
        else if(rp->rio_cnt==0)
        return 0;
        else
        rp->rio_bufptr=rp->rio_buf;
    }
    cnt=n;
    if(rp->rio_cnt<n)
    cnt=rp->rio_cnt;
    memcpy(userbuf,rp->rio_bufptr,cnt);
    rp->rio_bufptr+=cnt;
    rp->rio_cnt-=cnt;
    return cnt;
}

void rio_readinitb(rio_t* rp,int fd)
{
    rp->rio_fd=fd;
    rp->rio_cnt=0;
    rp->rio_bufptr=rp->rio_buf;
}

ssize_t rio_readlineb(rio_t* rp,void* userbuf,size_t maxlen)
{
    int n,rc;
    char c,*bufp=userbuf;
    for(n=1;n<maxlen;++n)//保留最后一位为空字符
    {
        if(rc=rio_read(rp,&c,1)==1)
        {
            *bufp++=c;
            if(c=='\n')
            {
                ++n;//?????
                break;
            }
        }
        else if(rc==0)
        {
            if(n==1)//eof,no data read;
            return 0;
            else
            break; //eof,somedata was read;
        }
        else
        {
            return -1;
        }
    }
    *bufp='\0';
    return n-1;
}

ssize_t rio_writen(int fd,void* userbuf,size_t n)
{
    size_t nleft=n;
    ssize_t nwrittrn;
    char* bufp=userbuf;
    while(nleft>0)
    {
        if((nwrittrn=write(fd,bufp,nleft))<=0)
        {
            if(errno!=EINTR)
            {
                return -1;
            }
            else
            nwrittrn=0;
        }
        nleft-=nwrittrn;
        bufp+=nwrittrn;
    }
    return n;
}