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

# The following dir is defined in configure.ac
libdir = $(audacious_effect_plugin_dir)

I_LIBS =  $(gtk3_LIBS) $(audacious_LIBS) -laudgui

include plugins.make

AM_CPPFLAGS += $(gtk3_CFLAGS) $(audacious_CFLAGS) -DAUDACIOUS

if BUILD_AUDACIOUS36
AM_CPPFLAGS += -DAUDACIOUS36 -fvisibility=hidden
endif

if BUILD_AUDACIOUS39
AM_CPPFLAGS += -DAUDACIOUS39 -fvisibility=hidden
endif

DSP_PLUGINS = librevmodel.la libmbcomp.la libstenh.la libstrev.la libprogenitor.la libgd_lroom.la libearlyr.la libearlyr2.la \
	libnoise1.la libzrev1.la libzrev2.la libnrev.la libimpulser2.la

if BUILD_AUDACIOUS
lib_LTLIBRARIES = $(DSP_PLUGINS)
noinst_LTLIBRARIES = libcommon.la
else
lib_LTLIBRARIES =
endif

if BUILD_AUDACIOUS36
libcommon_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libcommon_la_SOURCES = so36.cpp so.h libxmmsplugin.hpp libxmmsplugin_table.hpp
else
libcommon_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libcommon_la_SOURCES = so.c so.h libxmmsplugin.hpp libxmmsplugin_table.hpp
endif

librevmodel_la_LDFLAGS = -module -avoid-version -Wl,-Bsymbolic
librevmodel_la_LIBADD = $(I_LIBS) libcommon.la
librevmodel_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
librevmodel_la_SOURCES = revmodel.cpp

libmbcomp_la_LDFLAGS = -module -avoid-version
libmbcomp_la_LIBADD = $(I_LIBS) libcommon.la
libmbcomp_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libmbcomp_la_SOURCES = compressor.cpp

libstenh_la_LDFLAGS = -module -avoid-version
libstenh_la_LIBADD = $(I_LIBS) libcommon.la
libstenh_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libstenh_la_SOURCES = stenh.cpp

libstrev_la_LDFLAGS = -module -avoid-version
libstrev_la_LIBADD = $(I_LIBS) libcommon.la
libstrev_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libstrev_la_SOURCES = strev.cpp

libprogenitor_la_LDFLAGS = -module -avoid-version
libprogenitor_la_LIBADD = $(I_LIBS) libcommon.la
libprogenitor_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libprogenitor_la_SOURCES = progenitor.cpp

libgd_lroom_la_LDFLAGS = -module -avoid-version
libgd_lroom_la_LIBADD = $(I_LIBS) libcommon.la
libgd_lroom_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libgd_lroom_la_SOURCES = gd_largeroom.cpp

libearlyr_la_LDFLAGS = -module -avoid-version
libearlyr_la_LIBADD = $(I_LIBS) libcommon.la
libearlyr_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libearlyr_la_SOURCES = earlyr.cpp

libearlyr2_la_LDFLAGS = -module -avoid-version
libearlyr2_la_LIBADD = $(I_LIBS) libcommon.la
libearlyr2_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libearlyr2_la_SOURCES = earlyr2.cpp

libnoise1_la_LDFLAGS = -module -avoid-version
libnoise1_la_LIBADD = $(I_LIBS) libcommon.la
libnoise1_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libnoise1_la_SOURCES = noise.cpp

libzrev1_la_LDFLAGS = -module -avoid-version
libzrev1_la_LIBADD = $(I_LIBS) libcommon.la
libzrev1_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libzrev1_la_SOURCES = zrev1.cpp

libzrev2_la_LDFLAGS = -module -avoid-version
libzrev2_la_LIBADD = $(I_LIBS) libcommon.la
libzrev2_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libzrev2_la_SOURCES = zrev2.cpp

libnrev_la_LDFLAGS = -module -avoid-version
libnrev_la_LIBADD = $(I_LIBS) libcommon.la
libnrev_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
libnrev_la_SOURCES = nrev.cpp

libimpulser2_la_LDFLAGS = -module -avoid-version
libimpulser2_la_LIBADD = $(I_LIBS) ../libgdither/libgdither.la $(sndfile_LIBS)
libimpulser2_la_CPPFLAGS = -DAUDACIOUS $(AM_CPPFLAGS)
if BUILD_AUDACIOUS36
libimpulser2_la_SOURCES = CFileLoader.hpp  CFileLoader_t.hpp impulser2.cpp wave.xpm so36.cpp
else
libimpulser2_la_SOURCES = CFileLoader.hpp  CFileLoader_t.hpp impulser2.cpp wave.xpm so.c
endif
