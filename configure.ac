# initialise autoconf and set up some basic information about the program we’re packaging
AC_INIT([diya-fm], [0.1.0], [xsang.le@gmail.com])
# We’re going to use automake for this project
# [subdir-objects] if needed
AM_INIT_AUTOMAKE()

# dependencies
# C compiler
AC_PROG_CC
# libtool for linking
AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_LIBTOOL
GLIB_GSETTINGS

PKG_CHECK_MODULES(GTK, gtk+-3.0, [], [
    AC_MSG_ERROR([Unable to find gtk+-3.0 module])
])

# AC_DEFINE([_GNU_SOURCE], [1],[Use GNU source])

# debug option
AC_ARG_ENABLE([debug],
    [  --enable-debug    Turn on debugging],
    [case "${enableval}" in
        yes) AC_DEFINE([DEBUG], [1],[Enable debug]) ;;
        no) ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
    esac],[debug=false])

AC_CANONICAL_HOST

# find a file called Makefile.in, substitute placeholders
# like @PACKAGE_VERSION@ with values like 0.1.0a, 
# and write the results to Makefile.
AC_CONFIG_FILES([Makefile])

# output the script:
AC_OUTPUT