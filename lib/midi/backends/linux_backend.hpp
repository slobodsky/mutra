#ifndef LINUX_SEQUENCER_HPP
#define LINUX_SEQUENCER_HPP
#include "../midi_core.hpp"
#include <iostream>
#include <alsa/asoundlib.h>

namespace MuTraMIDI {
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

#ifdef USE_ALSA_BACKEND
  class ALSASequencer : public LinuxSequencer
  {
  public:
    static std::vector<Info> get_alsa_devices( bool Input = false );
    ALSASequencer( int OutClient0 = 128, int OutPort0 = 0, std::ostream& Device0 = std::cout );
    ~ALSASequencer();
    //! \todo Implement all possible events.
    void note_on( int Channel, int Note, int Velocity );
    void note_off( int Channel, int Note, int Velocity );
    void after_touch( int Channel, int Note, int Velocity );
    void program_change( int Channel, int NewProgram );
    void control_change( int Channel, int Control, int Value );
    void pitch_bend( int Channel, int Bend );

    void start();
  protected:
    snd_seq_t* Seq;
    int Client;
    int Port;
    int OutClient;
    int OutPort;
  }; // ALSASequencer
#endif // USE_ALSA_BACKEND
} // MuTraMIDI
#endif // LINUX_SEQUENCER_HPP
