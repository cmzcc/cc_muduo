#!/bin/bash

set -e

if [ ! -d `pwd`/build ];then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build &&
    cmake .. &&
    make
#回到项目根目录
cd ..

#把头文件拷贝到/usr/include/cc_muduo    so库拷贝到 /usr/lib  PATH
if [ ! -d /usr/include/cc_muduo ]; then 
    mkdir /usr/include/cc_muduo
fi

for header in `ls *.h`
do
    cp $header /usr/include/cc_muduo
done

cp `pwd`/build/libcc_muduo.so /usr/lib

ldconfig