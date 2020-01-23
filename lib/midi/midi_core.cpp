// -*- coding: utf-8; -*-
#include "midi_events.hpp"
using std::string;

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
using std::endl;
using std::cout;
using std::cerr;

namespace MuTraMIDI {
  int get_int( istream& Str, int Size ) {
    int Val = 0;
    for( int I = 0; I < Size && Str.good() && !Str.eof(); ++I ) {
      Val <<= 8;
      Val |= (unsigned char)( Str.get() );
    }
    return Val;
  } // get_int( std::istream& )
  int get_var( istream& Str, int& Length ) {
    int Val = 0;
    while( Length > 0 && Str.good() && !Str.eof() ) {
      Val <<= 7;
      unsigned char Ch = (unsigned char)( Str.get() );
      Val |= Ch & ~0x80;
      --Length;
      if( !( Ch & 0x80 ) ) return Val;
    }
    return Val;
  } // get_var( istream&, int& )
  int put_int( ostream& File, int Size, int Number ) {
    unsigned Num = Number;
    int ToGo = Size;
    for( ; ToGo > sizeof( Num ); ToGo-- )
      File.put( 0 );
    for( ; ToGo; ToGo-- )
      File.put( Num >> ( 8 * ( ToGo-1 ) ) & 0xFF );
    return Size;
  } // put_int( ostream&, int, int )
  int put_var( ostream& File, int Number ) {
    int Result = 0;
    unsigned Buffer = 0;
    for( unsigned Num = Number; Num; Num >>= 7 )
      Buffer = ( Buffer << 7 ) | ( Num  & 0x7F );
    for( ; Buffer > 0x7F ; Buffer >>= 7 ) {
      File.put( ( Buffer & 0x7F ) | 0x80 );
      Result++;
    }
    File.put( Buffer );
    Result++;
    return Result;
  } // put_var( ostream&, int )
  
  void InStream::read( unsigned char* Buffer, size_t Length ) {
    for( size_t I = 0; I < Length; ++I ) Buffer[ I ] = get();
    LastCount = Length;
  } // read( unsigned char*, size_t )
  void InStream::skip( size_t Length ) {
    for( size_t I = 0; I < Length; ++I ) get();
    LastCount = Length;
  } // skip( size_t )
  int InStream::get_int( int Length ) {
    int Val = 0;
    for( int I = 0; I < Length; ++I ) {
      Val <<= 8;
      Val |= get();
    }
    LastCount = Length;
    return Val;
  } // get_int( int )
  int InStream::get_var() {
    size_t Length = 0;
    int Val = 0;
    while( true ) {
      Val <<= 7;
      uint8_t Ch = get();
      Val |= Ch & ~0x80;
      ++Length;
      if( !( Ch & 0x80 ) ) break;
    }
    LastCount = Length;
    return Val;
  } // get_var()
  uint8_t InStream::get_status() {
    uint8_t In = peek();
    if( In & 0x80 ) {
      Status = In;
      LastCount = 1;
      get();
    }
    else { // Running status
      if( ( Status & 0xF0 ) == 0xF0 ) throw MIDIException( "Bad status byte and no running status." ); // Sysx & meta can't have running status
      LastCount = 0;
    }
    return Status;
  } // get_status()
  Event* InStream::get_event() {
    size_t Count = 0;
    Event* Ev = Event::get( *this, Count );
    LastCount = Count;
    return Ev;
  } // get_event()

  uint8_t FileInStream::get() { return Stream.get(); }
  uint8_t FileInStream::peek() { return Stream.peek(); }
  
  void Event::print( ostream& Stream ) { Stream << "Пустое сообщение"; }

  void InputDevice::Client::event_received( Event& /*Ev*/ ) {}
  void InputDevice::Client::connected( InputDevice& /*Dev*/ ) {}
  void InputDevice::Client::disconnected( InputDevice& /*Dev*/ ) {}
  void InputDevice::Client::started( InputDevice& /*Dev*/ ) {}
  void InputDevice::Client::stopped( InputDevice& /*Dev*/ ) {}

  InputDevice::~InputDevice()
  {
    while( Clients.size() > 0 )
    {
      Clients.back()->disconnected( *this );
      Clients.pop_back();
    }
  } // ~InputDevice()
  void InputDevice::add_client( Client& Cli )
  { //! \todo Check that it's not there yet.
    if( find( Clients.begin(), Clients.end(), &Cli ) == Clients.end() ) {
      Clients.push_back( &Cli );
      Cli.connected( *this );
    }
  } // add_client( Client& )
  void InputDevice::remove_client( Client& Cli )
  {
    Clients.erase( find( Clients.begin(), Clients.end(), &Cli ) );
    Cli.disconnected( *this );
  } // remove_client( Client* )
  void InputDevice::event_received( Event& Ev ) { for( int I = 0; I < Clients.size(); ++I ) if( Clients[ I ] ) Clients[ I ]->event_received( Ev ); }
 
  Event* Event::get( InStream& Str, size_t& Count ) {
    //! \todo Parse to MIDI events. Create pluggable parsers (the parser itself detects if it can parse this message.
    uint8_t Status = Str.get_status();
    Count = Str.count();
    // cout << "Get event. Status " << hex << int( Status ) << dec << endl;
    switch( Status ) {
    case SysEx:
    case SysExEscape:
      return SysExEvent::get( Str, Count );
    case MetaCode:
      return MetaEvent::get( Str, Count );
    default:
      return ChannelEvent::get( Str, Count );
    }
    cerr << "Unknown event " << hex << Status << " last count " << dec << endl;
    return nullptr;
  } // get_event( InStream&, size_t& )

#if 0
  int InputDevice::parse( const unsigned char* Buffer, size_t Count ) {
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
#endif // 0
} // MuTraMIDI
