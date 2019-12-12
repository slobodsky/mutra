// -*- coding: utf-8; -*-
#include <string>
using std::string;

#include <fstream>
using std::ifstream;
using std::istream;
using std::ostream;
using std::ios;
using std::hex;
using std::dec;
#include <iomanip>
using std::setfill;
using std::setw;
#include <algorithm>
using std::for_each;
using std::sort;
#include <sstream>
using std::stringstream;
#include <vector>
using std::vector;
#include <cstring>
using std::memcpy;
#include "midi_core.hpp"

void Event::print( ostream& Stream ) { Stream << "Пустое сообщение"; }

void MIDIInputDevice::Client::event_received( Event& /*Ev*/ ) {}
void MIDIInputDevice::Client::connected( MIDIInputDevice& /*Dev*/ ) {}
void MIDIInputDevice::Client::disconnected( MIDIInputDevice& /*Dev*/ ) {}

MIDIInputDevice::~MIDIInputDevice()
{
  while( Clients.size() > 0 )
  {
    Clients.back()->disconnected( *this );
    Clients.pop_back();
  }
} // ~MIDIInputDevice()
void MIDIInputDevice::add_client( Client& Cli )
{ //! \todo Check that it's not there yet.
  if( find( Clients.begin(), Clients.end(), &Cli ) == Clients.end() ) {
    Clients.push_back( &Cli );
    Cli.connected( *this );
  }
} // add_client( Client& )
void MIDIInputDevice::remove_client( Client& Cli )
{
  Clients.erase( find( Clients.begin(), Clients.end(), &Cli ) );
  Cli.disconnected( *this );
} // remove_client( Client* )
void MIDIInputDevice::event_received( Event& Ev ) { for( int I = 0; I < Clients.size(); ++I ) if( Clients[ I ] ) Clients[ I ]->event_received( Ev ); }
int MIDIInputDevice::parse( const unsigned char* Buffer, size_t Count ) {
  int ToGo = Count;
  const unsigned char* Pos = Buffer;
  while( ToGo > 0 ) {
    if( Event* Ev = Event::parse( Pos, ToGo ) ) event_received( *Ev );
  }
  return Count-ToGo;
} // parse( const char*, size_t )

int Event::get_int( const unsigned char* Pos, int Size )
{
  int Val = 0;
  for( int I = 0; I < Size; I++ )
  {
    Val <<= 8;
    Val |= Pos[I];
  }
  return Val;
} // get_int( const unsigned char*, int )

int Event::get_var( const unsigned char*& Pos, int& ToGo )
{
  int Val = 0;
  while( ToGo > 0 )
  {
    Val <<= 7;
    Val |= *Pos & ~0x80;
    ToGo--;
    if( !( *(Pos++) & 0x80 ) ) return Val;
  }
  ToGo--; // Indicate failure.
  return Val;
} // get_var( const unsigned char*&, int& )

Event* Event::parse( const unsigned char*& Pos, int& ToGo ) {
  switch( *Pos ) {
  case 0xF0:
  case 0xF7:
    return SysExEvent::parse( Pos, ToGo );
  case 0xFF: return MetaEvent::parse( Pos, ToGo );
  default: return MIDIEvent::parse( Pos, ToGo );
  }
  return nullptr;
} // parse( unsigned char*, int& )

