#include "sevenzipstream.hpp"

#include <sevenzip.h>
#include <string.h>
#include <wchar.h>

#ifdef _WIN32
#include <sys/utime.h>
#else
#include <utime.h>
#endif

#include <sys/stat.h>
#ifndef S_ISDIR 
#define S_ISDIR(_m_) (((_m_) & _S_IFDIR) == _S_IFDIR)
#endif

#if defined(SEVENZIPSTREAM_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::wcerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

enum {ATTR_READONLY, ATTR_HIDDEN, ATTR_SYSTEM, ATTR_ARCHIVE, ATTR_PERMISSIONS, ATTR_COUNT};
static const char* const attrStrings[ATTR_COUNT] = {"-readonly", "-hidden", "-system", "-archive", "-permissions"};
static UInt32 attrMasks[ATTR_COUNT] = {0x01, 0x02, 0x04, 0x20, 0x00};
static void getAttrIndices(Tcl_Obj *name, int *indices);

SevenzipInStream::SevenzipInStream(Tcl_Interp *interp) : 
        tclInterp(interp), tclChannel(NULL), attached(false),
        statBuf(Tcl_AllocStatBuf()), statPath(NULL) {
    DEBUGLOG(this << " SevenzipInStream");            
}

SevenzipInStream::~SevenzipInStream() {
    DEBUGLOG(this << " ~SevenzipInStream");
    if (statPath)
        ckfree(statPath);
    ckfree(statBuf);
    Close();
}

HRESULT SevenzipInStream::Open(const wchar_t *filename) {
    DEBUGLOG(this << " SevenzipInStream::Open " << (filename ? filename : L"NULL"));
    if (attached)
        return S_OK;
    if (!filename)
        return E_FAIL;

    tclChannel = getFileChannel(tclInterp,
            Tcl_NewStringObj(sevenzip::toBytes(filename), -1), false);
    DEBUGLOG(this << " SevenzipInStream::Open channel " << tclChannel << " errno " << Tcl_GetErrno());
    return sevenzip::getResult(tclChannel);
}

HRESULT SevenzipInStream::Read(void* data, UInt32 size, UInt32 &processed) {
    DEBUGLOG(this << " SevenzipInStream::Read " << size);
    if (!tclChannel)
        return S_FALSE;

    Tcl_Size result = Tcl_Read(tclChannel, (char *)data, (Tcl_Size)size);
    processed = (UInt32)result;
    if (result < 0)
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                "couldn't read from input stream: %s", Tcl_PosixError(tclInterp)));
    DEBUGLOG(this << " SevenzipInStream::Read processed " << result << " errno " << Tcl_GetErrno());
    return sevenzip::getResult(result >= 0);
}

HRESULT SevenzipInStream::Seek(Int64 offset, UInt32 origin, UInt64 &position) {
    DEBUGLOG(this << " SevenzipInStream::Seek " << offset << " as " << origin);
    if (!tclChannel)
        return S_FALSE;

    long long result = Tcl_Seek(tclChannel, offset, origin);
    position = (UInt64)result;
    if (result < 0)
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                "couldn't seek on input stream: %s", Tcl_PosixError(tclInterp)));
    DEBUGLOG(this << " SevenzipInStream::Seek position " << result << " errno " << Tcl_GetErrno());
    return sevenzip::getResult(result >= 0);
}

void SevenzipInStream::Close() {
    DEBUGLOG(this << " SevenzipInStream::Close channel " << tclChannel << " attached " << attached);
    if (tclChannel && !attached) {
        Tcl_Close(tclInterp, tclChannel);
        tclChannel = NULL;
    }
}

sevenzip::Istream *SevenzipInStream::Clone() const {
    DEBUGLOG(this << " SevenzipInStream::Clone");
    return new SevenzipInStream(tclInterp);
}

bool SevenzipInStream::IsDir(const wchar_t* pathname) {
    DEBUGLOG(this << " SevenzipInStream::IsDir " << (pathname ? pathname : L"NULL"));
    if (attached)
        return false;

    auto *stat = getStatBuf(pathname);
    if (stat)
        return S_ISDIR(Tcl_GetModeFromStat(stat));
    return false;
}

