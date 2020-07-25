#include <cstring>
#include "alsa_backend.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <sstream>
using std::stringstream;
#include "../midi_events.hpp"
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
using std::ostream; // LinuxSequencer legacy.

namespace MuTraMIDI {
  class ALSASequencer : public LinuxSequencer
  {
  public:
    static std::vector<Info> get_alsa_devices( bool Input = false );
    ALSASequencer( int OutClient0 = 128, int OutPort0 = 0, std::ostream& Device0 = std::cout );
    ~ALSASequencer();
    //! \todo Implement all possible events.
    void note_on( int Channel, int Note, int Velocity );
    void note_off( int Channel, int Note, int Velocity );
    void after_touch( int Channel, int Note, int Velocity );
    void program_change( int Channel, int NewProgram );
    void control_change( int Channel, int Control, int Value );
    void pitch_bend( int Channel, int Bend );

    void start();
  protected:
    snd_seq_t* Seq;
    int Client;
    int Port;
    int OutClient;
    int OutPort;
  }; // ALSASequencer

  ALSASequencer::ALSASequencer( int OutClient0, int OutPort0, ostream& Device0 ) : LinuxSequencer( Device0 ), Seq( nullptr ), Client( 0 ), Port( 0 ), OutClient( OutClient0 ), OutPort( OutPort0 )
  {
    int Err = snd_seq_open( &Seq, "default", SND_SEQ_OPEN_OUTPUT, 0 );
    if( Err < 0 ) cerr << "Can't open output sequencer." << Err << endl;
    Err = snd_seq_set_client_name( Seq, "Wholeman" );
    if( Err < 0 ) cerr << "Can't set output client name." << Err << endl;
    Client = snd_seq_client_id( Seq );
    if( Client < 0 ) cerr << "Can't get output client id." << Err << endl;
    else cerr << "Client: " << Client << endl;
    Port = snd_seq_create_simple_port( Seq, "MuTra Out", SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ, SND_SEQ_PORT_TYPE_APPLICATION );
    if( Port < 0 ) cerr << "Can't create output port." << Port << endl;
    //! \todo Use connection below & send events to the subscribed clients instead of the fixed destination:
    snd_seq_connect_to( Seq, Port, OutClient, OutPort );
  } // конструктор по девайсу
  ALSASequencer::~ALSASequencer()
  {
    snd_seq_close( Seq );
  }

