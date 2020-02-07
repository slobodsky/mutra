#include "exercise_sequence.hpp"
using std::string;
using std::ifstream;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
#include <algorithm>
using std::find_if;
using std::for_each;
using MuTraMIDI::Event;
using MuTraMIDI::ChannelEvent;
using MuTraMIDI::NoteEvent;
using MuTraMIDI::EventsList;
using MuTraMIDI::MIDITrack;
using MuTraMIDI::MIDIException;
using MuTraMIDI::MIDISequence;
using MuTraMIDI::Sequencer;

namespace MuTraTrain {
  bool ExerciseSequence::load( const string& FileName )
  {
    bool Result = false;
    try
    {
      ifstream Ex( FileName );
      if( Ex.good() )
      {
	string Name;
	Ex >> Name;
	cout << "Load play from " << Name << endl;
	Play = new MIDISequence( Name );
	Play->play( *this );
	Ex >> TargetTracks >> Channels >> StartPoint >> StopPoint >> StartThreshold >> StopThreshold >> VelocityThreshold >> TempoSkew;
#ifdef SHOW_WINDOWS_TAILS
	Stats.clear();
	if( Les )
	  Repeat = Les->retries();
	else
	  Repeat = 3;
	if( Excersize )
	{
	  Record = 0;
	  delete Excersize;
	  Excersize = 0;
	}
#endif
	cout << "Exercise " << FileName << " loaded." << endl;
	Result = true;
      }
    }
    catch( const MIDIException& E )
    {
      cerr << "Can't load exercise: " << E.what() << endl;
    }
    return Result;
  } // load( const string& )

  void ExerciseSequence::reset()
  {
    Sequencer::reset();
  } // reset()
  // MSVC pragma
#pragma warning ( 4 : 4018 )
  void ExerciseSequence::note_on( int Channel, int Note, int Velocity )
  {
    if( Velocity > 0 )
    {
      if( Channels & ( 1 << Channel ) && TargetTracks & ( 1 << Track ) && Clock >= StartPoint && ( StopPoint < 0 || Clock < StopPoint ) )
	add_original_event( new NoteEvent( ChannelEvent::NoteOn, Channel, Note, Velocity, Clock * tempo() / division() ) );
    }
    else
      note_off( Channel, Note, Velocity );
  } // note_on( int, int, int )
  void ExerciseSequence::note_off( int Channel, int Note, int Velocity )
  {
    if( Channels & ( 1 << Channel ) && TargetTracks & ( 1 << Track ) && Clock > StartPoint && ( StopPoint < 0 || Clock <= StopPoint ) )
      add_original_event( new NoteEvent( ChannelEvent::NoteOff, Channel, Note, Velocity, Clock * tempo() / division() ) );
  } // note_off( int, int, int )
  void ExerciseSequence::tempo( unsigned uSecForQuarter )
  {
#if 0
    if( Tempo == 0 )
#endif
      Tempo = uSecForQuarter;
  } // tempo( unsigned )
  void ExerciseSequence::add_original_event( Event* NewEvent )
  {
    int ClockFromStart = Clock - StartPoint;
    if( Tracks.empty() ) Tracks.push_back( new MIDITrack() );
    cout << "O: ";
    if( Tracks.front()->events().empty() || Tracks.front()->events().back()->time() != ClockFromStart )
    {
      cout << "@" << ClockFromStart << " ";
      Tracks.front()->events().push_back( new EventsList( ClockFromStart ) );
      if( OriginalStart < 0 )
      {
	OriginalStart = ClockFromStart;
	OriginalLength = 0;
      }
      else if( ClockFromStart - OriginalStart > OriginalLength )
	OriginalLength = ClockFromStart - OriginalStart;
    }
    NewEvent->print( cout );
    cout << endl;
    Tracks.front()->events().back()->add( NewEvent );
  } // add_original_event( Event* )
  void ExerciseSequence::add_played_event( Event* NewEvent ) {
    Event::TimeuS EvClock = NewEvent->time() * division() / tempo();
    if( PlayedStart < 0 ) {
      if( NewEvent->status() == ChannelEvent::NoteOn ) PlayedStart = EvClock;
      else return; // throw away events before start
    }
#if 0
    std::cout << "Add event @" << EvClock << " (" << NewEvent->time() << "us): ";
#endif
    NewEvent->print( std::cout );
    std::cout << std::endl;
    EvClock -= PlayedStart;
    if( Tracks.back()->events().empty() || Tracks.back()->events().back()->time() != EvClock )
      Tracks.back()->events().push_back( new EventsList( EvClock ) );
    Tracks.back()->events().back()->add( NewEvent );
  } // add_played_event( Event* )
  void ExerciseSequence::event_received( const Event& Ev ) {
#if 0
    std::cout << "Recieved event: ";
    Ev.print( std::cout );
    std::cout << std::endl;
#endif
    if( Ev.status() == ChannelEvent::NoteOn || ( Ev.status() == ChannelEvent::NoteOff && PlayedStart >= 0 ) )
      add_played_event( Ev.clone() );
  } // event_received( const Event& )

  bool ExerciseSequence::beat( Event::TimeuS Time )
  {
    unsigned Clocks = static_cast<unsigned>( ( Time / tempo() ) * division() );
#if 0
    cout << "Beat @" << Clocks << " (" << Time << "us)";
    if( PlayedStart >= 0 ) {
      cout << " " << Clocks - PlayedStart << " from start (" << PlayedStart << ") & " << PlayedStart + OriginalLength - Clocks << " to end.";
    }
    cout << endl;
#endif
    Dump << "Clocks: " << Clocks << " (" << ( Clocks % MIDISequence::Division ) << ")\n";
    return PlayedStart >= 0 && Clocks > PlayedStart+OriginalLength;
  } // beat( unsigned )