UInt64 SevenzipInStream::GetSize(const wchar_t* pathname) {
    DEBUGLOG(this << " SevenzipInStream::GetSize " << (pathname ? pathname : L"NULL"));
    if (attached)
        return 0;

    auto *stat = getStatBuf(pathname);
    if (stat)
        return Tcl_GetSizeFromStat(stat);
    return 0;
}

UInt32 SevenzipInStream::GetMode(const wchar_t *pathname) {
    DEBUGLOG(this << " SevenzipInStream::GetMode " << (pathname ? pathname : L"NULL"));
    if (attached)
        return 0;

    auto *stat = getStatBuf(pathname);
    if (stat)
        return Tcl_GetModeFromStat(stat);
    return 0;

    // NOTE: could also get mode using attributes (see SetMode)
    // NOTE: stat buf can return 100644 vs 00644 from attributes
    // UInt32 mode = 0;
    // int indices[ATTR_COUNT];
    // Tcl_Obj *name = Tcl_NewStringObj(sevenzip::toBytes(pathname), -1);
    // Tcl_IncrRefCount(name);
    // getAttrIndices(name, indices);
    // if (indices[ATTR_PERMISSIONS] >= 0) {
    //     Tcl_Obj *modeValue;
    //     if (Tcl_FSFileAttrsGet(NULL, indices[ATTR_PERMISSIONS], name, &modeValue) == TCL_OK) {
    //         mode = strtol(Tcl_GetString(modeValue), NULL, 8);
    //         DEBUGLOG(this << " SevenzipOutStream::GetMode modeValue " << Tcl_GetString(modeValue) << " " << mode);
    //     }
    // }
    // Tcl_DecrRefCount(name);
    // return mode;
}

UInt32 SevenzipInStream::GetAttr(const wchar_t *pathname) {
    DEBUGLOG(this << " SevenzipInStream::GetAttr " << (pathname ? pathname : L"NULL"));
    if (!pathname)
        return 0;
    if (attached)
        return 0;

    UInt32 attr = 0;
    int indices[ATTR_COUNT];
    Tcl_Obj *name = Tcl_NewStringObj(sevenzip::toBytes(pathname), -1);
    Tcl_IncrRefCount(name);
    getAttrIndices(name, indices);
    for (int i = 0; i < ATTR_COUNT; i++)
        if (indices[i] >= 0) {
            int attrSet;
            Tcl_Obj *attrValue;
            if (Tcl_FSFileAttrsGet(NULL, indices[i], name, &attrValue) != TCL_OK)
                continue;
            if (Tcl_GetBooleanFromObj(NULL, attrValue, &attrSet) != TCL_OK)
                continue;
            if (attrSet)
                attr |= attrMasks[i];
        }
    Tcl_DecrRefCount(name);
    return attr;
}

UInt32 SevenzipInStream::GetTime(const wchar_t *pathname) {
    DEBUGLOG(this << " SevenzipInStream::GetTime " << (pathname ? pathname : L"NULL"));
    if (attached)
        return 0;

    auto *stat = getStatBuf(pathname);
    if (stat)
        return (UInt32)Tcl_GetModificationTimeFromStat(stat);        
    return 0;
}

HRESULT SevenzipInStream::AttachOpenChannel(Tcl_Obj *channel) {
    DEBUGLOG(this << " SevenzipInStream::AttachOpenChannel " << (channel ? Tcl_GetString(channel) : "NULL"));
    if (tclChannel || attached)
        return S_FALSE;

    tclChannel = getOpenChannel(tclInterp, channel, false);
    if (!tclChannel)
        return E_FAIL;
    attached = true;
    return S_OK;
};

