#ifndef SEVENZIPARCHIVECMD_H
#define SEVENZIPARCHIVECMD_H

#include <locale>
#include <codecvt>
#include <sevenzip.h>
#include <tcl.h>

#include "tclcmd.hpp"
#include "sevenzipstream.hpp"

class SevenzipArchiveCmd : public TclCmd {

public:

    SevenzipArchiveCmd (Tcl_Interp *interp, const char *name,
            TclCmd *parent, sevenzip::Lib& lib);
    virtual ~SevenzipArchiveCmd();

    HRESULT Open(SevenzipInStream* stream, Tcl_Obj* filename, int formatIndex = -1);
    HRESULT Open(SevenzipInStream* stream, Tcl_Obj* filename, Tcl_Obj* password, int formatIndex = -1);

    void Close();

private:

    SevenzipInStream* stream = NULL;
    sevenzip::Iarchive archive;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

    int LastError(HRESULT hr = S_OK);

    int Info(Tcl_Obj *info);
    int List(Tcl_Obj *list, Tcl_Obj *pattern, char type, int flags, bool info);
    int Extract(Tcl_Obj *source, Tcl_Obj *destination, Tcl_Obj *password, bool usechannel);

    // bool Valid ();

    virtual int Command (int objc, Tcl_Obj * const objv[]);
    virtual void Cleanup();
};

#endif
