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
using MuTraMIDI::get_time_us;
using MuTraMIDI::Event;
using MuTraMIDI::TimeSignatureEvent;
using MuTraMIDI::KeySignatureEvent;
using MuTraMIDI::ChannelEvent;
using MuTraMIDI::NoteEvent;
using MuTraMIDI::EventsList;
using MuTraMIDI::MIDITrack;
using MuTraMIDI::MIDIException;
using MuTraMIDI::MIDISequence;
using MuTraMIDI::Sequencer;
#include <unistd.h>

namespace MuTraTrain {
  bool is_absolute_path_name( const string& FileName ) { return !FileName.empty() && FileName[0] == '/'; }
  string absolute_dir_name( const string& FileName ) {
    string Result;
    size_t Pos = FileName.rfind( '/' );
    if( Pos != string::npos ) {
      if( Pos == 0 ) return "/";
      else Result = FileName.substr( 0, Pos );
    }
    if( !is_absolute_path_name( Result ) ) {
      char CWD[ PATH_MAX ];
      if( getcwd( CWD, PATH_MAX ) ) return prepend_path( CWD, Result );
    }
    return Result;
  } // absolute_dir_name( cosnt std::string& )
  string prepend_path( const string& DirName, const string& FileName ) {
    if( FileName.empty() ) return DirName;
    if( DirName.empty() ) return FileName;
    if( DirName.back() != '/' ) return DirName + '/' + FileName;
    return DirName + FileName;
  } // prepend_path( const string&, const string& )

  void ExerciseSequence::TryResult::start_note( int Note, int Velocity, int Clock ) {
    mNotes.push_back( PlayedNote( Note, Velocity, Clock ) );
    if( mStat.Start < 0 ) mStat.Start = Clock; //!< \todo Stat's Start & Finish should be in real time, not sequencer's clock!
    if( mStat.Finish < Clock ) mStat.Finish = Clock;
  } // start_note( int, int, int )
  void ExerciseSequence::TryResult::stop_note( int Note, int Clock ) {
    if( find_if( mNotes.rbegin(), mNotes.rend(),
		 [Note, Clock]( PlayedNote& N ) {
		   if( N.note() == Note ) {
		     if( N.stop() < 0 ) N.stop( Clock );
		     else cerr << "Warning: note double stop." << endl;
		     return true;
		   }
		   return false;
		 } ) != mNotes.rend()
	&& mStat.Finish < Clock )
      mStat.Finish = Clock;
  } // stop_note( int, int )

