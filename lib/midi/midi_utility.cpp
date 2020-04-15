#include "midi_events.hpp"
#include "midi_utility.hpp"
#include <algorithm>
using std::string;
using std::cout;
using std::endl;
using std::find;

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

  InputFilter::~InputFilter() {
    while( mClients.size() > 0 ) remove_client( *mClients.front() );
    if( mInput ) mInput->remove_client( *this );
  } // ~InputFilter()
  void InputFilter::event_received( const Event& Ev ) { for( int I = 0; I < mClients.size(); ++I ) if( mClients[ I ] ) mClients[ I ]->event_received( Ev ); }
  void InputFilter::connected( InputDevice& Dev ) {
    if( !mInput ) mInput = &Dev;
    for( int I = 0; I < mClients.size(); ++I ) if( mClients[ I ] ) mClients[ I ]->connected( Dev );
  } // connected( InputDevice& )
  void InputFilter::disconnected( InputDevice& Dev ) {
    if( mInput == &Dev ) mInput = nullptr;
    for( int I = 0; I < mClients.size(); ++I ) if( mClients[ I ] ) mClients[ I ]->disconnected( Dev );
  } // disconnected( InputDevice& )
  void InputFilter::started( InputDevice& Dev ) { for( int I = 0; I < mClients.size(); ++I ) if( mClients[ I ] ) mClients[ I ]->started( Dev ); }
  void InputFilter::stopped( InputDevice& Dev ) { for( int I = 0; I < mClients.size(); ++I ) if( mClients[ I ] ) mClients[ I ]->stopped( Dev ); }
  void InputFilter::add_client( InputDevice::Client& Cli ) {
    if( find( mClients.begin(), mClients.end(), &Cli ) == mClients.end() ) {
      mClients.push_back( &Cli );
      if( mInput ) Cli.connected( *mInput );
    }
  } // add_client( Client& )
  void InputFilter::remove_client( InputDevice::Client& Cli ) {
    auto It = find( mClients.begin(), mClients.end(), &Cli );
    if( It == mClients.end() ) return;
    mClients.erase( It );
    if( mInput ) Cli.disconnected( *mInput );
  } // remove_client( Client* )

  void Transposer::event_received( const Event& Ev ) {
    if( ( Ev.status() == Event::NoteOn || Ev.status() == Event::NoteOff ) && mHalfTones != 0 ) {
      int NewNote = dynamic_cast<const NoteEvent&>( Ev ).note() + mHalfTones;
      if( NewNote < 0 || NewNote > 127 ) return; // Discard events transposed out of MIDI notes range.
      NoteEvent* NewEv = dynamic_cast<NoteEvent*>( Ev.clone() );
      NewEv->note( NewNote );
      InputFilter::event_received( *NewEv );
      delete NewEv;
    }
    else
      InputFilter::event_received( Ev );
  } // event_received( const Event& )

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
