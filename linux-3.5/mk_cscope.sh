#!/bin/sh
find ./ -name "*.c" -o -name "*.h" > cscope.files
cscope -bkq -i cscope.files
