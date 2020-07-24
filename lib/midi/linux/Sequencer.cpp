#include "Sequencer.hpp"
#include <sstream>
#include "../midi_events.hpp"
using std::ostream;
using std::flush;
#include <time.h>
#include <sys/time.h>
using std::string;
using std::cerr;
using std::endl;
using std::cout;
using std::vector;
using std::stringstream;

typedef unsigned char BYTE;

namespace MuTraMIDI {
  vector<Sequencer::Info> ALSASequencer::get_alsa_devices( bool Input ) {
    vector<Sequencer::Info> Result;
    int TargetCaps = Input ? SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ : SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
    snd_seq_t* Seq = nullptr;
    if( snd_seq_open( &Seq, "default", SND_SEQ_OPEN_DUPLEX, 0 ) < 0 ) {
      cerr << "Can't open ALSA sequencer in output mode to get available devices." << endl;
      return Result;
    }
    snd_seq_client_info_t* ClientInfo = nullptr;
    if( snd_seq_client_info_malloc( &ClientInfo ) < 0 ) cerr << "Can't allocate memory ALSA client info." << endl;
    else {
      snd_seq_port_info_t* PortInfo = nullptr;
      if( snd_seq_port_info_malloc( &PortInfo ) < 0 ) cerr << "Can't allocate memory ALSA port info." << endl;
      else {
	snd_seq_client_info_set_client( ClientInfo, -1 );
	while( snd_seq_query_next_client( Seq, ClientInfo ) >= 0 ) {
	  int Client = snd_seq_client_info_get_client( ClientInfo );
	  string ClientName = snd_seq_client_info_get_name( ClientInfo );
	  cout << "ALSA Client " << Client << ": " << ClientName << endl;
	  snd_seq_port_info_set_client( PortInfo, Client );
	  snd_seq_port_info_set_port( PortInfo, -1 );
	  while( snd_seq_query_next_port( Seq, PortInfo ) >= 0 ) {
	    cout << "port " << snd_seq_port_info_get_port( PortInfo ) << ": " << snd_seq_port_info_get_name( PortInfo ) << " type: " << std::hex << snd_seq_port_info_get_type( PortInfo )
		 << " caps: " << snd_seq_port_info_get_capability( PortInfo ) << std::dec << endl;
	    if( ( snd_seq_port_info_get_type( PortInfo ) & SND_SEQ_PORT_TYPE_MIDI_GENERIC ) && ( snd_seq_port_info_get_capability( PortInfo ) & TargetCaps ) == TargetCaps )
	    {
	      stringstream URI;
	      URI << "alsa://" << Client;
	      int Port = snd_seq_port_info_get_port( PortInfo );
	      if( Port ) URI << ":" << Port;
	      Sequencer::Info I( ClientName + " : " + snd_seq_port_info_get_name( PortInfo ), URI.str() );
	      cout << "\tPort: " << I.name() << " " << I.uri() << endl;
	      Result.push_back( I );
	    }
	  }
	}
      }
      snd_seq_port_info_free( PortInfo );
    }
    snd_seq_client_info_free( ClientInfo );
    snd_seq_close( Seq );
    return Result;
  } // get_alsa_devices( bool )

  vector<Sequencer::Info> Sequencer::get_available_devices( const string& Backend ) {
    vector<Sequencer::Info> Result;
    if( Backend.empty() || Backend == "alsa" ) {
      Result = ALSASequencer::get_alsa_devices();
    }
#ifdef MUTRA_BACKENDS
    vector<Sequencer::Info> Tmp = MIDIBackend::get_manager().list_devices( MIDIBackend::Output );
    Result.insert( Result.end(), Tmp.begin(), Tmp.end() );
#endif // MUTRA_BACKENDS
    return Result;
  } // get_available_devices( const string& )
  Sequencer* Sequencer::get_instance( const string& URI ) {
#ifdef MUTRA_BACKENDS
    if( Sequencer* Result = MIDIBackend::get_manager().get_sequencer( URI ) ) return Result;
#endif // MUTRA_BACKENDS
    if( URI.empty() ) return new ALSASequencer();
    if( URI.substr( 0, 7 ) == "alsa://" ) {
      int Client = atoi( URI.substr( 7 ).c_str() );
      return new ALSASequencer( Client );
    }
    return nullptr;
  } // get_instance( const string& URI )

  LinuxSequencer::LinuxSequencer( ostream& Device0 ) : Device( Device0 )
  {} // конструктор по девайсу
  LinuxSequencer::~LinuxSequencer()
  {}