  namespace Helpers
  {
    class NoteStopper
    {
      int Note;
      int Clocks;
    public:
      NoteStopper( int Note0, int Clocks0 ) : Note( Note0 ), Clocks( Clocks0 ) {}
      bool operator()( ExerciseSequence::NotePlay& Play )
      {
	bool Result = false;
	if( Play.Note == Note && Play.Stop < 0 )
	{
	  Play.Stop = Clocks;
	  Result = true;
	}
	return Result;
      }
    }; // NoteStopper
    class NoteFinder
    {
      int Note;
    public:
      NoteFinder( int Note0 ) : Note( Note0 ) {}
      bool operator()( ExerciseSequence::NotePlay& Play ) { return !Play.Visited && Play.Note == Note; }
    }; // NoteFinder
    struct UnVisited { bool operator()( ExerciseSequence::NotePlay& Play ) { return !Play.Visited; } }; // UnVisited
    class NotesSequence : public Sequencer
    {
      vector<ExerciseSequence::NotePlay> Notes;
      int Start;
      int Finish;
    public:
      NotesSequence() : Start( -1 ), Finish( 0 ) {}
      vector<ExerciseSequence::NotePlay>& notes() { return Notes; }


      void note_on( int Channel, int Note, int Velocity )
      {
	if( Start < 0 )
	  Start = Clock;
	if( Finish < Clock )
	  Finish = Clock;
	if( Velocity > 0 )
	  Notes.push_back( ExerciseSequence::NotePlay( Note, Velocity, Clock ) );
	else
	  note_off( Channel, Note, Velocity );
      } // note_on( int, int, int )
      void note_off( int Channel, int Note, int Velocity )
      { 
	if( Start < 0 )
	  Start = Clock;
	if( Finish < Clock )
	  Finish = Clock;
	find_if( Notes.begin(), Notes.end(), NoteStopper( Note, Clock ) );
      }
      int start() const { return Start; }
      int finish() const { return Finish; }
    }; // NotesSequence
  }; // Helpers

  unsigned ExerciseSequence::compare( NotesStat& Stat )
  {
    Stat.Result = NoError;

    Helpers::NotesSequence Original;
    Tracks.front()->play( Original );
    Helpers::NotesSequence Played;
    Tracks.back()->play( Played );
    Stat.Start = Played.start();
    Stat.Finish = Played.finish();

    if( Played.notes().size() != Original.notes().size() )
    {
      Stat.Result |= NoteError;
      if( Played.notes().empty() )
	Stat.Result |= EmptyPlay;
    }
    else if( !Played.notes().empty() )
    {
      vector<NotePlay> Diff;

      for( vector<NotePlay>::const_iterator OIt = Original.notes().begin(); OIt != Original.notes().end() && Stat.Result == NoError; OIt++ )
      {
	vector<NotePlay>::iterator PIt = find_if( Played.notes().begin(), Played.notes().end(), Helpers::NoteFinder( OIt->Note ) );
	if( PIt == Played.notes().end() )
	  Stat.Result |= NoteError;
	else
	{
	  Diff.push_back( NotePlay( PIt->Note, PIt->Velocity-OIt->Velocity, PIt->Start-OIt->Start, PIt->Stop-OIt->Stop ) );
	  PIt->Visited = true;
	}
      }
      if( Stat.Result == NoError && find_if( Played.notes().begin(), Played.notes().end(), Helpers::UnVisited() ) != Played.notes().end() )
	Stat.Result |= NoteError;
      Stat = for_each( Diff.begin(), Diff.end(), Stat );
      int Round = MIDISequence::Division / ( ( 1 << Denominator ) / 4.0 ) * Numerator; // Округляем до целого числа тактов
      int Min = Stat.StartMin;
      int TimeDiff = ( Min / Round ) * Round;
      if( Min % Round > Round / 2 )
	TimeDiff += Round;
      Stat.StartMin -= TimeDiff;
      Stat.StartMax -= TimeDiff;
      Stat.StopMin -= TimeDiff;
      Stat.StopMax -= TimeDiff;
      double Correction = MIDISequence::Division / 120.0;
      if( abs( Stat.StartMax ) > StartThreshold * Correction || abs( Stat.StartMin ) > StartThreshold * Correction
	  || abs( Stat.StopMax ) > StopThreshold * Correction || abs( Stat.StopMin ) > StopThreshold * Correction )
	Stat.Result |= RythmError;
      if( Correction > 0 )
      {
	Stat.StartMin /= Correction;
	Stat.StartMax /= Correction;
	Stat.StopMin /= Correction;
	Stat.StopMax /= Correction;
      }
      if( abs( Stat.VelocityMax ) > VelocityThreshold || abs( Stat.VelocityMin ) > VelocityThreshold )
	Stat.Result |= VelocityError;
    }
    return Stat.Result;
  } // compare( NotesStat& )

  void ExerciseSequence::clear()
  {
    while( !Tracks.empty() )
    {
      delete Tracks.back();
      Tracks.pop_back();
    }
    Type = 1;
    Tempo = 0;
    TracksNum = 1;
    Tracks.push_back( new MIDITrack ); // Exercise
    OriginalStart = -1;
    reset();
  } // clear()

  void ExerciseSequence::new_take()
  {
    TracksNum++;
    Tracks.push_back( new MIDITrack ); // Record
    PlayedStart = -1;
  } // new_take()
} // MuTraTrain