HRESULT SevenzipInStream::AttachFileChannel(Tcl_Obj *filename) {
    DEBUGLOG(this << " SevenzipInStream::AttachFileChannel " << (filename ? Tcl_GetString(filename) : "NULL"));
    if (tclChannel || attached)
        return S_FALSE;

    tclChannel = getFileChannel(tclInterp, filename, true);
    if (!tclChannel)
        return E_FAIL;
    attached = true;
    return S_OK;
};

Tcl_Channel SevenzipInStream::DetachChannel() {
    DEBUGLOG(this << " SevenzipInStream::DetachChannel channel " << tclChannel << " attached " << attached);
    if (!tclChannel)
        return NULL;

    Tcl_Channel channel = tclChannel;
    tclChannel = NULL;
    attached = false;
    return channel;
};

Tcl_StatBuf *SevenzipInStream::getStatBuf(const wchar_t* pathname) {
    if (!pathname)
        return NULL;
    if (statPath && wcscmp(statPath, pathname) == 0)
        return statBuf;
    if (statPath)
        ckfree((char *)statPath);

    statPath = (wchar_t *)ckalloc((wcslen(pathname) + 1) * sizeof(wchar_t));
    wcscpy(statPath, pathname);

    Tcl_Obj *name = Tcl_NewStringObj(sevenzip::toBytes(pathname), -1);
    Tcl_IncrRefCount(name);
    int result = Tcl_FSStat(name, statBuf);
    Tcl_DecrRefCount(name);
    if (result != TCL_OK)
        return NULL;
    return statBuf;
}


SevenzipOutStream::SevenzipOutStream(Tcl_Interp *interp):
        tclInterp(interp), tclChannel(NULL), attached(false) {
    DEBUGLOG(this << " SevenzipOutStream");
}

SevenzipOutStream::~SevenzipOutStream() {
    DEBUGLOG(this << " ~SevenzipOutStream");
    Close();
}

HRESULT SevenzipOutStream::Open(const wchar_t *filename) {
    DEBUGLOG(this << " SevenzipOutStream::Open " << (filename ? filename : L"NULL"));
    if (attached)
        return S_OK;
    if (!filename)
        return E_FAIL;

    tclChannel = getFileChannel(tclInterp,
            Tcl_NewStringObj(sevenzip::toBytes(filename), -1), true);
    DEBUGLOG(this << " SevenzipOutStream::Open channel " << tclChannel << " errno " << Tcl_GetErrno());
    return sevenzip::getResult(tclChannel);
}

HRESULT SevenzipOutStream::Write(const void *data, UInt32 size, UInt32 &processed) {
    DEBUGLOG(this << " SevenzipOutStream::Write " << size);
    if (!tclChannel)
        return S_FALSE;

    Tcl_Size result = Tcl_Write(tclChannel, (const char *)data, (Tcl_Size)size);
    processed = (UInt32)result;
    if (result < 0)
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                "couldn't write to output stream: %s", Tcl_PosixError(tclInterp)));
    DEBUGLOG(this << " SevenzipOutStream::Write processed " << result << " errno " << Tcl_GetErrno());
    return sevenzip::getResult(result >= 0);
}

HRESULT SevenzipOutStream::Seek(Int64 offset, UInt32 origin, UInt64 &position) {
    DEBUGLOG(this << " SevenzipOutStream::Seek " << offset << " as " << origin);
    if (!tclChannel)
        return S_FALSE;

    long long result = Tcl_Seek(tclChannel, offset, origin);
    position = (UInt64)result;
    if (result < 0)
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                "couldn't seek on output stream: %s", Tcl_PosixError(tclInterp)));
    DEBUGLOG(this << " SevenzipOutStream::Seek position " << result << " errno " << Tcl_GetErrno());
    return sevenzip::getResult(result >= 0);
}

void SevenzipOutStream::Close() {
    DEBUGLOG(this << " SevenzipOutStream::Close channel " << tclChannel << " attached " << attached);
    if (tclChannel && !attached) {
        Tcl_Close(tclInterp, tclChannel);
        tclChannel = NULL;
    }
}

