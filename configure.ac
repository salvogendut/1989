AC_INIT([1989], [0.2.0], [], [1989])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])

# Large file support + POSIX extensions (before any compiler checks)
AC_SYS_LARGEFILE
AC_USE_SYSTEM_EXTENSIONS

# C and C++ compilers — GNU extensions are used by the Previous/UAE-derived core
AC_PROG_CC
AC_PROG_CXX
AC_PROG_OBJC
AX_CHECK_COMPILE_FLAG([-std=gnu99],
    [AM_CFLAGS="$AM_CFLAGS -std=gnu99"],
    [AC_MSG_ERROR([GNU C99-capable compiler required])])
AC_SUBST([AM_CFLAGS])

# pkg-config
PKG_PROG_PKG_CONFIG

# Required libraries
PKG_CHECK_MODULES([SDL3], [sdl3 >= 3.2.0], [], [AC_MSG_ERROR([SDL3 not found])])
AC_CHECK_LIB([m], [sqrt], [], [AC_MSG_ERROR([libm not found])])

# Optional libraries
PKG_CHECK_MODULES([PNG], [libpng],
    [AC_DEFINE([HAVE_LIBPNG], [1], [Define if you have a PNG compatible library])],
    [:])
PKG_CHECK_MODULES([PCAP], [libpcap],
    [AC_DEFINE([HAVE_PCAP], [1], [Define if you have a PCAP compatible library])],
    [AC_CHECK_LIB([pcap], [pcap_open_dead],
        [AC_DEFINE([HAVE_PCAP], [1], [Define if you have a PCAP compatible library])
         PCAP_LIBS="-lpcap"],
        [:])])
AC_CHECK_HEADERS([readline/readline.h])
PKG_CHECK_MODULES([READLINE], [readline],
    [AC_DEFINE([HAVE_LIBREADLINE], [1], [Define if you have a readline compatible library])],
    [AC_CHECK_LIB([readline], [readline],
        [AC_DEFINE([HAVE_LIBREADLINE], [1], [Define if you have a readline compatible library])
         READLINE_LIBS="-lreadline"], [:])])

# ffmpeg - optional. Enables the post-capture GIF optimization pass selected
# with the "GIF encoder" option. The binary path is hardcoded as FFMPEG_PATH
# for subprocess invocation. The built-in GIF encoder works without it.
AC_ARG_WITH([ffmpeg],
    [AS_HELP_STRING([--without-ffmpeg], [disable external GIF optimization])],
    [], [with_ffmpeg=yes])
AS_IF([test "x$with_ffmpeg" != "xno"], [AC_PATH_PROG([FFMPEG], [ffmpeg], [])],
      [FFMPEG=""])
AS_IF([test -n "$FFMPEG"],
      [AC_DEFINE_UNQUOTED([FFMPEG_PATH], ["$FFMPEG"],
           [Path to ffmpeg binary for GIF optimization])
       AC_DEFINE([HAVE_FFMPEG], [1], [ffmpeg available for GIF optimization])
       AC_MSG_NOTICE([GIF optimization: enabled via $FFMPEG])],
      [AC_MSG_NOTICE([GIF optimization: disabled (no ffmpeg found)])])

# Headers
AC_CHECK_HEADERS([byteswap.h strings.h sys/time.h sys/xattr.h arpa/inet.h
                  netinet/in.h sys/ioctl.h sys/select.h sys/signal.h sys/stat.h
                  sys/stropts.h sys/types.h sys/utime.h sys/wait.h sys/filio.h
                  unistd.h utime.h values.h stdint.h stdlib.h string.h])

# Functions
AC_CHECK_FUNCS([setenv select gettimeofday nanosleep strerror
                alphasort scandir fseeko ftello flock strdup
                lsetxattr lgetxattr posix_memalign aligned_alloc])

# Previous uses the combined HAVE_LXETXATTR to select the portable 5-argument
# lsetxattr/lgetxattr calls instead of the macOS-style setxattr(XATTR_NOFOLLOW).
AS_IF([test "x$ac_cv_func_lsetxattr" = "xyes" &&
       test "x$ac_cv_func_lgetxattr" = "xyes"],
      [AC_DEFINE([HAVE_LXETXATTR], [1],
                 [Define to 1 if you have the lsetxattr and lgetxattr functions])])

# bswap_16/bswap_32 are macros from byteswap.h rather than functions
AC_CHECK_DECLS([bswap_16, bswap_32], [], [], [[#include <byteswap.h>]])
AS_IF([test "x$ac_cv_have_decl_bswap_16" = "xyes"],
      [AC_DEFINE([HAVE_BSWAP_16], [1], [Define to 1 if you have the bswap_16 function])])
AS_IF([test "x$ac_cv_have_decl_bswap_32" = "xyes"],
      [AC_DEFINE([HAVE_BSWAP_32], [1], [Define to 1 if you have the bswap_32 function])])

# Platform specific struct members (non-POSIX systems such as macOS; 'no' is fine)
AC_CHECK_MEMBERS([struct stat.st_atimespec, struct stat.st_atimespec.tv_nsec], [], [],
    [[#include <sys/types.h>
#include <sys/stat.h>]])
AC_CHECK_MEMBERS([struct stat.st_mtimespec, struct stat.st_mtimespec.tv_nsec], [], [],
    [[#include <sys/types.h>
#include <sys/stat.h>]])
AC_CHECK_MEMBERS([struct dirent.d_type], [], [],
    [[#include <sys/types.h>
#include <dirent.h>]])

# Feature defines, matching the upstream CMake build of Previous
AC_DEFINE([ENABLE_DSP_EMU], [1], [Define to 1 to enable DSP 56k emulation])
AC_DEFINE([ENABLE_TRACING], [1], [Define to 1 to enable trace logs])
AC_DEFINE_UNQUOTED([BIN2DATADIR], ["../share/1989"],
    [Relative path from bindir to datadir])

AM_CONDITIONAL([OS_MACOS], [test "x$host_os" != "x${host_os#darwin}"])
case $host_os in
  darwin*)
    MAC_LIBS="-framework Foundation"
    AC_DEFINE([ENABLE_RENDERING_THREAD], [1], [Define to 1 to enable rendering threads])
    ;;
esac

AC_SUBST([MAC_LIBS])

# Embed the application icon in native Windows executables.
case $host_os in
  mingw*|cygwin*|msys*)
    AC_CHECK_TOOL([WINDRES], [windres], [windres])
    os_windows=yes
    WIN_RC_OBJ="icons/1989-rc.o"
    WIN_LIBS="-lws2_32 -liphlpapi -lshell32"
    ;;
  *)
    WIN_RC_OBJ=""
    WIN_LIBS=""
    ;;
esac
AM_CONDITIONAL([OS_WINDOWS], [test "x$os_windows" = "xyes"])
AC_SUBST([WINDRES])
AC_SUBST([WIN_RC_OBJ])
AC_SUBST([WIN_LIBS])

# Short git commit shown in the About dialog alongside PACKAGE_VERSION.
PROG_GIT_COMMIT=`git -C "$srcdir" rev-parse --short HEAD 2>/dev/null || echo unknown`
AC_MSG_NOTICE([git commit: $PROG_GIT_COMMIT])
AC_SUBST([PROG_GIT_COMMIT])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
