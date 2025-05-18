#!/bin/bash

# LUA setup (one time):
#git submodule update --init
#git submodule add -b v5.4 https://github.com/lua/lua.git liblua

# LUA setup (every time):
(cd liblua ; git pull origin v5.4 ; make)
export LUA=`pwd`/liblua/lua
export LUA_INCLUDE=-I`pwd`/liblua
export LUA_LIB="`pwd`/liblua/liblua.a -lm"
