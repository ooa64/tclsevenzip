#include "tclcmd.hpp"

#if defined(TCLCMD_DEBUG)
#   include <iostream>
#   define DEBUGLOG(_x_) (std::cerr << "DEBUG: " << _x_ << "\n")
#else
#   define DEBUGLOG(_x_)
#endif

TclCmd::TclCmd(Tcl_Interp * interp, const char * name) {
  tclInterp = NULL;
  tclToken = NULL;
  pParent = NULL;
  pNext = NULL;
  pPrev = NULL;
  pChildren = NULL;
  Rename(interp, name);
};

TclCmd::TclCmd(Tcl_Interp * interp, const char * name, TclCmd * parent) {
  tclInterp = NULL;
  tclToken = NULL;
  pParent = NULL;
  pNext = NULL;
  pPrev = NULL;
  pChildren = NULL;
  Rename(interp, name);
  SetParent(parent);
};

TclCmd::~TclCmd() {
  DEBUGLOG("TclCmd::Destruct *" << this);
  while (pChildren) delete pChildren;
  SetParent(NULL);
  Rename(NULL, NULL);
}

void TclCmd::Rename(Tcl_Interp * interp, const char * name) {
  if (interp && name) {
    DEBUGLOG("TclCmd::Rename *" << this << " '" << \
	     Tcl_GetCommandName(tclInterp, tclToken) << \
	     "' to '" << name << "'");
    tclInterp = interp;
    tclToken = Tcl_CreateObjCommand(interp, name, 
				    &TclCmd::Dispatch, this, TclCmd::Destroy);
  } else if (IsNamed()) {
    DEBUGLOG("TclCmd::Rename *" << this << " '" << \
	     Tcl_GetCommandName(tclInterp, tclToken) << "' to ''");
    Tcl_Interp * oldInterp = tclInterp; 
    Tcl_Command  oldToken = tclToken;

    Unname();
    Tcl_DeleteCommandFromToken(oldInterp, oldToken);
  }
}

void TclCmd::SetParent(TclCmd * parent) {
  DEBUGLOG("TclCmd::SetParent *" << this << " " << parent);
  if (pParent)
    pParent->RemoveChild(this);
  if (parent)
    parent->AddChild(this);
  pParent = parent;
};

void TclCmd::AddChild(TclCmd * child) {
  DEBUGLOG("TclCmd::AddChild *" << this << " " << child);
  if (pChildren)
    pChildren->pPrev = child;
  child->pNext = pChildren;
  child->pPrev = NULL;
  pChildren = child;
}

void TclCmd::RemoveChild(TclCmd * child) {
  DEBUGLOG("TclCmd::RemoveChild *" << this << " " << child);
  if (child->pPrev)
    child->pPrev->pNext = child->pNext;
  if (child->pNext)
    child->pNext->pPrev = child->pPrev;
  if (pChildren == child)
    pChildren = child->pNext;
}

int TclCmd::Dispatch (ClientData clientData, Tcl_Interp * interp, 
		     int objc, Tcl_Obj * const objv[]) {
  TclCmd *o = (TclCmd *) clientData;

#ifdef TCLCMD_DEBUG
  if (interp != o->tclInterp) {
    DEBUGLOG("TclCmd::Dispatch *" << o << " mismatched '" << \
	     Tcl_GetString(objv[0]) << " " << (objc > 1 ? Tcl_GetString(objv[1]) : "") << "'");
  } else {
    DEBUGLOG("TclCmd::Dispatch *" << o << " '" << \
	     Tcl_GetString(objv[0]) << " " << (objc > 1 ? Tcl_GetString(objv[1]) : "") << "'");
  }
#endif
  return o->Command(objc, objv);
}

void TclCmd::Destroy(ClientData clientData) {
  TclCmd *o = (TclCmd *) clientData;

  DEBUGLOG("TclCmd::Destroy *" << o);
  if (o->IsNamed()) {
    o->Cleanup();
    o->Unname();
    delete o;
  }
}
