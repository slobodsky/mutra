#include "input_device.hpp"
using std::cout;
using std::cerr;
using std::endl;

class EventsPrinter : public MIDIInputDevice::Client {
public: 
  void event_received( Event& Ev ) { Ev.print( cout ); cout << endl; }
}; // EventsPrinter

int main( int argc, char* argv[] ) {
  const char* MIDIName = nullptr;
  const char* DevName = "/dev/midi2";
  switch( argc ) {
  case 1: break;
  case 2: DevName = argv[1]; break;
  case 3:
    MIDIName = argv[1];
    DevName = argv[2];
    break;
  default:
    cout << "Usage:\nmidi_input_test <filename> <devicename>\nmidi_input_test <devicename>\nmidi_input_file\nwhere 'filename' is midi file to compare and 'devicename' is the name of the input device." << endl;
    return 1;
  }

  cout << "Read MIDI events from " << DevName << endl;
  FileInputDevice Dev( DevName );
  EventsPrinter Pr;
  Dev.add_client( Pr );
  Dev.process();
  Dev.remove_client( Pr );
  return 0;
} // main()