  void LinuxSequencer::note_on( int Channel, int Note, int Velocity ) 
  {
    Device << BYTE( Event::NoteOn | (Channel & 0x0F) ) // Note on по каналу
	   << BYTE( (Note & 0x7F) << 8 )                   // Нота
	   << BYTE( (Velocity & 0x7F) << 16 ) << flush;    // Сила
  } // note_on( int, int, int ) 
  void LinuxSequencer::note_off( int Channel, int Note, int Velocity )
  {
    Device << BYTE( Event::NoteOff | (Channel & 0x0F) ) // Note off по каналу
	   << BYTE( (Note & 0x7F) << 8 )                    // Нота
	   << BYTE( (Velocity & 0x7F) << 16 ) << flush;     // Сила
  } // note_off( int, int, int ) 
  void LinuxSequencer::after_touch( int Channel, int Note, int Velocity )
  {
    Device << BYTE( Event::AfterTouch | (Channel & 0x0F) ) // After touch по каналу
	   << BYTE( (Note & 0x7F) << 8 )                       // Нота
	   << BYTE( (Velocity & 0x7F) << 16 ) << flush;        // Сила
  } // after_touch( int, int, int )
  void LinuxSequencer::program_change( int Channel, int NewProgram )
  {
    Device << BYTE( Event::ProgramChange | (Channel & 0x0F) )	// Смена инструмента по каналу
	   << BYTE( (NewProgram & 0x7F) << 8 ) << flush;          // Инструмент
  } // program_change( int, int )
  void LinuxSequencer::control_change( int Channel, int Control, int Value )
  {
    Device << BYTE( Event::ControlChange | (Channel & 0x0F) )	// Колесо по каналу
	   << BYTE( (Control & 0x7F) << 8 )                       // Колесо
	   << BYTE( (Value & 0x7F) << 16 ) << flush;              // Значение
  } // control_change( int, int, int )
  void LinuxSequencer::pitch_bend( int Channel, int Bend )
  {
    Device << BYTE( Event::PitchBend | (Channel & 0x0F) ) // Оттяг по каналу
	   << BYTE( (Bend & 0x7F) << 8 )                      // LSB
	   << BYTE( (Bend & (0x7F<<7) ) << (16-7) ) << flush; // MSB
  } // control_change( int, int, int )

  const int64_t Million64 = 1000000;
  void LinuxSequencer::start()
  {
    MaxDiff = 0;
    Number = 0;
    TotalDiff = 0;
    MaxInDiff = 0;
    TotalInDiff = 0;
    timeval TimeVal;
    gettimeofday( &TimeVal, 0 );
    Start = TimeVal.tv_sec*Million64 + TimeVal.tv_usec;
  } // start()
  void LinuxSequencer::wait_for_usec( int64_t WaitMicroSecs )
  {
#ifdef MUTRA_DEBUG
    cout << "Wait for " << WaitMicroSecs << " µs" << endl;
#endif // MUTRA_DEBUG
    timeval TimeVal;
    gettimeofday( &TimeVal, 0 );
    int64_t Now = TimeVal.tv_sec*Million64 + TimeVal.tv_usec;
    int Diff = static_cast<int>( Now - WaitMicroSecs );
    if( Diff > 0 ) // Только если вызов пришёл позже ожидаемого времени
    {
      if( Diff > MaxInDiff )
	MaxInDiff = Diff;
      TotalInDiff += Diff;
    }
    while( Now < WaitMicroSecs )
    {
      int64_t Period = WaitMicroSecs - Now;
      if( Period > 20000 ) // Иначе будет заметна погрешность nanosleep 
      {
	Period -= 20000;
	timespec Wait;
	Wait.tv_sec = static_cast<time_t>( Period / Million64 );
	Wait.tv_nsec = static_cast<long>( ( Period - Wait.tv_sec * Million64 ) * 1000 );
	nanosleep( &Wait, 0 );
      }
      gettimeofday( &TimeVal, 0 );
      Now = TimeVal.tv_sec*Million64 + TimeVal.tv_usec;
    }
    Diff = static_cast<int>( Now - WaitMicroSecs );
    if( Diff < 0 )
      Diff = -Diff;
    if( Diff > MaxDiff )
      MaxDiff = Diff;
    Number++;
    TotalDiff += Diff;
  } // wait_for_usec( double )

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
} // MuTraMIDI