  void ALSASequencer::note_on( int Channel, int Note, int Velocity ) 
  { //! \todo Consider use a single method for preparing event structure.
    snd_seq_event_t Event;
    snd_seq_ev_clear( &Event );
    snd_seq_ev_set_direct( &Event );
    snd_seq_ev_set_fixed( &Event );

    Event.type = SND_SEQ_EVENT_NOTEON;
    Event.data.note.channel = Channel;
    Event.data.note.note = Note;
    Event.data.note.velocity = Velocity;
    Event.source.client = Client;
    Event.source.port = Port;
    Event.dest.client = OutClient;
    Event.dest.port = OutPort;
    int Err = snd_seq_event_output_direct( Seq, &Event );
    if( Err < 0 ) cerr << "Can't send note on from " << Client << ':' << Port << " to " << OutClient << ':' << OutPort << ": "  << snd_strerror( Err ) << endl;
    // else cerr << "Note on sent." << endl;
  } // note_on( int, int, int ) 
  void ALSASequencer::note_off( int Channel, int Note, int Velocity )
  {
    snd_seq_event_t Event;
    snd_seq_ev_clear( &Event );
    snd_seq_ev_set_direct( &Event );
    snd_seq_ev_set_fixed( &Event );

    Event.type = SND_SEQ_EVENT_NOTEOFF;
    Event.data.note.channel = Channel;
    Event.data.note.note = Note;
    Event.data.note.velocity = Velocity;
    Event.source.client = Client;
    Event.source.port = Port;
    Event.dest.client = OutClient;
    Event.dest.port = OutPort;
    int Err = snd_seq_event_output_direct( Seq, &Event );
    if( Err < 0 ) cerr << "Can't send note off." << Err << endl;
    // else cerr << "Note off sent." << endl;
  } // note_off( int, int, int ) 
  void ALSASequencer::after_touch( int Channel, int Note, int Velocity )
  {
    snd_seq_event_t Event;
    snd_seq_ev_clear( &Event );
    snd_seq_ev_set_direct( &Event );
    snd_seq_ev_set_fixed( &Event );

    Event.type = SND_SEQ_EVENT_KEYPRESS;
    Event.data.note.channel = Channel;
    Event.data.note.note = Note;
    Event.data.note.velocity = Velocity;
    Event.source.client = Client;
    Event.source.port = Port;
    Event.dest.client = OutClient;
    Event.dest.port = OutPort;
    int Err = snd_seq_event_output_direct( Seq, &Event );
    if( Err < 0 ) cerr << "Can't send aftertouch." << Err << endl;
    // else cerr << "Aftertouch sent." << endl;
  } // after_touch( int, int, int )
  void ALSASequencer::program_change( int Channel, int NewProgram )
  {
    snd_seq_event_t Event;
    snd_seq_ev_clear( &Event );
    snd_seq_ev_set_direct( &Event );
    snd_seq_ev_set_fixed( &Event );

    Event.type = SND_SEQ_EVENT_PGMCHANGE;
    Event.data.control.channel = Channel;
    Event.data.control.value = NewProgram;
    Event.source.client = Client;
    Event.source.port = Port;
    Event.dest.client = OutClient;
    Event.dest.port = OutPort;
    int Err = snd_seq_event_output_direct( Seq, &Event );
    if( Err < 0 ) cerr << "Can't send program change." << Err << " " << EAGAIN << endl;
    // else cerr << "Program changed." << endl;
  } // program_change( int, int )
  void ALSASequencer::control_change( int Channel, int Control, int Value )
  {
    snd_seq_event_t Event;
    snd_seq_ev_clear( &Event );
    snd_seq_ev_set_direct( &Event );
    snd_seq_ev_set_fixed( &Event );

    Event.type = SND_SEQ_EVENT_CONTROLLER;
    Event.data.control.channel = Channel;
    Event.data.control.param = Control;
    Event.data.control.value = Value;
    Event.source.client = Client;
    Event.source.port = Port;
    Event.dest.client = OutClient;
    Event.dest.port = OutPort;
    int Err = snd_seq_event_output_direct( Seq, &Event );
    if( Err < 0 ) cerr << "Can't send controller event." << Err << " " << EAGAIN << endl;
    // else cerr << "Controller sent." << endl;
  } // control_change( int, int, int )
  void ALSASequencer::pitch_bend( int Channel, int Bend )
  {
    snd_seq_event_t Event;
    snd_seq_ev_clear( &Event );
    snd_seq_ev_set_direct( &Event );
    snd_seq_ev_set_fixed( &Event );

    Event.type = SND_SEQ_EVENT_PITCHBEND;
    Event.data.control.channel = Channel;
    Event.data.control.value = Bend;
    Event.source.client = Client;
    Event.source.port = Port;
    Event.dest.client = OutClient;
    Event.dest.port = OutPort;
    int Err = snd_seq_event_output_direct( Seq, &Event );
    if( Err < 0 ) cerr << "Can't send pitch band." << Err << " " << EAGAIN << endl;
    // else cerr << "Pitch bended." << endl;
  } // control_change( int, int, int )

  void ALSASequencer::start()
  {
    LinuxSequencer::start();
  } // start()

  static Event::TimeuS alsa_time_to_us( const snd_seq_real_time_t& Time ) { return Time.tv_sec * Event::TimeuS( 1000000 ) + Time.tv_nsec / 1000; }

