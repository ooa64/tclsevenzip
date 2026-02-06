#include "sevenziparchivecmd.hpp"

#include <string.h>
#include <wchar.h>

#if defined(SEVENZIPARCHIVECMD_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::cerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

#define LIST_MATCH_NOCASE TCL_MATCH_NOCASE
#define LIST_MATCH_EXACT (1 << 16)

// from CPP/Common/MyWindows.h - only the needed values
enum {
    VT_I2 = 2,
    VT_I4 = 3,
    VT_BSTR = 8,
    VT_BOOL = 11,
    VT_I1 = 16,
    VT_UI1 = 17,
    VT_UI2 = 18,
    VT_UI4 = 19,
    VT_I8 = 20,
    VT_UI8 = 21,
    VT_FILETIME = 64
};

// from /CPP/7zip/PropID.h - only the needed values
enum {
    kpidPath = 3,
    kpidIsDir = 6,
    kpidPhySize = 44
};

// from /CPP/7zip/PropID.h - all property names
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
    DEBUGLOG(this << " SevenzipArchiveCmd " << (name ? name : "NULL"));
}

SevenzipArchiveCmd::~SevenzipArchiveCmd() {
    DEBUGLOG(this << " ~SevenzipArchiveCmd");
    Close();
}

HRESULT SevenzipArchiveCmd::Open(sevenzip::Lib& lib, SevenzipInStream* stream,
        Tcl_Obj* filename, Tcl_Obj* password, int formatIndex) {
    DEBUGLOG(this << " SevenzipArchiveCmd::Open"
            << " " << (filename ? Tcl_GetString(filename) : "NULL")
            << " " << (password ? Tcl_GetString(password) : "NULL"));
    if (!stream)
        return E_FAIL;
    if (this->stream)
        return E_FAIL;
    this->stream = stream;

    wchar_t buffer[1024];
    return archive.open(lib, *stream,
            filename ? sevenzip::fromBytes(Tcl_GetString(filename)) : NULL,
            password ? sevenzip::fromBytes(buffer, sizeof(buffer)/sizeof(buffer[0]), Tcl_GetString(password)) : NULL,
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
                return lastError(tclInterp, 0);
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
            if (Extract(objv[objc-1], objv[objc-2], password, usechannel) != TCL_OK)
                return TCL_ERROR;
        } else {
            Tcl_WrongNumArgs(tclInterp, 2, objv, "?options? path item");
            return TCL_ERROR;
        }
        break;

    case cmClose:
        if (objc > 2) {
            Tcl_WrongNumArgs(tclInterp, 2, objv, NULL);
            return TCL_ERROR;
        } else {
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
            const wchar_t* stringValue = NULL;
            bool boolValue = false;
            UInt32 uint32Value = 0;
            UInt64 uint64Value = 0;
            Tcl_Obj *value = NULL;
            switch (propType) {
            case VT_BSTR:
                if (archive.getStringProperty(propId, stringValue) == S_OK)
                    value = Tcl_NewStringObj(sevenzip::toBytes(stringValue), -1);
                break;
            case VT_BOOL:
                if (archive.getBoolProperty(propId, boolValue) == S_OK)
                    value = Tcl_NewBooleanObj(boolValue);
                break;
            case VT_I1:
            case VT_I2:
            case VT_I4:
            case VT_UI1:
            case VT_UI2:
            case VT_UI4:
                if (archive.getIntProperty(propId, uint32Value) == S_OK)
                    value = Tcl_NewWideIntObj(uint32Value);
                break;
            case VT_I8:
            case VT_UI8:
                if (archive.getWideProperty(propId, uint64Value) == S_OK)
                    value = Tcl_NewWideIntObj(uint64Value);
                break;
            case VT_FILETIME:
                if (archive.getTimeProperty(propId, uint32Value) == S_OK)
                    value = Tcl_NewWideIntObj(uint32Value);
                break;
            default:
                DEBUGLOG(this << " SevenzipArchiveCmd unknown prop id " << propId << " type " << propType);
                break;
            }
            if (value) {
                Tcl_ListObjAppendElement(NULL, info, 
                    propId < sizeof(SevenzipProperties)/sizeof(SevenzipProperties[0]) 
                        ? Tcl_NewStringObj(SevenzipProperties[propId], -1)
                        : Tcl_ObjPrintf("prop%d", propId));
                Tcl_ListObjAppendElement(NULL, info, value);
            } else {
                // DEBUGLOG(this << " SevenzipArchiveCmd unhandled prop id " << propId << " type " << propType);
            }
        }
    }
    return TCL_OK;
}