Event* MIDIEvent::parse( const unsigned char*& Pos, int& ToGo ) {
  unsigned char EventCode = *Pos & 0xF0;
  int Channel = *Pos & 0x0F;
  switch( EventCode ) // В зависимости от сообщения
  {
  case 0x80: // Note off
  case 0x90: // Note on
  case 0xA0: // After touch
    if( ToGo < 3 ) return nullptr;
    else {
      unsigned char Note = Pos[1];
      unsigned char Velocity = Pos[2];
      Pos += 3;
      ToGo -= 3;
      return new NoteEvent( MIDIEvent::MIDIStatus( EventCode ), Channel, Note, Velocity );
    }
  case 0xB0: // Control change
    if( ToGo < 3 ) return nullptr;
    else {
      unsigned char Control = Pos[1];
      unsigned char Value = Pos[2];
      Pos += 3;
      ToGo -= 3;
      return new ControlChangeEvent( Channel, Control, Value );
    }
  case 0xC0: // Program Change
    if( ToGo < 2 ) return nullptr;
    else {
      unsigned char Program = Pos[1];
      Pos += 2;
      ToGo -= 2;
      return new ProgramChangeEvent( Channel, Program );
    }
  case 0xD0: // Channel AfterTouch
    if( ToGo < 2 ) return nullptr;
    else {
      unsigned char Value = Pos[1];
      Pos += 2;
      ToGo -= 2;
      return new ChannelAfterTouchEvent( Channel, Value );
    }
  case 0xE0: // Pitch bend
    if( ToGo < 3 ) return nullptr;
    else {
      int Bend = Pos[1] | ( Pos[2] << 7 );
      Pos += 3;
      ToGo -= 3;
      return new PitchBendEvent( Channel, Bend );
    }
  }
  return nullptr;
} // parse( unsigned char&*, int& )
void MIDIEvent::print( ostream& Stream ) { Stream << "MIDI сообщение " << hex << Status << ", канал " << dec << Channel; }
void NoteEvent::print( ostream& Stream )
{ 
  switch( Status )
  {
  case NoteOff:
    Stream << "Note Off";
    break;
  case NoteOn:
    Stream << "Note On";
    break;
  case AfterTouch:
    Stream << "After Touch";
    break;
  default:
    Stream << "!!! Неизвестное сообщение: " << hex << Status << dec;
    break;
  }
  Stream << ", канал " << Channel << ", нота " << Note << ", сила " << Velocity;
} // print( std::ostream& )
void NoteEvent::play( Sequencer& S )
{
  switch( Status )
  {
  case NoteOff:
    S.note_off( Channel, Note, Velocity );
    break;
  case NoteOn:
    S.note_on( Channel, Note, Velocity );
    break;
  case AfterTouch:
    S.after_touch( Channel, Note, Velocity );
    break;
  }
} // play( Sequencer& )
int NoteEvent::write( std::ostream& File ) const
{
  File.put( Status | Channel );
  File.put( Note );
  File.put( Velocity );
  return 3;
} // write( std::ostream& ) const
void ControlChangeEvent::print( ostream& Stream )
{ Stream << "Control Change, канал " << Channel << ", контроллер " << Control << ", значение " << Value; }
void ControlChangeEvent::play( Sequencer& S ) { S.control_change( Channel, Control, Value ); }
int ControlChangeEvent::write( std::ostream& File ) const
{
  File.put( Status | Channel );
  File.put( Control );
  File.put( Value );
  return 3;
} // write( std::ostream& ) const
void ProgramChangeEvent::print( ostream& Stream )
{ Stream << "Program Change, канал " << Channel << ", " << Instruments[ Program ] << " (" << Program << ")"; }
void ProgramChangeEvent::play( Sequencer& S ) { S.program_change( Channel, Program ); }
int ProgramChangeEvent::write( std::ostream& File ) const
{
  File.put( Status | Channel );
  File.put( Program );
  return 2;
} // write( std::ostream& ) const
void ChannelAfterTouchEvent::print( ostream& Stream ) { Stream << "Channel After Touch, канал " << Channel << ", значение " << Velocity; }
int ChannelAfterTouchEvent::write( std::ostream& File ) const
{
  File.put( Status | Channel );
  File.put( Velocity );
  return 2;
} // write( std::ostream& ) const
void PitchBendEvent::print( ostream& Stream ) { Stream << "Pitch Bend, канал " << Channel << ", значение " << Bend; }
void PitchBendEvent::play( Sequencer& S ) { S.pitch_bend( Channel, Bend ); }
int PitchBendEvent::write( std::ostream& File ) const
{
  File.put( Status | Channel );
  File.put( ( Bend >> 7 ) & 0x7F );
  File.put( Bend & 0x7F );
  return 3;
} // write( std::ostream& ) const

