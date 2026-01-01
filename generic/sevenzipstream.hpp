#ifndef SEVENZIPSTREAM_H
#define SEVENZIPSTREAM_H

#include <sevenzip.h>
#include <tcl.h>

class SevenzipInStream:  public sevenzip::Istream {

public:

    SevenzipInStream(Tcl_Interp *interp, Tcl_Obj *channel);
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

private:

    Tcl_Interp *tclInterp;
    Tcl_Channel tclChannel;
    bool usechannel;
};

class SevenzipOutStream:  public sevenzip::Ostream {

public:

    SevenzipOutStream(Tcl_Interp *interp, Tcl_Obj *channel);
    virtual ~SevenzipOutStream();

    virtual HRESULT Open(const wchar_t *filename) override;
    virtual HRESULT Write(const void *data, UInt32 size, UInt32 &processedSize) override;
    virtual HRESULT Seek(Int64 offset, UInt32 seekOrigin, UInt64 &newPosition) override;
    // virtual HRESULT SetSize(UInt64 size) override;
    virtual void Close() override;

    virtual HRESULT Mkdir(const wchar_t* dirname) override;
    virtual HRESULT SetMode(const wchar_t* path, UInt32 mode) override;
    virtual HRESULT SetTime(const wchar_t* filename, UInt32 time) override;

    // bool Valid() {return !!tclChannel;};

private:

    Tcl_Interp *tclInterp;
    Tcl_Channel tclChannel;
    bool usechannel;
};

#endif
