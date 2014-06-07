dnl
dnl  freeverb3 configure.ac
dnl
dnl  Copyright (C) 2006-2013 Teru Kamogashira
dnl
dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 2 of the License, or
dnl  (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
dnl

m4_define([_pkg_name],freeverb3)
m4_define([_RELEASE], 3)
m4_define([_MAJOR],   1)
m4_define([_MINOR],   1)
m4_define([_VERSION],m4_defn([_RELEASE]).m4_defn([_MAJOR]).m4_defn([_MINOR])) 

AC_COPYRIGHT([Copyright (C) 2005-2013 Teru Kamogashira])
AC_INIT([_pkg_name],[_VERSION])
AC_CONFIG_SRCDIR([freeverb/fv3_defs.h])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])
AC_REQUIRE_AUX_FILE([config.rpath])

AC_CANONICAL_TARGET
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([tar-ustar dist-xz no-dist-gzip subdir-objects])

AC_PROG_MAKE_SET
AC_LIB_RPATH

AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

m4_define([_LIBVERSION],m4_eval(_RELEASE + _MAJOR))
m4_define(_LIBTOOL_VERSION_INFO,m4_defn([_LIBVERSION]):m4_defn([_MINOR]):m4_defn([_MAJOR]))
AC_SUBST(LIBTOOL_VERSION_INFO, [_LIBTOOL_VERSION_INFO])
AC_CONFIG_HEADERS(fv3_config.h)

CFLAGS="$CFLAGS -fPIC"

AC_ARG_ENABLE(profile, AC_HELP_STRING([--enable-profile], [Enable gprof for --enable-static --enable-sample (default=no)]),
  [cv_profile="$enable_profile"], [cv_profile="no"])
if test "x$cv_profile" = "xyes"; then
  CFLAGS="$CFLAGS -pg -g"
fi

dnl fftw3 ... autodetect all(float/double/long double) versions

AC_ARG_ENABLE(float, AC_HELP_STRING([--enable-float], [Enable Float (single precision) (default=auto)]), [cv_float="$enable_float"], [cv_float="yes"])
AC_ARG_ENABLE(double, AC_HELP_STRING([--enable-double], [Enable Double (double precision) (default=auto)]), [cv_double="$enable_double"], [cv_double="yes"])
AC_ARG_ENABLE(ldouble, AC_HELP_STRING([--enable-ldouble], [Enable Long Double (long double precision) (default=auto)]), [cv_ldouble="$enable_ldouble"], [cv_ldouble="yes"])

if test "x$cv_float" = "xno"; then
if test "x$cv_double" = "xno"; then
if test "x$cv_ldouble" = "xno"; then
  AC_MSG_ERROR([*** All precisions were disabled. ***])
fi;fi;fi

# init pkg-config

PKG_CHECK_MODULES(fftw3f, fftw3f,, [cv_float="no"])

# test FFTW3 versions

if test "x$cv_float" = "xyes"; then
  PKG_CHECK_MODULES(fftw3f, [fftw3f >= 3.0],, [cv_float="no"])
  AC_SUBST(fftw3f_CFLAGS)
  AC_SUBST(fftw3f_LIBS)
fi

if test "x$cv_double" = "xyes"; then
  PKG_CHECK_MODULES(fftw3, [fftw3 >= 3.0],, [cv_double="no"])
  AC_SUBST(fftw3_CFLAGS)
  AC_SUBST(fftw3_LIBS)
fi

if test "x$cv_ldouble" = "xyes"; then
  PKG_CHECK_MODULES(fftw3l, [fftw3l >= 3.0],, [cv_ldouble="no"])
  AC_SUBST(fftw3l_CFLAGS)
  AC_SUBST(fftw3l_LIBS)
fi

if test "x$cv_float" = "xno"; then
if test "x$cv_double" = "xno"; then
if test "x$cv_ldouble" = "xno"; then
  AC_MSG_ERROR([*** any FFTW3 (single or double or ldouble) are not installed ***])
fi;fi;fi

