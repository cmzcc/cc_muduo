#pragma once

#include <iostream>

class Timestamp
{
private:
    int64_t microSecondsSinceEpoch_ ;//末尾带_表示类中的成员变量,不带表示函数参数
public:
    Timestamp();
    Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;
};