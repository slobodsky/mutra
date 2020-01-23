#include <cstring>
#include "input_device.hpp"
using std::ios;
using std::memcpy;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace MuTraMIDI {
  InputDevice* InputDevice::get_instance( const std::string& URI ) {
    if( URI.substr( 0, 7 ) == "file://" )
      return new FileInputDevice( URI.substr( 7 ) );
    return nullptr;
  } // get_instance( const std::string& )
  
  FileInputDevice::FileInputDevice( const string& FileName0 ) : FileName( FileName0 ), Str( FileName, ios::binary | ios::in ), Stream( Str ) {}
  void FileInputDevice::start() {
    unsigned char Status = 0;
    int ToGo = 4*1024*1024; //!< \todo Make this not required for endless streams like midi-devices.
    while( Event* Ev = Stream.get_event() ) {
      Ev->print( cout );
      cout << endl;
    }
  } // start()
#if 0
  void FileInputDevice::process() {
    cout << "Process input." << endl;
    const int BuffSize = 4096;
    char Buffer[ BuffSize ];
    int TailSize = BuffSize;
    char* ReadPos = Buffer;
    char* ParsePos = Buffer;
    int Count = 0;
    while( Str.good() ) {
      int ReadCount = Str.readsome( ReadPos, TailSize );
      if( ReadCount <= 0 ) {
	if( Str.eof() ) break;
	if( TailSize <= 0 ) {
	  cerr << "The buffer is full!" << endl;
	  break;
	}
	*ReadPos = Str.get();
	if( Str.good() ) {
	  ReadPos++;
	  Count++;
	  TailSize--;
	  ReadCount = Str.readsome( ReadPos, TailSize );
	}
	else {
	  cerr << "Can't get data from the input." << endl;
	  break;
	}
      }
      Count += ReadCount;
      ReadPos += ReadCount;
      TailSize -= ReadCount;
      cout << "Got " << ReadCount << " bytes from input. Now count is: " << Count << endl;
      if( Count > 2 ) {
	int Parsed = parse( (unsigned char*)ParsePos, Count );
	if( Parsed > 0 ) {
	  Count -= Parsed;
	  ParsePos += Parsed;
	  if( Count <= 0 ) { //!< \todo Should be ==, indicate error if <
	    ReadPos = Buffer;
	    ParsePos = Buffer;
	    TailSize = BuffSize;
	    Count = 0;
	  }
	  else if( TailSize < 32 ) { // Or any other small value you consider appropriate.
	    memcpy( Buffer, ParsePos, Count );
	    ParsePos = Buffer;
	    ReadPos = Buffer+Count;
	    TailSize = BuffSize-Count;
	  }
	}
	else cerr << "No input data parsed." << endl;
      }
    }
    cout << "Process complete" << endl;
  } // process()
#endif
} // MuTraMIDI
