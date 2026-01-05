#include "sevenzipstream.hpp"

#include <sevenzip.h>

#if defined(SEVENZIPSTREAM_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::wcerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

SevenzipInStream::SevenzipInStream(Tcl_Interp *interp) : 
        tclInterp(interp), tclChannel(NULL), usechannel(false) {
    DEBUGLOG(this << " SevenzipInStream");            
}

SevenzipInStream::~SevenzipInStream() {
    DEBUGLOG(this << " ~SevenzipInStream, tclChannel " << tclChannel << " usechannel " << usechannel);
    Close();
}

HRESULT SevenzipInStream::Open(const wchar_t *filename) {
    DEBUGLOG(this << " SevenzipInStream::Open " << filename);
    if (usechannel)
        return S_OK;
    if (!filename)
        return E_FAIL;

    tclChannel = getFileChannel(tclInterp,
            Tcl_NewStringObj(sevenzip::toBytes(filename), -1), false);
    DEBUGLOG(this << " SevenzipInStream::Opened " << tclChannel << " errno " << Tcl_GetErrno());
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
    DEBUGLOG(this << " SevenzipInStream::Close");
    if (tclChannel && !usechannel) {
        Tcl_Close(tclInterp, tclChannel);
        tclChannel = NULL;
    }
}

sevenzip::Istream *SevenzipInStream::Clone() const {
    DEBUGLOG(this << " SevenzipInStream::Clone");
    return new SevenzipInStream(tclInterp);
}

bool SevenzipInStream::IsDir(const wchar_t* filename) const {
    DEBUGLOG(this << " SevenzipInStream::IsDir " << filename);
    if (usechannel)
        return false;
    // TODO: implement directory checking
    return false;
}

//UInt64 GetSize(const wchar_t* /*filename*/) const {}

UInt32 SevenzipInStream::GetMode(const wchar_t *filename) const {
    DEBUGLOG(this << "  SevenzipInStream::GetMode " << filename);
    if (usechannel)
        return 0;
    // TODO: implement mode retrieval
    return 0;
}

UInt32 SevenzipInStream::GetTime(const wchar_t *filename) const {
    DEBUGLOG(this << " SevenzipInStream::GetTime " << filename);
    if (usechannel)
        return 0;
    // TODO: implement time retrieval
    return 0;
}

HRESULT SevenzipInStream::AttachOpenChannel(Tcl_Obj *channel) {
    DEBUGLOG(this << " SevenzipInStream::AttachOpenChannel " << Tcl_GetString(channel));
    if (tclChannel || usechannel)
        return S_FALSE;

    tclChannel = getOpenChannel(tclInterp, channel, false);
    if (!tclChannel)
        return E_FAIL;
    usechannel = true;
    return S_OK;
};

HRESULT SevenzipInStream::AttachFileChannel(Tcl_Obj *filename) {
    DEBUGLOG(this << " SevenzipInStream::AttachFileChannel " << Tcl_GetString(filename));
    if (tclChannel || usechannel)
        return S_FALSE;

    tclChannel = getFileChannel(tclInterp, filename, true);
    if (!tclChannel)
        return E_FAIL;
    usechannel = true;
    return S_OK;
};

Tcl_Channel SevenzipInStream::DetachChannel() {
    DEBUGLOG(this << " SevenzipInStream::DetachChannel");
    if (!tclChannel)
        return NULL;

    Tcl_Channel channel = tclChannel;
    tclChannel = NULL;
    usechannel = false;
    return channel;
};


SevenzipOutStream::SevenzipOutStream(Tcl_Interp *interp):
        tclInterp(interp), tclChannel(NULL), usechannel(false) {
    DEBUGLOG(this << " SevenzipOutStream");
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

    tclChannel = getFileChannel(tclInterp,
            Tcl_NewStringObj(sevenzip::toBytes(filename), -1), true);
    DEBUGLOG(this << " SevenzipOutStream::Opened " << tclChannel << " errno " << Tcl_GetErrno());
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
    DEBUGLOG(this << " SevenzipOutStream::Close");
    if (tclChannel && !usechannel) {
        Tcl_Close(tclInterp, tclChannel);
        tclChannel = NULL;
    }
}

HRESULT SevenzipOutStream::Mkdir(const wchar_t* dirname) {
    DEBUGLOG(this << " SevenzipOutStream::Mkdir " << dirname);
    if (usechannel)
        return S_OK;
    return createDirectory(tclInterp,
            Tcl_NewStringObj(sevenzip::toBytes(dirname), -1)) ? S_OK : S_FALSE;
}

HRESULT SevenzipOutStream::SetMode(const wchar_t* path, UInt32 mode) {
    DEBUGLOG(this << " SevenzipOutStream::SetMode " << path << " " << std::oct << mode);
    if (usechannel)
        return S_OK;
    // TODO: implement mode setting
    return S_FALSE;
}

HRESULT SevenzipOutStream::SetTime(const wchar_t* filename, UInt32 time) {
    DEBUGLOG(this << " SevenzipOutStream::SetTime " << filename << " " << time);
    if (usechannel)
        return S_OK;
    // TODO: implement time setting
    return S_FALSE;
}

HRESULT SevenzipOutStream::AttachOpenChannel(Tcl_Obj *channel) {
    DEBUGLOG(this << " SevenzipOutStream::AttachOpenChannel " << Tcl_GetString(channel));
    if (tclChannel || usechannel)
        return S_FALSE;

    tclChannel = getOpenChannel(tclInterp, channel, true);
    if (!tclChannel)
        return E_FAIL;
    usechannel = true;
    return S_OK;
};

HRESULT SevenzipOutStream::AttachFileChannel(Tcl_Obj *filename) {
    DEBUGLOG(this << " SevenzipOutStream::AttachFileChannel " << Tcl_GetString(filename));
    if (tclChannel || usechannel)
        return S_FALSE;

    tclChannel = getFileChannel(tclInterp, filename, true);
    if (!tclChannel)
        return E_FAIL;
    usechannel = true;
    return S_OK;
};

Tcl_Channel SevenzipOutStream::DetachChannel() {
    DEBUGLOG(this << " SevenzipOutStream::DetachChannel");
    if (!tclChannel)
        return NULL;
    Tcl_Channel channel = tclChannel;
    tclChannel = NULL;
    usechannel = false;
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
