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
    int64_t start_time() const { return mStartTime; }
  private:
    int mCount;
    int mDelay;
    int64_t mStartTime;
    int64_t mLastTime;
    std::thread mThread;
    Client& mClient;
  }; // Timer

  //! \todo Fix names for meter: measure (bar) & beat, probable create a special class representing meter
  class MetronomeOptions {
  public:
    MetronomeOptions();
    //! \todo Check range for the values.
    int tempo() const { return mTempo; }
    MetronomeOptions& tempo( int NewTempo ) { mTempo = NewTempo; return *this; }
    int beat() const { return mBeat; }
    MetronomeOptions& beat( int NewBeat ) { mBeat = NewBeat; return *this; }
    int measure() const { return mMeasure; }
    MetronomeOptions& measure( int NewMeasure ) { mMeasure = NewMeasure; return *this; }
    int note() const { return mNote; }
    MetronomeOptions& note( int NewNote ) { mNote = NewNote; return *this; }
    int velocity() const { return mVelocity; }
    MetronomeOptions& velocity( int NewVelocity ) { mVelocity = NewVelocity; return *this; }
    int medium_note() const { return mMediumNote; }
    MetronomeOptions& medium_note( int NewNote ) { mMediumNote = NewNote; return *this; }
    int medium_velocity() const { return mMediumVelocity; }
    MetronomeOptions& medium_velocity( int NewVelocity ) { mMediumVelocity = NewVelocity; return *this; }
    int power_note() const { return mPowerNote; }
    MetronomeOptions& power_note( int NewNote ) { mPowerNote = NewNote; return *this; }
    int power_velocity() const { return mPowerVelocity; }
    MetronomeOptions& power_velocity( int NewVelocity ) { mPowerVelocity = NewVelocity; return *this; }
  private:
    int mTempo;
    int mBeat;
    int mMeasure;
    int mNote;
    int mVelocity;
    int mMediumNote;
    int mMediumVelocity;
    int mPowerNote;
    int mPowerVelocity;
  }; // MetronomeOptions

  class Metronome : Timer::Client {
  public:
    // Note duration constant
    enum Duration { Whole = 0, Half = 1, Quarter = 2, Eighth = 3, Sixteenth = 4, ThirtySecond = 5, SixtyFourth = 6 };
    Metronome( MuTraMIDI::Sequencer* Seq ) : mTempouS( 60000000 / 120 ), mDivision( 96 ), mSequencer( Seq ), mTimer( *this ) {}
    // Stop metronome before changing this values
    //! \todo use tempo for BPM & tempo_us for microseconds
    //! \todo Сделать корректную настройку темпа в миросекундах и работу метронома для долей отличных от четверти. Пока с этим полные непонятки.
    //! \todo Make changes on the fly for files with changing tempo
    //! Set microseconds count for a quarter note
    void tempo( MuTraMIDI::Event::TimeuS NewQuarter ) {
      mTempouS = NewQuarter;
      mOptions.tempo( 60*1000000 / NewQuarter );
    } // tempo( MuTraMIDI::Event::TimeuS )
    MuTraMIDI::Event::TimeuS tempo() const { return mTempouS; }
    //! Set ticks count for a quarter note
    void division( int NewDivision ) { mDivision = NewDivision; }
    const MetronomeOptions& options() const { return mOptions; }
    void options( const MetronomeOptions& NewOptions ) {
      mOptions = NewOptions;
      mTempouS = 60*1000000 / mOptions.tempo();
    } // options( const MetronomeOptions& )
    //! Установить размер - число долей и доля, как степень двойки - 0 - целая, 1 - половина, 2 - четверть, 3 - восьмая и т.д.
    void meter( int Count = 4, int Measure = 2 ) { mOptions.beat( Count ).measure( Measure ); } // meter
    void start();
    void stop();
    int64_t start_time() const { return mTimer.start_time(); }
    //! \todo This is a signal from the internal timer. No one must call it. Protect it (e.g. change to lambda).
    void timer( int Count, int64_t Target, int64_t Now );
  private:
    int mTempouS; //!< Темп, как число микросекунд на четвертную ноту.
    int mDivision; //!< Число тиков MIDI на четвертную ноту
    MetronomeOptions mOptions;
    MuTraMIDI::Sequencer* mSequencer;
    Timer mTimer;
  }; // Metronome

  class NoteTrainer : public MuTraMIDI::InputDevice::Client {
  public:
    NoteTrainer( MuTraMIDI::Sequencer& Seq, int Low = 36, int High = 96 ) : mSeq( Seq ), mLowNote( Low ), mHighNote( High ), mRightSignal( 74 ), mWrongSignal( 78 ) { new_note(); }
    void new_note();
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
