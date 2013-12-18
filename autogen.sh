#!/bin/sh
#
libtoolize --copy --force
aclocal -I m4 --force
automake -f --add-missing --copy
autoconf -I m4
autoheader

# Run configure for this platform
#./configure $*

