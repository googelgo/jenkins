#!/bin/sh


find . -name "*.h" -o -name "*.c" > cscope.files

#escope -bkq xxx
#-b: 只生成索引文件，不进入cscope的界面
#-k: 在生成索引文件时，不搜索/usr/include目录
#-q: 生成cscope.in.out和cscope.po.out文件，加快cscope的索引速度

cscope -bkq -i cscope.files

ctags -R