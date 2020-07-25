#include "rtmidi_backend.hpp"
#include <sstream>
using std::vector;
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::stringstream;

namespace MuTraMIDI {
  RTMIDIBackend::RTMIDIBackend() : MIDIBackend( "rtmidi", "RTMIDI" ) {} // ALSABackend()
  RTMIDIBackend::~RTMIDIBackend() {}
  vector<Sequencer::Info> RTMIDIBackend::list_devices( DeviceType Filter ) {
    vector<Sequencer::Info> Result;
    if( Filter == Both ) return Result;
    if( Filter & Input ) {
      RtMidiIn In;
      int PortsCount = In.getPortCount();
      for( int I = 0; I < PortsCount; ++I ) {
	stringstream URI;
	URI << "rtmidi://" << I;
	Sequencer::Info Inf( In.getPortName( I ), URI.str() );
	cout << "RTMIDI Input device: " << Inf.name() << " " << Inf.uri() << endl;
	Result.push_back( Inf );
      }
    }
    return Result;
  } // list_devices( DeviceType )
  InputDevice* RTMIDIBackend::get_input( const std::string& URI ) {
    if( URI.substr( 0, 9 ) == "rtmidi://" )
      return new RtMIDIInputDevice( stoi( URI.substr( 9 ) ) ); //!< \todo Use try/catch to catch malformed URIs.
    return nullptr;
  } // get_input( const std::string& )

  void RtMIDIInputDevice::message_received( double Delta, vector<unsigned char>* Msg, void* This ) {
    if( This ) static_cast<RtMIDIInputDevice*>( This )->message_received( Delta, Msg );
  } // message_received( double, std::vector<unsigned char>, void* )

  RtMIDIInputDevice::RtMIDIInputDevice( int Index0 ) : In( nullptr ), Index( Index0 ) {}
  RtMIDIInputDevice::~RtMIDIInputDevice() { stop(); }
  void RtMIDIInputDevice::start() {
    stop();
    In = new RtMidiIn();
    Time = 0; //!< \todo Consider using just the real time (but notice that the first event will come with delta time set to zero anyway).
    In->openPort( Index );
    In->setCallback( &message_received, this );
    cout << "RtMIDI Input port " << Index << " ready." << endl;
  } // start()
  void RtMIDIInputDevice::stop() {
    if( In ) {
      In->cancelCallback();
      delete In;
      In = nullptr;
    }
  } // stop()

  class VectorInStream : public InStream {
  public:
    VectorInStream( const vector<unsigned char>& Msg ) : Data( Msg ), Pos( 0 ) {}
    uint8_t get() { return Data[ Pos++ ]; }
    //! Посмотреть первый байт в потоке, не убирая его оттуда (т.е. следующая операция peek/get вернёт тот же байт).
    uint8_t peek() { return Data[ Pos ]; }
    bool eof() const { return Pos >= Data.size(); }
  private:
    const vector<unsigned char>& Data;
    size_t Pos;
  }; // VectorInStream
  
  void RtMIDIInputDevice::message_received( double Delta, vector<unsigned char>* Msg ) {
    Time += Delta;
    if( Msg ) {
#ifdef MUTRA_DEBUG
      cout << "RtMIDIInput got " << Msg->size() << " bytes @ " << Time << " delta: " << Delta << endl;
#endif
      VectorInStream InSt( *Msg );
      while( Event* Ev = InSt.get_event() ) {
#ifdef MUTRA_DEBUG
	cout << "Got event" << endl;
#endif
	Ev->time( Time * 1000000 );
	event_received( *Ev );
	delete Ev;
      }
    }
  } // message_received( double, std::vector<unsigned char>, void* )
} // MuTraMIDI