AM_CONDITIONAL(BUILD_FLOAT, test "x$cv_float" = "xyes")
AM_CONDITIONAL(BUILD_DOUBLE, test "x$cv_double" = "xyes")
AM_CONDITIONAL(BUILD_LDOUBLE, test "x$cv_ldouble" = "xyes")

AC_ARG_ENABLE(release, AC_HELP_STRING([--enable-release], [Build a release version.(default=yes)]), [cv_release="$enable_release"], [cv_release="yes"])
AC_ARG_ENABLE(autocflags, AC_HELP_STRING([--enable-autocflags],
  [Set the GCC SIMD optimization CFLAGS automatically. Not good for backward compatibility.
  If you want to build SIMD autodetect binary, you should disable this option.(default=no)]),
  [cv_autocflags="$enable_autocflags"], [cv_autocflags="no"])

if test "x$cv_release" = "xno"; then
  AC_DEFINE(DEBUG,1,Define to 1 if you are in debug mode)
  CFLAGS="$CFLAGS -ggdb -Wall -O -DDEBUG -fstrict-aliasing"
fi
if test "x$cv_release" = "xyes"; then
  AC_DEFINE(NDEBUG,1,Define to 1 if you are NOT in debug mode)
  if test "x$cv_autocflags" = "xyes"; then
    CFLAGS="$CFLAGS -Wall -O3 -fstrict-aliasing -ffast-math -fomit-frame-pointer -fprefetch-loop-arrays -fstrength-reduce"
  fi
fi

AC_ARG_ENABLE(undenormal, AC_HELP_STRING([--enable-undenormal], [Enable undenormal code. You can disable this in SSE for the optimization.(default=yes)]),
  [cv_undenormal="$enable_undenormal"], [cv_undenormal="yes"])
if test "x$cv_undenormal" = "xno"; then
  AC_DEFINE(DISABLE_UNDENORMAL,1,Define to 1 to disable undenormal code)
fi

AC_ARG_ENABLE(pthread, AC_HELP_STRING([--enable-pthread], [Enable pthread multithreaded convolution engine.(default=no)]),
 [cv_pthread="$enable_pthread"], [cv_pthread="no"])
if test "x$cv_pthread" = "xyes"; then
  AC_CHECK_HEADERS(pthread.h)
  AC_CHECK_LIB(pthread,main,LIBPTHREADFOUND=1)
    if test "$LIBPTHREADFOUND" != 1 ; then
      AC_MSG_ERROR([*** You need pthread library to build the pthread version. ***])
    fi
  AC_DEFINE(ENABLE_PTHREAD,1,Define to 1 if you enable the pthread)
  LIBS="$LIBS -lpthread"	
fi
AM_CONDITIONAL(BUILD_PTHREAD, test "x$cv_pthread" = "xyes")

AC_ARG_ENABLE(forcefpu,AC_HELP_STRING([--enable-forcefpu], [Disable SIMD codes.(default=no)]),[cv_forcefpu="$enable_forcefpu"], [cv_forcefpu="no"])
AC_ARG_ENABLE(force3dnow,AC_HELP_STRING([--enable-force3dnow], [Force use of 3DNow! codes instead of SSE with --enable-3dnow.(default=no)]),
  [cv_force3dnow="$enable_force3dnow"], [cv_force3dnow="no"])
AC_ARG_ENABLE(3dnow,AC_HELP_STRING([--enable-3dnow], [Use 3DNow!.(default=no)]), [cv_3dnow="$enable_3dnow"], [cv_3dnow="no"])
AC_ARG_ENABLE(sse,  AC_HELP_STRING([--enable-sse], [Use SSE.(v1,v2,yes=v2,no,default=no,AMD64=yes)]),
  [cv_sse="$enable_sse"], [cv_sse="no"])
