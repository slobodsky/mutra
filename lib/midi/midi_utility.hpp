#ifndef MUTRA_MIDI_UTILITY_HPP
#define  MUTRA_MIDI_UTILITY_HPP
#include <midi/midi_files.hpp>
namespace MuTraMIDI {
  std::string note_name( int Note );
  //! \todo Move to music.hpp
  class Names {
  public:
    //! \note Notes are from 0 (C) to 6 (B (or H in some notations))
    static std::string note_letter( int Note );
    static std::string note_name( int Note );
    static std::string octave_name( int Octava );
  private:
    static std::string sNoteLetters[];
    static std::string sNoteNames[];
    static std::string sOctaveNames[];
  }; // Names

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

  //! Соединяет вход (или несколько) с выходом.
  class InputConnector : public InputDevice::Client {
  public:
    InputConnector( Sequencer* Output = nullptr ) : mOutput( Output ) {}
    Sequencer* output() const { return mOutput; }
    void output( Sequencer* NewOutput ) { mOutput = NewOutput; }
    void event_received( const Event& Ev );
  private:
    Sequencer* mOutput;
  }; // InputConnector

  class InputFilter : public InputDevice::Client {
  public:
    InputFilter( InputDevice* Input = nullptr ) : mInput( Input ) { if( mInput ) mInput->add_client( *this ); }
    ~InputFilter();
    void event_received( const Event& Ev ) override;
    void connected( InputDevice& Dev ) override;
    void disconnected( InputDevice& Dev ) override;
    void started( InputDevice& Dev ) override;
    void stopped( InputDevice& Dev ) override;
    void add_client( InputDevice::Client& Cli );
    void remove_client( InputDevice::Client& Cli );
    const std::vector<InputDevice::Client*> clients() const { return mClients; }
  private:
    InputDevice* mInput;
    std::vector<InputDevice::Client*> mClients;
  }; // InputFilter

  class Transposer : public InputFilter {
  public:
    Transposer( int HalfTones = 0, InputDevice* Input = nullptr ) : InputFilter( Input ), mHalfTones( HalfTones ) {}
    int halftones() const { return mHalfTones; }
    Transposer& halftones( int NewHalfTones ) { mHalfTones = NewHalfTones; return *this; }
    void event_received( const Event& Ev ) override;
  private:
    int mHalfTones;
  }; // Transposer

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
