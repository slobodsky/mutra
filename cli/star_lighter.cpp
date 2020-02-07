#include "star_lighter.hpp"
#include <algorithm>
#include <termios.h>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>
using std::memset;
using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::sort;
using std::stringstream;

string note_name( int Note ) {
  static const string Notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
  static const char* Octavas = "0123456789a";
  return Notes[ ( Note & 0x7F ) % 12 ] + Octavas[ ( Note & 0x7F ) / 12 ];
} // note_name( int )

void MultiSequencer::add( MuTraMIDI::Sequencer* Seq ) { Clients.push_back( Seq ); }
void MultiSequencer::remove( MuTraMIDI::Sequencer* Seq ) {} //!< \todo Implement this!
void MultiSequencer::start() { for( MuTraMIDI::Sequencer* Client : Clients ) Client->start(); }
void MultiSequencer::name( string Text ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->name( Text ); }
void MultiSequencer::copyright( string Text ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->copyright( Text ); }
void MultiSequencer::text( string Text ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->text( Text ); }
void MultiSequencer::lyric( string Text ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->lyric( Text ); }
void MultiSequencer::instrument( string Text ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->instrument( Text ); }
void MultiSequencer::marker( string Text ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->marker( Text ); }
void MultiSequencer::cue( string Text ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->cue( Text ); }

void MultiSequencer::meter( int Numerator, int Denominator ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->meter( Numerator, Denominator ); }
void MultiSequencer::note_on( int Channel, int Note, int Velocity ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->note_on( Channel, Note, Velocity ); }
void MultiSequencer::note_off( int Channel, int Note, int Velocity ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->note_off( Channel, Note, Velocity ); }
void MultiSequencer::after_touch( int Channel, int Note, int Velocity ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->after_touch( Channel, Note, Velocity ); }
void MultiSequencer::program_change( int Channel, int NewProgram ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->program_change( Channel, NewProgram ); }
void MultiSequencer::control_change( int Channel, int Control, int Value ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->control_change( Channel, Control, Value ); }
void MultiSequencer::pitch_bend( int Channel, int Bend ) { for( MuTraMIDI::Sequencer* Client : Clients ) Client->pitch_bend( Channel, Bend ); }
void MultiSequencer::division( unsigned MIDIClockForQuarter ) {
  Sequencer::division( MIDIClockForQuarter );
  if( Clients.size() > 0 ) Clients.front()->division( MIDIClockForQuarter );
} // division( MIDIClockForQuarter )
void MultiSequencer::tempo( unsigned uSecForQuarter ) {
  Sequencer::tempo( uSecForQuarter );
  if( Clients.size() > 0 ) Clients.front()->tempo( uSecForQuarter );
} // tempo( unsigned )
void MultiSequencer::wait_for( unsigned WaitClock ) { if( Clients.size() > 0 ) Clients.front()->wait_for( WaitClock ); } // wait_for( unsigned )

