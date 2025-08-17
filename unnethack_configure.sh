#!/bin/bash -x

version="6.0.14"

./configure CFLAGS="-g3 -O0 -Wno-format-overflow" \
--with-compression=gzip \
--enable-realtime-on-botl \
--enable-score-on-botl \
--enable-simple-mail \
--enable-livelog \
--enable-livelog-shout \
--enable-livelog-killing \
--enable-whereis-file="/unnethack-$version/var/whereis/%n.whereis" \
--enable-dump-file="/dgldir/userdata/%N/%n/unnethack/dumplog/%t.un.txt" \
--enable-dump-html \
--enable-dump-html-css-embedded="/unnethack-$version/unnethack_dump.css" \
--enable-sysconf \
--enable-curses-graphics \
--disable-wizmode \
--prefix=/unnethack-$version \
--datarootdir=$prefix \
--localstatedir=$prefix/var \
--with-gamesdir=$localstatedir \
--with-sharedir=$datarootdir \
--with-unsharedir=$datarootdir \
--docdir=$datarootdir/doc
