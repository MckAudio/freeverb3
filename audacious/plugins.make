I_LIBS += ../freeverb/libfreeverb3.la

if BUILD_PLUGDOUBLE
I_LIBS += $(fftw3_LIBS)
else
I_LIBS += $(fftw3f_LIBS)
endif

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/libgdither -I$(top_srcdir)/audacious $(fftw3f_CFLAGS) $(fftw3_CFLAGS)

if BUILD_PLUGDOUBLE
AM_CPPFLAGS += -DPLUGDOUBLE
endif

if ENABLE_PLUGINIT
AM_CPPFLAGS += -DPLUGINIT
endif