  bool ExerciseSequence::load( const string& FileName )
  {
    bool Result = false;
    clear();
    try
    {
      ifstream Ex( FileName );
      if( Ex.good() )
      {
	string Name;
	Ex >> Name >> TargetTracks >> Channels >> StartPoint >> StopPoint >> StartThreshold >> StopThreshold >> VelocityThreshold >> TempoSkew;
	if( !is_absolute_path_name( Name ) ) Name = prepend_path( absolute_dir_name( FileName ), Name );
#ifdef MUTRA_DEBUG
	cout << "Load play from " << Name << " tracks: " << TargetTracks << " channels: " << Channels << " [" << StartPoint << "-" << StopPoint << " limits: start: " << StartThreshold
	     << " stop: " << StopThreshold << " velocity: " << VelocityThreshold << " tempo " << TempoSkew / 100.0 << "%" << endl;
#endif
	clear();
	Play = new MIDISequence( Name );
	start();
	Play->play( *this );
	adjust_length();
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
#ifdef MUTRA_DEBUG
	cout << "Exercise " << FileName << " loaded." << endl;
#endif
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
      if( Channels & ( 1 << Channel ) && TargetTracks & ( 1 << Track ) && Clock >= StartPoint && ( StopPoint < 0 || Clock < StopPoint ) ) {
	HangingNotes.push_back( ChanNote( Note, Channel ) );
	mOriginal.push_back( new OriginalNote( Note, Channel, Velocity, Clock ) );
	add_original_event( new NoteEvent( ChannelEvent::NoteOn, Channel, Note, Velocity, clocks_to_us( Clock ) ) );
      }
    }
    else
      note_off( Channel, Note, Velocity );
  } // note_on( int, int, int )
  void ExerciseSequence::note_off( int Channel, int Note, int Velocity )
  {
    if( Channels & ( 1 << Channel ) && TargetTracks & ( 1 << Track ) && Clock > StartPoint ) {
      auto It = find( HangingNotes.begin(), HangingNotes.end(), ChanNote( Note, Channel ) );
      if( It != HangingNotes.end() ) {
	HangingNotes.erase( It );
	add_original_event( new NoteEvent( ChannelEvent::NoteOff, Channel, Note, Velocity, clocks_to_us( Clock ) ) );
	auto N = find_if( mOriginal.rbegin(), mOriginal.rend(), [Note](const OriginalNote* Try){ return Try->note() == Note; } );
	if( N != mOriginal.rend() ) (*N)->stop( Clock );
      }
    }
  } // note_off( int, int, int )
  void ExerciseSequence::tempo( unsigned uSecForQuarter )
  {
#if 0
    if( Tempo == 0 )
#endif
      Tempo = uSecForQuarter;
  } // tempo( unsigned )
  void ExerciseSequence::meter( int N, int D ) {
    Numerator = N;
    Denominator = D;
  } // meter( int, int )
  void ExerciseSequence::key_signature( int T, bool M ) {
    Tonal = T;
    Minor = M;
  } // key_signature( int, bool )
  void ExerciseSequence::add_original_event( Event* NewEvent )
  {
    int OriginalAlignment = StartPoint % bar_clocks();
    int ClockFromStart = Clock - StartPoint + OriginalAlignment;
    if( Tracks.empty() ) add_track();
#ifdef MUTRA_DEBUG
    cout << "O: ";
#endif
    if( Tracks.front()->events().empty() || Tracks.front()->events().back()->time() != ClockFromStart )
    {
#ifdef MUTRA_DEBUG
      cout << "@" << ClockFromStart << " ";
#endif
      Tracks.front()->events().push_back( new EventsList( ClockFromStart ) );
      if( OriginalStart < 0 )
      {
#ifdef MUTRA_DEBUG
	cout << "Add time signature " << Numerator << "/" << (1 << Denominator) << endl;
#endif
	Tracks.front()->events().back()->add( new TimeSignatureEvent( Numerator, Denominator ) );
	Tracks.front()->events().back()->add( new KeySignatureEvent( Tonal, Minor ) );
	OriginalStart = ClockFromStart;
	OriginalLength = 0;
      }
      else if( ClockFromStart - OriginalStart > OriginalLength )
	OriginalLength = ClockFromStart - OriginalStart;
    }
#ifdef MUTRA_DEBUG
    NewEvent->print( cout );
    cout << endl;
#endif
    Tracks.front()->events().back()->add( NewEvent );
  } // add_original_event( Event* )
  void ExerciseSequence::add_played_event( Event* NewEvent ) {
    Event::TimeuS EvTime = get_time_us();
    if( PlayedStartuS <= 0 ) {
      if( NewEvent->status() == ChannelEvent::NoteOn ) {
	PlayedStartuS = EvTime;
	if( align_start() > 0 ) { //! \todo Make this ugly code clean.
	  int BaruS = bar_us();
	  int Offset = ( PlayedStartuS - align_start() ) % BaruS;
	  int OriginalToBar = clocks_to_us( OriginalStart % bar_clocks() );
#define MUTRA_BEAT_ALIGN
#ifdef MUTRA_BEAT_ALIGN
	  int BeatuS = beat_us();
	  int Diff = Offset - OriginalToBar;
	  if( Diff % BeatuS > BeatuS / 1.5 ) Diff = Diff / BeatuS + 1;
	  else Diff /= BeatuS;
	  Offset += Diff * BeatuS;
#endif // MUTRA_BEAT_ALIGN
	  Offset -= OriginalToBar;
	  if( Offset > BaruS / 1.5 ) Offset -= BaruS;
	  cout << "Align to " << Offset << " µs, PlayedStart " << PlayedStartuS << " align: " << align_start() << " tempo: " << tempo() << " diff: " << PlayedStartuS - align_start() << endl;
	  PlayedStartuS -= Offset;
	}
      }
      else return; // Skip events before start
    }
    int EvClock = us_to_clocks( EvTime-PlayedStartuS ) + OriginalStart;
#ifdef MUTRA_DEBUG
    std::cout << "Add event @" << EvClock << " (" << NewEvent->time() << "us): ";
    NewEvent->print( std::cout );
    std::cout << std::endl;
#endif
    add_event( EvClock, NewEvent );
  } // add_played_event( Event* )
  void ExerciseSequence::event_received( const Event& Ev ) {
#ifdef MUTRA_DEBUG
    std::cout << "Recieved event @\t" << get_time_us() << '\t';
    Ev.print( std::cout );
    std::cout << std::endl;
#endif
    if( Ev.status() == ChannelEvent::NoteOn || ( Ev.status() == ChannelEvent::NoteOff && PlayedStartuS > 0 ) )
      add_played_event( Ev.clone() );
  } // event_received( const Event& )
  bool ExerciseSequence::beat( Event::TimeuS Time )
  {
    if( PlayedStartuS <= 0 ) return false;
    int Clocks = us_to_clocks( Time-PlayedStartuS );
#ifdef MUTRA_DEBUG
    cout << "Beat @" << Clocks << " (" << Time << "us)";
    cout << " " << Clocks - PlayedStart << " from start (" << PlayedStart << ") & " << PlayedStart + OriginalLength - Clocks << " to end.";
    cout << endl;
#endif
    Dump << "Clocks: " << Clocks << " (" << ( Clocks % MIDISequence::Division ) << ")" << endl;
    return Clocks > OriginalLength;
  } // beat( unsigned )

  namespace Helpers
  {
    class NotesSequence : public Sequencer {
    public:
      NotesSequence( ExerciseSequence::TryResult& Try ) : mTry( Try ) {}
      ExerciseSequence::TryResult& notes() { return mTry; }

      void note_on( int Channel, int Note, int Velocity ) {
	if( Velocity > 0 )
	  mTry.start_note( Note, Velocity, Clock );
	else
	  note_off( Channel, Note, Velocity );
      } // note_on( int, int, int )
      void note_off( int Channel, int Note, int Velocity ) { mTry.stop_note( Note, Clock ); }
    private:
      ExerciseSequence::TryResult& mTry;
    }; // NotesSequence
  }; // Helpers

  unsigned ExerciseSequence::compare() {
    if( mPlayed.size() >= Tracks.size()-1 ) return mPlayed.back()->stat().Result;
    TryResult* Res = new TryResult;
    Helpers::NotesSequence Filler( *Res );
    Tracks.back()->play( Filler );
    mPlayed.push_back( Res );
    return Res->compare( *this );
  } // compare()

  unsigned ExerciseSequence::TryResult::compare( const ExerciseSequence& Original ) {
    for( const OriginalNote* ON : Original.original() ) {
      if( find_if( mNotes.begin(), mNotes.end(), [ON]( PlayedNote& Note ) { //!< \todo Optimize: search not from the begin, but from the last note that was not found.
						   if( !Note.original() && Note.note() == ON->note() ) {
						     Note.original( ON );
						     return true;
						   }
						   return false;
						 } )  == mNotes.end() )
	mStat.Result |= NoteError;
    }
    if( mStat.Result == NoError && find_if( mNotes.begin(), mNotes.end(), []( const PlayedNote& Note ) { return !Note.original(); } ) != mNotes.end() )
      mStat.Result |= NoteError;
    for( const PlayedNote& Note : mNotes )
      if( Note.original() ) {
	if( Note.start_diff() > mStat.StartMax ) mStat.StartMax = Note.start_diff();
	if( Note.start_diff() < mStat.StartMin ) mStat.StartMin = Note.start_diff();
	if( Note.stop() > 0 ) { // Skip lost NoteOff messages from buggy keyboards.
	  if( Note.stop_diff() > mStat.StopMax ) mStat.StopMax = Note.stop_diff();
	  if( Note.stop_diff() < mStat.StopMin ) mStat.StopMin = Note.stop_diff();
	}
	if( Note.velocity_diff() > mStat.VelocityMax ) mStat.VelocityMax = Note.velocity_diff();
	if( Note.velocity_diff() < mStat.VelocityMin ) mStat.VelocityMin = Note.velocity_diff();
      } //! \todo (below) Remove all access to the Original's member variables.
    double Correction = Original.division() / 120.0;
    if( abs( mStat.StartMax ) > Original.StartThreshold * Correction || abs( mStat.StartMin ) > Original.StartThreshold * Correction
	|| abs( mStat.StopMax ) > Original.StopThreshold * Correction || abs( mStat.StopMin ) > Original.StopThreshold * Correction )
      mStat.Result |= RythmError;
    if( Correction > 0 ) {
      mStat.StartMin /= Correction;
      mStat.StartMax /= Correction;
      mStat.StopMin /= Correction;
      mStat.StopMax /= Correction;
    }
    if( abs( mStat.VelocityMax ) > Original.VelocityThreshold || abs( mStat.VelocityMin ) > Original.VelocityThreshold )
      mStat.Result |= VelocityError;
    return mStat.Result;
  } // compare( const ExerciseSequence& )

  void ExerciseSequence::clear()
  { //! \todo Thread protection
    for( auto Try : mPlayed ) delete Try;
    mPlayed.clear();
    for( auto Note : mOriginal ) delete Note;
    mOriginal.clear();
    for( auto Track : Tracks ) delete Track;
    Tracks.clear();
    if( Play ) delete Play;
    Play = nullptr;
    Type = 0;
    Tempo = 0;
    TracksNum = 0;
    add_track(); // Exercise (original)
    OriginalStart = -1;
    reset();
  } // clear()

  void ExerciseSequence::rescan() {
    while( !Tracks.empty() )
    {
      delete Tracks.back();
      Tracks.pop_back();
    }
    add_track();
    OriginalStart = -1;
    reset();
    start();
    Play->play( *this );
  } // rescan()
  void ExerciseSequence::new_take()
  {
    add_track(); // Record
    PlayedStartuS = 0;
  } // new_take()
} // MuTraTrain
