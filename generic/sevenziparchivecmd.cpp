#include "sevenziparchivecmd.hpp"

#include <string.h>

#if defined(SEVENZIPARCHIVECMD_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::cerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

#define LIST_MATCH_NOCASE TCL_MATCH_NOCASE
#define LIST_MATCH_EXACT (1 << 16)

const int kpidPath = 3;
const int kpidIsDir = 6;
const int kpidPhysSize = 44;

static const char *const SevenzipProperties[] = {
    "noproperty",
    "mainsubfile",
    "handleritemindex",
    "path",
    "name",
    "extension",
    "isdir",
    "size",
    "packsize",
    "attrib",
    "ctime",
    "atime",
    "mtime",
    "solid",
    "commented",
    "encrypted",
    "splitbefore",
    "splitafter",
    "dictionarysize",
    "crc",
    "type",
    "isanti",
    "method",
    "hostos",
    "filesystem",
    "user",
    "group",
    "block",
    "comment",
    "position",
    "prefix",
    "numsubdirs",
    "numsubfiles",
    "unpackver",
    "volume",
    "isvolume",
    "offset",
    "links",
    "numblocks",
    "numvolumes",
    "timetype",
    "bit64",
    "bigendian",
    "cpu",
    "physize",
    "headerssize",
    "checksum",
    "characts",
    "va",
    "id",
    "shortname",
    "creatorapp",
    "sectorsize",
    "posixattrib",
    "symlink",
    "error",
    "totalsize",
    "freespace",
    "clustersize",
    "volumename",
    "localname",
    "provider",
    "ntsecure",
    "isaltstream",
    "isaux",
    "isdeleted",
    "istree",
    "sha1",
    "sha256",
    "errortype",
    "numerrors",
    "errorflags",
    "warningflags",
    "warning",
    "numstreams",
    "numaltstreams",
    "altstreamssize",
    "virtualsize",
    "unpacksize",
    "totalphysize",
    "volumeindex",
    "subtype",
    "shortcomment",
    "codepage",
    "isnotarctype",
    "physizecantbedetected",
    "zerostailisallowed",
    "tailsize",
    "embeddedstubsize",
    "ntreparse",
    "hardlink",
    "inode",
    "streamid",
    "readonly",
    "outname",
    "copylink",
    "arcfilename",
    "ishash",
    "changetime",
    "userid",
    "groupid",
    "devicemajor",
    "deviceminor",
    "devmajor",
    "devminor"
};

static int Tcl_StringCaseEqual(const char *str1, const char *str2, int nocase);
#ifdef _WIN32
static char *Path_WindowsPathToUnixPath(char *path);
#endif

SevenzipArchiveCmd::SevenzipArchiveCmd (Tcl_Interp *interp, const char *name, TclCmd *parent) :
        TclCmd(interp, name, parent), archive() {
    DEBUGLOG(this << " SevenzipArchiveCmd " << name);
}

SevenzipArchiveCmd::~SevenzipArchiveCmd() {
    DEBUGLOG(this << " ~SevenzipArchiveCmd");
    Close();
}

HRESULT SevenzipArchiveCmd::Open(sevenzip::Lib& lib, SevenzipInStream* stream,
        Tcl_Obj* filename, int formatIndex) {
    DEBUGLOG(this << " SevenzipArchiveCmd::Open " << (filename ? Tcl_GetString(filename) : "NULL"));
    if (!stream)
        return E_FAIL;
    if (stream && this->stream)
        return E_FAIL;
    if (stream)
        this->stream = stream;
    return archive.open(lib, *stream,
            filename ? sevenzip::fromBytes(Tcl_GetString(filename)) : NULL,
            formatIndex);
}

