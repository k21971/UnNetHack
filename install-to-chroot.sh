#!/bin/bash
#
# Ideas and some parts from the original dgl-create-chroot (by joshk@triplehelix.org, modifications by jilles@stack.nl)
# More by <paxed@alt.org>
# More by Michael Andrew Streib <dtype@dtype.org>
#
# Adapted for install of unnethack in NAO-Style chroot by Tangles
#
# Licensed under the MIT License
# https://opensource.org/licenses/MIT

# autonamed chroot directory. Can rename.
DATESTAMP=`date +%Y%m%d-%H%M%S`
NAO_CHROOT="/opt/nethack/chroot"
NETHACK_GIT="/home/build/UnNetHack"
# the user & group from dgamelaunch config file.
USRGRP="games:games"
# COMPRESS from include/config.h; the compression binary to copy. leave blank to skip.
COMPRESSBIN="/bin/gzip"
# fixed data to copy (leave blank to skip)
NH_GIT="/home/build/UnNetHack"
NH_BRANCH="master"
# HACKDIR from include/config.h; aka nethack subdir inside chroot
NHSUBDIR="unnethack-6.0.13"
# VAR_PLAYGROUND from include/unixconf.h
NH_VAR_PLAYGROUND="/unnethack-6.0.13/var"
# nhdat location
NHDAT_DIR="/unnethack-6.0.13"
# END OF CONFIG
##############################################################################

errorexit()
{
    echo "Error: $@" >&2
    exit 1
}

findlibs()
{
  for i in "$@"; do
      if [ -z "`ldd "$i" | grep 'not a dynamic executable'`" ]; then
         echo $(ldd "$i" | awk '{ print $3 }' | egrep -v ^'\(' | grep lib)
         echo $(ldd "$i" | grep 'ld-linux' | awk '{ print $1 }')
      fi
  done
}

set -e

umask 022

echo "Creating inprogress and userdata directories"
mkdir -p "$NAO_CHROOT/dgldir/inprogress-un6013"
chown "$USRGRP" "$NAO_CHROOT/dgldir/inprogress-un6013"
mkdir -p "$NAO_CHROOT/dgldir/extrainfo-un"
chown "$USRGRP" "$NAO_CHROOT/dgldir/extrainfo-un"

echo "Making $NAO_CHROOT/$NHSUBDIR"
mkdir -p "$NAO_CHROOT/$NHSUBDIR"

echo "Creating NetHack variable dir stuff."
mkdir -p "$NAO_CHROOT$NH_VAR_PLAYGROUND"
mkdir -p "$NAO_CHROOT$NH_VAR_PLAYGROUND/save"
mkdir -p "$NAO_CHROOT$NH_VAR_PLAYGROUND/save/backup"
mkdir -p "$NAO_CHROOT$NH_VAR_PLAYGROUND/bones"
mkdir -p "$NAO_CHROOT$NH_VAR_PLAYGROUND/whereis"
touch "$NAO_CHROOT$NH_VAR_PLAYGROUND/logfile"
touch "$NAO_CHROOT$NH_VAR_PLAYGROUND/perm"
touch "$NAO_CHROOT$NH_VAR_PLAYGROUND/record"
touch "$NAO_CHROOT$NH_VAR_PLAYGROUND/xlogfile"
touch "$NAO_CHROOT$NH_VAR_PLAYGROUND/livelog"

# everything created so far needs the chown.
( cd $NAO_CHROOT/$NHSUBDIR ; chown -R "$USRGRP" * )

# Everything below here should remain owned by root.
echo "Copying sysconf file"
mkdir -p "$NAO_CHROOT$NHDAT_DIR"
cp "$NETHACK_GIT/dat/sysconf" "$NAO_CHROOT$NHDAT_DIR"
chmod 644 "$NAO_CHROOT$NHDAT_DIR/sysconf"

echo "Copying NetHack nhdat and .css files"
cp "$NETHACK_GIT/dat/nhdat" "$NAO_CHROOT$NHDAT_DIR"
chmod 644 "$NAO_CHROOT$NHDAT_DIR/nhdat"
cp "$NETHACK_GIT/dat/license" "$NAO_CHROOT$NHDAT_DIR"
chmod 644 "$NAO_CHROOT$NHDAT_DIR/license"
cp "$NETHACK_GIT/dat/unnethack_dump.css" "$NAO_CHROOT$NHDAT_DIR"
chmod 644 "$NAO_CHROOT$NHDAT_DIR/unnethack_dump.css"

NETHACKBIN="$NETHACK_GIT/src/unnethack"
if [ -n "$NETHACKBIN" -a ! -e "$NETHACKBIN" ]; then
  errorexit "Cannot find unnethack binary $NETHACKBIN"
fi

if [ -n "$NETHACKBIN" -a -e "$NETHACKBIN" ]; then
  echo "Copying $NETHACKBIN"
  cd "$NAO_CHROOT/$NHSUBDIR"
  NHBINFILE="`basename $NETHACKBIN`-$DATESTAMP"
  cp "$NETHACKBIN" "$NHBINFILE"
  rm -f unnethack
  ln -s "$NHBINFILE" unnethack
  LIBS="$LIBS `findlibs $NETHACKBIN`"
  cd "$NAO_CHROOT"
fi

RECOVER="$NETHACK_GIT/util/recover"

if [ -n "$RECOVER" -a -e "$RECOVER" ]; then
  echo "Copying $RECOVER"
  cp "$RECOVER" "$NAO_CHROOT/$NHSUBDIR/var"
  LIBS="$LIBS `findlibs $RECOVER`"
  cd "$NAO_CHROOT"
fi


LIBS=`for lib in $LIBS; do echo $lib; done | sort | uniq`
echo "Copying libraries:" $LIBS
for lib in $LIBS; do
  mkdir -p "$NAO_CHROOT`dirname $lib`"
  if [ -f "$NAO_CHROOT$lib" ]
  then
    echo "$NAO_CHROOT$lib already exists - skipping."
  else
    cp $lib "$NAO_CHROOT$lib"
  fi
done

echo "Finished."

