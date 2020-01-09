#include "midi_files.hpp"
#include <algorithm>
#include <fstream>
using std::ios;
using std::istream;
using std::ostream;
using std::ifstream;
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
      Written += put_var( File, Time-Clock );
      vector<Event*>::const_iterator It = Events.begin();
      Written += (*It)->write( File );
      Written += for_each( ++It, Events.end(), Helpers::Writer( File ) );
    }
    return Written;
  } // write( ostream&, unsigned ) const
#ifdef MUTRA_SHOW_DISABLED
  int MIDISequence::put_int( std::ostream& File, int Size, int Number )
  {
    unsigned Num = Number;
    int ToGo = Size;
    for( ; ToGo > sizeof( Num ); ToGo-- )
      File.put( 0 );
    for( ; ToGo; ToGo-- )
      File.put( Num >> ( 8 * ( ToGo-1 ) ) & 0xFF );
    return Size;
  }
  int MIDISequence::put_var( std::ostream& File, int Number )
  {
    int Result = 0;
    unsigned Buffer = 0;
    for( unsigned Num = Number; Num; Num >>= 7 )
      Buffer = ( Buffer << 7 ) | ( Num  & 0x7F );
    for( ; Buffer > 0x7F ; Buffer >>= 7 )
    {
      File.put( ( Buffer & 0x7F ) | 0x80 );
      Result++;
    }
    File.put( Buffer );
    Result++;
    return Result;
  }
  int MIDISequence::get_int( istream& Stream, int Size )
  {
    int Val = 0;
    for( int I = 0; Stream.good() && I < Size; I++ )
    {
      Val <<= 8;
      Val |= Stream.get();
    }
    return Val;
  } // get_int( istream&, int )

  int MIDISequence::get_var( istream& Stream, int& ToGo )
  {
    int Val = 0;
    unsigned int In = 0;
    do
    {
      In = Stream.get();
      Val <<= 7;
      Val |= In & ~0x80;
      ToGo--;
    }
    while( Stream.good() && (In & 0x80) );
    return Val;
  } // get_var( istream&, int& )

  void MIDISequence::get_meta( istream& Stream, int& ToGo )
  {
    unsigned char Type = Stream.get();
    ToGo--;
    int Length = get_var( Stream, ToGo );
    ToGo -= Length;
    switch( Type )
    {
    case MetaEvent::SequenceNumber: // Sequence Number
      add( new SequenceNumberEvent( get_int( Stream, Length ) ) );
      break;
    case MetaEvent::Text: // Text event
      {
	char* Data = new char[ Length+1 ];
	Stream.read( Data, Length );
	Data[ Length ] = 0;
	add( new TextEvent( Data ) );
	delete Data;
      }
      break;
    case MetaEvent::Copyright: // Copyright
      {
	char* Data = new char[ Length+1 ];
	Stream.read( Data, Length );
	Data[ Length ] = 0;
	add( new CopyrightEvent( Data ) );
	delete Data;
      }
      break;
    case MetaEvent::TrackName: // Sequence/track name
      {
	char* Data = new char[ Length+1 ];
	Stream.read( Data, Length );
	Data[ Length ] = 0;
	add( new TrackNameEvent( Data ) );
	delete Data;
      }
      break;
    case MetaEvent::InstrumentName: // Instrument name
      {
	char* Data = new char[ Length+1 ];
	Stream.read( Data, Length );
	Data[ Length ] = 0;
	add( new InstrumentNameEvent( Data ) );
	delete Data;
      }
      break;
    case MetaEvent::Lyric: // Lyric
      {
	char* Data = new char[ Length+1 ];
	Stream.read( Data, Length );
	Data[ Length ] = 0;
	add( new LyricEvent( Data ) );
	delete Data;
      }
      break;
    case MetaEvent::Marker: // Marker
      {
	char* Data = new char[ Length+1 ];
	Stream.read( Data, Length );
	Data[ Length ] = 0;
	add( new MarkerEvent( Data ) );
	delete Data;
      }
      break;
    case MetaEvent::CuePoint: // Cue point
      {
	char* Data = new char[ Length+1 ];
	Stream.read( Data, Length );
	Data[ Length ] = 0;
	add( new CuePointEvent( Data ) );
	delete Data;
      }
      break;
    case MetaEvent::ChannelPrefix: // MIDI Channel Prefix
      add( new ChannelPrefixEvent( Stream.get() ) );
      break;
    case MetaEvent::TrackEnd: // Track end
      add( new TrackEndEvent() );
      break;
    case MetaEvent::Tempo: // Tempo (us/quarter)
      add( new TempoEvent( get_int( Stream, Length ) ) );
      break;
    case MetaEvent::SMTPEOffset: // SMTPE offset
      {
	char Hour		= Stream.get();
	char Min		= Stream.get();
	char Sec		= Stream.get();
	char Frames		= Stream.get();
	char Hundredths	= Stream.get();
	add( new SMTPEOffsetEvent( Hour, Min, Sec, Frames, Hundredths ) );
      }
      break;
    case MetaEvent::TimeSignature: // Time signature
      {
	char Numerator		= Stream.get();
	char Denominator	= Stream.get();
	char ClickClocks	= Stream.get();
	char ThirtySeconds	= Stream.get();
	add( new TimeSignatureEvent( Numerator, Denominator, ClickClocks, ThirtySeconds ) );
      }
      break;
    case MetaEvent::KeySignature: // Key signature
      {
	char Tonal	= Stream.get();
	char Minor	= Stream.get();
	add( new KeySignatureEvent( Tonal, Minor != 0 ) );
      }
      break;
    case MetaEvent::SequencerMeta: // Sequencer Meta-event
      { 
	unsigned char* Data = new unsigned char[ Length ];
	Stream.read( (char*)Data, Length );
	add( new SequencerMetaEvent( Length, Data ) );
      }
      break;
    default:
      {
	unsigned char* Data = new unsigned char[ Length ];
	Stream.read( (char*)Data, Length );
	add( new UnknownMetaEvent( Type, Length, Data ) );
      }
      break;
    }
  } // get_meta( istream&, int )

  void MIDISequence::get_sysex( istream& Stream, int& ToGo, bool AddF0 )
  {
    int Length = get_var( Stream, ToGo );
    unsigned char* Data = 0;
    int Offset = 0;
    if( AddF0 )
    {
      Data = new unsigned char[ Length+1 ];
      Data[ 0 ] = 0xF0;
      Offset = 1;
    }
    else
      Data = new unsigned char[ Length ];
    ToGo -= Stream.read( (char*)( Data+Offset ), Length ).gcount();
    add( new SysExEvent( Length, Data ) );
  } // get_sysex( istream&, int )

  void MIDISequence::get_event( istream& Stream, int& ToGo, unsigned char& Status, unsigned char FirstByte )
  {
    unsigned char In;
    if( FirstByte & 0x80 ) // Status byte (иначе - running Status)
    {
      Status = FirstByte;
      In = Stream.get();
      ToGo--;
    }
    else
      In = FirstByte;
    unsigned char Event = Status & 0xF0;
    int Channel = Status & 0x0F;
    switch( Event ) // В зависимости от сообщения
    {
    case 0x80: // Note off
    case 0x90: // Note on
    case 0xA0: // After touch
      add( new NoteEvent( MIDIEvent::MIDIStatus( Event ), Channel, In, Stream.get() ) );
      ToGo--;
      break;
    case 0xB0: // Control change
      add( new ControlChangeEvent( Channel, In, Stream.get() ) );
      ToGo--;
      break;
    case 0xC0: // Program Change
      add( new ProgramChangeEvent( Channel, In ) );
      break;
    case 0xD0: // Channel AfterTouch
      add( new ChannelAfterTouchEvent( Channel, In ) );
      break;
    case 0xE0: // Pitch bend
      int Bend = In;
      Bend |= Stream.get() << 7;
      ToGo--;
      add( new PitchBendEvent( Channel, Bend ) );
      break;
    }
  } // get_event( istream&, int )
