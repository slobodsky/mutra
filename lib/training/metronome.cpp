#include <midi/midi_events.hpp>
#include "metronome.hpp"
#include <unistd.h>
using std::cerr;
using std::endl;

using MuTraMIDI::get_time_us;
using MuTraMIDI::Event;
using MuTraMIDI::NoteEvent;

namespace MuTraTrain {
  Timer::Timer( Timer::Client& Cli ) : mCount( 0 ), mDelay( 0 ), mStartTime( 0 ), mLastTime( 0 ), mClient( Cli ) {}
  void Timer::start( int Delay ) {
    if( mDelay > 0 ) return; // Already started.
    mStartTime = get_time_us();
    mLastTime = mStartTime;
    mDelay = Delay;
    if( mThread.joinable() ) {
      cerr << "Try to assign to joinable thread." << endl;
      mThread.detach();
    }
    mThread = std::thread( [this]() {
			     int64_t Target = this->mStartTime + this->mDelay;
			     while( this->mDelay > 0 ) {
			       Target += this->mDelay;
			       if( Target > this->mLastTime ) std::this_thread::sleep_for( std::chrono::microseconds( Target - this->mLastTime ) );
			       if( mDelay == 0 ) break;
			       ++(this->mCount);
			       this->mLastTime = get_time_us();
			       this->mClient.timer( this->mCount, Target - this->mStartTime, this->mLastTime - this->mStartTime );
			     }
			   });
  } // start()
  void Timer::stop() {
    if( mDelay == 0 ) return;
    mDelay = 0;
    mThread.join();
  } // stop()

  MetronomeOptions::MetronomeOptions() : mTempo( 120 ), mBeat( 4 ), mMeasure( 2 ), mNote( 42 ), mVelocity( 64 ), mMediumNote( 38 ), mMediumVelocity( 80 ), mPowerNote( 35 ), mPowerVelocity( 100 ) {
  } // MetronomeOptions()
  void Metronome::start() { mTimer.start( mTempouS ); }
  void Metronome::stop() { mTimer.stop(); }
  void Metronome::timer( int Count, int64_t Target, int64_t Now ) {
    if( mSequencer ) {
      mSequencer->note_on( 9, mOptions.note(), mOptions.velocity() );
      if( ( Count % mOptions.beat() ) == 0 ) mSequencer->note_on( 9, mOptions.power_note(), mOptions.power_velocity() );
    }
  } // tiemr( int, int64_t, int64_t )

  void NoteTrainer::event_received( const Event& Ev ) {
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
} // MuTraTrain