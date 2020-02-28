#ifndef MUTRA_MIDI_UTILITY_HPP
#define  MUTRA_MIDI_UTILITY_HPP
#include <midi/midi_files.hpp>
namespace MuTraMIDI {
  std::string note_name( int Note );

  class EventsPrinter : public InputDevice::Client {
  public:
    void event_received( const Event& Ev );
  }; // EventsPrinter

  //! \todo Sync with metronome (so move it from the 
  class Recorder : public InputDevice::Client, public MIDISequence {
    Event::TimeuS mTempo;
  public:
    Recorder( int BeatsPerMinute = 120 );
    void event_received( const Event& Ev );
    void meter( int Numerator, int Denominator );
  }; // Recorder

  class InputConnector : public InputDevice::Client {
  public:
    InputConnector( Sequencer* Output = nullptr ) : mOutput( Output ) {}
    Sequencer* output() const { return mOutput; }
    void output( Sequencer* NewOutput ) { mOutput = NewOutput; }
    void event_received( const Event& Ev );
  private:
    Sequencer* mOutput;
  }; // InputConnector

  class MultiSequencer : public Sequencer {
  public:
    void add( Sequencer* Seq );
    void remove( Sequencer* Seq );
    void start();
    void name( std::string Text );
    void copyright( std::string Text );
    void text( std::string Text );
    void lyric( std::string Text );
    void instrument( std::string Text );
    void marker( std::string Text );
    void cue( std::string Text );

    void meter( int Numerator, int Denominator );

    void note_on( int Channel, int Note, int Velocity );
    void note_off( int Channel, int Note, int Velocity );
    void after_touch( int Channel, int Note, int Velocity );
    void program_change( int Channel, int NewProgram );
    void control_change( int Channel, int Control, int Value );
    void pitch_bend( int Channel, int Bend );
    void division( unsigned MIDIClockForQuarter );
    void tempo( unsigned uSecForQuarter );
    void wait_for( unsigned WaitClock );
  private:
    std::vector<Sequencer*> Clients;
  }; // MultiSequencer
} // MuTraMIDI
#endif //  MUTRA_MIDI_UTILITY_HPP
