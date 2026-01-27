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
        "initialize", "isinitialized", "format", "formats", "extensions", "updatable", "open", "create", 0L
    };
    enum commands {
        cmInitialize, cmIsInitialized, cmFormat, cmFormats, cmExtensions, cmUpdatable, cmOpen, cmCreate
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

    case cmFormat:

        if (objc == 3) {
            if (!lib.isLoaded() && (Initialize(NULL) != TCL_OK))
                return TCL_ERROR;
            Tcl_SetObjResult(tclInterp, Tcl_NewIntObj(
                    lib.getFormatByExtension(sevenzip::fromBytes(Tcl_GetString(objv[2])))));
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "extension");
            return TCL_ERROR;
        } 

        break;

    case cmFormats:

        if (objc == 2) {
            if (!lib.isLoaded() && (Initialize(NULL) != TCL_OK))
                return TCL_ERROR;
            if (SupportedFormats(Tcl_GetObjResult(tclInterp)) != TCL_OK) {
                return TCL_ERROR;
            }
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

    case cmUpdatable:

        if (objc == 3) {
            if (!lib.isLoaded() && (Initialize(NULL) != TCL_OK))
                return TCL_ERROR;
            int type;
            if (GetFormat(objv[2], type) != TCL_OK)
                return TCL_ERROR;
            Tcl_SetObjResult(tclInterp, Tcl_NewBooleanObj(lib.getFormatUpdatable(type)));
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "type");
            return TCL_ERROR;
        } 

        break;

    case cmOpen:

        // open ?-detecttype|-forcetype? ?-password password? -channel -- chan | filename
        if (objc > 2) {
            static const char *const options[] = {
                "-detecttype", "-forcetype", "-password", "-channel", 0L
            };
            enum options {
                opDetecttype, opForcetype, opPassword, opChannel
            };
            int index;
            bool detecttype = false;
            bool usechannel = false;
            Tcl_Obj *password = NULL;
            Tcl_Obj *forcetype = NULL;
            for (int i = 2; i < objc - 1; i++) {
                if (Tcl_GetIndexFromObj(tclInterp, objv[i], options, "option", 0, &index) != TCL_OK) {
                    return TCL_ERROR;
                }
                switch ((enum options)(index)) {
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

            if (!lib.isLoaded() && (Initialize(NULL) != TCL_OK))
                return TCL_ERROR;

            int type = (usechannel || detecttype) ? -2 : -1;
            if (forcetype)
                if (GetFormat(forcetype, type) != TCL_OK)
                    return TCL_ERROR;

            static unsigned long archiveCounter = 0;
            auto command = Tcl_ObjPrintf("sevenzip%lu", archiveCounter++);
            return OpenArchive(command, objv[objc-1], password, type, usechannel);
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?options? path");
            return TCL_ERROR;
        }

        break;

    case cmCreate:

        // create ?-properties proplist? ?-forcetype type? ?-password password? ?-inputchannel channel? ?-channel? channel | filename files
        if (objc > 3) {
            static const char *const options[] = {
                "-properties", "-forcetype", "-password", "-inputchannel", "-channel", 0L
            };
            enum options {
                opProperties, opForcetype, opPassword, opInputChannel, opChannel
            };
            int index;
            bool usechannel = false;
            Tcl_Obj *inputchannel = NULL;
            Tcl_Obj *properties = NULL;
            Tcl_Obj *password = NULL;
            Tcl_Obj *forcetype = NULL;
            for (int i = 2; i < objc - 2; i++) {
                if (Tcl_GetIndexFromObj(tclInterp, objv[i], options, "option", 0, &index) != TCL_OK) {
                    return TCL_ERROR;
                }
                switch ((enum options)(index)) {
                case opProperties:
                    if (i < objc - 3) {
                        properties = objv[++i];
                        Tcl_Size length;
                        if (Tcl_ListObjLength(tclInterp, properties, &length) != TCL_OK)
                            return TCL_ERROR;
                        if (length % 2 != 0) {
                            Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                                "\"-properties\" option must be followed by an even-length list", -1));
                            return TCL_ERROR;
                        }
                    } else {
                        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-properties\" option must be followed by property dictionary", -1));
                        return TCL_ERROR;
                    }
                    break;
                case opForcetype:
                    if (i < objc - 3) {
                        forcetype = objv[++i];
                    } else {
                        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-forcetype\" option must be followed by type", -1));
                        return TCL_ERROR;
                    }
                    break;
                case opPassword:
                    if (i < objc - 3) {
                        password = objv[++i];
                    } else {
                        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-password\" option must be followed by password", -1));
                        return TCL_ERROR;
                    }
                    break;
                case opInputChannel:
                    if (i < objc - 3) {
                        inputchannel = objv[++i];
                    } else {
                        Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-inputchannel\" option must be followed by channel", -1));
                        return TCL_ERROR;
                    }
                    break;                   
                case opChannel:
                    usechannel = true;
                    break;
                }
            }

            if (!lib.isLoaded() && (Initialize(NULL) != TCL_OK))
                return TCL_ERROR;

            int type = -1;
            if (forcetype)
                if (GetFormat(forcetype, type) != TCL_OK)
                    return TCL_ERROR;

            return CreateArchive(objv[objc-1], objv[objc-2],
                    inputchannel, password, type, usechannel, properties);
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?options? path list");
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
    return TCL_ERROR;
}

