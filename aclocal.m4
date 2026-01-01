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
	CFLAGS_DEBUGLOG="-DTCLCMD_DEBUG -DSEVENCMD_DEBUG -DSEVENZIPARCHIVECMD_DEBUG -DSEVENZIPSTREAM_DEBUG"
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

    AC_MSG_CHECKING([for sevenzip/libsevenzip.a])
    if test "$sevenzipdir" != "no"; then
        if test -f "$sevenzipdir/libsevenzip.a"; then
            AC_MSG_RESULT([ok])
        else
            AC_MSG_RESULT([fail])
            AC_MSG_ERROR([Unable to locate sevenzip/libsevenzip.a])
        fi
    else
            AC_MSG_RESULT([skip])
    fi

    AC_MSG_CHECKING([for 7zip source directory])
    AC_ARG_WITH([7zip],
        AS_HELP_STRING([--with-7zip=<dir>],
            [path to the 7zip source directory]
        ), [
            src7zipdir="$withval"
        ], [
            if test "$sevenzipdir" != "no"; then
                src7zipdir="$sevenzipdir/externals/7zip"
            else
                src7zipdir="no"
            fi
        ]
    )
    AC_MSG_RESULT([$src7zipdir])

    AC_MSG_CHECKING([for 7zip/C/7zVersion.h])
    if test "$src7zipdir" != "no"; then
        if test -f "$src7zipdir/C/7zVersion.h"; then
            AC_MSG_RESULT([ok])
        else
            AC_MSG_RESULT([fail])
            AC_MSG_ERROR([Unable to locate 7zip/C/7zVersion.h])
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
    if test "$src7zipdir" != "no"; then
        TEA_ADD_INCLUDES([-I\"`${CYGPATH} $src7zipdir`\"])
        AC_SUBST(src7zipdir)
    fi
])