int SevenzipArchiveCmd::List(Tcl_Obj *list, Tcl_Obj *pattern, char type, int flags, bool info) {
    DEBUGLOG(this << " SevenzipArchiveCmd::List " << (pattern ? Tcl_GetString(pattern) : "NULL")
            << " " << type << " " << flags << " " << info);
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
            Tcl_Obj *prop = Tcl_NewObj();
            bool haveIsDirProperty = false;
            int n = archive.getNumberOfItemProperties();
            for (int j = 0; j < n; j++) {
                PROPID propId;
                VARTYPE propType;
                if (archive.getItemPropertyInfo(j, propId, propType) == S_OK) {
                    const wchar_t* stringValue = NULL;
                    bool boolValue = false;
                    UInt32 uint32Value = 0;
                    UInt64 uint64Value = 0;
                    Tcl_Obj *value = NULL;
                    switch (propType) {
                    case VT_BSTR:
#ifdef _WIN32
                        if (propId == kpidPath) {
                            if (archive.getStringItemProperty(i, propId, stringValue) == S_OK)
                                value = Tcl_NewStringObj(Path_WindowsPathToUnixPath(sevenzip::toBytes(stringValue)), -1);
                            break;
                        }
#endif
                        if (archive.getStringItemProperty(i, propId, stringValue) == S_OK)
                            value = Tcl_NewStringObj(sevenzip::toBytes(stringValue), -1);
                        break;
                    case VT_BOOL:
                        if (archive.getBoolItemProperty(i, propId, boolValue) == S_OK)
                            value = Tcl_NewBooleanObj(boolValue);
                        break;
                    case VT_I1:
                    case VT_I2:
                    case VT_I4:
                    case VT_UI1:
                    case VT_UI2:
                    case VT_UI4:
                        if (archive.getIntItemProperty(i, propId, uint32Value) == S_OK)
                            value = Tcl_NewWideIntObj(uint32Value);
                        // NOTE: see note below about some 64bit values
                        else if (archive.getWideItemProperty(i, propId, uint64Value) == S_OK)
                            value = Tcl_NewWideIntObj(uint64Value);
                        break;
                    case VT_I8:
                    case VT_UI8:
                        if (archive.getWideItemProperty(i, propId, uint64Value) == S_OK)
                            value = Tcl_NewWideIntObj(uint64Value);
                        // NOTE: some 64bit values (like arj size) are returned as VT_UI4
                        else if (archive.getIntItemProperty(i, propId, uint32Value) == S_OK)
                            value = Tcl_NewWideIntObj(uint32Value);
                        break;
                    case VT_FILETIME:
                        if (archive.getTimeItemProperty(i, propId, uint32Value) == S_OK)
                            value = Tcl_NewWideIntObj(uint32Value);
                        break;
                    default:
                        // DEBUGLOG(this << " SevenzipArchiveCmd::List info unknown item " << i << " prop id " << propId << " type " << propType);
                        break;
                    }
                    if (value) {
                        Tcl_ListObjAppendElement(NULL, prop, 
                                propId < sizeof(SevenzipProperties)/sizeof(SevenzipProperties[0]) 
                                    ? Tcl_NewStringObj(SevenzipProperties[propId], -1)
                                    : Tcl_ObjPrintf("prop%d", propId));
                        Tcl_ListObjAppendElement(NULL, prop, value);
                    } else {
                        // DEBUGLOG(this << " SevenzipArchiveCmd::List info unhandled item " << i << " prop id " << propId << " type " << propType);
                    }
                    if (propId == kpidIsDir)
                        haveIsDirProperty = true;
                }
            }
            if (!haveIsDirProperty) {
                // append missing but useful isdir property
                Tcl_ListObjAppendElement(NULL, prop, Tcl_NewStringObj("isdir", -1));
                Tcl_ListObjAppendElement(NULL, prop, Tcl_NewBooleanObj(archive.getItemIsDir(i)));
            }

            // NOTE: above code may not list all properties (isdir,isanti,...?)
            // NOTE: There is an alternative way to get all properties
            
            // for (PROPID propId = 0; propId < sizeof(SevenzipProperties)/sizeof(SevenzipProperties[0]); propId++) {
            //     const wchar_t* stringValue;
            //     bool boolValue;
            //     UInt32 uint32Value;
            //     UInt64 uint64Value;
            //     if (archive.getStringItemProperty(i, propId, stringValue) == S_OK) {
            //         Tcl_ListObjAppendElement(NULL, prop, Tcl_NewStringObj(SevenzipProperties[propId], -1));
            //         Tcl_ListObjAppendElement(NULL, prop, Tcl_NewStringObj(sevenzip::toBytes(stringValue), -1));
            //     } else if (archive.getBoolItemProperty(i, propId, boolValue) == S_OK) {
            //         Tcl_ListObjAppendElement(NULL, prop, Tcl_NewStringObj(SevenzipProperties[propId], -1));
            //         Tcl_ListObjAppendElement(NULL, prop, Tcl_NewBooleanObj(boolValue));
            //     } else if (archive.getIntItemProperty(i, propId, uint32Value) == S_OK) {
            //         Tcl_ListObjAppendElement(NULL, prop, Tcl_NewStringObj(SevenzipProperties[propId], -1));
            //         Tcl_ListObjAppendElement(NULL, prop, Tcl_NewIntObj(uint32Value));
            //     } else if (archive.getWideItemProperty(i, propId, uint64Value) == S_OK) {
            //         Tcl_ListObjAppendElement(NULL, prop, Tcl_NewStringObj(SevenzipProperties[propId], -1));
            //         Tcl_ListObjAppendElement(NULL, prop, Tcl_NewWideIntObj(uint64Value));
            //     } else {
            //         DEBUGLOG(this << "SevenzipArchiveCmd unhandled item " << i << " prop " << SevenzipProperties[propId]);
            //     }
            // }
            Tcl_ListObjAppendElement(NULL, list, prop);
        } else {
            Tcl_ListObjAppendElement(NULL, list, Tcl_NewStringObj(path, -1));
        }
    }
    return TCL_OK;
}

