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

SUBDIRS =

AM_LDFLAGS = -static

I_LIBS = $(jack_LIBS) $(gtk3_LIBS)

include $(top_srcdir)/audacious/plugins.make

AM_CPPFLAGS += $(jack_CFLAGS) $(gtk3_CFLAGS) -I$(top_srcdir)/samples

if BUILD_JACK
noinst_LTLIBRARIES = libbmp_int.la libjack_common.la
bin_PROGRAMS = fv3_jack_strev fv3_jack_nrev fv3_jack_revmodel fv3_jack_impulser2 fv3_jack_mbcomp fv3_jack_stenh \
 fv3_jack_progenitor fv3_jack_gd_lroom fv3_jack_earlyr fv3_jack_earlyr2 fv3_jack_noise1
endif

libbmp_int_la_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
libbmp_int_la_SOURCES = configdb.h configdb_rcfile.c rcfile.c rcfile.h util.c util.h
libjack_common_la_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
libjack_common_la_SOURCES = plugin.h jack_main.cpp ../samples/CArg.cpp ../samples/CArg.hpp
libjack_common_la_LIBADD = $(jack_LIBS) $(gtk3_LIBS)

I_LIBS += libbmp_int.la libjack_common.la

fv3_jack_strev_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_strev_SOURCES = ../audacious/strev.cpp
fv3_jack_strev_LDADD = $(I_LIBS)

fv3_jack_nrev_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_nrev_SOURCES = ../audacious/nrev.cpp
fv3_jack_nrev_LDADD = $(I_LIBS)

fv3_jack_revmodel_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_revmodel_SOURCES = ../audacious/revmodel.cpp
fv3_jack_revmodel_LDADD = $(I_LIBS)

fv3_jack_impulser2_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_impulser2_SOURCES = ../audacious/CFileLoader.hpp  ../audacious/CFileLoader_t.hpp ../audacious/impulser2.cpp \
 ../audacious/wave.xpm
fv3_jack_impulser2_LDADD = $(I_LIBS) $(top_builddir)/libgdither/libgdither.la $(sndfile_LIBS)

fv3_jack_mbcomp_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_mbcomp_SOURCES = ../audacious/compressor.cpp
fv3_jack_mbcomp_LDADD = $(I_LIBS)

fv3_jack_stenh_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_stenh_SOURCES = ../audacious/stenh.cpp
fv3_jack_stenh_LDADD = $(I_LIBS)

fv3_jack_progenitor_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_progenitor_SOURCES = ../audacious/progenitor.cpp
fv3_jack_progenitor_LDADD = $(I_LIBS)

fv3_jack_gd_lroom_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_gd_lroom_SOURCES = ../audacious/gd_largeroom.cpp
fv3_jack_gd_lroom_LDADD = $(I_LIBS)

fv3_jack_earlyr_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_earlyr_SOURCES = ../audacious/earlyr.cpp
fv3_jack_earlyr_LDADD = $(I_LIBS)

fv3_jack_earlyr2_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_earlyr2_SOURCES = ../audacious/earlyr2.cpp
fv3_jack_earlyr2_LDADD = $(I_LIBS)

fv3_jack_noise1_CPPFLAGS = -DJACK $(AM_CPPFLAGS)
fv3_jack_noise1_SOURCES = ../audacious/noise.cpp
fv3_jack_noise1_LDADD = $(I_LIBS)
