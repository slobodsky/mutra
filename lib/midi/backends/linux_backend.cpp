#include "linux_backend.hpp"
#include <sstream>
#include <fstream>
#include "../midi_events.hpp"
using std::ostream;
using std::ios;
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

  class FileInputDevice : public InputDevice
  {
  public:
    FileInputDevice( const std::string& FileName0 );
    void start();
  private:
    std::string FileName;
    std::ifstream Str;
    FileInStream Stream;
  }; // FileInputDevice

  FileInputDevice::FileInputDevice( const string& FileName0 ) : FileName( FileName0 ), Str( FileName, ios::binary | ios::in ), Stream( Str ) {}
  void FileInputDevice::start() {
    while( Event* Ev = Stream.get_event() ) { // Do all IO in the separate thread, like ALSA input. 
      Ev->print( cout );
      cout << endl;
      event_received( *Ev );
      delete Ev;
    }
  } // start()
  
  Sequencer* FileBackend::get_sequencer( const std::string& URI ) {
#if 0
    if( URI.substr( 0, 7 ) == "file://" )  return new LinuxSequencer( URI.substr( 7 ) ); //!< \todo Check if the file is readable by the current user.
#endif
    return nullptr;
  } // get_sequencer( const std::string& )

  InputDevice* FileBackend::get_input( const std::string& URI ) {
    if( URI.substr( 0, 7 ) == "file://" )  return new FileInputDevice( URI.substr( 7 ) ); //!< \todo Check if the file is readable by the current user.
    return nullptr;
  } // get_input( const std::string& )
} // MuTraMIDI
