#ifndef SEVENZIPCMD_H
#define SEVENZIPCMD_H

#include "tclcmd.hpp"

#include <sevenzip.h>

class SevenzipCmd : public TclCmd {

public:

    SevenzipCmd (Tcl_Interp * interp, const char * name): TclCmd(interp, name), lib() {};

    virtual ~SevenzipCmd () {};

private:

    sevenzip::Lib lib;

    int Initialize (Tcl_Obj *dll);
    int SupportedExts (Tcl_Obj *exts);
    int SupportedFormats (Tcl_Obj *formats);
    int OpenArchive(Tcl_Obj *command, Tcl_Obj *source,
            Tcl_Obj *password, int type, bool usechannel);
    int CreateArchive(Tcl_Obj *pathnames, Tcl_Obj *destination, 
            Tcl_Obj *password, int type, bool usechannel, Tcl_Obj *properties);
    int GetFormat(Tcl_Obj *index, int &type);

    virtual int Command (int objc, Tcl_Obj * const objv[]);
};

#endif
