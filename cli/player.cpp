#include <midi/midi_files.hpp>
#include <midi/midi_utility.hpp>
#include "star_lighter.hpp"
#include <unistd.h>
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using MuTraMIDI::Sequencer;
using MuTraMIDI::MIDISequence;
using MuTraMIDI::MultiSequencer;
using MuTraMIDI::MIDIBackend;

int main( int argc, char* argv[] )
{
  const char* LightPortName = nullptr; // "/dev/ttyACM0";
  Sequencer* Seq = MIDIBackend::get_manager().get_sequencer();
  MultiSequencer Mult;
  Mult.add( Seq );
  try {
    for( int I = 1; I < argc; I++ )
    {
      MIDISequence Play( argv[ I ] );
      //LinuxSequencer Seq( cout );
#ifdef MUTRA_DEBUG
      cerr << "Файл: " << argv[ I ] << " notes: " << int(Play.low()) << " - " << int(Play.high()) << endl;
      Play.print( cout );
#endif
      if( LightPortName ) {
	StarLighter Light( LightPortName );
	cerr << "Wait a few seconds..." << flush;
	sleep( 3 );
	cerr << " Go on." << endl;
	Mult.add( &Light );
	Light.low( Play.low() );
	Light.high( Play.high() );
      }
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
