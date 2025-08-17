# LUA setup (debian-based system, one-time only)
apt install lua5.4 liblua5.4-dev

# configure (old method)
#./configure CFLAGS="-g3 -O0 -Wno-format-overflow" --with-compression=gzip --enable-realtime-on-botl --enable-score-on-botl --enable-simple-mail --enable-livelog --enable-livelog-shout --enable-livelog-killing --enable-whereis-file --enable-dump-file --enable-dump-html --enable-dump-html-css-embedded --enable-sysconf --enable-curses-graphics --disable-wizmode --prefix=/unnethack-6.0.14

# configure script
./unnethack_configure.sh

# misc
To capture compiler output - make all &> output.txt