int SevenzipCmd::SupportedFormats (Tcl_Obj *formats) {
    int n = lib.getNumberOfFormats();
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            Tcl_Obj *format = Tcl_NewObj();
            Tcl_ListObjAppendElement(NULL, format, Tcl_NewStringObj("id", -1));
            Tcl_ListObjAppendElement(NULL, format, Tcl_NewIntObj(i));
            Tcl_ListObjAppendElement(NULL, format, Tcl_NewStringObj("name", -1));
            Tcl_ListObjAppendElement(NULL, format, Tcl_NewStringObj(sevenzip::toBytes(lib.getFormatName(i)), -1));
            Tcl_ListObjAppendElement(NULL, format, Tcl_NewStringObj("updatable", -1));
            Tcl_ListObjAppendElement(NULL, format, Tcl_NewBooleanObj(lib.getFormatUpdatable(i)));
            Tcl_ListObjAppendElement(NULL, format, Tcl_NewStringObj("extensions", -1));
            Tcl_ListObjAppendElement(NULL, format, Tcl_NewStringObj(sevenzip::toBytes(lib.getFormatExtensions(i)), -1));
            Tcl_ListObjAppendElement(tclInterp, formats, format);
        }
        return TCL_OK;
    }
    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
            sevenzip::toBytes(sevenzip::getMessage(lastError(tclInterp, S_OK))), -1));
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
    if (hr != S_OK)
        delete stream;
    if (hr == S_OK)
        hr = archive->Open(lib, stream, usechannel ? NULL : source, password, type);
    if (hr != S_OK) {
        delete archive; 
        Tcl_DecrRefCount(command);
        return lastError(tclInterp, hr);
    }
    Tcl_SetObjResult(tclInterp, command);
    return TCL_OK;
}

int SevenzipCmd::CreateArchive(Tcl_Obj *pathnames, Tcl_Obj *destination, Tcl_Obj *source,
        Tcl_Obj *password, int type, bool usechannel, Tcl_Obj *properties) {
    sevenzip::Oarchive archive;
    SevenzipInStream istream(tclInterp);
    SevenzipOutStream ostream(tclInterp);
    wchar_t buffer[1024];
    HRESULT hr = S_OK;
    if (source)
        if (istream.AttachOpenChannel(source) != S_OK)
            return lastError(tclInterp, E_FAIL);
    if (usechannel)
        if (ostream.AttachOpenChannel(destination) != S_OK)
            return lastError(tclInterp, E_FAIL);
    if (hr == S_OK)
        hr = archive.open(lib, istream, ostream,
                usechannel ? NULL : sevenzip::fromBytes(Tcl_GetString(destination)),
                password ? sevenzip::fromBytes(buffer, sizeof(buffer)/sizeof(wchar_t), Tcl_GetString(password)) : NULL,
                type);
    if (hr == S_OK && properties) {
        Tcl_Size length;
        if (Tcl_ListObjLength(tclInterp, properties, &length) != TCL_OK)
            return TCL_ERROR;
        for (int i = 0; i < length; i += 2) {
            Tcl_Obj *key;
            Tcl_Obj *value;
            if (Tcl_ListObjIndex(tclInterp, properties, i, &key) != TCL_OK)
                return TCL_ERROR;
            if (Tcl_ListObjIndex(tclInterp, properties, i + 1, &value) != TCL_OK)
                return TCL_ERROR;
            int intValue;
            if (Tcl_GetIntFromObj(tclInterp, value, &intValue) == TCL_OK)
                hr = archive.setIntProperty(
                        sevenzip::fromBytes(Tcl_GetString(key)), intValue);
            else if (Tcl_GetBooleanFromObj(tclInterp, value, &intValue) == TCL_OK)
                hr = archive.setBoolProperty(
                        sevenzip::fromBytes(Tcl_GetString(key)), intValue);
            else
                hr = archive.setStringProperty(
                        sevenzip::fromBytes(Tcl_GetString(key)),
                        sevenzip::fromBytes(buffer, sizeof(buffer)/sizeof(wchar_t), Tcl_GetString(value)));
            Tcl_ResetResult(tclInterp);
        }
    }
    if (hr == S_OK) {
        Tcl_Size length;
        if (Tcl_ListObjLength(tclInterp, pathnames, &length) != TCL_OK)
            return TCL_ERROR;
        // NOTE: when source is specified, only one item can be added
        // if (source && length > 1)
        //     length = 1;
        for (int i = 0; i < length; i++) {
            Tcl_Obj *item;
            if (Tcl_ListObjIndex(tclInterp, pathnames, i, &item) != TCL_OK)
                return TCL_ERROR;
            archive.addItem(sevenzip::fromBytes(Tcl_GetString(item)));
        }
    }
    if (hr == S_OK)
        hr = archive.update();
    if (hr != S_OK)
        return lastError(tclInterp, hr);
    return TCL_OK;
}

int SevenzipCmd::GetFormat(Tcl_Obj *index, int &type) {
    if (Tcl_GetIntFromObj(NULL, index, &type) == TCL_OK) {
        if (type < 0 || type >= lib.getNumberOfFormats())
            return lastError(tclInterp, E_NOTSUPPORTED);
    } else {
        type = lib.getFormatByExtension(sevenzip::fromBytes(Tcl_GetString(index)));
        if (type < 0)
            return lastError(tclInterp, E_NOTSUPPORTED);
    }
    return TCL_OK;
}
