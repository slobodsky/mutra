#include "Sequencer.hpp"
using std::ostream;
using std::flush;
#include <time.h>
#include <sys/time.h>
using std::cerr;
using std::endl;
using std::cout;

typedef unsigned char BYTE;

Sequencer* Sequencer::create_instance() { return new ALSASequencer( cout ); } // create_default()

LinuxSequencer::LinuxSequencer( ostream& Device0 ) : Device( Device0 )
{} // конструктор по девайсу
LinuxSequencer::~LinuxSequencer()
{}

void LinuxSequencer::note_on( int Channel, int Note, int Velocity ) 
{
  Device << BYTE( MIDIEvent::NoteOn | (Channel & 0x0F) ) // Note on по каналу
	 << BYTE( (Note & 0x7F) << 8 )                   // Нота
	 << BYTE( (Velocity & 0x7F) << 16 ) << flush;    // Сила
} // note_on( int, int, int ) 
void LinuxSequencer::note_off( int Channel, int Note, int Velocity )
{
  Device << BYTE( MIDIEvent::NoteOff | (Channel & 0x0F) ) // Note off по каналу
	 << BYTE( (Note & 0x7F) << 8 )                    // Нота
	 << BYTE( (Velocity & 0x7F) << 16 ) << flush;     // Сила
} // note_off( int, int, int ) 
void LinuxSequencer::after_touch( int Channel, int Note, int Velocity )
{
  Device << BYTE( MIDIEvent::AfterTouch | (Channel & 0x0F) ) // After touch по каналу
	 << BYTE( (Note & 0x7F) << 8 )                       // Нота
	 << BYTE( (Velocity & 0x7F) << 16 ) << flush;        // Сила
} // after_touch( int, int, int )
void LinuxSequencer::program_change( int Channel, int NewProgram )
{
  Device << BYTE( MIDIEvent::ProgramChange | (Channel & 0x0F) )	// Смена инструмента по каналу
	 << BYTE( (NewProgram & 0x7F) << 8 ) << flush;          // Инструмент
} // program_change( int, int )
void LinuxSequencer::control_change( int Channel, int Control, int Value )
{
  Device << BYTE( MIDIEvent::ControlChange | (Channel & 0x0F) )	// Колесо по каналу
	 << BYTE( (Control & 0x7F) << 8 )                       // Колесо
	 << BYTE( (Value & 0x7F) << 16 ) << flush;              // Значение
} // control_change( int, int, int )
void LinuxSequencer::pitch_bend( int Channel, int Bend )
{
  Device << BYTE( MIDIEvent::PitchBend | (Channel & 0x0F) ) // Оттяг по каналу
	 << BYTE( (Bend & 0x7F) << 8 )                      // LSB
	 << BYTE( (Bend & (0x7F<<7) ) << (16-7) ) << flush; // MSB
} // control_change( int, int, int )

void LinuxSequencer::start()
{
  MaxDiff = 0;
  Number = 0;
  TotalDiff = 0;
  MaxInDiff = 0;
  TotalInDiff = 0;
  timeval TimeVal;
  gettimeofday( &TimeVal, 0 );
  Start = TimeVal.tv_sec*1000000.0 + TimeVal.tv_usec;
} // start()
void LinuxSequencer::wait_for_usec( double WaitMicroSecs )
{
  timeval TimeVal;
  gettimeofday( &TimeVal, 0 );
  double Now = TimeVal.tv_sec*1000000.0 + TimeVal.tv_usec;
  int Diff = static_cast<int>( Now - WaitMicroSecs );
  if( Diff > 0 ) // Только если вызов пришёл позже ожидаемого времени
  {
    if( Diff > MaxInDiff )
      MaxInDiff = Diff;
    TotalInDiff += Diff;
  }
  while( Now < WaitMicroSecs )
  {
    double Period = WaitMicroSecs - Now;
    if( Period > 20000 ) // Иначе будет заметна погрешность nanosleep 
    {
      Period -= 20000;
      timespec Wait;
      Wait.tv_sec = static_cast<time_t>( Period / 1000000 );
      Wait.tv_nsec = static_cast<long>( ( Period - Wait.tv_sec * 1000000.0 ) * 1000 );
      nanosleep( &Wait, 0 );
    }
    gettimeofday( &TimeVal, 0 );
    Now = TimeVal.tv_sec*1000000.0 + TimeVal.tv_usec;
  }
  Diff = static_cast<int>( Now - WaitMicroSecs );
  if( Diff < 0 )
    Diff = -Diff;
  if( Diff > MaxDiff )
    MaxDiff = Diff;
  Number++;
  TotalDiff += Diff;
} // wait_for_usec( double )