HRESULT SevenzipArchiveCmd::Open(sevenzip::Lib& lib, SevenzipInStream* stream,
        Tcl_Obj* filename, Tcl_Obj* password, int formatIndex) {
    DEBUGLOG(this << " SevenzipArchiveCmd::Open " << (filename ? Tcl_GetString(filename) : "NULL")
            << " " << (password ? Tcl_GetString(password) : "NULL"));
    if (!stream)
        return E_FAIL;
    if (stream && this->stream)
        return E_FAIL;
    if (stream)
        this->stream = stream;

    // NOTE: need to rewrite this or rewrite fromBytes to accept external buffer
    wchar_t buffer[128];
    if (password) {
        wcsncpy(buffer, sevenzip::fromBytes(Tcl_GetString(password)), sizeof(buffer)/sizeof(buffer[0])-1);
        buffer[sizeof(buffer)/sizeof(buffer[0])-1] = L'\0';
    }

    return archive.open(lib, *stream,
            filename ? sevenzip::fromBytes(Tcl_GetString(filename)) : NULL,
            password ? buffer : NULL,
            formatIndex);
}

void SevenzipArchiveCmd::Close() {
    DEBUGLOG(this << " SevenzipArchiveCmd::Close");
    archive.close();
    if (stream) {
        delete stream;
        stream = NULL;
    }
}

void SevenzipArchiveCmd::Cleanup() {
    DEBUGLOG(this << " SevenzipArchiveCmd::Cleanup");
    // Close();
};

