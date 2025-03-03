#pragma once

#include<vector>
#include<string>
#include<algorithm>
//网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend=8;
    static const size_t kInitialSize=1024;

    explicit Buffer(size_t initialSize=kInitialSize)
            :buffer_(kCheapPrepend+initialSize)
            ,readerIndex_(kCheapPrepend)
            ,writerIndex_(kCheapPrepend)
            {

            }
    size_t readableBytes()const
    {
        return writerIndex_-readerIndex_;
    }
    size_t writableBytes()const
    {
        return buffer_.size()-writerIndex_;
    }
    size_t prependableBytes()const
    {
        return readerIndex_;
    }

    //返回缓冲区中可读数据的起始地址  
    const char* peek()const
    {
        return begin()+readerIndex_;
    }
    //onMessage   buffer-> string 
    void retrieve(size_t len)
    {
        if (len<readableBytes())
        {
            readerIndex_ += len; // 读取了缓冲区可读部分的一部分，剩余的部分是从readerIndex_+=len到writerIndex_
        }
        else
        {
            retrieveAll();
        }
        
    }
    void retrieveAll()
    {
        readerIndex_=writerIndex_=kCheapPrepend;
    }
    //把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());//应用可读取数据的长度
    }
    std::string retrieveAsString(size_t len)
    {
      std::string result(peek(),len);
      retrieve(len);//将缓冲区中可读的数据已读取之后，这里要对缓冲区进行复位操作
      return result;
    }
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes()<len)
        {
            makeSpace(len);
        }
        
    }
    //把[data,data+len]内存上的数据，添加到writeable缓冲区当中 
    void append(const char* data,size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;
    }
    char* beginWrite()
    {
        return begin()+writerIndex_;
    }
    const char* beginWrite()const
    {
        return begin() + writerIndex_;
    }
    //从fd中读取数据
    ssize_t readFd(int fd,int*saveErrno);
    //通过Fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    void makeSpace(size_t len)
    { 
        if (writableBytes()+prependableBytes()<len+kCheapPrepend)
        {
            buffer_.resize(writerIndex_+len);
        }
        else
        {
            size_t readable=readableBytes();
            std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;
        }
    }
    char* begin()
    {
        return &*buffer_.begin();//vector底层数组首元素的地址，即数组的起始地址
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};