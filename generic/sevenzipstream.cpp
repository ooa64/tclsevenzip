#include "sevenzipstream.hpp"

#if defined(SEVENZIPSTREAM_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::wcerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

SevenzipInStream::SevenzipInStream(Tcl_Interp *interp, Tcl_Obj *channel) : 
        tclInterp(interp), tclChannel(NULL), usechannel(false) {
    DEBUGLOG(this << " SevenzipInStream::SevenzipInStream, tclChannel " << tclChannel << " usechannel " << usechannel);
    if (channel) {
        int mode;
        tclChannel = Tcl_GetChannel(interp, Tcl_GetString(channel), &mode);
        if (tclChannel == NULL) {
            return;
        }
        if ((mode & TCL_READABLE) != TCL_READABLE) {
            Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                    "channel \"%s\" wasn't opened for reading", Tcl_GetString(channel)));
            tclChannel = NULL;
            return;
        }
        if (Tcl_SetChannelOption(tclInterp, tclChannel, "-translation", "binary") != TCL_OK ||
                Tcl_SetChannelOption(tclInterp, tclChannel, "-blocking", "0") != TCL_OK) {
            tclChannel = NULL;
            return;
        }
        usechannel = true;
    }            
}

SevenzipInStream::~SevenzipInStream() {
    DEBUGLOG(this << " SevenzipInStream::~SevenzipInStream, tclChannel " << tclChannel << " usechannel " << usechannel);
    Close();
}

HRESULT SevenzipInStream::Open(const wchar_t *filename) {
    DEBUGLOG(this << " SevenzipInStream::Open " << filename);
    if (usechannel)
        return S_OK;
    if (!filename)
        return E_FAIL;

    Tcl_Obj *file = Tcl_NewStringObj(sevenzip::toBytes(filename), -1);
    Tcl_IncrRefCount(file);
    tclChannel = Tcl_FSOpenFileChannel(tclInterp, file, "rb", 0644);
    if (tclChannel == NULL) {
        Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
            "couldn't create file \"%s\": %s", Tcl_GetString(file), Tcl_PosixError(tclInterp)));
        Tcl_DecrRefCount(file);
        return sevenzip::getResult(false);
    }    
    Tcl_DecrRefCount(file);
    return S_OK;
}

HRESULT SevenzipInStream::Read(void* data, UInt32 size, UInt32 &processed) {
    DEBUGLOG(this << " SevenzipInStream::Read " << size);
    if (tclChannel) {
        processed = (unsigned int)Tcl_Read(tclChannel, (char *)data, (Tcl_Size)size);
        if (processed >= 0)
            return S_OK;
    }
    return E_FAIL;
}

HRESULT SevenzipInStream::Seek(Int64 offset, UInt32 origin, UInt64 &position) {
    DEBUGLOG(this << " SevenzipInStream::Seek " << offset << " as " << origin);
    if (tclChannel) {
        position = Tcl_Seek(tclChannel, offset, origin);
        if (position >= 0)
            return S_OK;
    }
    return E_FAIL;
}

void SevenzipInStream::Close() {
    DEBUGLOG(this << " SevenzipInStream::Close");
    if (tclChannel && !usechannel) {
        Tcl_Close(tclInterp, tclChannel);
        tclChannel = NULL;
    }
}

sevenzip::Istream *SevenzipInStream::Clone() const {
    DEBUGLOG(this << " SevenzipInStream::Clone");
    return new SevenzipInStream(tclInterp, NULL);
}

bool SevenzipInStream::IsDir(const wchar_t* filename) const {
    DEBUGLOG(this << " SevenzipInStream::IsDir " << filename);
    return false;
}

//UInt64 GetSize(const wchar_t* /*filename*/) const {}

UInt32 SevenzipInStream::GetMode(const wchar_t *filename) const {
    DEBUGLOG(this << "  SevenzipInStream::GetMode " << filename);
    return 0;
}

