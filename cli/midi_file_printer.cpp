#include "midi/midi_files.hpp"
#include <iostream>
using MuTraMIDI::MIDISequence;
using std::cout;
using std::cerr;
using std::endl;

int main( int argc, char* argv[] ) {
  if( argc < 2 ) cerr << "Usage: events_printer <file name>..." << endl;
  for( int I = 1; I < argc; ++I ) {
    MIDISequence File( argv[ I ] );
    File.print( cout );
  }
  return 0;
} // main( int, char*[] )
