/////////////////////////////////////////////////////////////////////
//
//  THaCodaFile
//  File of CODA data
//
//  CODA data file, and facilities to open, close, read,
//  write, filter CODA data to disk files, etc.
//  A lot of this relies on the "evio.cpp" code from
//  DAQ group which is a C++ rendition of evio.c that
//  we have used for years, but here are some useful
//  added features.
//
//  author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "THaCodaFile.h"

#ifndef STANDALONE
ClassImp(THaCodaFile)
#endif

//Constructors 

  THaCodaFile::THaCodaFile() {       // do nothing (must open file separately)
       ffirst = 0;
       init(" no name ");
  }
  THaCodaFile::THaCodaFile(TString fname) {
       ffirst = 0;
       init(fname);
       int status = codaOpen(fname.Data(),"r");       // read only 
       staterr("open",status);
  }
  THaCodaFile::THaCodaFile(TString fname, TString readwrite) {
       ffirst = 0;
       init(fname);
       int status = codaOpen(fname.Data(),readwrite.Data());  // pass read or write flag
       staterr("open",status);
  }

//Destructor
  THaCodaFile::~THaCodaFile () {
       int status = codaClose();
       staterr("close",status);
  };       

  int THaCodaFile::codaOpen(TString fname) {  
       init(fname);
       int status = evOpen((char*)fname.Data(),"r",&handle);
       staterr("open",status);
       return status;
  };

  int THaCodaFile::codaOpen(TString fname, TString readwrite) {  
      init(fname);
      int status = evOpen((char*)fname.Data(),(char*)readwrite.Data(),&handle);
      staterr("open",status);
      return status;
  };


  int THaCodaFile::codaClose() {
// Close the file. Do nothing if file not opened.
    if( handle ) {
      int status = evClose(handle);
      handle = 0;
      return status;
    }
    return CODA_OK;
  }


  int THaCodaFile::codaRead() {
// codaRead: Reads data from file, stored in evbuffer.
// Must be called once per event.
    int status;
    if ( handle ) {
       status = evRead(handle, evbuffer, MAXEVLEN);
       staterr("read",status);
       if (status != S_SUCCESS) {
  	  if (status == EOF) return status;  // ok, end of file
          status = CODA_ERROR;
       }
    } else {
      if(CODA_VERBOSE) {
         cout << "codaRead ERROR: tried to access a file with handle = 0" << endl;
         cout << "You need to call codaOpen(filename)" << endl;
         cout << "or use the constructor with (filename) arg" << endl;
      }
      status = CODA_ERROR;
    }
    return status;
  };



  int THaCodaFile::codaWrite(unsigned *evbuffer) {
// codaWrite: Writes data to file
     int status;
     if ( handle ) {
       status = evWrite(handle, evbuffer);
       staterr("write",status);
     } else {
       cout << "codaWrite ERROR: tried to access file with handle = 0" << endl;
       status = CODA_ERROR;
     }
     return status;
   };



  unsigned* THaCodaFile::getEvBuffer() {
// Here's how to get raw event buffer, evbuffer, after codaRead call
      return evbuffer;
  }


  int THaCodaFile::filterToFile(TString output_file) {
// A call to filterToFile filters from present file to output_file
// using filter criteria defined by evtypes, evlist, and max_to_filt 
// which are loaded by public methods of this class.  If no conditions 
// were loaded, it makes a copy of the input file (i.e. no filtering).

       int i;
       if(output_file == filename) {
	 if(CODA_VERBOSE) {
           cout << "filterToFile: ERROR: ";
           cout << "Input and output files cannot be same " << endl;
           cout << "This is to protect you against overwriting data" << endl;
         }
         return CODA_ERROR;
       }
       FILE *fp;
       if ((fp = fopen(output_file.Data(),"r")) != NULL) {
          if(CODA_VERBOSE) {
  	    cout << "filterToFile:  ERROR:  ";
            cout << "Output file `" << output_file << "' exists " << endl;
            cout << "You must remove it by hand first. " << endl;
            cout << "This forces you to think and not overwrite data." << endl;
	  }
          fclose(fp);
          return CODA_ERROR;
       }
       THaCodaFile* fout = new THaCodaFile(output_file.Data(),"w"); 
       int nfilt = 0;

       while (codaRead() == S_SUCCESS) {
           unsigned* rawbuff = getEvBuffer();
           int evtype = rawbuff[1]>>16;
           int evnum = rawbuff[4];
           int oktofilt = 1;
           if (CODA_DEBUG) { 
	     cout << "Input evtype " << dec << evtype;
             cout << "  evnum " << evnum << endl; 
             cout << "max_to_filt = " << max_to_filt << endl;
             cout << "evtype size = " << evtypes[0] << endl;
             cout << "evlist size = " << evlist[0] << endl;
	   }
           if ( evtypes[0] > 0 ) {
               oktofilt = 0;
               for (i=1; i<=evtypes[0]; i++) {               
                   if (evtype == evtypes[i]) {
                       oktofilt = 1;
                       goto Cont1;
	 	   }
	       }
	   }
Cont1:
           if ( evlist[0] > 0 ) {
               oktofilt = 0;
               for (i=1; i<=evlist[0]; i++) {               
                   if (evnum == evlist[i]) {
                       oktofilt = 1;
                       goto Cont2;
		   }
	       }
	   }
Cont2:
	   if (oktofilt) {
             nfilt++;
             if (CODA_DEBUG) {
	       cout << "Filtering event, nfilt " << dec << nfilt << endl;
	     }
	     int status = fout->codaWrite(getEvBuffer());
             if (status != S_SUCCESS) {
	       if (CODA_VERBOSE) {
		 cout << "Error in filterToFile ! " << endl;
                 cout << "codaWrite returned status " << status << endl;
	       }
               goto Finish;
	     }
             if (max_to_filt > 0) {
    	        if (nfilt == max_to_filt) {
                  goto Finish;
	        }
	     }
	   }
       }
Finish:
       delete fout;
       return S_SUCCESS;
  };



  void THaCodaFile::addEvTypeFilt(int evtype_to_filt)
