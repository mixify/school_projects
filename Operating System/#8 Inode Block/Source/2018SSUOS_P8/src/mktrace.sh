#!/bin/sh
rm -rf cscope.files cscope.files
rm -rf tags

find . \( -name '*.c' -o -name '*.cpp' -o -name '*.cc' -o -name '*.h' -o -name '*.s' -o -name '*.S' -o -name '*.asm' \) -print > cscope.files
ctags -R

cscope -i cscope.files
