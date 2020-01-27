#include <midi_files.hpp>
#include "star_lighter.hpp"
#include <unistd.h>
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using MuTraMIDI::Sequencer;
using MuTraMIDI::MIDISequence;

int main( int argc, char* argv[] )
{
  Sequencer* Seq = Sequencer::get_instance();
  MultiSequencer Mult;
  Mult.add( Seq );
  StarLighter Light( "/dev/ttyACM0" );
  cerr << "Wait a few seconds..." << flush;
  sleep( 3 );
  cerr << " Go on." << endl;
  Mult.add( &Light );
  try {
    for( int I = 1; I < argc; I++ )
    {
      MIDISequence Play( argv[ I ] );
      //LinuxSequencer Seq( cout );
      cerr << "Файл: " << argv[ I ] << " notes: " << int(Play.low()) << " - " << int(Play.high()) << endl;
      Light.low( Play.low() );
      Light.high( Play.high() );
      Play.play( Mult );
#if 0
      cerr << "\tСреднее отклонение: " << Seq->average_diff()
	   << "\n\tМаксимальное: " << Seq->max_diff()
	   << "\n\tСредняя задержка: " << Seq->average_in_diff()
	   << "\n\tМаксимальная: " << Seq->max_in_diff() << endl;
#endif
    }
  }
  catch( MuTraMIDI::MIDIException& Ex ) {
    cerr << "MIDI exception: " << Ex.what() << endl;
  }
  delete Seq;
  return 0;
} // main()
