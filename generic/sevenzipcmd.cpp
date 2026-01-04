#include "sevenzipcmd.hpp"
#include "sevenziparchivecmd.hpp"

#include <wchar.h>

#if defined(SEVENZIPCMD_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::cerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

int SevenzipCmd::Command (int objc, Tcl_Obj *const objv[]) {
    static const char *const commands[] = {
        "initialize", "isinitialized", "extensions", "open", 0L
    };
    enum commands {
        cmInitialize, cmIsInitialized, cmExtensions, cmOpen
    };
    int index;

    if (objc < 2) {
        Tcl_WrongNumArgs(tclInterp, 1, objv, "subcommand");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(tclInterp, objv[1], commands, "subcommand", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum commands)(index)) {

    case cmInitialize:

        if (objc == 2 || objc == 3) {
            if (Initialize(objc == 3 ? objv[2] : NULL) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?library?");
            return TCL_ERROR;
        }

        break;

    case cmIsInitialized:

        if (objc == 2) {
            Tcl_SetObjResult(tclInterp, Tcl_NewBooleanObj(lib.isLoaded()));
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        }

        break;

    case cmExtensions:

        if (objc == 2) {
            if (!lib.isLoaded() && (Initialize(NULL) != TCL_OK))
                return TCL_ERROR;
            if (SupportedExts(Tcl_GetObjResult(tclInterp)) != TCL_OK) {
                return TCL_ERROR;
            }
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        } 

        break;

    case cmOpen:

        // open ?-multivolume? ?-detecttype|-forcetype? ?-password password? -channel -- chan | filename
        if (objc > 2) {
            static const char *const options[] = {
                "-multivolume", "-detecttype", "-forcetype", "-password", "-channel", 0L
            };
            enum options {
                opMultivolume, opDetecttype, opForcetype, opPassword, opChannel
            };
            int index;
            bool multivolume = false;
            bool detecttype = false;
            bool usechannel = false;
            Tcl_Obj *password = NULL;
            Tcl_Obj *forcetype = NULL;
            for (int i = 2; i < objc - 1; i++) {
                if (Tcl_GetIndexFromObj(tclInterp, objv[i], options, "option", 0, &index) != TCL_OK) {
                    return TCL_ERROR;
                }
                switch ((enum options)(index)) {
                case opMultivolume:
                    multivolume = true;
                    break;
                case opDetecttype:
                    detecttype = true;
                    break;
                case opForcetype:
                    if (i < objc - 2) {
                        forcetype = objv[++i];
                    } else {
                        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-forcetype\" option must be followed by type", -1));
                        return TCL_ERROR;
                    }
                    break;
                case opPassword:
                    if (i < objc - 2) {
                        password = objv[++i];
                    } else {
                        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-password\" option must be followed by password", -1));
                        return TCL_ERROR;
                    }
                    break;
                case opChannel:
                    usechannel = true;
                    break;
                }
            }
            if (detecttype && forcetype != NULL) {
                Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                    "only one of options \"-detecttype\" or \"-forcetype\" must be specified", -1));
                return TCL_ERROR;
            }
            if (multivolume && usechannel) {
                Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                    "only one of options \"-multivolume\" or \"-channel\" must be specified", -1));
                return TCL_ERROR;
            }

            if (!lib.isLoaded() && (Initialize(NULL) != TCL_OK))
                return TCL_ERROR;

            int type = (usechannel || detecttype) ? -2 : -1;
            if (type < 0 && forcetype) {
                type = lib.getFormatByExtension(sevenzip::fromBytes(Tcl_GetString(forcetype)));
                if (type < 0)
                    return lastError(tclInterp, E_NOTSUPPORTED);
            }
            static unsigned long archiveCounter = 0;
            auto command = Tcl_ObjPrintf("sevenzip%lu", archiveCounter++);
            return OpenArchive(command, objv[objc-1], password, type, usechannel);
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?options? filename");
            return TCL_ERROR;
        }

        break;

    }

    return TCL_OK;
};

int SevenzipCmd::Initialize (Tcl_Obj *dll) {
    if (lib.isLoaded()) {
        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("already initialized", -1));
        return TCL_ERROR;
    }
    if (dll) {
        if (lib.load(sevenzip::fromBytes(Tcl_GetString(dll)))) {
            return TCL_OK;
        }
    } else if (lib.load(SEVENZIPDLL)) {
        return TCL_OK;
    }
    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
            sevenzip::toBytes(lib.getLoadMessage()), -1));
    // Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("error loading 7z library", -1));
    return TCL_ERROR;
}

int SevenzipCmd::SupportedExts (Tcl_Obj *exts) {
    int n = lib.getNumberOfFormats();
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            wchar_t* b;
            wchar_t* t = wcstok(lib.getFormatExtensions(i), L" ", &b);
            while (t) {
                Tcl_ListObjAppendElement(tclInterp, exts,
                        Tcl_NewStringObj(sevenzip::toBytes(t), -1));
                t = wcstok(NULL, L" ", &b);
            }
        }
        return TCL_OK;
    }
    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
            sevenzip::toBytes(sevenzip::getMessage(lastError(tclInterp, S_OK))), -1));
    // Tcl_SetObjResult(tclInterp, Tcl_NewStringObj("error loading 7z library", -1));
    return TCL_ERROR;
}

int SevenzipCmd::OpenArchive(Tcl_Obj *command, Tcl_Obj *source,
        Tcl_Obj *password, int type, bool usechannel) {
    // TODO: stream should be owned by archive cmd, create it there?
    auto archive = new SevenzipArchiveCmd(tclInterp, Tcl_GetString(command), this);
    auto stream = new SevenzipInStream(tclInterp);
    HRESULT hr = S_OK;
    if (usechannel)
        hr = stream->AttachOpenChannel(source);
    if (hr == S_OK)
        hr = archive->Open(lib, stream, usechannel ? NULL : source, password, type);
    if (hr != S_OK) {
        delete archive; 
        return lastError(tclInterp, hr);
    }
    Tcl_SetObjResult(tclInterp, command);
    return TCL_OK;
}
