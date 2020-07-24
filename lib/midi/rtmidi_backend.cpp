#include "rtmidi_backend.hpp"
using std::vector;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

namespace MuTraMIDI {
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
