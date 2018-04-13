#  freeverb3 Makefile.am
#
#  Copyright (C) 2006-2014 Teru Kamogashira
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

SUBDIRS = IR

AM_CPPFLAGS = \
	-I$(top_srcdir)/libsamplerate2 -I$(top_srcdir)/libgdither -I$(top_srcdir) \
	$(sndfile_CFLAGS) $(jack_CFLAGS) $(fftw3f_CFLAGS) \
	$(fftw3_CFLAGS) $(fftw3l_CFLAGS)

if BUILD_PLUGDOUBLE
AM_CPPFLAGS += -DPLUGDOUBLE
endif

if BUILD_FLOAT
AM_CPPFLAGS += -DBUILD_FLOAT
endif

if BUILD_DOUBLE
AM_CPPFLAGS += -DBUILD_DOUBLE
endif

AM_LDFLAGS = -static

I_LIBS = ../libgdither/libgdither.la ../freeverb/libfreeverb3.la \
	$(sndfile_LIBS) $(jack_LIBS)

if BUILD_PLUGDOUBLE
I_LIBS += $(fftw3_LIBS)
else
I_LIBS += $(fftw3f_LIBS)
endif

if BUILD_SAMPLE
noinst_PROGRAMS = fv3_ir_test
bin_PROGRAMS = fv3_benchmark3 fv3_rateconv fv3_impulser fv3_fq_response fv3_mlsgen
else
noinst_PROGRAMS =
bin_PROGRAMS =
endif

if BUILD_JACK
bin_PROGRAMS += fv3_impulser_jack
AM_CPPFLAGS += $(jack_CFLAGS)
I_LIBS += $(jack_LIBS)
endif

fv3_mlsgen_SOURCES = mlsgen.cpp CArg.cpp CArg.hpp
fv3_mlsgen_LDADD = $(I_LIBS)

fv3_benchmark3_SOURCES = benchmark3.cpp CArg.cpp CArg.hpp
fv3_benchmark3_LDADD = $(I_LIBS)

fv3_rateconv_SOURCES = rateconv.cpp CArg.cpp CArg.hpp
fv3_rateconv_LDADD = $(I_LIBS)

fv3_impulser_SOURCES = impulser.cpp CArg.cpp CArg.hpp
fv3_impulser_LDADD = $(I_LIBS)

fv3_impulser_jack_CPPFLAGS = $(AM_CPPFLAGS)
fv3_impulser_jack_SOURCES = impulser_jack.cpp CArg.cpp CArg.hpp
fv3_impulser_jack_LDADD = $(I_LIBS)

fv3_fq_response_SOURCES = freq_response.cpp
fv3_fq_response_LDADD = $(I_LIBS)

fv3_ir_test_SOURCES = ir_test.cpp
fv3_ir_test_LDADD = $(I_LIBS)

