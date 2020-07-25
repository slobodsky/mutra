// #include <midi/linux/input_device.hpp>
#include <midi/midi_files.hpp>
#include <midi/midi_utility.hpp>
#include "star_lighter.hpp"
#include <fstream>
using std::string;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using MuTraMIDI::InputDevice;
using MuTraMIDI::Event;
using MuTraMIDI::EventsList;
using MuTraMIDI::MIDITrack;
using MuTraMIDI::MIDISequence;
using MuTraMIDI::EventsPrinter;
using MuTraMIDI::Recorder;
using MuTraMIDI::MIDIBackend;

//#define USE_LIGHTS

int main( int argc, char* argv[] ) {
#if 0
  const char* DevName = "file:///home/nick/projects/small/music_trainer/data/midi.capture";
#elif 0
  const char* DevName = "file:///dev/midi2";
#else
  const char* DevName = "rtmidi://2";
#endif
#if 0
  switch( argc ) {
  case 1: break;
  case 2: DevName = argv[1]; break;
  case 3:
    MIDIName = argv[1];
    DevName = argv[2];
    break;
  default:
    cout << "Usage:\nmidi_input_test <filename> <devicename>\nmidi_input_test <devicename>\nmidi_input_file\nwhere 'filename' is midi file to compare and 'devicename' is the name of the input device."
	 << endl;
    return 1;
  }
#endif
  cout << "Read MIDI events from " << DevName << endl;
  if( InputDevice* Dev = MIDIBackend::get_manager().get_input( DevName ) ) {
    EventsPrinter Pr;
    Dev->add_client( Pr );
    Recorder Rec;
    Dev->add_client( Rec );
    Dev->start();
    cin.get();
    Dev->stop();
    ofstream RecFile( "record.mid" );
    Rec.close_last_track();
    Rec.write( RecFile );
    Dev->remove_client( Rec );
    Dev->remove_client( Pr );
    delete Dev;
  }
  else cerr << "Can't get MIDI input device." << endl;
  return 0;
} // main()