ALSASequencer::ALSASequencer( ostream& Device0 = std::cout ) : LinuxSequencer( Device0 ), OutClient( 128 ), OutPort( 0 )
{
  int Err = snd_seq_open( &Seq, "default", SND_SEQ_OPEN_DUPLEX, 0 );
  if( Err < 0 ) cerr << "Can't open sequencer." << Err << endl;
  Err = snd_seq_set_client_name( Seq, "Wholeman" );
  if( Err < 0 ) cerr << "Can't client name." << Err << endl;
  Client = snd_seq_client_id( Seq );
  if( Client < 0 ) cerr << "Can't get client id." << Err << endl;
  else cerr << "Client: " << Client << endl;

  Err = snd_seq_create_simple_port( Seq, "WholemanOut", 0, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION );
  if( Err < 0 ) cerr << "Can't create port." << Err << endl;
  snd_seq_connect_to( Seq, 0, 128, 0 );
} // конструктор по девайсу
ALSASequencer::~ALSASequencer()
{
  snd_seq_close( Seq );
}

void ALSASequencer::note_on( int Channel, int Note, int Velocity ) 
{
  snd_seq_event_t Event;
  snd_seq_ev_clear( &Event );
  snd_seq_ev_set_direct( &Event );
  snd_seq_ev_set_fixed( &Event );

  Event.type = SND_SEQ_EVENT_NOTEON;
  Event.data.note.channel = Channel;
  Event.data.note.note = Note;
  Event.data.note.velocity = Velocity;
  Event.source.client = Client;
  Event.source.port = 0;
  Event.dest.client = OutClient;
  Event.dest.port = OutPort;
  int Err = snd_seq_event_output( Seq, &Event );
  if( Err < 0 ) cerr << "Can't send note on." << Err << endl;
  else cerr << "Note on sent." << endl;
  snd_seq_drain_output( Seq );
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
  Event.source.port = 0;
  Event.dest.client = OutClient;
  Event.dest.port = OutPort;
  int Err = snd_seq_event_output( Seq, &Event );
  if( Err < 0 ) cerr << "Can't send note off." << Err << endl;
  else cerr << "Note off sent." << endl;
  snd_seq_drain_output( Seq );
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
  Event.source.port = 0;
  Event.dest.client = OutClient;
  Event.dest.port = OutPort;
  int Err = snd_seq_event_output( Seq, &Event );
  if( Err < 0 ) cerr << "Can't send aftertouch." << Err << endl;
  else cerr << "Aftertouch sent." << endl;
  snd_seq_drain_output( Seq );
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
  Event.source.port = 0;
  Event.dest.client = OutClient;
  Event.dest.port = OutPort;
  int Err = snd_seq_event_output( Seq, &Event );
  if( Err < 0 ) cerr << "Can't send program change." << Err << " " << EAGAIN << endl;
  else cerr << "Program changed." << endl;
  snd_seq_drain_output( Seq );
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
  Event.source.port = 0;
  Event.dest.client = OutClient;
  Event.dest.port = OutPort;
  int Err = snd_seq_event_output( Seq, &Event );
  if( Err < 0 ) cerr << "Can't send controller event." << Err << " " << EAGAIN << endl;
  else cerr << "Controller sent." << endl;
  snd_seq_drain_output( Seq );
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
  Event.source.port = 0;
  Event.dest.client = OutClient;
  Event.dest.port = OutPort;
  int Err = snd_seq_event_output( Seq, &Event );
  if( Err < 0 ) cerr << "Can't send pitch band." << Err << " " << EAGAIN << endl;
  else cerr << "Pitch bended." << endl;
  snd_seq_drain_output( Seq );
} // control_change( int, int, int )

void ALSASequencer::start()
{
  LinuxSequencer::start();
} // start()
