#ifndef EXERCISE_SEQUENCE_HPP
#define EXERCISE_SEQUENCE_HPP
#include "../midi/midi_files.hpp"
#include <fstream>
#include <list>
#include <limits.h>

namespace MuTraTrain {
  struct ExerciseLimits
  {
    int StartFore;      // Максимальное опережение начала ноты
    int StartLate;      // Максимальное запаздывание начала ноты
    int StopFore;	      // Максимальное опережение конца ноты
    int StopLate;	      // Максимальное запаздывание начала ноты
    int VelocityHigher; // Максимальное отклонение силы ноты вверх
    int VelocityLower;  // Максимальное отклонение силы ноты вниз
    ExerciseLimits( int StartFore0, int StartLate0, int StopFore0, int StopLate0, int VelocityHigher0, int VelocityLower0 );
  }; // ExerciseLimits

  class ExerciseSequence : public MuTraMIDI::MIDISequence, public MuTraMIDI::Sequencer, public MuTraMIDI::InputDevice::Client
  {
  public:
    enum { NoError = 0, VelocityError, RythmError, NoteError, EmptyPlay };
    struct NotePlay
    {
      bool Visited;
      int Note;
      int Velocity;
      int Start;
      int Stop;
      NotePlay( int Note0, int Velocity0, int Start0, int Stop0 = -1 ) : Visited( false ), Note( Note0 ), Velocity( Velocity0 ), Start( Start0 ), Stop( Stop0 ) {}
    }; // NotePlay
    struct NotesStat
    {
      time_t Start;
      time_t Finish;
      unsigned Result;
      double StartMax;
      double StartMin;
      double StopMax;
      double StopMin;
      int VelocityMax;
      int VelocityMin;
      NotesStat() : Start( 0 ), Finish( 0 ), Result( NoError ), StartMax( INT_MIN ), StartMin( INT_MAX ), StopMax( INT_MIN ), StopMin( INT_MAX ), VelocityMax( INT_MIN ), VelocityMin( INT_MAX ) {}
      void operator()( const NotePlay& Note )
      {
	if( Note.Start > StartMax ) StartMax = Note.Start;
	if( Note.Start < StartMin ) StartMin = Note.Start;
	if( Note.Stop > StopMax ) StopMax = Note.Stop;
	if( Note.Stop < StopMin ) StopMin = Note.Stop;
	if( Note.Velocity > VelocityMax ) VelocityMax = Note.Velocity;
	if( Note.Velocity < VelocityMin ) VelocityMin = Note.Velocity;
      }
    }; // NotesStat
  private:
    unsigned Channels;		// Каналы файла, которые нужно играть
    unsigned TargetTracks;	// Дорожки (файла?), которые нужно играть
    struct ChanNote {
      ChanNote( int Note0 = 60, int Channel0 = 0 ) : Note( Note0 ), Channel( Channel0 ) {}
      int Note;
      int Channel;
      bool operator==( const ChanNote& Other ) const { return Note == Other.Note && Channel == Other.Channel; }
    }; // ChanNote
    std::list<ChanNote> HangingNotes;
  public:
    // Метроном
    int Numerator;
    int Denominator;
    // Тональность
    int Tonal;
    bool Minor;

    // Параметры? упражнения
    int StartThreshold;		// Предельное отклонение начала ноты (в единицах времени файла, MIDI-clockах)
    int StopThreshold;		// Предельное отклонение окончания ноты
    int VelocityThreshold;	// Предельное отклонение силы ноты

    // Пьеса / упражнение
    MuTraMIDI::MIDISequence* Play; 
    int StartPoint;		// Начало играемого участка в MIDI-clockах
    int StopPoint;		// Окончание играемого участка
    double TempoSkew;
    int OriginalStart;
    int OriginalLength;
    int PlayedStart;
    MuTraMIDI::Event::TimeuS PlayedStartuS;
    MuTraMIDI::Event::TimeuS AlignTimeuS;

    ExerciseSequence( unsigned Channels0 = 0xF, unsigned TargetTracks0 = 0xFFFF )
      : Channels( Channels0 ), TargetTracks( TargetTracks0 ), Numerator( 4 ), Denominator( 4 ), Tonal( 0 ), Minor( false ), StartThreshold( 45 ), StopThreshold( 45 ), VelocityThreshold( 64 ),
	Play( nullptr ), StartPoint( 0 ), StopPoint( -1 ), TempoSkew( 1.0 ), OriginalStart( -1 ), OriginalLength( -1 ), PlayedStart( -1 ), PlayedStartuS( 0 ), AlignTimeuS( -1 ), Dump( "beat.dump" )
    { Type = 1; }
    bool load( const std::string& FileName );
    const MuTraMIDI::MIDISequence* play() const { return Play; }
    unsigned channels() const { return Channels; }
    void channels( unsigned NewChannels ) { Channels = NewChannels; }
    unsigned tracks_filter() const { return TargetTracks; }
    void tracks_filter( unsigned NewTracks ) { TargetTracks = NewTracks; }
    MuTraMIDI::Event::TimeuS align_start() const { return AlignTimeuS; }
    void align_start( MuTraMIDI::Event::TimeuS NewStart ) { AlignTimeuS = NewStart; }
    // Переопределения Sequencer'а
    double division() const { return MIDISequence::division(); }
    void division( unsigned MIDIClockForQuarter )
    {
      Sequencer::division( MIDIClockForQuarter );
      MIDISequence::Division = MIDIClockForQuarter;
    }
    void meter( int N, int D );
    void key_signature( int T, bool M ) override;
    void reset();
    void note_on( int Channel, int Note, int Velocity );
    void note_off( int Channel, int Note, int Velocity );
    unsigned tempo() const { return static_cast<unsigned>( Tempo / TempoSkew ); }
    void tempo( unsigned uSecForQuarter );
    unsigned original_tempo() const { return Tempo; }
    void add_original_event( MuTraMIDI::Event* NewEvent );
    void add_played_event( MuTraMIDI::Event* NewEvent );
    // InputDevice::Client overload
    void event_received( const MuTraMIDI::Event& Ev );
    void adjust_length()
    {
      int Bar = int( Numerator * division() / (( 1 << Denominator ) / 4 ) );
      int Rest = OriginalLength % Bar;
      if( Rest > 0 ) OriginalLength += Bar - Rest;
      if( Rest == 0 || Bar-Rest < division() ) // Осталось меньше четверти
	OriginalLength += division(); //!< \todo This extra quarter just to wait if the user is late. Remove this because we add it on every call. It's wrong.
    }
    bool beat( MuTraMIDI::Event::TimeuS Time = MuTraMIDI::get_time_us() );
    unsigned compare( NotesStat& Stat );
    void clear();
    void rescan();
    void new_take();
    std::ofstream Dump;
  }; // ExerciseSequence
} // MuTraTrain

#endif // EXERCISE_SEQUENCE_HPP
