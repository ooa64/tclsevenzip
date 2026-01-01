#ifndef TCLCMD_H
#define TCLCMD_H

#include <tcl.h>

#if TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION == 0
#   define Tcl_GetString(a) \
           Tcl_GetStringFromObj(a,NULL)
#   define Tcl_AppendObjToObj(a,b) \
           Tcl_AppendToObj(a,Tcl_GetStringFromObj(b,NULL),-1)
#endif

#if TCL_MAJOR_VERSION == 8
#   define STUB_VERSION "8.5"
#else
#   define STUB_VERSION "9.0"
#endif

// Common TclCmd ancestor to all objects. This defines
// common destructing and command dispatching methods.

class TclCmd {

public:

  TclCmd() : tclInterp(NULL), tclToken(NULL), 
	     pParent(NULL), pNext(NULL), pPrev(NULL), pChildren(NULL) {};

  TclCmd(Tcl_Interp * interp, const char * name);
  TclCmd(Tcl_Interp * interp, const char * name, TclCmd * parent);

  virtual ~TclCmd();
  // virtual destructor for all objects

  void Rename(Tcl_Interp * interp, const char * name);
  int IsNamed() {return (tclInterp && tclToken);};
  const char * Name() {return IsNamed() ? 
		   Tcl_GetCommandName(tclInterp, tclToken) : NULL;};
  // create, delete, query tcl command name

  TclCmd * Parent() {return pParent;};
  TclCmd * Next() {return pNext;};
  TclCmd * Prev() {return pPrev;};
  TclCmd * Children() {return pChildren;};
  void SetParent(TclCmd * parent);

  static void Destroy(ClientData);
  // static function for destroying objects
  
  static int Dispatch(ClientData clientData, Tcl_Interp * interp,
		      int objc, Tcl_Obj * const objv[]);
  // static function for dispatching commands to objects
  
protected:

  void Unname () {
    tclInterp = NULL; 
    tclToken = NULL;
  };

  Tcl_Interp * tclInterp;
  Tcl_Command tclToken;
  // token for tcl command in the interpreter interp

  TclCmd * pParent;
  TclCmd * pNext;
  TclCmd * pPrev;
  TclCmd * pChildren;
  // links for linking certain objects to each other

private:

  void AddChild(TclCmd * child);
  void RemoveChild(TclCmd * child);
  // don't call this directly

  virtual int Command(int objc, Tcl_Obj * const objv[]) = 0;
  // virtual function for class dependent implementation of static Dispatch().

  virtual void Cleanup() {};
  // virtual function for class dependent cleanup.

};

#endif