#endif // MUTRA_SHOW_DISABLED

  MIDISequence::MIDISequence( string FileName )
  {
    ifstream File( FileName.c_str(), ios::in | ios::binary );
    unsigned Sign;
    // Читаем заголовок
    File.read( (char*)&Sign, 4 );
    if( Sign != 'dhTM' )
      throw MIDIException( "Нет заголовка файла!" );
    int ChunkSize	= get_int( File, 4 );
    Type		= get_int( File, 2 );
    TracksNum	= get_int( File, 2 );
    Division	= get_int( File, 2 );
    if( ChunkSize > 6 )
      File.ignore( ChunkSize-6 );
    for( int I = 0; I < TracksNum && File.good() && !File.eof(); I++ )
    {
      File.read( (char*)&Sign, 4 );
      int ToGo = get_int( File, 4 );
      if( Sign != 'krTM' )
	File.ignore( ToGo );
      else
      {
	Tracks.push_back( new MIDITrack );
	int Time = 0;
	unsigned char Status = 0;
	while( ToGo > 0 && !File.eof() && File.good() )
	{
	  int Delta = get_var( File, ToGo );
	  if( Delta > 0 || Tracks.back()->events().empty() )
	  {
	    Time += Delta;
	    Tracks.back()->events().push_back( new EventsList( Time ) );
	  }
#ifndef MUTRA_SHOW_DISABLED
	  if( Event* Ev = Event::get( File, ToGo, Status ) ) add( Ev );
	  else if( File.good() && !File.eof() ) {
	    cout << "Skip " << ToGo << " bytes." << endl; 
	    File.ignore( ToGo );
	  }
#else // MUTRA_SHOW_DISABLED
	  unsigned char In = File.get();
	  ToGo--;
	  switch( In )
	  {
	  case 0xF0:
	    get_sysex( File, ToGo, true );
	    break;
	  case 0xF7:
	    get_sysex( File, ToGo, false );
	    break;
	  case 0xFF:
	    get_meta( File, ToGo );
	    break;
	  default:
	    get_event( File, ToGo, Running, In );
	    break;
	  }
#endif // MT_DISABLED
	}
      }
    }
  }
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

