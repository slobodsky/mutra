#include "Sequencer.hpp"
using std::cout;
using std::cerr;
using std::endl;

int main( int argc, char* argv[] )
{
  for( int I = 1; I < argc; I++ )
  {
    MIDISequence Play( argv[ I ] );
    ALSASequencer Seq( cout );
    //LinuxSequencer Seq( cout );
    cerr << "Файл: " << argv[ I ] << endl;
    Play.play( Seq );
    cerr << "\tСреднее отклонение: " << Seq.average_diff()
	 << "\n\tМаксимальное: " << Seq.max_diff()
	 << "\n\tСредняя задержка: " << Seq.average_in_diff()
	 << "\n\tМаксимальная: " << Seq.max_in_diff() << endl;
  }
  return 0;
} // main()
