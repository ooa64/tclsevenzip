#ifndef SEVENZIPCMD_H
#define SEVENZIPCMD_H

#include <locale>
#include <codecvt>
#include <sevenzip.h>
#include <tcl.h>

#include "tclcmd.hpp"

class SevenzipCmd : public TclCmd {

public:

    SevenzipCmd (Tcl_Interp * interp, const char * name): TclCmd(interp, name), lib(), convert() {};

    virtual ~SevenzipCmd () {};

private:

    sevenzip::Lib lib;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

    int Initialize (Tcl_Obj * dll);
    int SupportedExts (Tcl_Obj * exts);
    // int OpenChannel(Tcl_Obj *channelName, tcl_Channel &tclChannel);
    int LastError (HRESULT hr = S_OK);


    virtual int Command (int objc, Tcl_Obj * const objv[]);
};

#endif
