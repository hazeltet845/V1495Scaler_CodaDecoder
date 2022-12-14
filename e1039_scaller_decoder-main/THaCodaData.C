/////////////////////////////////////////////////////////////////////
//
//   THaCodaData
//   Abstract class of CODA data.
//
//   THaCodaData is an abstract class of CODA data.  Derived
//   classes will be typically either a CODA file (a disk
//   file) or a connection to the ET system.  Public methods
//   allow to open (i.e. set up), read, write, etc.
//
//   author Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaCodaData.h"

#ifndef STANDALONE
ClassImp(THaCodaData)
#endif

THaCodaData::THaCodaData() {
   evbuffer = new unsigned[MAXEVLEN];         // Raw data     
};

THaCodaData::~THaCodaData() { 
// Dont delete evbuffer because client may have deleted it.  
// Anyway, the code usually quits after this object is deleted.
};







