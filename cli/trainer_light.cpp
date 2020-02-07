#include <iostream>
#include <training/Lesson.hpp>
#include <thread>
#include <sys/time.h>
#include <unistd.h>
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::string;
using MuTraMIDI::Event;
using MuTraMIDI::NoteEvent;
using MuTraMIDI::InputDevice;
using MuTraMIDI::Sequencer;
using MuTraMIDI::MIDISequence;
using MuTraTrain::ExerciseSequence;
using MuTraTrain::Lesson;

uint64_t get_time_us() {
  timeval TV;
  gettimeofday( &TV, nullptr );
  return TV.tv_sec * uint64_t( 1000000 ) + TV.tv_usec;
} // get_time_us()

#if 0
class LessonStub : public MuTraTrain::ExerciseSequence {
public:
  LessonStub( const string& FileName = std::string() ) {
    
  } // LessonStub( const string& )
}; // LessonStub
#endif

class Timer {
public:
  class Client {
  public:
    virtual ~Client() {}
    virtual void timer( int Count, int64_t Target, int64_t Now ) = 0;
  }; // Client
  Timer( Client& Cli );
  void start( int Delay );
  void stop();
private:
  int mCount;
  int mDelay;
  int64_t mStartTime;
  int64_t mLastTime;
  std::thread mThread;
  Client& mClient;
}; // Timer

//! \todo Fix names for meter: measure (bar) & beat, probable create a special class representing meter
class Metronome : Timer::Client {
public:
  // Note duration constant
  enum Duration { Whole = 0, Half = 1, Quarter = 2, Eighth = 3, Sixteenth = 4, ThirtySecond = 5, SixtyFourth = 6 };
  Metronome( Sequencer* Seq ) : mTempo( 500000 ), mDivision( 96 ), mMeterCount( 4 ), mMeterMeasure( 2 ), mSequencer( Seq ), mTimer( *this ) {}
  // Stop metronome before changing this values
  //! \todo Make changes on the fly for files with changing tempo
  //! Set microseconds count for a quarter note
  void tempo( Event::TimeuS NewQuarter ) { mTempo = NewQuarter; }
  //! Set ticks count for a quarter note
  void division( int NewDivision ) { mDivision = NewDivision; }
  //! Установить размер - число долей и доля, как степень двойки - 0 - целая, 1 - половина, 2 - четверть, 3 - восьмая и т.д.
  void meter( int Count = 4, int Measure = 2 ) {
    mMeterCount = Count;
    mMeterMeasure = Measure;
  } // meter
  void start();
  void stop();
  void timer( int Count, int64_t Target, int64_t Now );
private:
  int mTempo; //!< Число микросекунд на четвертную ноту
  int mDivision; //!< Число тиков MIDI на четвертную ноту
  int mMeterCount;
  int mMeterMeasure;
  Sequencer* mSequencer;
  Timer mTimer;
}; // Metronome

Timer::Timer( Timer::Client& Cli ) : mCount( 0 ), mDelay( 0 ), mStartTime( 0 ), mLastTime( 0 ), mClient( Cli ) {}
void Timer::start( int Delay ) {
  if( mDelay > 0 ) return; // Already started.
  timeval TV;
  gettimeofday( &TV, nullptr );
  mStartTime = TV.tv_sec * 1000000 + TV.tv_usec;
  mLastTime = mStartTime;
  mDelay = Delay;
  mThread = std::thread( [this]() {
			   int64_t Target = this->mStartTime + this->mDelay;
			   while( this->mDelay > 0 ) {
			     Target += this->mDelay;
			     if( Target > this->mLastTime ) std::this_thread::sleep_for( std::chrono::microseconds( Target - this->mLastTime ) );
			     if( mDelay == 0 ) break;
			     ++(this->mCount);
			     timeval TV;
			     gettimeofday( &TV, nullptr );
			     this->mLastTime = TV.tv_sec * 1000000 + TV.tv_usec;
			     this->mClient.timer( this->mCount, Target - this->mStartTime, this->mLastTime - this->mStartTime );
			   }
			 });
} // start()
void Timer::stop() {
  if( mDelay == 0 ) return;
  mDelay = 0;
  mThread.join();
} // stop()

