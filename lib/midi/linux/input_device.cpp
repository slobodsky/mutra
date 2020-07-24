#include <cstring>
#include "input_device.hpp"
#include "Sequencer.hpp"
#ifdef USE_RTMIDI_BACKEND
#include "../rtmidi_backend.hpp"
#include <sstream>
using std::stringstream;
#endif // USE_RTMIDI_BACKEND
#include "../midi_events.hpp"
using std::ios;
using std::memcpy;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::thread;
using std::mutex;
using std::unique_lock;
using std::lock_guard;
using std::vector;

namespace MuTraMIDI {
  vector<InputDevice::Info> InputDevice::get_available_devices( const string& Backend ) {
    vector<InputDevice::Info> Result;
    if( Backend.empty() || Backend == "alsa" ) Result = ALSASequencer::get_alsa_devices( true );
#ifdef USE_RTMIDI_BACKEND
    if( Backend.empty() || Backend == "rtmidi" ) {
      RtMidiIn In;
      int PortsCount = In.getPortCount();
      for( int I = 0; I < PortsCount; ++I ) {
	stringstream URI;
	URI << "rtmidi://" << I;
	Info Inf( In.getPortName( I ), URI.str() );
	cout << "RTMIDI Input device: " << Inf.name() << " " << Inf.uri() << endl;
	Result.push_back( Inf );
      }
    }
#endif // USE_RTMIDI_BACKEND
    return Result;
  } // get_available_devices( const string& )

  InputDevice* InputDevice::get_instance( const std::string& URI ) {
    if( URI.substr( 0, 7 ) == "alsa://" ) {
      size_t Pos = 0;
      int Client = stoi( URI.substr( 7 ), &Pos );
      int Port = 0;
      if( Pos < URI.length()-7 ) Port = stoi( URI.substr( Pos+7+1 ) );
      return new ALSAInputDevice( Client, Port );
    }
    if( URI.substr( 0, 7 ) == "file://" )
      return new FileInputDevice( URI.substr( 7 ) );
#ifdef USE_RTMIDI_BACKEND
    if( URI.substr( 0, 9 ) == "rtmidi://" )
      return new RtMIDIInputDevice( atoi( URI.substr( 9 ).c_str() ) );
#endif // USE_RTMIDI_BACKEND
    cerr << "Unknown schema in device URI: [" << URI << "], maybe missing backend." << endl;
    return nullptr;
  } // get_instance( const std::string& )
  
  FileInputDevice::FileInputDevice( const string& FileName0 ) : FileName( FileName0 ), Str( FileName, ios::binary | ios::in ), Stream( Str ) {}
  void FileInputDevice::start() {
    unsigned char Status = 0;
    int ToGo = 4*1024*1024; //!< \todo Make this not required for endless streams like midi-devices.
    while( Event* Ev = Stream.get_event() ) {
      Ev->print( cout );
      cout << endl;
    }
  } // start()
#ifdef USE_ALSA_INPUT
  static Event::TimeuS alsa_time_to_us( const snd_seq_real_time_t& Time ) { return Time.tv_sec * Event::TimeuS( 1000000 ) + Time.tv_nsec / 1000; }

