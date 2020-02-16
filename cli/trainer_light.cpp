#include <iostream>
#include <training/Lesson.hpp>
#include <training/metronome.hpp>
#include <unistd.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::string;
using std::ofstream;
using MuTraMIDI::get_time_us;
using MuTraMIDI::Event;
using MuTraMIDI::NoteEvent;
using MuTraMIDI::InputDevice;
using MuTraMIDI::Sequencer;
using MuTraMIDI::MIDISequence;
using MuTraTrain::ExerciseSequence;
using MuTraTrain::Lesson;
using MuTraTrain::NoteTrainer;
using MuTraTrain::Metronome;

#if 0
class LessonStub : public MuTraTrain::ExerciseSequence {
public:
  LessonStub( const string& FileName = std::string() ) {
    
  } // LessonStub( const string& )
}; // LessonStub
#endif

int main() {
  const char* LessonName = "/home/nick/projects/small/music_trainer/data/Lesson.mles";
  const char* DevName = "rtmidi://2";
  const char* SequencerName = "alsa://24";
  bool PlayNotes = false;
  if( InputDevice* Dev = InputDevice::get_instance( DevName ) ) {
    Dev->start();
    if( Sequencer* Seq = Sequencer::get_instance( SequencerName ) ) {
      Seq->start();
      if( PlayNotes ) {
	srand( time(0) );
	NoteTrainer NT( *Seq, 60, 71 );
	Dev->add_client( NT );
	cin.get();
      }
      else {
	Lesson Les( LessonName );
	cout << "Read MIDI events from " << DevName << " sequencer at " << SequencerName << " and lesson from " << LessonName << endl;
	Metronome Metr( Seq );
	do {
	  string MIDIName = Les.file_name();
	  cout << "Load exercise: " << MIDIName << flush;
	  ExerciseSequence Ex( 3, 1 );
	  cout << " retries: " << Les.retries() << " strike: " << Les.strike() << endl;
	  Dev->add_client( Ex );
	  if( Ex.load( MIDIName ) ) {
	    cout << "Exercise loaded. Original start: " << Ex.OriginalStart << " length: " << Ex.OriginalLength << endl;
	    if( Ex.OriginalLength < 0 ) {
	      cerr << "Exercise is empty. Skip it." << endl;
	      continue;
	    }
#ifdef MUTRA_DEBUG
	    {
	      cout << "Original track: ";
	      Ex.print( cout );
	      cout << endl;
	      ofstream File( "exercise.mid" );
	      Ex.write( File );
	      return 0;
	    }
#endif
	    Metr.division( Ex.division() );
	    Metr.meter( Ex.Numerator, Ex.Denominator );
	    Metr.tempo( Ex.tempo() );
	    cout << "Metronome: " << Ex.Numerator << '/' << (1<<Ex.Denominator) << " " << ( 60000000 / Ex.tempo() ) << " bpm " << Ex.division() << " MIDI clocks for quarter." << endl;
	    for( int Repeats = Les.retries(); Repeats > 0; ) {
	      Ex.new_take();
	      Ex.start();
	      Metr.start();
	      //! \todo Make some callback when the exercise time is out.
	      while( !Ex.beat() ) sleep( 1 );
	      Metr.stop();
	      ExerciseSequence::NotesStat Stat;
	      if( Ex.compare( Stat ) == ExerciseSequence::NoError ) {
		--Repeats;
		cout << "No errors!" << endl;
		Seq->note_on( 9, 74, 100 );
		cout << "Statistics: start: " << Stat.StartMin << " - " << Stat.StartMax << ", stop: " << Stat.StopMin << " - " << Stat.StopMax
		     << ", velocity: " << Stat.VelocityMin << " - " << Stat.VelocityMax << endl;
	      }
	      else if( Stat.Result & ExerciseSequence::EmptyPlay ) {
		cout << "Nothing to play." << endl;
		Repeats = 0;
	      }
	      else {
		Repeats = Les.retries();
		if( Stat.Result & ExerciseSequence::NoteError ) {
		  cout << "Wrong notes." << endl;
		  Seq->note_on( 9, 78, 100 );
		}
		else {
		  if( Stat.Result & ExerciseSequence::RythmError ) cout << "Rythm problems." << endl;
		  if( Stat.Result & ExerciseSequence::VelocityError ) cout << "Velocity problems." << endl;
		  Seq->note_on( 9, 84, 100 );
		  cout << "Statistics: start: " << Stat.StartMin << " - " << Stat.StartMax << ", stop: " << Stat.StopMin << " - " << Stat.StopMax
		       << ", velocity: " << Stat.VelocityMin << " - " << Stat.VelocityMax << endl;
		}
	      }
#if 0
	      Ex.print( cout );
#else
	      ofstream File( "exercise.mid" );
	      Ex.close_last_track();
	      Ex.write( File );
#endif
	      sleep( 3 );
	    }
	  }
	  cout << "Go to the next exercise." << endl;
	  Seq->note_on( 9, 52, 100 );
	} while( Les.next() );
	cout << "All done." << endl;
	Les.save();
      }
    }
    else cerr << "Can't get sequencer." << endl;
  }
  else cerr << "Can't get MIDI input device." << endl;
  return 0;
} // main()