Event* MetaEvent::parse( const unsigned char*& Pos, int& ToGo ) {
  int Count = ToGo;
  const unsigned char* P = Pos;
  if( Count < 3 ) return nullptr;
  unsigned char Type = P[1];
  P += 2;
  Count -= 2;
  int Length = get_var( P, Count );
  if( Count < 0 || Count < Length ) return nullptr;
  Pos = P + Length;
  ToGo = Count - Length;
  switch( Type )
  {
  case MetaEvent::SequenceNumber: // Sequence Number
    return new SequenceNumberEvent( get_int( P, Length ) );
  case MetaEvent::Text: // Text event
    return new TextEvent( string( (const char*)P, Length ) );
  case MetaEvent::Copyright: // Copyright
    return new CopyrightEvent( string( (const char*)P, Length ) );
  case MetaEvent::TrackName: // Sequence/track name
    return new TrackNameEvent( string( (const char*)P, Length ) );
  case MetaEvent::InstrumentName: // Instrument name
    return new InstrumentNameEvent( string( (const char*)P, Length ) );
  case MetaEvent::Lyric: // Lyric
    return new LyricEvent( string( (const char*)P, Length ) );
  case MetaEvent::Marker: // Marker
    return new MarkerEvent( string( (const char*)P, Length ) );
  case MetaEvent::CuePoint: // Cue point
    return new CuePointEvent( string( (const char*)P, Length ) );
  case MetaEvent::ChannelPrefix: // MIDI Channel Prefix
    //! \todo Check length
    return new ChannelPrefixEvent( P[0] );
    break;
  case MetaEvent::TrackEnd: // Track end
    //! \todo Check length
    return new TrackEndEvent();
  case MetaEvent::Tempo: // Tempo (us/quarter)
    return new TempoEvent( get_int( P, Length ) );
  case MetaEvent::SMTPEOffset: // SMTPE offset
    //! \todo Check length
    return new SMTPEOffsetEvent( P[0], P[1], P[2], P[3], P[4] );
  case MetaEvent::TimeSignature: // Time signature
    //! \todo Check length
    return new TimeSignatureEvent( P[0], P[1], P[2], P[3] );
  case MetaEvent::KeySignature: // Key signature
    //! \todo Check length
    return new KeySignatureEvent( P[0], P[1] != 0 );
  case MetaEvent::SequencerMeta: // Sequencer Meta-event
    { 
      unsigned char* Data = new unsigned char[ Length ];
      memcpy( Data, P, Length );
      return new SequencerMetaEvent( Length, Data );
    }
  default:
    {
      unsigned char* Data = new unsigned char[ Length ];
      memcpy( Data, P, Length );
      return new UnknownMetaEvent( Type, Length, Data );
    }
  }
  return nullptr;
} // parse( const unsigned char*&, int& )
void MetaEvent::print( ostream& Stream ) { Stream << "!!! Неопознанное метасообщение"; }
void SequenceNumberEvent::print( ostream& Stream ) { Stream << "Sequence Number " << Number; }
int SequenceNumberEvent::write( std::ostream& File ) const
{
  File.put( 0xFF );
  File.put( MetaEvent::SequenceNumber );
  File.put( 2 );
  MIDISequence::put_int( File, 2, Number );
  return 1 + 1 + 1 + 2;
} // write( std::ostream& ) const

UnknownMetaEvent::UnknownMetaEvent( int Type0, int Length0, unsigned char* Data0 ) : Type( Type0 ), Length( Length0 ), Data( Data0 ) {} // Конструктор
UnknownMetaEvent::~UnknownMetaEvent() { if( Data ) delete Data; }
void UnknownMetaEvent::print( ostream& Stream )
{
  Stream << "Неопознанное метасообщение " << setfill( '0' ) << setw( 2 ) << hex << (int)Type << " длиной " << dec << setw( 1 ) << Length
	 << " байт:" << hex << setfill( '0' );
  for( int I = 0; I < Length; I++ )
    Stream << ' ' << setw( 2 ) << (int)Data[ I ];
  Stream << dec;
}
int UnknownMetaEvent::write( std::ostream& File ) const
{
  File.put( 0xFF );
  File.put( Type );
  int Result = 1 + 1 + MIDISequence::put_var( File, Length ) + Length;
  File.write( reinterpret_cast<char*>( Data ), Length );
  return Result;
} // write( std::ostream& ) const

