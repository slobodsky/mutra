#include "input_device.hpp"
using std::string;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using MuTraMIDI::InputDevice;
using MuTraMIDI::Event;

class EventsPrinter : public InputDevice::Client {
public: 
  void event_received( Event& Ev ) { Ev.print( cout ); cout << endl; }
}; // EventsPrinter

class LessonStub : public InputDevice::Client {
public:
  LessonStub( const char* FileName = nullptr ) {}
  void event_received( Event& Ev ) {}
}; // LessonStub

int main( int argc, char* argv[] ) {
  const char* MIDIName = nullptr;
  const char* DevName = "/home/nick/projects/small/music_trainer/data/midi.capture"; // "/dev/midi2";
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

  cout << "Read MIDI events from " << DevName << endl;
  if( InputDevice* Dev = InputDevice::get_instance( string( "file://" ) + DevName ) ) {
    EventsPrinter Pr;
    Dev->add_client( Pr );
    Dev->start();
    cin.get();
    Dev->stop();
    Dev->remove_client( Pr );
    delete Dev;
  }
  return 0;
} // main()
