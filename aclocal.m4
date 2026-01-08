#
# Include the TEA standard macro set
#

builtin(include,tclconfig/tcl.m4)

#
# Add here whatever m4 macros you want to define for your package
#

AC_DEFUN([TCLSEVENZIP_SETUP_DEBUGLOG], [
    AC_MSG_CHECKING([for build with debuglog])
    AC_ARG_ENABLE(debuglog,
	AS_HELP_STRING([--enable-debuglog],
	    [build with debug printing (default: off)]),
	[tcl_ok=$enableval], [tcl_ok=no])
    if test "$tcl_ok" = "yes"; then
	CFLAGS_DEBUGLOG="-DTCLCMD_DEBUG -DSEVENZIPCMD_DEBUG -DSEVENZIPARCHIVECMD_DEBUG -DSEVENZIPSTREAM_DEBUG"
	AC_MSG_RESULT([yes])
    else
	CFLAGS_DEBUGLOG=""
	AC_MSG_RESULT([no])
    fi
    AC_SUBST(CFLAGS_DEBUGLOG)
])

AC_DEFUN([TCLSEVENZIP_CHECK_SEVENZIP], [
    AC_MSG_CHECKING([for sevenzip sources])
    AC_ARG_WITH([sevenzip],
        AS_HELP_STRING([--with-sevenzip=<dir>],
            [path to the sevenzip source directory]
        ), [
            sevenzipdir="$withval"
        ], [
            sevenzipdir="../libsevenzip"
        ]
    )
    AC_MSG_RESULT([$sevenzipdir])

    AC_MSG_CHECKING([for sevenzip.h])
    if test "$sevenzipdir" != "no"; then
        if test -f "$sevenzipdir/sevenzip.h"; then
            AC_MSG_RESULT([ok])
        else
            AC_MSG_RESULT([fail])
            AC_MSG_ERROR([Unable to locate sevenzip.h])
        fi
    else
            AC_MSG_RESULT([skip])
    fi

    AC_MSG_CHECKING([for C/7zTypes.h])
    if test "$sevenzipdir" != "no"; then
        if test -f "$sevenzipdir/C/7zTypes.h"; then
            AC_MSG_RESULT([ok])
        else
            AC_MSG_RESULT([fail])
            AC_MSG_ERROR([Unable to locate C/7zTypes.h])
        fi
    else
            AC_MSG_RESULT([skip])
    fi

    AC_MSG_CHECKING([for libsevenzip.a])
    if test "$sevenzipdir" != "no"; then
        if test -f "$sevenzipdir/libsevenzip.a"; then
            AC_MSG_RESULT([ok])
        else
            AC_MSG_RESULT([fail])
            AC_MSG_ERROR([Unable to locate libsevenzip.a])
        fi
    else
            AC_MSG_RESULT([skip])
    fi

    if test "$sevenzipdir" != "no"; then
        TEA_ADD_INCLUDES([-I\"`${CYGPATH} $sevenzipdir`\"])
    fi
    if test "$sevenzipdir" != "no"; then
        TEA_ADD_LIBS([-L"$sevenzipdir" -lsevenzip])
    fi

	SEVENZIP=$sevenzipdir
    AC_SUBST(SEVENZIP)
])
