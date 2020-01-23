#include <cstring>
#include "input_device.hpp"
#include "../rtmidi_backend.hpp"
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
    if( URI.substr( 0, 9 ) == "rtmidi://" )
      return new RtMIDIInputDevice( atoi( URI.substr( 9 ).c_str() ) );
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
} // MuTraMIDI
