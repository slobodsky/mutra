#include "rtmidi_backend.hpp"

namespace MuTraMIDI {
  void RtMIDIInputDevice::message_received( double Delta, std::vector<unsigned char>* Msg, void* This ) {
  } // message_received( double, std::vector<unsigned char>, void* )

  RtMIDIInputDevice::RtMIDIInputDevice( int Index0 ) : In( nullptr ), Index( Index0 ) {}
  RtMIDIInputDevice::~RtMIDIInputDevice() { stop(); }
  void RtMIDIInputDevice::start() {
    stop();
    In = new RtMidiIn();
    In->openPort( Index );
    In->setCallback( &message_received, this );
  } // start()
  void RtMIDIInputDevice::stop() {
    if( In ) {
      In->cancelCallback();
      delete In;
      In = nullptr;
    }
  } // stop()
} // MuTraMIDI