void TextEvent::print( ostream& Stream ) { Stream << "Text: \"" << text() << "\""; }
void TextEvent::play( Sequencer& S ) { S.text( text() ); }
int TextEvent::write_text( std::ostream& File, int Type ) const
{
  int Length = Text.length();
  File.put( 0xFF );
  File.put( Type );
  int Result = 1 + 1 + MIDISequence::put_var( File, Length ) + Length;
  File << Text;
  return Result;
} // write_text( std::ostream&, int ) const
void CopyrightEvent::print( ostream& Stream ) { Stream << "Copyright: \"" << text() << "\""; }
void CopyrightEvent::play( Sequencer& S ) { S.copyright( text() ); }
void TrackNameEvent::print( ostream& Stream ) { Stream << "Track Name: \"" << text() << "\""; }
void TrackNameEvent::play( Sequencer& S ) { S.name( text() ); }
void InstrumentNameEvent::print( ostream& Stream ) { Stream << "Инструмент: \"" << text() << "\""; }
void InstrumentNameEvent::play( Sequencer& S ) { S.instrument( text() ); }
void LyricEvent::print( ostream& Stream ) { Stream << "Стихи: \"" << text() << "\""; }
void LyricEvent::play( Sequencer& S ) { S.lyric( text() ); }
void MarkerEvent::print( ostream& Stream ) { Stream << "Маркер: \"" << text() << "\""; }
void MarkerEvent::play( Sequencer& S ) { S.marker( text() ); }
void CuePointEvent::print( ostream& Stream ) { Stream << "Точка подсказки: \"" << text() << "\""; }
void CuePointEvent::play( Sequencer& S ) { S.cue( text() ); }

void ChannelPrefixEvent::print( ostream& Stream ) { Stream << "Префикс канала: " << channel(); }
int ChannelPrefixEvent::write( std::ostream& File ) const
{
  File.put( 0xFF );
  File.put( MetaEvent::ChannelPrefix );
  File.put( 1 );
  File.put( Channel );
  return 1 + 1 + 1 + 1;
} // write( std::ostream& ) const
void TrackEndEvent::print( ostream& Stream ) { Stream << "Конец дорожки."; }
int TrackEndEvent::write( std::ostream& File ) const
{
  File.put( 0xFF );
  File.put( MetaEvent::TrackEnd );
  File.put( 0 );
  return 1 + 1 + 1;
} // write( std::ostream& ) const
void TempoEvent::print( ostream& Stream ) { Stream << "Длительность четвертной ноты " << tempo() << " микросекунд"; }
void TempoEvent::play( Sequencer& S ) { S.tempo( tempo() ); }
int TempoEvent::write( std::ostream& File ) const
{
  File.put( 0xFF );
  File.put( MetaEvent::Tempo );
  File.put( 3 );
  MIDISequence::put_int( File, 3, Tempo );
  return 1 + 1 + 1 + 3;
} // write( std::ostream& ) const
void SMTPEOffsetEvent::print( ostream& Stream )
{ Stream << "Задержка дорожки: " << setfill( '0' ) << setw( 2 ) << hour() << ':' << minute() << ':' << second() << ':' << frame() << '.' << hundredths(); }
int SMTPEOffsetEvent::write( std::ostream& File ) const
{
  File.put( 0xFF );
  File.put( MetaEvent::SMTPEOffset );
  File.put( 5 );
  File.put( Hour );
  File.put( Minute );
  File.put( Second );
  File.put( Frame );
  File.put( Hundredths );
  return 1 + 1 + 1 + 5;
} // write( std::ostream& ) const
void TimeSignatureEvent::play( Sequencer& S ) { S.meter( Numerator, Denominator ); }
void TimeSignatureEvent::print( ostream& Stream )
{ 
  Stream << "Time Signature: " << numerator() << '/' << ( 1 << denominator() ) << ", метроном: " << click_clocks() << ", в одной четверти "
	 << thirtyseconds() << " тридцатьвторых";
}
int TimeSignatureEvent::write( std::ostream& File ) const
{
  File.put( 0xFF );
  File.put( MetaEvent::TimeSignature );
  File.put( 4 );
  File.put( Numerator );
  File.put( Denominator );
  File.put( ClickClocks );
  File.put( ThirtySeconds );
  return 1 + 1 + 1 + 4;
} // write( std::ostream& ) const
void KeySignatureEvent::print( ostream& Stream )
{
  Stream << "Тональность " << ( minor() ? "минорная" : "мажорная" ) << ", при ключе ";
  if( tonal() < 0 )
    Stream << -tonal() << " бемолей";
  else
    Stream << tonal() << " диезов";
}
int KeySignatureEvent::write( std::ostream& File ) const
{
  File.put( 0xFF );
  File.put( MetaEvent::KeySignature );
  File.put( 2 );
  File.put( Tonal );
  File.put( Minor );
  return 1 + 1 + 1 + 2;
} // write( std::ostream& ) const
void SequencerMetaEvent::print( ostream& Stream )
{
  Stream << "Metaсообщение секвенсору (" << length() << " байт):" << hex << setfill( '0' ) << setw( 2 );
  for( int I = 0; I < length(); I++ )
    Stream << ' ' << int( Data[ I ] );
  Stream << dec;
}
int SequencerMetaEvent::write( std::ostream& File ) const
{
  File.put( 0xFF );
  File.put( MetaEvent::SequencerMeta );
  int Result = 1 + 1 + MIDISequence::put_var( File, Length ) + Length;
  File.write( reinterpret_cast<char*>( Data ), Length );
  return Result;
} // write( std::ostream& ) const

