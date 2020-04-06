#include "midi_events.hpp"
#include "midi_utility.hpp"
using std::string;
using std::cout;
using std::endl;

namespace MuTraMIDI {
  string note_name( int Note ) {
    static const string Notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* Octaves = "0123456789a";
    return Notes[ ( Note & 0x7F ) % 12 ] + Octaves[ ( Note & 0x7F ) / 12 ];
  } // note_name( int )

  string Names::note_letter( int Note ) {
    if( Note < 0 || Note > 6 ) return "";
    return sNoteLetters[ Note ];
  } // note_letter( int )
  string Names::note_name( int Note ) {
    if( Note < 0 || Note > 6 ) return "";
    return sNoteNames[ Note ];
  } // note_name( int )
  string Names::octave_name( int Octave ) {
    if( Octave < 0 || Octave > 8 ) return "";
    return sOctaveNames[ Octave ];
  } // octave_name( int )
  string Names::sNoteLetters[] = { "C", "D", "E", "F", "G", "A", "B" };
  string Names::sNoteNames[] = { "Do", "Re", "Mi", "Fa", "Sol", "La", "Si" };
  string Names::sOctaveNames[] = { "Sub-contra", "Contra", "Great", "Small", "1-line", "2-line", "3-line", "4-line", "5-line" };
  
  void EventsPrinter::event_received( const Event& Ev ) { Ev.print( cout ); cout << endl; }
  Recorder::Recorder( int BeatsPerMinute ) : mTempo( 60*1000000 / BeatsPerMinute ) { add_event( 0, new TempoEvent( mTempo ) ); }
  void Recorder::event_received( const Event& Ev ) {
    int Time = Ev.time() * division() / mTempo;
    add_event( Time, Ev.clone() );
#ifdef MUTRA_DEBUG
    cout << "Recorder add event @" << Time << " ";
    Ev.print( cout );
    cout<< endl;
#endif // MUTRA_DEBUG
  } // event_received( const Event& )
  void Recorder::meter( int Numerator, int Denominator ) { add_event( 0, new TimeSignatureEvent( Numerator, Denominator ) ); } // meter( int, int )
  void InputConnector::event_received( const Event& Ev ) { if( mOutput ) Ev.play( *mOutput ); }
  
  void MultiSequencer::add( Sequencer* Seq ) { Clients.push_back( Seq ); }
  void MultiSequencer::remove( Sequencer* Seq ) {} //!< \todo Implement this!
  void MultiSequencer::start() { for( Sequencer* Client : Clients ) Client->start(); }
  void MultiSequencer::name( string Text ) { for( Sequencer* Client : Clients ) Client->name( Text ); }
  void MultiSequencer::copyright( string Text ) { for( Sequencer* Client : Clients ) Client->copyright( Text ); }
  void MultiSequencer::text( string Text ) { for( Sequencer* Client : Clients ) Client->text( Text ); }
  void MultiSequencer::lyric( string Text ) { for( Sequencer* Client : Clients ) Client->lyric( Text ); }
  void MultiSequencer::instrument( string Text ) { for( Sequencer* Client : Clients ) Client->instrument( Text ); }
  void MultiSequencer::marker( string Text ) { for( Sequencer* Client : Clients ) Client->marker( Text ); }
  void MultiSequencer::cue( string Text ) { for( Sequencer* Client : Clients ) Client->cue( Text ); }

  void MultiSequencer::meter( int Numerator, int Denominator ) { for( Sequencer* Client : Clients ) Client->meter( Numerator, Denominator ); }
  void MultiSequencer::note_on( int Channel, int Note, int Velocity ) { for( Sequencer* Client : Clients ) Client->note_on( Channel, Note, Velocity ); }
  void MultiSequencer::note_off( int Channel, int Note, int Velocity ) { for( Sequencer* Client : Clients ) Client->note_off( Channel, Note, Velocity ); }
  void MultiSequencer::after_touch( int Channel, int Note, int Velocity ) { for( Sequencer* Client : Clients ) Client->after_touch( Channel, Note, Velocity ); }
  void MultiSequencer::program_change( int Channel, int NewProgram ) { for( Sequencer* Client : Clients ) Client->program_change( Channel, NewProgram ); }
  void MultiSequencer::control_change( int Channel, int Control, int Value ) { for( Sequencer* Client : Clients ) Client->control_change( Channel, Control, Value ); }
  void MultiSequencer::pitch_bend( int Channel, int Bend ) { for( Sequencer* Client : Clients ) Client->pitch_bend( Channel, Bend ); }
  void MultiSequencer::division( unsigned MIDIClockForQuarter ) {
    Sequencer::division( MIDIClockForQuarter );
    if( Clients.size() > 0 ) Clients.front()->division( MIDIClockForQuarter );
  } // division( MIDIClockForQuarter )
  void MultiSequencer::tempo( unsigned uSecForQuarter ) {
    Sequencer::tempo( uSecForQuarter );
    if( Clients.size() > 0 ) Clients.front()->tempo( uSecForQuarter );
  } // tempo( unsigned )
  void MultiSequencer::wait_for( unsigned WaitClock ) { if( Clients.size() > 0 ) Clients.front()->wait_for( WaitClock ); } // wait_for( unsigned )
} // MuTraMIDI
