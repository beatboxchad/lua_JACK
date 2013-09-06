#!/bin/bash

gcc -g -c -fPIC -Wall liblua_jack.c

gcc -shared -Wl,--export-dynamic,-soname,liblua_jack.so.1 -o liblua_jack.so.1.0 liblua_jack.o -ljack

rm liblua_jack.o