Event* SysExEvent::parse( const unsigned char*& Pos, int& ToGo ) {
  if( ToGo < 2 ) return nullptr;
  const unsigned char* P = Pos+1;
  int Count = ToGo;
  int Length = get_var( P, Count );
  if( Count < 0 || Count < Length ) return nullptr;
  unsigned char* Data = new unsigned char[ *Pos == 0xF0 ? Length+1 : Length ];
  if( *Pos == 0xF0 )
  {
    Data[ 0 ] = 0xF0;
    memcpy( Data+1, P, Length );
  }
  else
    memcpy( Data, P, Length );
  Pos = P + Length;
  ToGo = Count - Length;
  return new SysExEvent( Length, Data );
} // parse( const unsigned char*&, int& )
void SysExEvent::print( ostream& Stream )
{
  Stream << "SysEx (" << length() << " байт):" << hex << setfill( '0' ) << setw( 2 );
  for( int I = 0; I < length(); I++ )
    Stream << ' ' << int( Data[ I ] );
  Stream << dec;
}
int SysExEvent::write( std::ostream& File ) const
{
  int Result = 1;
  if( *Data == 0xF0 )
  {
    File.put( 0xF0 );
    Result += MIDISequence::put_var( File, Length-1 ) + Length-1;
    File.write( reinterpret_cast<char*>( Data+1 ), Length-1 );
  }
  else
  {
    File.put( 0xF7 );
    Result += MIDISequence::put_var( File, Length ) + Length;
    File.write( reinterpret_cast<char*>( Data ), Length );
  }
  return Result;
} // write( std::ostream& ) const

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
      Written += MIDISequence::put_var( File, 0 ) + Ev->write( File );
    }
    operator int() const { return Written; }
  }; // Writer
}; // Helpers
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
    Written += MIDISequence::put_var( File, Time-Clock );
    vector<Event*>::const_iterator It = Events.begin();
    Written += (*It)->write( File );
    Written += for_each( ++It, Events.end(), Helpers::Writer( File ) );
  }
  return Written;
} // write( ostream&, unsigned ) const

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
      unsigned char Running = 0;
      while( ToGo > 0 && !File.eof() && File.good() )
      {
	int Delta = get_var( File, ToGo );
	if( Delta > 0 || Tracks.back()->events().empty() )
	{
	  Time += Delta;
	  Tracks.back()->events().push_back( new EventsList( Time ) );
	}
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
  MIDISequence::put_int( File, 4, Written );
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
}; // Helpers
void MIDISequence::parse( unsigned Data, int Clock, int TrackNum )
{
  int Channel = Data & 0x0F;
  unsigned char Status = Data & 0xF0;
  Event* Eve = 0;
  switch( Status ) // В зависимости от сообщения
  {
  case MIDIEvent::NoteOff: // Note off
  case MIDIEvent::NoteOn: // Note on
  case MIDIEvent::AfterTouch: // After touch
    Eve = new NoteEvent( MIDIEvent::MIDIStatus( Status ), Channel, ( Data >> 8 ) & 0xFF, ( Data >> 16 ) & 0xFF );
    break;
  case MIDIEvent::ControlChange: // Control change
    Eve = new ControlChangeEvent( Channel, ( Data >> 8 ) & 0xFF, ( Data >> 16 ) & 0xFF );
    break;
  case MIDIEvent::ProgramChange: // Program Change
    Eve = new ProgramChangeEvent( Channel, ( Data >> 8 ) & 0xFF );
    break;
  case MIDIEvent::ChannelAfterTouch: // Channel AfterTouch
    Eve = new ChannelAfterTouchEvent( Channel, ( Data >> 8 ) & 0xFF );
    break;
  case MIDIEvent::PitchBend: // Pitch bend
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


const char* NoteEvent::Percussion[ PercussionLast-PercussionFirst+1 ] =
  { "Bass Drum", "Bass Drum 1", "Side Stick", "Acoustic Snare", "Hand Clap", "Electric Snare", "Low Floor Tom", "Closed Hi-Hat", "High Floor Tom",
    "Pedal Hi-Hat", "Low Tom", "Open Hi-Hat", "Low-Mid Tom", "Hi-Mid Tom", "Crash Cymbal 1", "High Tom", "Ride Cymbal 1", "Chinese Cymbal",
    "Ride Bell", "Tambourine", "Splash Cymbal", "Cowbell", "Crash Cymbal 2", "Vibraslap", "Ride Cymbal 2", "Hi Bongo", "Low Bongo", "Mute Hi Conga",
    "Open Hi Conga", "Low Conga", "High Timbale", "Low Timbale", "High Agogo", "Low Agogo", "Cabasa", "Maracas", "Short Whistle", "Long Whistle",
    "Short Guiro", "Long Guiro", "Claves", "Hi Wood Block", "Low Wood Block", "Mute Cuica", "Open Cuica", "Mute Triangle", "Open Triangle" };

const char* ProgramChangeEvent::Instruments[ 128 ] =
  { "Acoustic Grand", "Bright Acoustic", "Electric Grand", "Honky-Tonk", "Electric Piano1", "Electric Piano2", "Harpsichord", "Clav", "Celesta",
    "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer", "Drawbar Organ", "Percussive Organ",
    "Rock Organ", "Church Organ", "Reed Organ", "Accoridan", "Harmonica", "Tango Accordian", "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)",
    "Electric Guitar (jazz)", "Electric Guitar (clean)", "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar Harmonics",
    "Acoustic Bass", "Electric Bass (finger)	", "Electric Bass (pick)", "Fretless Bass", "Slap Bass 1", "Slap Bass 2", "Synth Bass 1",
    "Synth Bass 2", "Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings", "Pizzicato Strings", "Orchestral Strings", "Timpani",
    "String Ensemble 1", "String Ensemble 2", "SynthStrings 1", "SynthStrings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
    "Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "SynthBrass 1", "SynthBrass 2", "Soprano Sax", "Alto Sax",
    "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet", "Piccolo", "Flute", "Recorder", "Pan Flute", "Blown Bottle",
    "Skakuhachi", "Whistle", "Ocarina", "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)", "Lead 5 (charang)",
    "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass+lead)", "Pad 1 (new age)	", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)",
    "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)", "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)",
    "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)", "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba", "Bagpipe", "Fiddle",
    "Shanai", "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal", "Guitar Fret Noise",
    "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter", "Applause", "Gunshot" };
const char* ProgramChangeEvent::Families[ 16 ] = { "PIANO", "CHROM PERCUSSION", "ORGAN", "GUITAR", "BASS", "STRINGS", "ENSEMBLE", "BRASS",
						   "REED", "PIPE", "SYNTH LEAD", "SYNTH PAD", "SYNTH EFFECTS", "ETHNIC", "PERCUSSIVE", "SOUND EFFECTS" };
