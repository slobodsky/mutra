#ifndef MUTRA_METRONOME_HPP
#define MUTRA_METRONOME_HPP
#include "midi/midi_core.hpp"
#include <thread>

namespace MuTraTrain {
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
    Metronome( MuTraMIDI::Sequencer* Seq ) : mTempo( 500000 ), mDivision( 96 ), mMeterCount( 4 ), mMeterMeasure( 2 ), mSequencer( Seq ), mTimer( *this ) {}
    // Stop metronome before changing this values
    //! \todo Make changes on the fly for files with changing tempo
    //! Set microseconds count for a quarter note
    void tempo( MuTraMIDI::Event::TimeuS NewQuarter ) { mTempo = NewQuarter; }
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
    MuTraMIDI::Sequencer* mSequencer;
    Timer mTimer;
  }; // Metronome

  class NoteTrainer : public MuTraMIDI::InputDevice::Client {
  public:
    NoteTrainer( MuTraMIDI::Sequencer& Seq, int Low = 36, int High = 96 ) : mSeq( Seq ), mLowNote( Low ), mHighNote( High ), mRightSignal( 74 ), mWrongSignal( 78 ) { new_note(); }
    void new_note() {
      mTargetNote = mLowNote + random() % (mHighNote-mLowNote+1);
      mSeq.note_on( 0, mTargetNote, 100 );
    } // new_note()
    void event_received( const MuTraMIDI::Event& Ev );
  private:
    MuTraMIDI::Sequencer& mSeq;
    int mLowNote;
    int mHighNote;
    int mRightSignal;
    int mWrongSignal;
    int mTargetNote;
  }; // NoteTrainer
} // MuTraTrain
#endif // MUTRA_METRONOME_HPP