UInt32 SevenzipInStream::GetTime(const wchar_t *filename) const {
    DEBUGLOG(this << " SevenzipInStream::GetTime " << filename);
    return 0;
}


SevenzipOutStream::SevenzipOutStream(Tcl_Interp *interp, Tcl_Obj *channel):
        tclInterp(interp), tclChannel(NULL), usechannel(false) {
    DEBUGLOG(this << " SevenzipOutStream with channel " << (channel ? Tcl_GetString(channel) : "NULL"));
    if (channel) {
        int mode;
        tclChannel = Tcl_GetChannel(tclInterp, Tcl_GetString(channel), &mode);
        if (tclChannel == NULL) {
            return;
        }
        if ((mode & TCL_WRITABLE) != TCL_WRITABLE) {
            Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
                "channel \"%s\" wasn't opened for writing", Tcl_GetString(channel)));
            tclChannel = NULL;
            return;
        }
        if (Tcl_SetChannelOption(tclInterp, tclChannel, "-translation", "binary") != TCL_OK ||
                Tcl_SetChannelOption(tclInterp, tclChannel, "-blocking", "0") != TCL_OK) {
            tclChannel = NULL;
            return;
        }
        usechannel = true;
        }
    }

SevenzipOutStream::~SevenzipOutStream() {
    DEBUGLOG(this << " ~SevenzipOutStream");
    Close();
        }

HRESULT SevenzipOutStream::Open(const wchar_t *filename) {
    DEBUGLOG(this << " SevenzipOutStream::Open " << filename);
    if (usechannel)
        return S_OK;
    if (!filename)
        return E_FAIL;

    Tcl_Obj *file = Tcl_NewStringObj(sevenzip::toBytes(filename), -1);
    Tcl_IncrRefCount(file);
    tclChannel = Tcl_FSOpenFileChannel(tclInterp, file, "wb", 0644);
    if (tclChannel == NULL) {
            Tcl_SetObjResult(tclInterp, Tcl_ObjPrintf(
            "couldn't create file \"%s\": %s", Tcl_GetString(file), Tcl_PosixError(tclInterp)));
        Tcl_DecrRefCount(file);
        return sevenzip::getResult(false);
        }
    Tcl_DecrRefCount(file);
    return S_OK;
}

HRESULT SevenzipOutStream::Write(const void *data, UInt32 size, UInt32 &processed) {
    DEBUGLOG(this << " SevenzipOutStream::Write " << size);
    if (tclChannel) {
        processed = (UInt32)Tcl_Write(tclChannel, (char *)data, (Tcl_Size)size);
        if (processed >= 0)
            return S_OK;
    }
    return E_FAIL;
}

HRESULT SevenzipOutStream::Seek(Int64 offset, UInt32 origin, UInt64 &position) {
    DEBUGLOG(this << " SevenzipOutStream::Seek " << offset << " as " << origin);
    if (tclChannel) {
        position = Tcl_Seek(tclChannel, offset, origin);
        if (position >= 0)
            return S_OK;
        }
    return E_FAIL;
}

void SevenzipOutStream::Close() {
    DEBUGLOG(this << " SevenzipOutStream::Close");
    if (tclChannel && !usechannel) {
        Tcl_Close(tclInterp, tclChannel);
        tclChannel = NULL;
        }
    }

HRESULT SevenzipOutStream::Mkdir(const wchar_t* dirname) {
    DEBUGLOG(this << " SevenzipOutStream::Mkdir " << dirname);
    return S_FALSE;
}

HRESULT SevenzipOutStream::SetMode(const wchar_t* path, UInt32 mode) {
    DEBUGLOG(this << " SevenzipOutStream::SetMode " << path << " " << std::oct << mode);
    return S_FALSE;
    }

HRESULT SevenzipOutStream::SetTime(const wchar_t* filename, UInt32 time) {
    DEBUGLOG(this << " SevenzipOutStream::SetTime " << filename << " " << time);
    return S_FALSE;
}