  class ALSAInputDevice : public InputDevice {
  public:
    ALSAInputDevice( int InClient = 24, int InPort = 0 );
    ~ALSAInputDevice();
    void start();
    void stop();
    bool active() const { return mTime != 0; }
    Event::TimeuS time() const { return mTime; }
  private:
    snd_seq_event_t* get_event();
    void put_event( snd_seq_event_t* Event );
    snd_seq_t* mSeq; //! \todo Reuse the backend's sequencer.
    int mClient;
    int mPort;
    int mInClient;
    int mInPort;
    Event::TimeuS mTime; // Реальное время старта устройства. Все сообщения считаются от него. Также это признак остановки, поэтому для сообщений, обработанных после остановки, оно будет неверным.
    std::thread mInputThread;
    std::thread mQueueThread;
    std::queue<snd_seq_event_t*> mQueue;
    std::mutex mQueueMutex;
    std::condition_variable mQueueCondition;
  }; // ALSAInputDevice

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
  ALSAInputDevice::~ALSAInputDevice() {
    snd_seq_close( mSeq );
    if( mInputThread.joinable() ) mInputThread.join();
  }
  void ALSAInputDevice::start() {
    if( active() ) return; // This means that it's already started.
    mTime = get_time_us();
#ifdef MUTRA_DEBUG
    cout << "Starting ALSA input device." << endl;
#endif // MUTRA_DEBUG
    int Err = snd_seq_connect_from( mSeq, mPort, mInClient, mInPort );
    if( Err < 0 )
      cerr << "Can't connect from the input port." << snd_strerror( Err ) << endl;
    else
      snd_seq_drop_input( mSeq );
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
				 Event::TimeuS Time = alsa_time_to_us( Ev->time.time ) - mTime;
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

  ALSABackend::ALSABackend() : MIDIBackend( "alsa", "ALSA" ) {
    if( snd_seq_open( &mSeq, "default", SND_SEQ_OPEN_DUPLEX, 0 ) < 0 )
      cerr << "Can't open ALSA sequencer in output mode to get available devices." << endl;
  } // ALSABackend()

  ALSABackend::~ALSABackend() { snd_seq_close( mSeq ); }

  vector<Sequencer::Info> ALSABackend::list_devices( DeviceType Filter ) {
    vector<Sequencer::Info> Result;
    if( !mSeq ) return Result;
    int InCaps = SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
    int OutCaps = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
    snd_seq_client_info_t* ClientInfo = nullptr;
    if( snd_seq_client_info_malloc( &ClientInfo ) < 0 ) cerr << "Can't allocate memory ALSA client info." << endl;
    else {
      snd_seq_port_info_t* PortInfo = nullptr;
      if( snd_seq_port_info_malloc( &PortInfo ) < 0 ) cerr << "Can't allocate memory ALSA port info." << endl;
      else {
	snd_seq_client_info_set_client( ClientInfo, -1 );
	while( snd_seq_query_next_client( mSeq, ClientInfo ) >= 0 ) {
	  int Client = snd_seq_client_info_get_client( ClientInfo );
	  string ClientName = snd_seq_client_info_get_name( ClientInfo );
#ifdef MUTRA_DEBUG
	  cout << "ALSA Client " << Client << ": " << ClientName << endl;
#endif // MUTRA_DEBUG
	  snd_seq_port_info_set_client( PortInfo, Client );
	  snd_seq_port_info_set_port( PortInfo, -1 );
	  while( snd_seq_query_next_port( mSeq, PortInfo ) >= 0 ) {
	    int Caps = snd_seq_port_info_get_capability( PortInfo );
#ifdef MUTRA_DEBUG
	    cout << "port " << snd_seq_port_info_get_port( PortInfo ) << ": " << snd_seq_port_info_get_name( PortInfo ) << " type: " << std::hex << snd_seq_port_info_get_type( PortInfo )
		 << " caps: " << Caps << std::dec << endl;
#endif // MUTRA_DEBUG
	    if( snd_seq_port_info_get_type( PortInfo ) & SND_SEQ_PORT_TYPE_MIDI_GENERIC )
	      if( ( Filter == Both && ( ( Caps & ( InCaps | OutCaps ) ) == ( InCaps | OutCaps ) ) )
		  || ( ( Filter & Output ) && ( ( Caps & OutCaps ) == OutCaps ) ) || ( ( Filter & Input ) && ( ( Caps & InCaps ) == InCaps ) ) ) {
		stringstream URI;
		URI << schema() << "://" << Client;
		int Port = snd_seq_port_info_get_port( PortInfo );
		if( Port ) URI << ":" << Port;
		Sequencer::Info I( ClientName + " : " + snd_seq_port_info_get_name( PortInfo ), URI.str() );
#ifdef MUTRA_DEBUG
		cout << "\tPort: " << I.name() << " " << I.uri() << endl;
#endif // MUTRA_DEBUG
		Result.push_back( I );
	      }
	  }
	}
      }
      snd_seq_port_info_free( PortInfo );
    }
    snd_seq_client_info_free( ClientInfo );
    return Result;
  } // list_devices( DeviceType )

  Sequencer* ALSABackend::get_sequencer( const std::string& URI ) {
    if( URI.empty() ) return new ALSASequencer();
    if( URI.substr( 0, 7 ) == "alsa://" ) { //!< \todo Break down the URL && compare the schema to the schema() method. (Implement some URI parser)
      size_t Pos = 0;
      int Client = stoi( URI.substr( 7 ), &Pos );
      int Port = 0;
      if( Pos < URI.length()-7 ) Port = stoi( URI.substr( Pos+7+1 ) );
      return new ALSASequencer( Client, Port );
    }
    return nullptr;
  } // get_sequencer( const std::string& )

  InputDevice* ALSABackend::get_input( const std::string& URI ) {
    if( URI.empty() ) return new ALSAInputDevice();
    if( URI.substr( 0, 7 ) == "alsa://" ) {
      size_t Pos = 0;
      int Client = stoi( URI.substr( 7 ), &Pos );
      int Port = 0;
      if( Pos < URI.length()-7 ) Port = stoi( URI.substr( Pos+7+1 ) );
      return new ALSAInputDevice( Client, Port );
    }
    return nullptr;
  } // get_input( const std::string& )
} // MuTraMIDI