AC_ARG_ENABLE(sse2, AC_HELP_STRING([--enable-sse2], [Use SSE2.(default=no,AMD64=yes)]), [cv_sse2="$enable_sse2"], [cv_sse2="no"])
AC_ARG_ENABLE(sse3, AC_HELP_STRING([--enable-sse3], [Use SSE3 instead of SSE.(default=no)]), [cv_sse3="$enable_sse3"], [cv_sse3="no"])
AC_ARG_ENABLE(sse4, AC_HELP_STRING([--enable-sse4], [Use SSE4(SSE4.1).(default=no)]), [cv_sse4="$enable_sse4"], [cv_sse4="no"])
AC_ARG_ENABLE(avx,  AC_HELP_STRING([--enable-avx], [Use AVX.(default=no)(EXPERIMENTAL)]), [cv_avx="$enable_avx"], [cv_avx="no"])
AC_ARG_ENABLE(fma,  AC_HELP_STRING([--enable-fma], [Use Intel FMA (FMA3).(default=no)(EXPERIMENTAL)]), [cv_fma="$enable_fma"], [cv_fma="no"])
AC_ARG_ENABLE(fma4, AC_HELP_STRING([--enable-fma4], [Use AMD FMA4.(default=no)(EXPERIMENTAL)]), [cv_fma4="$enable_fma4"], [cv_fma4="no"])

case "${target_cpu}" in
  amd64*|x86_64*)
    if test "x$cv_sse" = "xno"; then
      AC_MSG_NOTICE([Auto-enabling SSE codes])
      cv_sse=yes
    fi
    if test "x$cv_sse2" = "xno"; then
      AC_MSG_NOTICE([Auto-enabling SSE2 codes])
      cv_sse2=yes
    fi
    ;;
  *)
    ;;
esac

dnl option checks

if test "x$cv_sse" = "xyes"; then
  AC_MSG_RESULT([Use --enable-sse=v1 to select the old version of SSE codes.])
  cv_sse="v2"
fi

if test "x$cv_sse" = "xyes" -a "x$cv_3dnow" = "xyes"; then
  if test "x$cv_force3dnow" = "xyes"; then
    AC_MSG_RESULT([SSE was overridden by 3DNow!, sse=no.])
    cv_sse="no"
  fi
fi

dnl definitions

if test "x$cv_fma4" = "xyes"; then
  if test "x$cv_forcefpu" = "xno"; then
    AC_DEFINE(ENABLE_FMA4,1,Define to 1 if you use AMD FMA4 code)
  else
    cv_fma4="no"
  fi
  if test "x$cv_autocflags" = "xyes"; then
    CFLAGS="$CFLAGS -mfpmath=sse -msse -msse2 -msse3 -msse4 -mavx"
  fi
fi

if test "x$cv_fma" = "xyes"; then
  if test "x$cv_forcefpu" = "xno"; then
    AC_DEFINE(ENABLE_FMA3,1,Define to 1 if you use Intel FMA3 code)
  else
    cv_fma="no"
  fi
  if test "x$cv_autocflags" = "xyes"; then
    CFLAGS="$CFLAGS -mfpmath=sse -msse -msse2 -msse3 -msse4 -mavx -mfma"
  fi
fi

if test "x$cv_avx" = "xyes"; then
  if test "x$cv_forcefpu" = "xno"; then
    AC_DEFINE(ENABLE_AVX,1,Define to 1 if you use AVX code)
  else
    cv_avx="no"
  fi
  if test "x$cv_autocflags" = "xyes"; then
    CFLAGS="$CFLAGS -mfpmath=sse -msse -msse2 -msse3 -msse4 -mavx"
  fi
fi

if test "x$cv_sse4" = "xyes"; then
  if test "x$cv_forcefpu" = "xno"; then
    AC_DEFINE(ENABLE_SSE4,1,Define to 1 if you use SSE4 code)
  else
    cv_sse4="no"
  fi
  if test "x$cv_autocflags" = "xyes"; then
    CFLAGS="$CFLAGS -mfpmath=sse -msse -msse2 -msse3 -msse4.1"
  fi
fi

if test "x$cv_sse3" = "xyes"; then
  if test "x$cv_forcefpu" = "xno"; then
    AC_DEFINE(ENABLE_SSE3,1,Define to 1 if you use SSE3 code)
  else
    cv_sse3="no"
  fi
  if test "x$cv_autocflags" = "xyes"; then
    CFLAGS="$CFLAGS -mfpmath=sse -msse -msse2 -msse3"
  fi