void* StarLighter::port_worker( void* This ) {
  StarLighter& Li = *static_cast<StarLighter*>( This );
  fd_set ReadSet;
  fd_set WriteSet;
  timeval TimeOut;
  TimeOut.tv_sec = 0;
  TimeOut.tv_usec = 500;
  int Port = -1;
  bool Paused = false;
  while( true )
  {
    FD_ZERO( &ReadSet );
    FD_ZERO( &WriteSet );
    {
      Locker Lock( Li.mPortMutex );
      Port = Li.Port;
      if( Port < 0 ) break;
      if( Li.mCount > 0 && !Paused ) FD_SET( Port, &WriteSet );
      FD_SET( Li.Port, &ReadSet );
    }
    if( select( FD_SETSIZE, &ReadSet, &WriteSet, nullptr, &TimeOut ) < 0 ) {
      perror( "Port select function." );
      return nullptr;
    }
    if( FD_ISSET( Port, &ReadSet ) ) {
      char Buffer[2049];
      int Count = read( Port, Buffer, 2048 );
      if( Count > 0 ) {
	Buffer[ Count ] = 0;
	for( char* Pos = Buffer+Count-1; Pos != Buffer-1; --Pos )
	  if( *Pos == char(19) ) {
	    write( Port, "[paused]\n", 9 );
	    fsync( Port );
	    Paused = true;
#if 0
	    cout << ">> Paused." << endl;
#endif
	    break;
	  }
	  else if( *Pos == char(17) ) {
#if 0
	    cout << ">> Resume." << endl;
#endif
	    Paused = false;
	    break;
	  }
	Li.data_received( string( Buffer, Count ) );
      }
    }
    if( !Paused && FD_ISSET( Port, &WriteSet ) ) {
      Locker Lock( Li.mPortMutex );
      if( Li.mCount > 0 ) {
	int I = 64; // Write up to the next comman
	while( I < Li.mCount && Li.mBuffer[ I ] != '[' ) ++I;
	if( I > Li.mCount ) I = Li.mCount;
	int Count = write( Port, Li.mBuffer, I );
	if( Count > 0 ) {
	  fsync( Port );
	  if( Count < I ) cerr << "<< Not all command data written in a single block." << endl;
	  cout << "<= " << string( Li.mBuffer, Li.mBuffer+Count ) << flush;
	  if( Count < Li.mCount ) {
	    Li.mCount -= Count;
	    memcpy( Li.mBuffer, Li.mBuffer+Count, Li.mCount );
	  }
	  else Li.mCount = 0;
	}
	else if( Count < 0 ) perror( "Write to lights." );
      }
    }
  }
  return nullptr;
} // port_worker( void* )
StarLighter::StarLighter( const string& FileName ) : Port( -1 ), mPortMutex( PTHREAD_MUTEX_INITIALIZER ), mCount( 0 ), High( 127 ), Low( 0 ), StarsForNote( 1 ) {
  //! \todo Move to some library or (better) use boost
  Port = open( FileName.c_str(), O_RDWR | O_NOCTTY );
  if( Port < 0 ) {
    perror( "Controller open" );
    return;
  }
  termios TCtl;
  memset( &TCtl, 0, sizeof( TCtl ) );
  TCtl.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
  TCtl.c_iflag = IGNPAR | IGNCR; // | IXON | IXOFF;
  TCtl.c_oflag = 0; // ONLCR;
  TCtl.c_lflag = ICANON;
  TCtl.c_cc[VMIN] = 0;
  TCtl.c_cc[VTIME] = 0;
  tcsetattr( Port, TCSANOW, &TCtl );
  pthread_create( &mPortThread, nullptr, &port_worker, this );
}
StarLighter::~StarLighter() {
  int tPort = -1;
  {
    Locker Lock( mPortMutex );
    tPort = Port;
    Port = -1;
  }
  pthread_join( mPortThread, nullptr );
  if( tPort >= 0 ) close( tPort );
} // ~StarLighter()
const int MaxStars = 100;
void StarLighter::note_on( int Channel, int Note, int Velocity ) {
  if( Velocity > 0 ) {
    if( Stars.size() >= MaxStars ) {
      cerr << "Too many stars to create a new one." << endl;
      return;
    }
    stringstream Str;
    Color C = color_for_note( Channel, Note, Velocity );
    for( int I = 0; I < StarsForNote; ++I ) {
      if( Stars.size() >= MaxStars ) break;
      Color C = color_for_note( Channel, Note, Velocity );
      Star NewStar( Channel, Note, Velocity );
      NewStar.Position = rand() % (MaxStars - Stars.size()); //!< \todo Сделать более равномерное распределение и брать предел из переменной.
      for( auto It = Stars.begin(); true; ++It )
	if( It != Stars.end() && It->Position <= NewStar.Position ) ++NewStar.Position;
	else {
	  Stars.insert( It, NewStar );
	  cout << "Create star " << Stars.size() << "@ position " << NewStar.Position << " for note " << note_name( Note ) << " v " << Velocity << " in channel " << Channel << endl;
	  break;
	}
      // We don't need this 'cause insert stars in the correct places. sort( Stars.begin(), Stars.end() ); //!< \todo Figure how remove comparision from the structure and give here
      Str << "[new:" << NewStar.Position << "," << (int)C.Red << "," << (int)C.Green << "," << (int)C.Blue << ']';
    }
    Str << '\n';
    send_command( Str.str() );
  }
  else note_off( Channel, Note, Velocity );
} // note_on( int, int, int );
void StarLighter::note_off( int Channel, int Note, int Velocity ) {
#if 1
  cout << "Remove star for note " << note_name( Note ) << " v " << Velocity << " in channel " << Channel << endl;
#endif
  for( auto It = Stars.begin(); It != Stars.end(); ) {
    if( It->Channel == Channel && It->Note == Note ) { // Velocity doesn't matter.
      stringstream Str;
      Str << "[off:" << It->Position << "]\n";
      send_command( Str.str() );
      It = Stars.erase( It );
      // break;
    }
    else ++It;
  }
} // note_off( int, int, int )
void StarLighter::program_change( int Channel, int NewProgram ) { cout << "Change instrument to " << NewProgram << " in channel " << Channel << endl; }
StarLighter::Color StarLighter::color_for_note( int Channel, int Note, int Velocity ) {
  Color Res;
  double K = Velocity / 127.0;
  int Index = ( double( Note - Low ) / ( High - Low ) ) * ( 255 * 5 + 1 );
  if( Index < 256 ) {
    Res.Red = 255;
    Res.Green = Index;
  }
  else if( Index < 511 ) {
    Res.Red = 255 - Index;
    Res.Green = 255;
  }
  else if( Index < 766 ) {
    Res.Green = 255;
    Res.Blue = Index - 510;
  }
  else if( Index < 1021 ) {
    Res.Green = 1020 - Index;
    Res.Blue = 255;
  }
  else if( Index < 1276 ) {
    Res.Blue = 255;
    Res.Red = Index - 1020;
  }
  else {
    Res.Blue = 1530 - Index;
    Res.Red = 255;
  }
  Res.Red *= K;
  Res.Green *= K;
  Res.Blue *= K;
  return Res;
} // color_for_note( int, int, int )

void StarLighter::send_command( const string& Command ) {
  Locker Lock( mPortMutex );
  if( Command.size() + mCount > 2048 ) {
    cerr << "<= Throw away " << mCount << " bytes: \"" << string( mBuffer, mBuffer+mCount ) << '\"' << endl;
    mCount = 0;
  }
  memcpy( mBuffer+mCount, Command.c_str(), Command.size() );
  mCount += Command.size();
}
void StarLighter::data_received( const string& Data ) {
#if 0
  cout << "=> " << Data << flush;
#endif
}
