#  freeverb3 Makefile.am
#
#  Copyright (C) 2006-2018 Teru Kamogashira
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

include ../freeverb/src.make
include ../libsamplerate2/src.make

if BUILD_STATIC
AM_LDFLAGS = -static
endif

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/freeverb -I$(top_srcdir)/libsamplerate2

if BUILD_FLOAT
noinst_LTLIBRARIES = libfv3_float.la libsr2_float.la
endif

libfv3_float_la_CPPFLAGS = -DLIBFV3_FLOAT $(AM_CPPFLAGS)
libfv3_float_la_SOURCES = $(FV3_SRC)

libsr2_float_la_CPPFLAGS = -DLIBSRATE2_FLOAT $(AM_CPPFLAGS)
libsr2_float_la_SOURCES = $(SR2_SRC)
