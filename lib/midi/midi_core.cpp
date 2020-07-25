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
using std::vector;
using std::endl;
using std::cout;
using std::cerr;
#include <sys/time.h>
#include <unistd.h>
#ifdef USE_ALSA_BACKEND
#include "linux/Sequencer.hpp"
#include "linux/input_device.hpp"
#endif // USE_ALSA_BACKEND
#ifdef USE_RTMIDI_BACKEND
#include "rtmidi_backend.hpp"
#endif // USE_RTMIDI_BACKEND

namespace MuTraMIDI {
  uint64_t get_time_us() {
    timeval TV;
    gettimeofday( &TV, nullptr );
    return TV.tv_sec * uint64_t( 1000000 ) + TV.tv_usec;
  } // get_time_us()

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
      Val |= Ch & 0x7F;
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
  int put_var( ostream& File, unsigned Number ) {
    int Count = 0;
    int Bits = 0;
    for( unsigned Num = Number; Num; Num >>= 7 ) Bits += 7;
    while( Bits > 7 ) {
      Bits -= 7;
      File.put( Number >> Bits & 0x7F | 0x80 );
      ++Count;
    }
    File.put( Number & 0x7F );
    ++Count;
    return Count;
  } // put_var( ostream&, int )
  
  void InStream::read( unsigned char* Buffer, size_t Length ) {
    for( size_t I = 0; I < Length; ++I ) Buffer[ I ] = get();
    LastCount = Length;
  } // read( unsigned char*, size_t )
  void InStream::skip( size_t Length ) {
    for( size_t I = 0; I < Length && !eof(); ++I ) get();
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
    if( eof() ) return nullptr;
    Event* Ev = Event::get( *this, Count );
    LastCount = Count;
    return Ev;
  } // get_event()

  uint8_t FileInStream::get() { return Stream.get(); }
  uint8_t FileInStream::peek() { return Stream.peek(); }
  bool FileInStream::eof() const { return Stream.eof(); }
  
  void Event::print( ostream& Stream ) const { Stream << "Пустое сообщение"; }

  void InputDevice::Client::event_received( const Event& /*Ev*/ ) {}
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
  {
    if( find( Clients.begin(), Clients.end(), &Cli ) == Clients.end() ) {
      Clients.push_back( &Cli );
      Cli.connected( *this );
    }
  } // add_client( Client& )
  void InputDevice::remove_client( Client& Cli )
  {
    auto It = find( Clients.begin(), Clients.end(), &Cli );
    if( It == Clients.end() ) return;
    Clients.erase( It );
    Cli.disconnected( *this );
  } // remove_client( Client* )
  void InputDevice::start() { for( int I = 0; I < Clients.size(); ++I ) if( Clients[ I ] ) Clients[ I ]->started( *this ); }
  void InputDevice::stop() { for( int I = 0; I < Clients.size(); ++I ) if( Clients[ I ] ) Clients[ I ]->stopped( *this ); }
  void InputDevice::event_received( const Event& Ev ) { for( int I = 0; I < Clients.size(); ++I ) if( Clients[ I ] ) Clients[ I ]->event_received( Ev ); }
 
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
    cerr << "Unknown event " << hex << Status << " last count " << dec << Count << endl;
    return nullptr;
  } // get_event( InStream&, size_t& )

  MIDIBackend::Manager::Manager() {
    //! \todo Create all known backends here & figure out a way to do it in them (e.g. loadable plugins).
#ifdef USE_ALSA_BACKEND
    mBackends.push_back( new ALSABackend );
#endif // USE_ALSA_BACKEND
#ifdef USE_RTMIDI_BACKEND
    mBackends.push_back( new RTMIDIBackend );
#endif // USE_RTMIDI_BACKEND
  } // Manager()
  MIDIBackend::Manager::~Manager() { while( !mBackends.empty() ) delete mBackends.back(); } // ~Manager()
  vector<Sequencer::Info> MIDIBackend::Manager::list_devices( DeviceType Filter ) const {
    vector<Sequencer::Info> Result;
    for( MIDIBackend* Backend : mBackends ) append( Result, Backend->list_devices( Filter ) );
    return Result;
  } // list_devices( DeviceType 
  MIDIBackend* MIDIBackend::Manager::get_backend( const std::string& Schema ) const {
    if( Schema.empty() && !mBackends.empty() ) return mBackends.front();
    for( MIDIBackend* Back : mBackends ) if( Back && Back->schema() == Schema ) return Back;
    return nullptr;
  } // get_backend( const std::string& )

  Sequencer* MIDIBackend::Manager::get_sequencer( const std::string& URI ) const {
    for( MIDIBackend* Backend : mBackends ) //! \todo If URI is empty return some default device. Don't let the first backend do it.
      if( Sequencer* Seq = Backend->get_sequencer( URI ) )
	return Seq;
    return nullptr;
  } // get_sequencer( const std::string& )
  InputDevice* MIDIBackend::Manager::get_input( const std::string& URI ) const {
    for( MIDIBackend* Backend : mBackends ) //! \todo If URI is empty return some default device. Don't let the first backend do it.
      if( InputDevice* Seq = Backend->get_input( URI ) )
	return Seq;
    cerr << "Unknown schema in device URI: [" << URI << "], maybe missing backend." << endl;
    return nullptr;
  } // get_input( const std::string& )

  void MIDIBackend::Manager::add_backend( MIDIBackend* NewBackend ) {
    if( find( mBackends.begin(), mBackends.end(), NewBackend ) == mBackends.end() )
      mBackends.push_back( NewBackend );
  } // add_backend( MIDIBackend* )
  void MIDIBackend::Manager::remove_backend( MIDIBackend* OldBackend ) {
    if( mBackends.empty() ) return;
    if( mBackends.back() == OldBackend ) mBackends.pop_back();
    else {
      auto It = find( mBackends.begin(), mBackends.end(), OldBackend );
      if( It != mBackends.end() ) mBackends.erase( It );
    }
  } // remove_backend( MIDIBackend* )
  MIDIBackend::Manager MIDIBackend::sManager;

  MIDIBackend::MIDIBackend( const std::string& Schema, const std::string& Name ) : mSchema( Schema ), mName( Name ) { if( mName.empty() ) mName = mSchema; }
  MIDIBackend::~MIDIBackend() { get_manager().remove_backend( this ); }
} // MuTraMIDI
