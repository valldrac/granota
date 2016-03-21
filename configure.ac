#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([granota], [0.27.afl])
AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])
AM_PATH_PYTHON([2.4])
LT_INIT([disable-static])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_MKDIR_P

AC_CHECK_PROG(GDB_CHECK,gdb,yes)
if test x"$GDB_CHECK" != x"yes" ; then
    AC_MSG_ERROR([Please install gdb before continue.])
fi

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([dl], [dlsym])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([rt], [timer_create])
AC_CHECK_LIB([crypto],[MD5])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h limits.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])
AC_CHECK_HEADERS(sys/procfs.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRCOLL
AC_CHECK_FUNCS([clock_gettime dup2 ftruncate getcwd gettimeofday memset mkdir munmap rmdir setenv socket sqrt strcasecmp strdup strerror strncasecmp strrchr strstr])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/crashdbg.sh
                 src/afl/Makefile
                 src/exploitable/Makefile
                 src/libgranota/Makefile])
AC_OUTPUT

echo "
  ($PACKAGE_NAME) version $PACKAGE_VERSION
  Prefix.........: $prefix
  C Compiler.....: $CC $CFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
"