fi

if test "x$cv_sse2" = "xyes"; then
  if test "x$cv_forcefpu" = "xno"; then
    AC_DEFINE(ENABLE_SSE2,1,Define to 1 if you use SSE2 code)
  else
    cv_sse2="no"
  fi
  if test "x$cv_autocflags" = "xyes"; then
    CFLAGS="$CFLAGS -mfpmath=sse -msse -msse2"
  fi
fi

if test "x$cv_forcefpu" = "xno"; then
  if test "x$cv_sse" = "xv1"; then
    AC_DEFINE(ENABLE_SSE,1,Define to 1 if you use SSE SIMD code)
  fi
  if test "x$cv_sse" = "xv2"; then
    AC_DEFINE(ENABLE_SSE_V2,1,Define to 1 if you use SSE SIMD code v2)
  fi
else
  cv_sse="no"
fi

if test "x$cv_autocflags" = "xyes"; then
  if test "x$cv_sse" = "xv1" -o "x$cv_sse" = "xv2"; then
    CFLAGS="$CFLAGS -mfpmath=sse -msse"
  fi
fi

if test "x$cv_3dnow" = "xyes"; then
  if test "x$cv_forcefpu" = "xno"; then
    AC_DEFINE(ENABLE_3DNOW,1,Define to 1 if you use 3DNOW code)
  else
    cv_3dnow="no"
  fi
  if test "x$cv_autocflags" = "xyes"; then
    CFLAGS="$CFLAGS -m3dnow -ffast-math"
  fi
fi

AC_ARG_ENABLE(omp, AC_HELP_STRING([--enable-omp], [Use OpenMP.(default=no)(EXPERIMENTAL)]), [cv_omp="$enable_omp"], [cv_omp="no"])
if test "x$cv_omp" = "xyes"; then
  AC_DEFINE(USEOMP,1,Define to 1 if you use OpenMP)
  AC_MSG_RESULT([Using GCC OpenMP])
  CFLAGS="$CFLAGS -fopenmp"
  LDFLAGS="$LDFLAGS -fopenmp -lgomp"
  AC_CHECK_HEADERS(omp.h)
fi

cv_need_sf="no"
cv_need_xmms="no"
cv_need_audacious="no"
cv_need_bmp="no"
cv_need_gtk3="no"
cv_need_jack="no"
cv_build_plugins="no"

AC_ARG_ENABLE(sample, AC_HELP_STRING([--enable-sample], [Build sample programs.(default=no)]), [cv_sample="$enable_sample"], [cv_sample="no"])
AM_CONDITIONAL(BUILD_SAMPLE, test "x$cv_sample" = "xyes")
if test "x$cv_sample" = "xyes"; then
  cv_need_sf="yes"
  cv_build_plugins="yes"
fi

AC_ARG_ENABLE(jack, AC_HELP_STRING([--enable-jack], [Build jack plugins.(default=no)]), [cv_jack="$enable_jack"], [cv_jack="no"])
AM_CONDITIONAL(BUILD_JACK, test "x$cv_jack" = "xyes")
if test "x$cv_jack" = "xyes"; then
  cv_need_sf="yes"
  cv_need_jack="yes"
  cv_need_gtk3="yes"
fi

AC_ARG_ENABLE(cv_jackconfdir,AS_HELP_STRING([--enable-jack-confdir],[Jack config dir for FV3.]),[cv_jackconfdir="$enable_jackconfdir"], [cv_jackconfdir=".fv3"])
AC_SUBST(cv_jackconfdir)
AC_DEFINE_UNQUOTED([BMP_RCPATH],"${cv_jackconfdir}", [Jack config dir for FV3.])

AC_ARG_ENABLE(audacious, AC_HELP_STRING([--enable-audacious], [Build audacious plugin.(default=no)]), [cv_audacious="$enable_audacious"], [cv_audacious="no"])
AM_CONDITIONAL(BUILD_AUDACIOUS, test "x$cv_audacious" = "xyes")
if test "x$cv_audacious" = "xyes"; then
  pluginstr="AUDACIOUS ${pluginstr}"
  cv_need_sf="yes"
  cv_need_audacious="yes"
  cv_need_gtk3="yes"
  cv_build_plugins="yes"