int SevenzipArchiveCmd::Command (int objc, Tcl_Obj *const objv[]) {
    static const char *const commands[] = {
        "info", "count", "list", "extract", "close", 0L
    };
    enum commands {
        cmInfo, cmCount, cmList, cmExtract, cmClose
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

    case cmInfo:

        if (objc == 2) {
        //     if (!Valid())
        //         return TCL_ERROR;
            if (Info(Tcl_GetObjResult(tclInterp)) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        } 
        break;

    case cmCount:

        if (objc == 2) {
            int count = archive.getNumberOfItems();
            if (count < 0)
                return LastError();
            Tcl_SetObjResult(tclInterp, Tcl_NewIntObj(count));
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        } 
        break;

    case cmList:

        if (objc >= 2) {
            static const char * const options[] = {
                "-info", "-nocase", "-exact", "-type", "--", 0L
            };
            enum options {
                opInfo, opNocase, opExact, opType, opEnd
            };
            int index;
            int flags = 0;
            char type = 'a';
            bool info = false;
            Tcl_Obj *patternObj = NULL;
            for (int i = 2; i < objc; i++) {
                if (Tcl_GetIndexFromObj(tclInterp, objv[i], options, "option", 0, &index) != TCL_OK) {
                    if (i < objc - 1)
                        return TCL_ERROR;
                    Tcl_ResetResult(tclInterp);
                    patternObj = objv[i];
                    break;
                }
                switch ((enum options)(index)) {
                case opInfo:
                    info = true;
                    continue;
                case opNocase:
                    flags |= LIST_MATCH_NOCASE;
                    continue;
                case opExact:
                    flags |= LIST_MATCH_EXACT;
                    continue;
                case opType:
                    i++;
                    if (i < objc && Tcl_GetCharLength(objv[i]) == 1) {
                        type = Tcl_GetString(objv[i])[0];
                        if (type == 'd' || type == 'f')
                            continue;
                    }
                    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"-type\" option must be followed by \"d\" or \"f\"", -1));
                    return TCL_ERROR;
                case opEnd:
                    if (i == objc - 2)
                        patternObj = objv[i+1];
                    if (i >= objc - 2)
                        break;
                    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
                            "\"--\" option can be followed by pattern only", -1));
                    return TCL_ERROR;
                };
                break;
            };
            if (List(Tcl_GetObjResult(tclInterp), patternObj, type, flags, info) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?options? ?pattern?");
            return TCL_ERROR;
        }
        break;

    case cmExtract:
        if (objc >= 4) {
            static const char *const options[] = {
                "-password", "-channel", 0L
            };
            enum options {
                opPassword, opChannel
            };
            int index;
            bool usechannel = false;
            Tcl_Obj *password = NULL;
            for (int i = 2; i < objc - 2; i++) {
                if (Tcl_GetIndexFromObj(tclInterp, objv[i], options, "option", 0, &index) != TCL_OK) {
                    return TCL_ERROR;
                }
                switch ((enum options)(index)) {
                case opPassword:
                    if (i < objc - 3) {
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
            if (Extract(objv[objc-2], objv[objc-1], password, usechannel) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?options? item path");
            return TCL_ERROR;
        }
        break;

    case cmClose:
        if (objc > 2) {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        } else {
            // Close();
            delete this;
        }
        break;
    }

    return TCL_OK;
};

int SevenzipArchiveCmd::Info(Tcl_Obj *info) {
    int n = archive.getNumberOfProperties();
    for (int i = 0; i < n; i++) {
        PROPID propId;
        VARTYPE propType;
        if (archive.getPropertyInfo(i, propId, propType) == S_OK) {
            const wchar_t* stringValue;
            bool boolValue;
            UInt32 uint32Value;
            UInt64 uint64Value;
            if (propId < sizeof(SevenzipProperties)/sizeof(SevenzipProperties[0]))
                Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(SevenzipProperties[propId], -1));
            else 
                Tcl_ListObjAppendElement(NULL, info, Tcl_ObjPrintf("prop%d", propId));                
            if (archive.getStringProperty(propId, stringValue) == S_OK)
                Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(sevenzip::toBytes(stringValue), -1));
            else if (archive.getBoolProperty(propId, boolValue) == S_OK)
                Tcl_ListObjAppendElement(NULL, info, Tcl_NewBooleanObj(boolValue));
            else if (archive.getIntProperty(propId, uint32Value) == S_OK)
                Tcl_ListObjAppendElement(NULL, info, Tcl_NewIntObj(uint32Value));
            else if (archive.getWideProperty(propId, uint64Value) == S_OK)
                Tcl_ListObjAppendElement(NULL, info, Tcl_NewWideIntObj(uint64Value));
            else if (archive.getTimeProperty(propId, uint32Value) == S_OK)
                Tcl_ListObjAppendElement(NULL, info, Tcl_NewIntObj(uint32Value));
            else
                Tcl_ListObjAppendElement(NULL, info, Tcl_ObjPrintf("type%d", propType));
       }
    }
    {
        // append missing physize property (compatibility with older versions)
        UInt64 uint64Value;
        if (archive.getWideProperty(kpidPhysSize, uint64Value) == S_OK) {
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj("physize", -1));
            Tcl_ListObjAppendElement(NULL, info, Tcl_NewWideIntObj(uint64Value));
        }
    }
    //
    // NOTE: above code may not list all properties (physize,errorflags,...?)
    // NOTE: There is an alternative way to get all properties
    // 
    // for (PROPID propId = 0; propId < sizeof(SevenzipProperties)/sizeof(SevenzipProperties[0]); propId++) {
    //     const wchar_t* stringValue;
    //     bool boolValue;
    //     UInt32 uint32Value;
    //     UInt64 uint64Value;
    //     if (archive.getStringProperty(propId, stringValue) == S_OK) {
    //         Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(SevenzipProperties[propId], -1));
    //         Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(sevenzip::toBytes(stringValue), -1));
    //     } else if (archive.getBoolProperty(propId, boolValue) == S_OK) {
    //         Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(SevenzipProperties[propId], -1));
    //         Tcl_ListObjAppendElement(NULL, info, Tcl_NewBooleanObj(boolValue));
    //     } else if (archive.getIntProperty(propId, uint32Value) == S_OK) {
    //         Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(SevenzipProperties[propId], -1));
    //         Tcl_ListObjAppendElement(NULL, info, Tcl_NewIntObj(uint32Value));
    //     } else if (archive.getWideProperty(propId, uint64Value) == S_OK) {
    //         Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(SevenzipProperties[propId], -1));
    //         Tcl_ListObjAppendElement(NULL, info, Tcl_NewWideIntObj(uint64Value));
    //        else if (archive.getTimeProperty(propId, uint32Value) == S_OK) {
    //         Tcl_ListObjAppendElement(NULL, info, Tcl_NewStringObj(SevenzipProperties[propId], -1));
    //            Tcl_ListObjAppendElement(NULL, info, Tcl_NewIntObj(uint32Value));
    //     } else {
    //         DEBUGLOG("SevenzipArchiveCmd unhandled prop " << SevenzipProperties[propId]);
    //     }
    // }
    return TCL_OK;
}

