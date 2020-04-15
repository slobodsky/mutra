#include "midi_files.hpp"
#include <algorithm>
#include <fstream>
using std::ios;
using std::istream;
using std::ostream;
using std::ifstream;
using std::ofstream;
using std::for_each;
using std::string;
using std::vector;
using std::cout;
using std::endl;

namespace MuTraMIDI {
  namespace Helpers
  {
    class Printer
    {
      ostream& Stream;
      int TrackNum;
    public:
      Printer( ostream& Stream0 ) : Stream( Stream0 ), TrackNum( 0 ) {}
      void operator()( Event* E )
      { 
	Stream << "\n\t";
	E->print( Stream );
      }
      void operator()( EventsList* E )
      { 
	Stream << '\n';
	E->print( Stream );
      }
      void operator()( MIDITrack* T )
      { 
	Stream << "\n\n*** Track № " << ++TrackNum;
	T->print( Stream );
      }
    }; // Printer
    struct TrackEventsList
    {
      int Track;
      EventsList* List;
      TrackEventsList( int T, EventsList* L ) : Track( T ), List( L ) {}
      bool operator< ( const TrackEventsList& L ) const
      { return List->time() < L.List->time() || ( List->time() == L.List->time() && Track < L.Track ); }
    }; // TrackEventsList
    class Player
    {
      Sequencer& Seq;
    public:
      Player( Sequencer& Seq0 ) : Seq( Seq0 ) {}
      void operator()( Event* E ) { E->play( Seq ); }
      void operator()( EventsList* L ) { L->play( Seq ); }
      void operator()( const TrackEventsList& L )
      { 
	Seq.track( L.Track );
	L.List->play( Seq );
      }
    }; // Player
    class Writer
    {
      ostream& File;
      int Written;
      unsigned Clock;
    public:
      Writer( ostream& File0 ) : File( File0 ), Written( 0 ), Clock( 0 ) {}
      void operator()( const MIDITrack* Track )
      {
	Track->write( File );
      }
      void operator()( const EventsList* List )
      {
	Written += List->write( File, Clock );
	Clock = List->time();
      }
      void operator()( const Event* Ev )
      {
	Written += put_var( File, 0 ) + Ev->write( File );
      }
      operator int() const { return Written; }
    }; // Writer
  } // Helpers

  void EventsList::print( ostream& Stream )
  {
    Stream << "Time: " << Time;
    for_each( Events.begin(), Events.end(), Helpers::Printer( Stream ) );
  }
  void EventsList::play( Sequencer& S ) { for_each( Events.begin(), Events.end(), Helpers::Player( S ) ); }
  int EventsList::write( std::ostream& File, unsigned Clock ) const
  {
    int Written = 0;
    if( !Events.empty() )
    {
#ifdef MUTRA_DEBUG
      cout << "Write events list @" << Time << " diff: " << Time-Clock << endl;
#endif
      Written += put_var( File, Time-Clock );
      vector<Event*>::const_iterator It = Events.begin();
      Written += (*It)->write( File );
      Written += for_each( ++It, Events.end(), Helpers::Writer( File ) );
    }
    return Written;
  } // write( ostream&, unsigned ) const

  MIDISequence::MIDISequence( string FileName ) : Low( 128 ), High( 0 )
  {
    ifstream File( FileName.c_str(), ios::in | ios::binary );
    FileInStream InSt( File );

    unsigned Sign;
    // Читаем заголовок
    //! \todo Make some kind of plug-in objects that handle different chunk types.
    InSt.read( (unsigned char*)&Sign, 4 );
    if( Sign != 0x6468544Du ) // MThd (in opposite direction, cause LSF)
      throw MIDIException( "Нет заголовка файла!" );
    int ChunkSize	= InSt.get_int( 4 );
    //! \todo Correctly support type 1 files with only the tempo events in the first trak.
    Type		= InSt.get_int( 2 );
    TracksNum	= InSt.get_int( 2 );
    //! \todo Support SMPTE timings (now only PPQN)
    Division	= InSt.get_int( 2 );
#ifdef MUTRA_DEBUG
    cout << "Reading MIDI file [" << FileName << "] type " << Type << " header chunk size " << ChunkSize << " tracks # " << TracksNum << " division " << Division << endl;
#endif
    if( ChunkSize > 6 )
      InSt.skip( ChunkSize-6 );
    for( int I = 0; I < TracksNum; I++ )
    {
      InSt.read( (unsigned char*)&Sign, 4 );
      int ToGo = InSt.get_int( 4 );
      if( Sign != 0x6B72544Du ) // MTrk
	InSt.skip( ToGo );
      else {
#ifdef MUTRA_DEBUG
	cout << "Reading track " << Tracks.size() << " " << ToGo << " bytes" << endl;
#endif
	Tracks.push_back( new MIDITrack );
	int Time = 0;
	// unsigned char Status = 0;
	while( ToGo > 0 )
	{
	  int Delta = InSt.get_var();
	  ToGo -= InSt.count();
	  if( Delta > 0 || Tracks.back()->events().empty() ) {
	    Time += Delta;
	    Tracks.back()->events().push_back( new EventsList( Time ) );
	  }
	  if( Event* Ev = InSt.get_event() ) {
	    if( Ev->status() == Event::NoteOn ) {
	      NoteEvent* N = static_cast<NoteEvent*>( Ev );
	      if( N->note() > High ) High = N->note();
	      if( N->note() < Low ) Low = N->note();
	    }
	    add( Ev );
	    ToGo -= InSt.count();
#ifdef MUTRA_DEBUG
	    cout << "Got event ";
	    Ev->print( cout );
	    cout << " @" << Tracks.back()->events().back()->time() << " " << InSt.count() << " bytes " << ToGo << " to go." << endl;
#endif
	  }
	  else {
	    cout << "Skip " << ToGo << " bytes." << endl;
	    InSt.skip( ToGo );
	    ToGo = 0;
	  }
	}
      }
    }
    if( Low > High ) {
      Low = 0;
      High = 127;
    }
  }
  void MIDITrack::close() {
    if( Events.empty() ) Events.push_back( new EventsList( 0 ) );
    vector<Event*>& LastList = Events.back()->events();
    if( LastList.size() == 0 || LastList.back()->status() != Event::MetaCode || static_cast<MetaEvent*>( LastList.back() )->type() != MetaEvent::TrackEnd )
      LastList.push_back( new TrackEndEvent );
  } // close()
  void MIDITrack::print( ostream& Stream )
  {
    for_each( Events.begin(), Events.end(), Helpers::Printer( Stream ) );
  } // print( ostream& )
  void MIDITrack::play( Sequencer& Seq )
  {
    for( vector<EventsList*>::const_iterator It = Events.begin(); It != Events.end(); It++ )
    {
      Seq.wait_for( (*It)->time() );
      (*It)->play( Seq );
    }
  } // play( Sequencer& )
  bool MIDITrack::write( std::ostream& File ) const
  {
    // Заголовок
    File << "MTrk";
    ostream::pos_type Pos = File.tellp();
    File << "????";
    int Written = for_each( Events.begin(), Events.end(), Helpers::Writer( File ) );
    File.seekp( Pos );
    put_int( File, 4, Written );
    File.seekp( 0, ios::end );
    return File.good();
  } // write( std::ostream& ) const