int SevenzipArchiveCmd::Extract(Tcl_Obj *source, Tcl_Obj *destination, Tcl_Obj *password, bool usechannel) {
    DEBUGLOG(this << " SevenzipArchiveCmd::Extract"
            << " " << (source ? Tcl_GetString(source) : "NULL")
            << " " << (destination ? Tcl_GetString(destination) : "NULL")
            << " " << (password ? Tcl_GetString(password) : "NULL")
            << " " << usechannel);
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
            SevenzipOutStream stream(tclInterp);

            // NOTE: use single thread to avoid Tcl threading issues    
            archive.addBoolOption(L"mt", false);

            HRESULT hr = usechannel
                ? stream.AttachOpenChannel(destination)
                : stream.AttachFileChannel(destination);

            if (hr == S_OK)
                hr = archive.extract(stream, 
                        password ? sevenzip::fromBytes(Tcl_GetString(password)) : NULL, i);

            if (!usechannel) {
                // NOTE: detach channel to 1) close it, and 2) enable SetXxxx functions.
                Tcl_Close(tclInterp, stream.DetachChannel());

                if (hr == S_OK) {
                    // NOTE: set file attrs that are not set for the attached channel
                    UInt32 time = archive.getItemTime(i);
                    UInt32 mode = archive.getItemMode(i);
                    UInt32 attr = archive.getItemAttr(i);
                    if (time > 0)
                        stream.SetTime(destination, time);
                    if (mode > 0)
                        stream.SetMode(destination, mode);
                    else if (attr & 0x8000) // unix 7zz/zip attr like  0x81a48020
                        stream.SetMode(destination, attr >> 16);
                    if ((attr & 0x7FFF) > 0)
                        stream.SetAttr(destination, (attr & 0x8000) ? (attr & 0x7FFF) : attr);
                }
            }

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
