#ifndef SEVENZIPSTREAM_H
#define SEVENZIPSTREAM_H

#include <sevenzip.h>
#include <tcl.h>

class SevenzipInStream:  public sevenzip::Istream {

public:

    SevenzipInStream(Tcl_Interp *interp);
    virtual ~SevenzipInStream();

    virtual HRESULT Open(const wchar_t *filename) override;
    virtual HRESULT Read(void* data, UInt32 size, UInt32 &processed) override;
    virtual HRESULT Seek(Int64 offset, UInt32 origin, UInt64 &position) override;
    virtual void Close() override;

    virtual sevenzip::Istream* Clone() const override;
    
    virtual bool IsDir(const wchar_t *pathname) override;
    virtual UInt64 GetSize(const wchar_t *pathname) override;
    virtual UInt32 GetMode(const wchar_t *pathname) override;
    virtual UInt32 GetAttr(const wchar_t *pathname) override;
    virtual UInt32 GetTime(const wchar_t *pathname) override;

    bool IsDir(Tcl_Obj *pathname);
    UInt64 GetSize(Tcl_Obj *pathname);
    UInt32 GetMode(Tcl_Obj *pathname);
    UInt32 GetAttr(Tcl_Obj *pathname);
    UInt32 GetTime(Tcl_Obj *pathname);
    HRESULT AttachOpenChannel(Tcl_Obj *channel);
    HRESULT AttachFileChannel(Tcl_Obj *filename);
    Tcl_Channel DetachChannel();    

private:

    Tcl_Interp *tclInterp;
    Tcl_Channel tclChannel;
    bool attached;

    Tcl_StatBuf *getStatBuf(Tcl_Obj *pathname);
    Tcl_StatBuf *statBuf;
    char *statPath;
};

class SevenzipOutStream:  public sevenzip::Ostream {

public:

    SevenzipOutStream(Tcl_Interp *interp);
    virtual ~SevenzipOutStream();

    virtual HRESULT Open(const wchar_t *filename) override;
    virtual HRESULT Write(const void *data, UInt32 size, UInt32 &processedSize) override;
    virtual HRESULT Seek(Int64 offset, UInt32 seekOrigin, UInt64 &newPosition) override;
    virtual void Close() override;

    virtual HRESULT Mkdir(const wchar_t* pathname) override;
    // virtual HRESULT SetSize(const wchar_t* pathname, UInt64 size) override;
    virtual HRESULT SetMode(const wchar_t* pathname, UInt32 mode) override;
    virtual HRESULT SetAttr(const wchar_t* pathname, UInt32 attr) override;
    virtual HRESULT SetTime(const wchar_t* pathname, UInt32 time) override;

    HRESULT Mkdir(Tcl_Obj* pathname);
    HRESULT SetMode(Tcl_Obj* pathname, UInt32 mode);
    HRESULT SetAttr(Tcl_Obj* pathname, UInt32 attr);
    HRESULT SetTime(Tcl_Obj* pathname, UInt32 time);
    HRESULT AttachOpenChannel(Tcl_Obj *channel);
    HRESULT AttachFileChannel(Tcl_Obj *filename);
    Tcl_Channel DetachChannel();

private:

    Tcl_Interp *tclInterp;
    Tcl_Channel tclChannel;
    bool attached;
};

int lastError(Tcl_Interp *interp, HRESULT hr);
Tcl_Channel getOpenChannel(Tcl_Interp *tclInterp, Tcl_Obj *channel, bool writable);
Tcl_Channel getFileChannel(Tcl_Interp *tclInterp, Tcl_Obj *filename, bool writable);

#endif