HRESULT SevenzipOutStream::Mkdir(const wchar_t* pathname) {
    DEBUGLOG(this << " SevenzipOutStream::Mkdir " << (pathname ? pathname : L"NULL"));
    if (attached)
        return S_OK;

    return createDirectory(tclInterp,
            Tcl_NewStringObj(sevenzip::toBytes(pathname), -1)) ? S_OK : S_FALSE;
}

// HRESULT SevenzipOutStream::SetSize(const wchar_t* pathname, UInt64 size) {
//     DEBUGLOG(this << " SevenzipOutStream::SetSize " << pathname << " " << size);
//     return S_OK;
// }

HRESULT SevenzipOutStream::SetMode(const wchar_t* pathname, UInt32 mode) {
    DEBUGLOG(this << " SevenzipOutStream::SetMode " << (pathname ? pathname : L"NULL") << " " << std::oct << mode);
    if (attached)
        return S_OK;

    int indices[ATTR_COUNT];
    Tcl_Obj *name = Tcl_NewStringObj(sevenzip::toBytes(pathname), -1);
    Tcl_IncrRefCount(name);
    getAttrIndices(name, indices);
    if (indices[ATTR_PERMISSIONS] >= 0) {
        // NOTE: using integer below looks less expensive
        // Tcl_Obj *modeValue = Tcl_ObjPrintf("%o", mode);
        Tcl_Obj *modeValue = Tcl_NewIntObj(mode);
        Tcl_IncrRefCount(modeValue);
        Tcl_FSFileAttrsSet(NULL, indices[ATTR_PERMISSIONS], name, modeValue);
        Tcl_DecrRefCount(modeValue);
    }
    Tcl_DecrRefCount(name);

    return S_FALSE;
}

HRESULT SevenzipOutStream::SetAttr(const wchar_t* pathname, UInt32 attr) {
    DEBUGLOG(this << " SevenzipOutStream::SetAttr " << (pathname ? pathname : L"NULL") << " " << std::hex << attr);
    if (!pathname)
        return 0;
    if (attached)
        return 0;

    int indices[ATTR_COUNT];
    Tcl_Obj *name = Tcl_NewStringObj(sevenzip::toBytes(pathname), -1);
    Tcl_IncrRefCount(name);
    getAttrIndices(name, indices);
    for (int i = 0; i < ATTR_COUNT; i++) {
        if ((attr & attrMasks[i]) && indices[i] >= 0) {
            Tcl_Obj *attrValue = Tcl_NewBooleanObj(true);
            // Tcl_Obj *attrValue = Tcl_ObjPrintf("%d",  != 0);
            Tcl_IncrRefCount(attrValue);
            Tcl_FSFileAttrsSet(NULL, indices[i], name, attrValue);
            Tcl_DecrRefCount(attrValue);
        }
    }
    Tcl_DecrRefCount(name);
    return S_OK;
}

HRESULT SevenzipOutStream::SetTime(const wchar_t* pathname, UInt32 time) {
    DEBUGLOG(this << " SevenzipOutStream::SetTime " << (pathname ? pathname : L"NULL") << " " << time);
    if (attached)
        return S_OK;

    struct utimbuf tval;
    memset(&tval, 0, sizeof(tval));
	tval.modtime = time;
    Tcl_Obj *name = Tcl_NewStringObj(sevenzip::toBytes(pathname), -1);
    Tcl_IncrRefCount(name);
	Tcl_FSUtime(name, &tval);
    Tcl_DecrRefCount(name);
    return S_FALSE;
}

HRESULT SevenzipOutStream::AttachOpenChannel(Tcl_Obj *channel) {
    DEBUGLOG(this << " SevenzipOutStream::AttachOpenChannel " << (channel ? Tcl_GetString(channel) : "NULL"));
    if (tclChannel || attached)
        return S_FALSE;

    tclChannel = getOpenChannel(tclInterp, channel, true);
    if (!tclChannel)
        return E_FAIL;
    attached = true;
    return S_OK;
};

