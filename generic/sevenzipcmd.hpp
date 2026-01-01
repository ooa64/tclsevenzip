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

    int Initialize (Tcl_Obj * dll);
    int SupportedExts (Tcl_Obj * exts);
    // int OpenChannel(Tcl_Obj *channelName, tcl_Channel &tclChannel);
    int LastError (HRESULT hr = S_OK);


    virtual int Command (int objc, Tcl_Obj * const objv[]);
};

#endif
