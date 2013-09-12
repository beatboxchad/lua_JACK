#!/bin/bash

gcc -g -c -fPIC -Wall lua_jack.c

gcc -shared -Wl,--export-dynamic,-soname,liblua_jack.so.1 -o liblua_jack.so.1.0 lua_jack.o -ljack

rm lua_jack.o
