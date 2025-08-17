#!/bin/bash -x

version="6.0.14"

./configure CFLAGS="-g3 -O0 -Wno-format-overflow" \
--with-compression=gzip \
--with-owner=games \
--with-group=games \
--enable-realtime-on-botl \
--enable-score-on-botl \
--enable-simple-mail \
--enable-livelog \
--enable-livelog-shout \
--enable-livelog-killing \
--enable-whereis-file="/unnethack-$version/var/whereis/%n.whereis" \
--enable-dump-file="/dgldir/userdata/%N/%n/unnethack/dumplog/%t.un.txt" \
--enable-dump-html \
--enable-dump-html-css-file="/unnethack-$version/unnethack_dump.css" \
--enable-dump-html-css-embedded \
--enable-sysconf \
--enable-curses-graphics \
--disable-wizmode \
--prefix=/unnethack-$version \
--datarootdir=/unnethack-$version \
--localstatedir=/unnethack-$version/var \
--with-gamesdir=/unnethack-$version/var \
--with-leveldir=/unnethack-$version/var \
--with-savesdir=/unnethack-$version/var/save \
--with-sharedir=/unnethack-$version \
--with-unsharedir=/unnethack-$version \
--docdir=/unnethack-$version/doc
