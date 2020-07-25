#ifndef RTMIDI_BACKEND_HPP
#define RTMIDI_BACKEND_HPP
#include <rtmidi/RtMidi.h>
#include "midi_core.hpp"

namespace MuTraMIDI {
  class RTMIDIBackend : public MIDIBackend {
  public:
    RTMIDIBackend();
    ~RTMIDIBackend();
    std::vector<Sequencer::Info> list_devices( DeviceType Filter = All );
    InputDevice* get_input( const std::string& URI = std::string() );
  }; // RTMIDIBackend

  class RtMIDIInputDevice : public InputDevice {
    static void message_received( double Delta, std::vector<unsigned char>* Msg, void* This );
  public:
    RtMIDIInputDevice( int Index = 0 );
    ~RtMIDIInputDevice();
    void start();
    void stop();
    Event::TimeuS time() const { return Time * 1000000; }
  private:
    void message_received( double Delta, std::vector<unsigned char>* Msg );
    RtMidiIn* In;
    double Time;
    int Index;
  }; // RtMIDIInputDevice

  class RtMIDISequencer : public Sequencer {
  }; // RtMIDISequencer
} // MuTraMIDI
#endif // RTMIDI_BACKEND_HPP
