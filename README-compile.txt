# LUA setup
One time:
 git submodule update --init
 git submodule add -b v5.4 https://github.com/lua/lua.git liblua

Every time:
 (cd liblua ; git pull ; make)
 export LUA=`pwd`/liblua/lua
 export LUA_INCLUDE=-I`pwd`/liblua
 export LUA_LIB="`pwd`/liblua/liblua.a -lm"

# configure
./configure CFLAGS="-g3 -O0 -Wno-format-overflow" --with-compression=gzip --enable-realtime-on-botl --enable-score-on-botl --enable-simple-mail --enable-livelog --enable-livelog-shout --enable-livelog-killing --enable-whereis-file --enable-dump-file --enable-dump-html --enable-dump-html-css-embedded --enable-sysconf --enable-curses-graphics --disable-wizmode --prefix=/unnethack-6.0.13

# misc
To capture compiler output - make all &> output.txt
