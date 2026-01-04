#ifndef SEVENZIPARCHIVECMD_H
#define SEVENZIPARCHIVECMD_H

#include "sevenzipstream.hpp"

#include "tclcmd.hpp"

class SevenzipArchiveCmd : public TclCmd {

public:

    SevenzipArchiveCmd (Tcl_Interp *interp, const char *name,
            TclCmd *parent);
    virtual ~SevenzipArchiveCmd();

    HRESULT Open(sevenzip::Lib& lib, SevenzipInStream* stream, Tcl_Obj* filename, int formatIndex = -1);
    HRESULT Open(sevenzip::Lib& lib, SevenzipInStream* stream, Tcl_Obj* filename, Tcl_Obj* password, int formatIndex = -1);

    void Close();

private:

    SevenzipInStream* stream = NULL;
    sevenzip::Iarchive archive;

    int Info(Tcl_Obj *info);
    int List(Tcl_Obj *list, Tcl_Obj *pattern, char type, int flags, bool info);
    int Extract(Tcl_Obj *source, Tcl_Obj *destination, Tcl_Obj *password, bool usechannel);

    virtual int Command (int objc, Tcl_Obj * const objv[]);
    virtual void Cleanup();
};

#endif