  ALSAInputDevice::ALSAInputDevice( int InClient, int InPort ) : mSeq( nullptr ), mClient( -1 ), mPort( -1 ), mInClient( InClient ), mInPort( InPort ), mTime( 0 ) {
    //! \todo Write this. Probably it will be better to share sequencer with ALSASequencer.
    int Err = snd_seq_open( &mSeq, "default", SND_SEQ_OPEN_DUPLEX, 0 );
    if( Err < 0 ) { cerr << "Can't open sequencer for input." << Err << endl; return; }
    Err = snd_seq_set_client_name( mSeq, "MuTra input" );
    if( Err < 0 ) { cerr << "Can't set input client name." << Err << endl; return; }
    mClient = snd_seq_client_id( mSeq );
    if( mClient < 0 ) { cerr << "Can't get input client id." << mClient << endl; return; }
    else cerr << "Client: " << mClient << endl;
    mPort = snd_seq_create_simple_port( mSeq, "MuTra Input port", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION );
    if( mPort < 0 ) { cerr << "Can't create input port." << mPort << endl; return; }
#ifdef MUTRA_DEBUG
    else {
      cout << "ALSA input device readey." << endl;
    }
#endif // MUTRA_DEBUG
  } // конструктор по девайсу
  ALSAInputDevice::~ALSAInputDevice()
  {
    snd_seq_close( mSeq );
    if( mInputThread.joinable() ) mInputThread.join();
  }
  void ALSAInputDevice::start() {
    if( active() ) return; // This means that it's already started.
#ifdef MUTRA_DEBUG
    cout << "Starting ALSA input device." << endl;
#endif // MUTRA_DEBUG
    int Err = snd_seq_connect_from( mSeq, mPort, mInClient, mInPort );
    if( Err < 0 )
      cerr << "Can't connect from the input port." << snd_strerror( Err ) << endl;
    else
      snd_seq_drop_input( mSeq );
    mTime = get_time_us();
#ifdef MUTRA_DEBUG
    cout << "Start the threads for ALSA input device " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << endl;
#endif // MUTRA_DEBUG
    mInputThread = thread( [this]() {
#ifdef MUTRA_DEBUG
			     cout << "Start input thread " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << endl;
#endif // MUTRA_DEBUG
			     snd_seq_event_t* Ev;
			     while( snd_seq_event_input( mSeq, &Ev ) >= 0 ) {
			       if( !active() ) break;
#ifdef MUTRA_DEBUG
			       cout << "Got input event. Force current time && put into the queue." << endl;
#endif // MUTRA_DEBUG
			       //! \todo Better time processing (check if the event has the timestamp).
			       timespec TS;
			       clock_gettime( CLOCK_REALTIME, &TS );
			       Ev->time.time.tv_sec = TS.tv_sec;
			       Ev->time.time.tv_nsec = TS.tv_nsec;
			       put_event( Ev );
#ifdef MUTRA_DEBUG
			       cout << "Done. Notify the queue thread." << endl;
#endif // MUTRA_DEBUG
			       mQueueCondition.notify_all();
#ifdef MUTRA_DEBUG
			       cout << "Wait for another event." << endl;
#endif // MUTRA_DEBUG
			     }
			     mQueueCondition.notify_all();
#ifdef MUTRA_DEBUG
			     cout << "Exit input thread " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << endl;
#endif // MUTRA_DEBUG
			   } );
    mQueueThread = thread( [this]() {
			     while( true ) {
#ifdef MUTRA_DEBUG
			       cout << "Process Queue." << endl;
#endif // MUTRA_DEBUG
			       while( snd_seq_event_t* Ev = get_event() ) {
				 Event* NewEvent = nullptr;
				 Event::TimeuS Time = alsa_time_to_us( Ev->time.time );
				 {
				   lock_guard<mutex> Lock( mQueueMutex );
				   mTime = Time;
				 }
				 switch( Ev->type ) { //! \todo Process it.
				 case SND_SEQ_EVENT_NOTEON:
				   NewEvent = new NoteEvent( Event::NoteOn, Ev->data.note.channel, Ev->data.note.note, Ev->data.note.velocity, Time ); break;
				 case SND_SEQ_EVENT_NOTEOFF:
				   NewEvent = new NoteEvent( Event::NoteOff, Ev->data.note.channel, Ev->data.note.note, Ev->data.note.velocity, Time ); break;
				 case SND_SEQ_EVENT_KEYPRESS:
				   NewEvent = new NoteEvent( Event::AfterTouch, Ev->data.note.channel, Ev->data.note.note, Ev->data.note.velocity, Time ); break;
				 case SND_SEQ_EVENT_CONTROLLER:
				   NewEvent = new ControlChangeEvent( Ev->data.control.channel, Ev->data.control.param, Ev->data.control.value, Time ); break;
				 case SND_SEQ_EVENT_PGMCHANGE:
				   NewEvent = new ProgramChangeEvent( Ev->data.control.channel, Ev->data.control.value, Time ); break;
				 case SND_SEQ_EVENT_CHANPRESS:
				   NewEvent = new ChannelAfterTouchEvent( Ev->data.control.channel, Ev->data.control.value, Time ); break;
				 case SND_SEQ_EVENT_PITCHBEND:
				   NewEvent = new PitchBendEvent( Ev->data.control.channel, Ev->data.control.value, Time ); break;
				 case SND_SEQ_EVENT_TIMESIGN:
				 case SND_SEQ_EVENT_KEYSIGN:
				 case SND_SEQ_EVENT_TEMPO:
				   cout << "Got known meta event: " << int( Ev->type ) << " channel: " << Ev->data.control.channel << " param: " << Ev->data.control.param << " value: "
					<< Ev->data.control.value;
				   break;
				 default:
				   cout << "Got unknown (not MIDI?) event of type " << int( Ev->type ) << endl;
				   break;
				 }
				 snd_seq_free_event( Ev );
				 if( NewEvent ) {
#ifdef MUTRA_DEBUG
				   cout << "New event: ";
				   NewEvent->print( cout );
				   cout << endl;
#endif // MUTRA_DEBUG
				   event_received( *NewEvent );
				   delete NewEvent;
				 }
			       }
			       if( !active() ) {
#ifdef MUTRA_DEBUG
				 cout << "Queue thread complete." << endl;
#endif // MUTRA_DEBUG
				 return;
			       }
			       {
#ifdef MUTRA_DEBUG
				 cout << "Waiting for the queue." << endl;
#endif // MUTRA_DEBUG
				 unique_lock<mutex> Lock( mQueueMutex );
				 mQueueCondition.wait( Lock, [this]{ return !mQueue.empty() || !active(); } );
			       }
			     }
			   } );
#ifdef MUTRA_DEBUG
    cout << "Start the threads for " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << " started." << endl;
#endif // MUTRA_DEBUG
    InputDevice::start();
#ifdef MUTRA_DEBUG
    cout << "Start complete for " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << endl;
#endif // MUTRA_DEBUG
  } // start()
  void ALSAInputDevice::stop() {
    if( !active() ) return;
#ifdef MUTRA_DEBUG
    cout << "Stop ALSA input device: " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << endl;
#endif // MUTRA_DEBUG
    mTime = 0;
    //! \todo Stop the threads.
#ifdef MUTRA_DEBUG
    cout << "Waiting for the input thread for " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << endl;
#endif // MUTRA_DEBUG
    if( mInputThread.joinable() ) {
      snd_seq_event_t Event;
      snd_seq_ev_clear( &Event );
      snd_seq_ev_set_direct( &Event );
      snd_seq_ev_set_fixed( &Event );
      Event.type = SND_SEQ_EVENT_RESET;
      Event.dest.client = mClient;
      Event.dest.port = mPort;
#ifdef MUTRA_DEBUG
      cout << "Send the final message." << endl;
#endif // MUTRA_DEBUG
      int Err = snd_seq_event_output_direct( mSeq, &Event );
      if( Err < 0 )
	cerr << "Can't send the final message. " << snd_strerror( Err ) << endl;
      else {
#ifdef MUTRA_DEBUG
	cout << "The final message is sent. Waiting." << endl;
#endif // MUTRA_DEBUG
	mInputThread.join();
      }
    }
#ifdef MUTRA_DEBUG
    cout << "Waiting for the queue thread for " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << endl;
#endif // MUTRA_DEBUG
    if( mQueueThread.joinable() ) mQueueThread.join();
    int Err = snd_seq_disconnect_from( mSeq, mPort, mInClient, mInPort );
    if( Err < 0 )
      cerr << "Can't disconnect from the input port." << snd_strerror( Err ) << endl;
#ifdef MUTRA_DEBUG
    cout << "Inform subscribers for " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << endl;
#endif // MUTRA_DEBUG
    InputDevice::stop();
#ifdef MUTRA_DEBUG
    cout << "Input stopped " << mInClient << ":" << mInPort << "->" << mClient << ":" << mPort << endl;
#endif // MUTRA_DEBUG
  } // stop();
  snd_seq_event_t* ALSAInputDevice::get_event() {
    lock_guard<mutex> Lock( mQueueMutex );
    if( mQueue.empty() ) return nullptr;
    snd_seq_event_t* Event = mQueue.front();
    mQueue.pop();
    return Event;
  } // put_event( snd_seq_event_t* )
  void ALSAInputDevice::put_event( snd_seq_event_t* Event ) {
    lock_guard<mutex> Lock( mQueueMutex );
    mQueue.push( Event );
  } // put_event( snd_seq_event_t* )
#endif // USE_ALSA_INPUT
} // MuTraMIDI