int SevenzipArchiveCmd::List(Tcl_Obj *list, Tcl_Obj *pattern, char type, int flags, bool info) {
    unsigned int count = archive.getNumberOfItems();
    for (unsigned int i = 0; i < count; ++i) {
        if (type == 'd' && !archive.getItemIsDir(i))
            continue;
        if (type == 'f' && archive.getItemIsDir(i))
            continue;

#ifdef _WIN32
        char *path = Path_WindowsPathToUnixPath(sevenzip::toBytes(archive.getItemPath(i)));
#else
        char *path = sevenzip::toBytes(archive.getItemPath(i));
#endif
        if (pattern) {
            if (flags & LIST_MATCH_EXACT) {
                if (!Tcl_StringCaseEqual(path, Tcl_GetString(pattern), flags & TCL_MATCH_NOCASE))
                    continue;
            } else { 
                if (!Tcl_StringCaseMatch(path, Tcl_GetString(pattern), flags & TCL_MATCH_NOCASE))
                    continue;
            }
        }
        if (info) {
            Tcl_Obj *propObj = Tcl_NewObj();
            int n = archive.getNumberOfItemProperties();
            for (int j = 0; j < n; j++) {
                PROPID propId;
                VARTYPE propType;
                if (archive.getItemPropertyInfo(j, propId, propType) == S_OK) {
                    const wchar_t* stringValue;
                    bool boolValue;
                    UInt32 uint32Value;
                    UInt64 uint64Value;
                    Tcl_Obj *nameObj = propId < sizeof(SevenzipProperties)/sizeof(SevenzipProperties[0]) 
                        ? Tcl_NewStringObj(SevenzipProperties[propId], -1)
                        : Tcl_ObjPrintf("prop%d", propId);
#ifdef _WIN32
                    if (propId == kpidPath && archive.getStringItemProperty(i, propId, stringValue) == S_OK)
                        Tcl_ListObjAppendElement(NULL, propObj, nameObj);
                        Tcl_ListObjAppendElement(NULL, propObj, 
                                Tcl_NewStringObj(Path_WindowsPathToUnixPath(sevenzip::toBytes(stringValue)), -1));
                    } else                        
#endif
                    if (archive.getStringItemProperty(i, propId, stringValue) == S_OK) {
                        Tcl_ListObjAppendElement(NULL, propObj, nameObj);
                        Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(sevenzip::toBytes(stringValue), -1));
                    } else if (archive.getBoolItemProperty(i, propId, boolValue) == S_OK) {
                        Tcl_ListObjAppendElement(NULL, propObj, nameObj);
                        Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewBooleanObj(boolValue));
                    } else if (archive.getIntItemProperty(i, propId, uint32Value) == S_OK) {
                        Tcl_ListObjAppendElement(NULL, propObj, nameObj);
                        Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewIntObj(uint32Value));
                    } else if (archive.getWideItemProperty(i, propId, uint64Value) == S_OK) {
                        Tcl_ListObjAppendElement(NULL, propObj, nameObj);
                        Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewWideIntObj(uint64Value));
                    } else if (archive.getTimeItemProperty(i, propId, uint32Value) == S_OK) {
                        Tcl_ListObjAppendElement(NULL, propObj, nameObj);
                        Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewIntObj(uint32Value));
                    } else {
                        DEBUGLOG(this << "SevenzipArchiveCmd unhandled item " << i << " prop " << SevenzipProperties[propId] << " type? " << propType);
                    }
                }
            }
            {
                // append missing isdir property (compatibility with older versions)
                Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj("isdir", -1));
                Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewBooleanObj(archive.getItemIsDir(i)));
            }

            // NOTE: above code may not list all properties (isdir,isanti,...?)
            // NOTE: There is an alternative way to get all properties
            
            // for (PROPID propId = 0; propId < sizeof(SevenzipProperties)/sizeof(SevenzipProperties[0]); propId++) {
            //     const wchar_t* stringValue;
            //     bool boolValue;
            //     UInt32 uint32Value;
            //     UInt64 uint64Value;
            //     if (archive.getStringItemProperty(i, propId, stringValue) == S_OK) {
            //         Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(SevenzipProperties[propId], -1));
            //         Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(sevenzip::toBytes(stringValue), -1));
            //     } else if (archive.getBoolItemProperty(i, propId, boolValue) == S_OK) {
            //         Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(SevenzipProperties[propId], -1));
            //         Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewBooleanObj(boolValue));
            //     } else if (archive.getIntItemProperty(i, propId, uint32Value) == S_OK) {
            //         Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(SevenzipProperties[propId], -1));
            //         Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewIntObj(uint32Value));
            //     } else if (archive.getWideItemProperty(i, propId, uint64Value) == S_OK) {
            //         Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewStringObj(SevenzipProperties[propId], -1));
            //         Tcl_ListObjAppendElement(NULL, propObj, Tcl_NewWideIntObj(uint64Value));
            //     } else {
            //         DEBUGLOG(this << "SevenzipArchiveCmd unhandled item " << i << " prop " << SevenzipProperties[propId]);
            //     }
            // }
            Tcl_ListObjAppendElement(NULL, list, propObj);
        } else {
            Tcl_ListObjAppendElement(NULL, list, Tcl_NewStringObj(path, -1));
        }
    }
    return TCL_OK;
}

