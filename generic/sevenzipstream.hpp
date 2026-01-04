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
    
    virtual bool IsDir(const wchar_t *filename) const override;
    //virtual UInt64 GetSize(const wchar_t *filename) const override;
    virtual UInt32 GetMode(const wchar_t *filename) const override;
    virtual UInt32 GetTime(const wchar_t *filename) const override;

    HRESULT AttachOpenChannel(Tcl_Obj *channel);
    HRESULT AttachFileChannel(Tcl_Obj *filename);
    Tcl_Channel DetachChannel();    

private:

    Tcl_Interp *tclInterp;
    Tcl_Channel tclChannel;
    bool usechannel;
};

class SevenzipOutStream:  public sevenzip::Ostream {

public:

    SevenzipOutStream(Tcl_Interp *interp);
    virtual ~SevenzipOutStream();

    virtual HRESULT Open(const wchar_t *filename) override;
    virtual HRESULT Write(const void *data, UInt32 size, UInt32 &processedSize) override;
    virtual HRESULT Seek(Int64 offset, UInt32 seekOrigin, UInt64 &newPosition) override;
    // virtual HRESULT SetSize(UInt64 size) override;
    virtual void Close() override;

    virtual HRESULT Mkdir(const wchar_t* dirname) override;
    virtual HRESULT SetMode(const wchar_t* path, UInt32 mode) override;
    virtual HRESULT SetTime(const wchar_t* filename, UInt32 time) override;

    HRESULT AttachOpenChannel(Tcl_Obj *channel);
    HRESULT AttachFileChannel(Tcl_Obj *filename);
    Tcl_Channel DetachChannel();

private:

    Tcl_Interp *tclInterp;
    Tcl_Channel tclChannel;
    bool usechannel;
};

int lastError(Tcl_Interp *interp, HRESULT hr);
bool createDirectory(Tcl_Interp *tclInterp, Tcl_Obj *dirname);
Tcl_Channel getOpenChannel(Tcl_Interp *tclInterp, Tcl_Obj *channel, bool writable);
Tcl_Channel getFileChannel(Tcl_Interp *tclInterp, Tcl_Obj *filename, bool writable);

#endif
