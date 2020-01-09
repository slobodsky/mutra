#include <midi_files.hpp>
using std::cout;
using std::cerr;
using std::endl;
using MuTraMIDI::Sequencer;
using MuTraMIDI::MIDISequence;

int main( int argc, char* argv[] )
{
  Sequencer* Seq = Sequencer::get_instance();
  for( int I = 1; I < argc; I++ )
  {
    MIDISequence Play( argv[ I ] );
    //LinuxSequencer Seq( cout );
    cerr << "Файл: " << argv[ I ] << endl;
    Play.play( *Seq );
#if 0
    cerr << "\tСреднее отклонение: " << Seq->average_diff()
	 << "\n\tМаксимальное: " << Seq->max_diff()
	 << "\n\tСредняя задержка: " << Seq->average_in_diff()
	 << "\n\tМаксимальная: " << Seq->max_in_diff() << endl;
#endif
  }
  delete Seq;
  return 0;
} // main()
