#ifndef MUTRA_LINUX_BACKEND_HPP
#define MUTRA_LINUX_BACKEND_HPP
#include "../midi_core.hpp"
#include <iostream>

namespace MuTraMIDI {
  class FileBackend : public MIDIBackend {
  public:
    FileBackend() : MIDIBackend( "file://", "Device file" ) {}
    Sequencer* get_sequencer( const std::string& URI = std::string() );
    InputDevice* get_input( const std::string& URI = std::string() );
  }; // FileBackend

  class LinuxSequencer : public Sequencer
  {
  protected:
    std::ostream& Device;
    int MaxDiff;
    double TotalDiff;
    int MaxInDiff;
    double TotalInDiff;
    int Number;
  public:
    LinuxSequencer( std::ostream& Device0 );
    ~LinuxSequencer();
    void note_on( int Channel, int Note, int Velocity );
    void note_off( int Channel, int Note, int Velocity );
    void after_touch( int Channel, int Note, int Velocity );
    void program_change( int Channel, int NewProgram );
    void control_change( int Channel, int Control, int Value );
    void pitch_bend( int Channel, int Bend );

    void start();
    double average_diff() { return Number > 0 ? TotalDiff / Number : 0; }
    int max_diff() { return MaxDiff; }
    double average_in_diff() { return Number > 0 ? TotalInDiff / Number : 0; }
    int max_in_diff() { return MaxInDiff; }
  protected:
    void wait_for_usec( int64_t WaitMicroSecs ) override;
  }; // LinuxSequencer
} // MuTraMIDI
#endif // MUTRA_LINUX_BACKEND_HPP
