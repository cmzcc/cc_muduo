#include"Buffer.h"
#include <sys/uio.h>

#include<errno.h>
#include<unistd.h>
//从fd上读取数据     Pooler工作在LT模式
//Buffer缓冲区是有大小的，但是从fd上读数据的时候，却不知道tcp数据的最终大小
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536]={0};//栈上的内存空间
    struct iovec vec[2];
    const size_t writeable=writableBytes();//这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base=begin()+writerIndex_;
    vec[0].iov_len=writeable;

    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof extrabuf;

    const int iovcnt=(writeable<sizeof extrabuf)?2:1;
    const ssize_t n=::readv(fd,vec,iovcnt);
    if (n<0)
    {
        *saveErrno=errno;
    }
    else if (n<=writeable)//Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_+=n;

    }
    else//extrabuf里面也写了数据
    {
        writerIndex_=buffer_.size();
        append(extrabuf,n-writeable);//writerIndex_开始写n-writeable大小的数据


    }
    return n;
    
}
ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n=::write(fd,peek(),readableBytes());
    if (n<0)
    {
        *saveErrno=errno;
    }
    return n;
    
}