fi

AC_ARG_ENABLE(srcnewcoeffs, AC_HELP_STRING([--enable-srcnewcoeffs], [Build internal libsamplerate with new coeffs which are very big.(default=no)]),
  [cv_srcnewcoeffs="$enable_srcnewcoeffs"], [cv_srcnewcoeffs="no"])
AM_CONDITIONAL(BUILD_SRCNEWCOEFFS, test "x$cv_srcnewcoeffs" = "xyes")

AC_ARG_ENABLE(plugdouble, AC_HELP_STRING([--enable-plugdouble], [Build Samples or XMMS/BMP/Audacious plugins in double precision mode.(default=no)]),
  [cv_plugdouble="$enable_plugdouble"], [cv_plugdouble="no"])
AM_CONDITIONAL(BUILD_PLUGDOUBLE, test "x$cv_plugdouble" = "xyes")

AC_ARG_ENABLE(pluginit, AC_HELP_STRING([--enable-pluginit],
    [Build XMMS/BMP/Audacious plugins with self init/cleanup and thread mutex. This feature protect the plugins from unexpected calls and Segmentation Faults. This requires GCC attributes.(default=no)(OBSOLETE)]),
  [cv_pluginit="$enable_pluginit"], [cv_pluginit="no"])
AM_CONDITIONAL(ENABLE_PLUGINIT, test "x$cv_pluginit" = "xyes")

if test "x$cv_build_plugins" = "xyes"; then
  if test "x$cv_plugdouble" = "xno" -a "x$cv_float" = "xno"; then
    AC_MSG_ERROR([*** You need float precision version (maybe fftw3f) to enable plugins or samples (You can use --enable-plugdouble if you build double precision version) ***])
  fi
  if test "x$cv_plugdouble" = "xyes" -a "x$cv_double" = "xno"; then
    AC_MSG_ERROR([*** You need double precision version (maybe fftw3) to enable plugdouble for plugins or samples ***])
  fi
fi

AM_CONDITIONAL(BUILD_STATIC, test "x$enable_static" = "xyes")

if test "x$pluginstr" = "x"; then
  pluginstr="no"
fi

# Test optional packages

if test "x$cv_need_sf" = "xyes"; then
  PKG_CHECK_MODULES(sndfile, sndfile)
  AC_SUBST(sndfile_CFLAGS)
  AC_SUBST(sndfile_LIBS)
fi

if test "x$cv_need_jack" = "xyes"; then
  PKG_CHECK_MODULES(jack, jack)
  AC_SUBST(jack_CFLAGS)
  AC_SUBST(jack_LIBS)
fi

if test "x$cv_need_gtk3" = "xyes"; then
  PKG_CHECK_MODULES(gtk3, [glib-2.0 >= 2.4.7 gtk+-3.0 >= 3.0.0 gthread-2.0 pango cairo],, [AC_MSG_ERROR([Cannot find glib2/gtk3/pango/gthread2/cairo])])
  AC_SUBST(gtk3_CFLAGS)
  AC_SUBST(gtk3_LIBS)
fi

if test "x$cv_need_audacious" = "xyes"; then
  PKG_CHECK_MODULES(audacious, [audacious >= 3.5],,  [AC_MSG_ERROR([Cannot find audacious >= 3.5])])
  AC_SUBST(audacious_CFLAGS)
  AC_SUBST(audacious_LIBS)
  EFFECT_PLUGIN_DIR=Effect
  audacious_effect_plugin_dir="`pkg-config audacious --variable=plugin_dir`/${EFFECT_PLUGIN_DIR}"
  AC_SUBST(audacious_effect_plugin_dir)
fi

