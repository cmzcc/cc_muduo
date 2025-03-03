#pragma once
/*
noncopyable被继承之后，派生类对象可以正常的构造和析构，
但无法进行拷贝构造和赋值操作
eg:
noncopyable obj1;
noncopyable obj2 = obj1;  // 错误：拷贝构造函数被删除
obj2 = obj1;              // 错误：拷贝赋值运算符被删除
为了保证资源唯一性
*/
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};