#ifdef MT_SHOW_DISABLED
  void MIDISequence::parse( unsigned Data, int Clock, int TrackNum )
  {
    int Channel = Data & 0x0F;
    unsigned char Status = Data & 0xF0;
    Event* Eve = 0;
    switch( Status ) // В зависимости от сообщения
    {
    case Event::NoteOff: // Note off
    case Event::NoteOn: // Note on
    case Event::AfterTouch: // After touch
      Eve = new NoteEvent( Event::StatusCode( Status ), Channel, ( Data >> 8 ) & 0xFF, ( Data >> 16 ) & 0xFF );
      break;
    case Event::ControlChange: // Control change
      Eve = new ControlChangeEvent( Channel, ( Data >> 8 ) & 0xFF, ( Data >> 16 ) & 0xFF );
      break;
    case Event::ProgramChange: // Program Change
      Eve = new ProgramChangeEvent( Channel, ( Data >> 8 ) & 0xFF );
      break;
    case Event::ChannelAfterTouch: // Channel AfterTouch
      Eve = new ChannelAfterTouchEvent( Channel, ( Data >> 8 ) & 0xFF );
      break;
    case Event::PitchBend: // Pitch bend
      int Bend = ( ( Data >> 8 ) & 0xFF ) | ( ( Data >> 9 ) & 0x3F80 );
      Eve = new PitchBendEvent( Channel, Bend );
      break;
    }
    if( Eve )
    {
      MIDITrack* ToTrack = 0;
      if( TrackNum < 0 || TrackNum >= Tracks.size() ) // Добавляем в конец
      {
	if( Tracks.empty() )
	{
	  Tracks.push_back( new MIDITrack );
	  TracksNum++;
	}
	ToTrack = Tracks.back();
      }
      else
	ToTrack = Tracks[ TrackNum ];
      if( ToTrack )
      {
	if( Clock >= 0 )
	{
	  if( ToTrack->events().empty() || ToTrack->events().back()->time() != Clock )
	    ToTrack->events().push_back( new EventsList( Clock ) );
	}
	else
	{
	  if( ToTrack->events().empty() )
	    ToTrack->events().push_back( new EventsList( 0 ) );
	}
	ToTrack->events().back()->add( Eve );
      }
    }
  } // parse( unsigned, int, int )
#endif // MT_SHOW_DISABLED
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
} // MuTraMIDI