void Metronome::start() { mTimer.start( mTempo ); }
void Metronome::stop() { mTimer.stop(); }
void Metronome::timer( int Count, int64_t Target, int64_t Now ) {
  if( mSequencer ) {
    mSequencer->note_on( 9, 42, 80 );
    if( ( Count % mMeterCount ) == 0 ) mSequencer->note_on( 9, 35, 100 );
  }
} // tiemr( int, int64_t, int64_t )

class NoteTrainer : public InputDevice::Client {
public:
  NoteTrainer( Sequencer& Seq, int Low = 36, int High = 96 ) : mSeq( Seq ), mLowNote( Low ), mHighNote( High ), mRightSignal( 74 ), mWrongSignal( 78 ) { new_note(); }
  void new_note() {
    mTargetNote = mLowNote + random() % (mHighNote-mLowNote+1);
    mSeq.note_on( 0, mTargetNote, 100 );
  } // new_note()
  void event_received( const Event& Ev ) {
    if( Ev.status() == Event::NoteOff || ( Ev.status() == Event::NoteOn && static_cast<const NoteEvent&>(Ev).velocity() == 0 ) ) {
      if( static_cast<const NoteEvent&>(Ev).note() == mTargetNote ) {
	mSeq.note_on( 9, mRightSignal, 80 );
	sleep( 1 );
	new_note();
      }
      else {
	mSeq.note_on( 9, mWrongSignal, 64 );
	sleep( 1 );
	mSeq.note_on( 0, mTargetNote, 100 );
	//! \todo Count tires.
      }
    }
  } // event_received( const Event& )
private:
  Sequencer& mSeq;
  int mLowNote;
  int mHighNote;
  int mRightSignal;
  int mWrongSignal;
  int mTargetNote;
}; // NoteTrainer

int main() {
  const char* LessonName = "/home/nick/projects/small/music_trainer/data/les.mles";
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
	uint64_t StartTime = 0;
	do {
	  string MIDIName = Les.file_name();
	  cout << "Load exercise: " << MIDIName << flush;
	  ExerciseSequence Ex( 3, 1 );
	  cout << " retries: " << Les.retries() << " strike: " << Les.strike() << endl;
	  Dev->add_client( Ex );
	  if( Ex.load( MIDIName ) ) {
	    Metr.division( Ex.division() );
	    Metr.meter( Ex.Numerator, Ex.Denominator );
	    Metr.tempo( Ex.tempo() );
	    cout << "Metronome: " << Ex.Numerator << '/' << Ex.Denominator << " " << ( 60000000 / Ex.tempo() ) << " bpm " << Ex.division() << " MIDI clocks for quarter." << endl;
	    for( int Repeats = Les.retries(); Repeats > 0; ) {
	      Ex.new_take();
	      Ex.start();
	      Metr.start();
	      //! \todo Make some callback when the exercise time is out.
	      while( Ex.PlayedStart < 0 ) sleep( 1 );
	      if( StartTime == 0 ) StartTime = get_time_us();
	      cout << "Play's started @" << StartTime << "us" << endl;
	      while( !Ex.beat( get_time_us() - StartTime ) ) sleep( 1 );
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
#if 0
		  Ex.print( cout );
		  std::ofstream File( "exercise.mid" );
		  Ex.write( File );
#endif
		}
		else {
		  if( Stat.Result & ExerciseSequence::RythmError ) cout << "Rythm problems." << endl;
		  if( Stat.Result & ExerciseSequence::VelocityError ) cout << "Velocity problems." << endl;
		  Seq->note_on( 9, 84, 100 );
		  cout << "Statistics: start: " << Stat.StartMin << " - " << Stat.StartMax << ", stop: " << Stat.StopMin << " - " << Stat.StopMax
		       << ", velocity: " << Stat.VelocityMin << " - " << Stat.VelocityMax << endl;
		}
	      }
	    }
	  }
	  cout << "Go to the next exercise." << endl;
	  Seq->note_on( 9, 60, 100 );
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
