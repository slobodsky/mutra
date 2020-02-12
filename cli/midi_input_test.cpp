// #include <midi/linux/input_device.hpp>
#include <midi/midi_files.hpp>
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

//#define USE_LIGHTS

class EventsPrinter : public InputDevice::Client {
public:
  EventsPrinter() : Li( nullptr ) {
#ifdef USE_LIGHTS
      Li = new StarLighter( "/dev/ttyACM0" );
      Li->high( 77 );
      Li->low( 36 );
      Li->stars_for_note( 3 );

      Li->start();
#endif
  } // EventsPrinter()
  ~EventsPrinter() { if( Li ) delete Li; }
  void event_received( const Event& Ev ) {
    Ev.print( cout ); cout << endl;
    if( Li ) Ev.play( *Li );
  }
private:
  StarLighter* Li;
}; // EventsPrinter

class Recorder : public InputDevice::Client, public MIDISequence {
  Event::TimeuS mTempo;
public:
  Recorder( int BeatsPerMinute = 120 ) : mTempo( 60*1000000 / BeatsPerMinute ) {}
  void event_received( const Event& Ev ) {
    int Time = Ev.time() * division() / mTempo;
    add_event( Time, Ev.clone() );
  }
}; // Recorder

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
  if( InputDevice* Dev = InputDevice::get_instance( DevName ) ) {
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