int SevenzipArchiveCmd::Extract(Tcl_Obj *source, Tcl_Obj *destination, Tcl_Obj *password, bool usechannel) {
    DEBUGLOG(this << " SevenzipArchiveCmd::Extract item \"" << Tcl_GetString(source)
            << "\" to \"" << Tcl_GetString(destination) << "\""
            << (password ? " with password" : " without password")
            << (usechannel ? " using channel" : " without channel"));
    int result = TCL_OK;
    unsigned int count = archive.getNumberOfItems();
    if (count < 0)
        return TCL_ERROR;
    unsigned int i;
    for (i = 0; i < count; ++i) {
        if (archive.getItemIsDir(i))
            continue;            

#ifdef _WIN32
        char* path = Path_WindowsPathToUnixPath(sevenzip::toBytes(archive.getItemPath(i)));
#else
        char* path = sevenzip::toBytes(archive.getItemPath(i));
#endif
        if (strcmp(path, Tcl_GetString(source)) == 0) {
            // FIXME: use automatic memory management for stream?
            HRESULT hr = S_OK;
            auto *stream = new SevenzipOutStream(tclInterp);

            if (usechannel) {
                hr = stream->AttachOpenChannel(destination);
            } else {
                char *sep = strrchr(path, '/');
#ifdef _WIN32
                char *sep2 = strrchr(path, '\\');
                if (sep2 && (!sep || sep2 > sep))
                    sep = sep2;
#endif
                if (sep)
                    *sep = '\0';
                if (sep)               
                    createDirectory(tclInterp, Tcl_NewStringObj(path, -1));
                hr = stream->AttachFileChannel(destination);
            }

            if (hr == S_OK)
                hr = archive.extract(*stream, 
                        password ? sevenzip::fromBytes(Tcl_GetString(password)) : NULL, i);

            if (!usechannel)
                Tcl_Close(tclInterp, stream->DetachChannel());

            delete stream;

            if (hr != S_OK)
                result = lastError(tclInterp, hr);

            break;
        }
    }
    if (i >= count) {
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf("no such item \"%s\" in the archive", Tcl_GetString(source)));
        result = TCL_ERROR;
    }
    return result;
}

int SevenzipArchiveCmd::LastError(HRESULT hr) {
    if (hr == S_OK)
        hr = sevenzip::getResult(false);
    Tcl_SetObjResult(tclInterp, Tcl_NewStringObj(
            sevenzip::toBytes(sevenzip::getMessage(hr)), -1));
    return TCL_ERROR;
}

static int Tcl_StringCaseEqual(const char *str1, const char *str2, int nocase) {
    Tcl_Size len1 = Tcl_NumUtfChars(str1, -1);
    Tcl_Size len2 = Tcl_NumUtfChars(str2, -1);
    if (len1 != len2)
        return 0;
    return 0 == (nocase ? Tcl_UtfNcasecmp(str1, str2, len1) : Tcl_UtfNcmp(str1, str2, len1));
}

#ifdef _WIN32
static char *Path_WindowsPathToUnixPath(char *path) {
    if (path)
        for (auto p = path; *p; p++)
            if (*p == '\\')
                *p = '/';
    return path;
}
#endif
