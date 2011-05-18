#!/bin/sh

cd "`dirname $0`"
git log > ChangeLog
[ -d m4 ] || mkdir m4
autoreconf -f -i -s