HRESULT SevenzipOutStream::AttachFileChannel(Tcl_Obj *filename) {
    DEBUGLOG(this << " SevenzipOutStream::AttachFileChannel " << (filename ? Tcl_GetString(filename) : "NULL"));
    if (tclChannel || attached)
        return S_FALSE;

    tclChannel = getFileChannel(tclInterp, filename, true);
    if (!tclChannel)
        return E_FAIL;
    attached = true;
    return S_OK;
};

Tcl_Channel SevenzipOutStream::DetachChannel() {
    DEBUGLOG(this << " SevenzipOutStream::DetachChannel");
    if (!tclChannel)
        return NULL;

    Tcl_Channel channel = tclChannel;
    tclChannel = NULL;
    attached = false;
    return channel;
};

bool createDirectory(Tcl_Interp *tclInterp, Tcl_Obj *dirname) {
    Tcl_IncrRefCount(dirname);
    int result = Tcl_FSCreateDirectory(dirname);
    Tcl_DecrRefCount(dirname);
    return result == TCL_OK;
}

int lastError(Tcl_Interp *interp, HRESULT hr) {
    if (Tcl_GetCharLength(Tcl_GetObjResult(interp)) == 0) {
        if (hr == S_OK)
            hr = sevenzip::getResult(true);
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
                sevenzip::toBytes(sevenzip::getMessage(hr)), -1));
    }
    return TCL_ERROR;
}

Tcl_Channel getOpenChannel(Tcl_Interp *tclInterp, Tcl_Obj *channel, bool writable) {
    int mode;
    Tcl_Channel tclChannel = Tcl_GetChannel(tclInterp, Tcl_GetString(channel), &mode);
    if (tclChannel == NULL) {
        return NULL;
    }
    if (((writable && (mode & TCL_WRITABLE) != TCL_WRITABLE) ||
            (!writable && (mode & TCL_READABLE) != TCL_READABLE))) {
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf("channel \"%s\" wasn't opened for %s",
                Tcl_GetString(channel), writable ? "writing" : "reading" ));
        return NULL;
    }
    if (Tcl_SetChannelOption(tclInterp, tclChannel, "-translation", "binary") != TCL_OK ||
            Tcl_SetChannelOption(tclInterp, tclChannel, "-blocking", "0") != TCL_OK) {
        return NULL;
    }
    return tclChannel;
}

Tcl_Channel getFileChannel(Tcl_Interp *tclInterp, Tcl_Obj *filename, bool writable) {
    Tcl_IncrRefCount(filename);
    Tcl_Channel tclChannel = Tcl_FSOpenFileChannel(tclInterp, filename, writable ? "wb" : "rb", 0644);
    if (tclChannel == NULL) {
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf("couldn't open file \"%s\": %s",
                Tcl_GetString(filename), Tcl_PosixError(tclInterp)));
    }
    Tcl_DecrRefCount(filename);
    return tclChannel;
}

static void getAttrIndices(Tcl_Obj *name, int *indices) {
    for (int i = 0; i < ATTR_COUNT; i++)
        indices[i] = -1;
    const char **strings = NULL;
    Tcl_Obj *stringsList = NULL;
    strings = (const char **)Tcl_FSFileAttrStrings(name, &stringsList);
    if (!strings && stringsList) {
        int length = 0;
    	Tcl_IncrRefCount(stringsList);
	    Tcl_ListObjLength(NULL, stringsList, &length);
    	strings = (const char **)ckalloc((length + 1) * sizeof(char *));
        int index;
        Tcl_Obj *elem;
    	for (index = 0; index < length; index++) {
	        Tcl_ListObjIndex(NULL, stringsList, index, &elem);
	        strings[index] = Tcl_GetString(elem);
        }
    	strings[index] = NULL;
	}
    if (strings) {
        for (int index = 0; strings[index]; index++)
            for (int i = 0; i < ATTR_COUNT; i++)
                if (strcmp(strings[index], attrStrings[i]) == 0)
                    indices[i] = index;
        if (stringsList) {
            ckfree((char *)strings);
            Tcl_DecrRefCount(stringsList);
        }
    }
}