// Function to set up filtering by event type
  {
     int i;
     initFilter();
     if (evtypes[0] >= maxftype-1) {
        TArrayI temp = evtypes;
        maxftype = maxftype + 100;
        evtypes.Set(maxftype);
        for (i=0; i<=temp[0]; i++) evtypes[i]=temp[i];
        temp.~TArrayI();
     }
     evtypes[0] = evtypes[0] + 1;  // 0th element = num elements in list
     int n = evtypes[0];
     evtypes[n] = evtype_to_filt;
     return;
  };


  void THaCodaFile::addEvListFilt(int event_num_to_filt)
// Function to set up filtering by list of event numbers
  {
     int i;
     initFilter();
     if (evlist[0] >= maxflist-1) {
        TArrayI temp = evlist;
        maxflist = maxflist + 100;
        evlist.Set(maxflist);
        for (i=0; i<=temp[0]; i++) evlist[i]=temp[i];
        temp.~TArrayI();
     }
     evlist[0] = evlist[0] + 1;  // 0th element = num elements in list
     int n = evlist[0];
     evlist[n] = event_num_to_filt;
     return;
  };

  void THaCodaFile::setMaxEvFilt(int max_event)
// Function to set up the max number of events to filter
  {
     max_to_filt = max_event;
     return;
  };



void THaCodaFile::staterr(TString tried_to, int status) {
// staterr gives the non-expert user a reasonable clue
// of what the status returns from evio mean.
// Note: severe errors can cause job to exit(0)
// and the user has to pay attention to why.
    if (status == S_SUCCESS) return;  // everything is fine.
    if (tried_to == "open") {
       cout << "THaCodaFile: ERROR opening file = " << filename << endl;
       cout << "Most likely errors are: " << endl;
       cout << "   1.  You mistyped the name of file ?" << endl;
       cout << "   2.  The file has length zero ? " << endl;
       cout << status << " Job aborting !! " << endl;
       exit(0);
    }
    switch (status) {
      case S_EVFILE_TRUNC :
	 cout << "THaCodaFile ERROR:  Truncated event on file read" << endl;
         cout << "Evbuffer size is too small.  Job aborted." << endl;
         exit(0);  //  If this ever happens, recompile with MAXEVLEN
	           //  bigger, and mutter under your breath at the author.    
      case S_EVFILE_BADBLOCK : 
        cout << "Bad block number encountered " << endl;
        break;
      case S_EVFILE_BADHANDLE :
        cout << "Bad handle (file/stream not open) " << endl;
        break;
      case S_EVFILE_ALLOCFAIL : 
        cout << "Failed to allocate event I/O" << endl;
        break;
      case S_EVFILE_BADFILE :
        cout << "File format error" << endl;
        break;
      case S_EVFILE_UNKOPTION :
        cout << "Unknown option specified" << endl;
        break;
      case S_EVFILE_UNXPTDEOF :
        cout << "Unexpected end of file while reading event" << endl;
        break;
      case EOF: 
        /*
        if(CODA_VERBOSE) {
          cout << "Normal end of file " << filename << " encountered" << endl;
	}*/
        break;
      default:
        cout << "Error status  0x" << hex << status << endl;
      }
  };

  void THaCodaFile::init(TString fname) {
    handle = 0;
    filename = fname;
  };

  void THaCodaFile::initFilter() {
    if (!ffirst) {
       ffirst = 1;
       maxflist = 100;
       maxftype = 100;
       evlist.Set(maxflist);
       evtypes.Set(maxftype);
       evlist[0] = 0;
       evtypes[0] = 0;
    }
  };












