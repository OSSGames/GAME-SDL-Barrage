dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(barrage, 1.0.5)
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_EXEEXT

# Checks for libraries.
AC_CHECK_LIB(m, main,, AC_MSG_ERROR(lib math is needed))
AM_PATH_SDL(1.2.0,, AC_MSG_ERROR(lib SDL is needed))                            
#AC_CHECK_LIB(SDL, main,, AC_MSG_ERROR(lib SDL is needed))

# Mixer is optional
audio_flag="-DAUDIO_ENABLED"
audio_lib="-lSDL_mixer"
AC_CHECK_LIB(SDL_mixer, main,, AC_MSG_RESULT(lib SDL mixer is missing); audio_flag=""; audio_lib="")

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memset strchr strdup strrchr])

# Flags
datdir=$datadir/$PACKAGE
hiscoredir=$localstatedir
configdir="~/.config"
debugflags=""
AC_ARG_ENABLE( install,
[  --disable-install       No installation. Start from source directory.], 
datdir="." hiscoredir="." configdir=".")
AC_ARG_ENABLE( debug,
[  --enable-debug       Add debug symbols, no optimization.], 
debugflags="-O0 -g")
AC_ARG_ENABLE( audio,
[  --disable-audio      Disable Audio], audio_flag=""; audio_lib="")
AC_SUBST(datdir)
AC_SUBST(hiscoredir)

CFLAGS="$CFLAGS -Wall -Wno-int-conversion -Wno-format -Wno-implicit-function-declaration"
CFLAGS="$CFLAGS $SDL_CFLAGS $audio_flag $debugflags -DSRC_DIR=\\\"$datdir\\\" -DCONFIGDIRNAME=\\\"$configdir\\\" -DHISCOREDIR=\\\"$hiscoredir\\\""
LIBS="$LIBS -lm $SDL_LIBS $audio_lib"

AC_OUTPUT([
Makefile
src/Makefile
src/gfx/Makefile
src/sounds/Makefile
barrage.spec
])