  void MIDISequence::add_track() {
    ++TracksNum;
    if( TracksNum > 1 && Type == 0 ) Type = 1; // 2; - Maybe this is more correct, but not every program understand this.
    if( Tracks.size() > 0 ) Tracks.back()->close();
    Tracks.push_back( new MIDITrack );
  } // add_track()
  void MIDISequence::add_event( int Time, Event* NewEvent ) {
    if( Tracks.size() == 0 ) add_track();
    MIDITrack& Track = *Tracks.back();
    if( Track.events().size() == 0 || Track.events().back()->time() != Time ) Track.events().push_back( new EventsList( Time ) );
    add( NewEvent );
  } // add_event( int, Event* )
  void MIDISequence::print( ostream& Stream )
  {
    Stream << "Файл типа " << Type << ", " << TracksNum << " дорожек, дивизион: " << Division;
    for_each( Tracks.begin(), Tracks.end(), Helpers::Printer( Stream ) );
  } // print( ostream& )

  namespace Helpers
  {
    class ListFiller
    {
      int Track;
      vector<TrackEventsList>& List;
    public:
      ListFiller( vector<TrackEventsList>& List0 ) : List( List0 ), Track( 0 ) {}
      void operator()( MIDITrack* T )
      {
	for_each( T->events().begin(), T->events().end(), *this );
	Track++;
      }
      void operator()( EventsList* E ) { List.push_back( TrackEventsList( Track, E ) ); }
    }; // ListFiller
  } // Helpers

  void MIDISequence::play( Sequencer& S )
  {
    S.division( Division );
    S.reset();
    vector<Helpers::TrackEventsList> TracksEvents;
    for_each( Tracks.begin(), Tracks.end(), Helpers::ListFiller( TracksEvents ) );
    sort( TracksEvents.begin(), TracksEvents.end() );
    vector<Helpers::TrackEventsList>::const_iterator It = TracksEvents.begin();
    unsigned Time = 0;
    vector<Helpers::TrackEventsList> ToPlay;
    while( It != TracksEvents.end() && It->List->time() == Time )
    {
      ToPlay.push_back( *It );
      It++;
    }
    for_each( ToPlay.begin(), ToPlay.end(), Helpers::Player( S ) );
    ToPlay.clear();
    S.start();
    while( It != TracksEvents.end() )
    {
      if( Time > It->List->time() )
	throw MIDIException( "Check your sort function!" );
      Time = It->List->time();
      while( It != TracksEvents.end() && It->List->time() == Time )
      {
	ToPlay.push_back( *It );
	It++;
      }
      S.wait_for( Time );
      for_each( ToPlay.begin(), ToPlay.end(), Helpers::Player( S ) );
      ToPlay.clear();
    }
  } // play( Sequencer& )

  bool MIDISequence::write( std::ostream& File ) const
  {
    // Заголовок
    File << "MThd";
    int ChunkSize = 6;
    put_int( File, 4, ChunkSize );
    put_int( File, 2, Type );
    put_int( File, 2, TracksNum );
    put_int( File, 2, Division );
    // Дорожки
    for_each( Tracks.begin(), Tracks.end(), Helpers::Writer( File ) );
    return File.good();
  } // write( std::ostream& ) const
  bool MIDISequence::close_and_write( const string& FileName ) {
    close_last_track();
    ofstream File( FileName );
    return write( File );
  } // close_and_write( const string& )
} // MuTraMIDI