dnl check standard headers and functions
AC_HEADER_STDC
dnl AC_CHECK_HEADERS(endian.h)
dnl AC_CHECK_HEADERS(byteswap.h)
dnl AC_CHECK_HEADERS(locale.h)
dnl AC_CHECK_HEADERS(inttypes.h)
dnl AC_HEADER_SYS_WAIT
AC_CHECK_SIZEOF(short,2)
AC_CHECK_SIZEOF(int,4)
AC_CHECK_SIZEOF(long,4)
AC_CHECK_SIZEOF(float,4)
AC_CHECK_SIZEOF(double,4)
AC_CHECK_SIZEOF(long double,4)
AC_CHECK_SIZEOF(void*,8)
AC_CHECK_SIZEOF(size_t,4)
AC_CHECK_SIZEOF(int64_t,8)
AC_CHECK_SIZEOF(long long,8)

AC_CHECK_FUNCS(malloc calloc realloc free memcpy memmove)
AC_CHECK_FUNCS(floor ceil)
AC_CHECK_LIB([m],floor)
AC_C99_FUNC_LRINT
AC_C99_FUNC_LRINTF

CFLAGS="$CFLAGS"
CXXFLAGS="$CXXFLAGS $CFLAGS"
LTCC="$CXX"

AC_OUTPUT([
m4/Makefile
doc/Makefile
patch/Makefile
freeverb/Makefile
float/Makefile
double/Makefile
ldouble/Makefile
libsamplerate2/Makefile
libgdither/Makefile
samples/Makefile
samples/IR/Makefile
audacious/Makefile
jack/Makefile
Makefile
freeverb3-3.pc
])

AC_MSG_RESULT([
-=-=-=-=-=-=-=-=-=-= Configuration Complete =-=-=-=-=-=-=-=-=-=-=-

  Freeverb3 Configuration summary :])

AC_MSG_RESULT([    Version : ................... ${VERSION}])
AC_MSG_RESULT([    LibTool : ................... ${LIBTOOL_VERSION_INFO}])
AC_MSG_RESULT([    Release build : ............. ${cv_release}])
AC_MSG_RESULT([    Undenormal code : ........... ${cv_undenormal}])
AC_MSG_RESULT([    Build float : ............... ${cv_float}])
AC_MSG_RESULT([    Build double : .............. ${cv_double}])
AC_MSG_RESULT([    Build long double : ......... ${cv_ldouble}])
AC_MSG_RESULT([    Build effect plugins : ...... ${pluginstr}])
AC_MSG_RESULT([    Double precision plugin : ... ${cv_plugdouble}])
AC_MSG_RESULT([    plugin_init()/fini() : ...... ${cv_pluginit}])
AC_MSG_RESULT([    Build sample codes : ........ ${cv_sample}])
AC_MSG_RESULT([    JACK effect plugins : ....... ${cv_jack}])
AC_MSG_RESULT([    Static build : .............. ${enable_static}])
AC_MSG_RESULT([    Profiling build : ........... ${cv_profile}])
AC_MSG_RESULT([    pthread support : ........... ${cv_pthread}])
AC_MSG_RESULT([    OpenMP : .................... ${cv_omp}])
AC_MSG_RESULT([    3DNow! : .................... ${cv_3dnow}])
AC_MSG_RESULT([    SSE : ....................... ${cv_sse}])
AC_MSG_RESULT([    SSE2 : ...................... ${cv_sse2}])
AC_MSG_RESULT([    SSE3 : ...................... ${cv_sse3}])
AC_MSG_RESULT([    SSE4 (SSE4.1) : ............. ${cv_sse4}])
AC_MSG_RESULT([    AVX : ....................... ${cv_avx}])
AC_MSG_RESULT([    Intel FMA (FMA3) : .......... ${cv_fma}])
AC_MSG_RESULT([    AMD FMA4 : .................. ${cv_fma4}])
AC_MSG_RESULT([    CC : ........................ ${CC}])
AC_MSG_RESULT([    CXX : ....................... ${CXX}])
AC_MSG_RESULT([    CFLAGS : ....................
  ${CFLAGS}])
AC_MSG_RESULT([    CXXFLAGS : ..................
  ${CXXFLAGS}])

AC_MSG_RESULT([
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
])
