#ifndef STAR_LIGHTER_HPP
#define STAR_LIGHTER_HPP
#include "midi_core.hpp"
#include <string>

class MultiSequencer : public MuTraMIDI::Sequencer {
public:
  void add( MuTraMIDI::Sequencer* Seq );
  void remove( MuTraMIDI::Sequencer* Seq );
  void start();
  void name( std::string Text );
  void copyright( std::string Text );
  void text( std::string Text );
  void lyric( std::string Text );
  void instrument( std::string Text );
  void marker( std::string Text );
  void cue( std::string Text );

  void meter( int Numerator, int Denominator );

  void note_on( int Channel, int Note, int Velocity );
  void note_off( int Channel, int Note, int Velocity );
  void after_touch( int Channel, int Note, int Velocity );
  void program_change( int Channel, int NewProgram );
  void control_change( int Channel, int Control, int Value );
  void pitch_bend( int Channel, int Bend );
  void division( unsigned MIDIClockForQuarter );
  void tempo( unsigned uSecForQuarter );
  void wait_for( unsigned WaitClock );
private:
  std::vector<MuTraMIDI::Sequencer*> Clients;
}; // MultiSequencer

class StarLighter : public MuTraMIDI::Sequencer {
  struct Color {
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
    Color( uint8_t Red0 = 0, uint8_t Green0 = 0, uint8_t Blue0 = 0 ) : Red( Red0 ), Green( Green0 ), Blue( Blue0 ) {}
  }; // Color
  struct Star {
    Star( int Channel0 = 0, int Note0 = 0, int Velocity0 = 0, int Position0 = 0 ) : Channel( Channel0 ), Note( Note0 ), Velocity( Velocity0 ), Position( Position0 ) {}
    bool operator>( const Star& O ) { return Position > O.Position; }
    bool operator<( const Star& O ) { return Position < O.Position; }
    int Channel;
    int Note;
    int Velocity;
    int Position;
  }; // Star
  class Locker {
    // No nulls, no copies.
    explicit Locker( const Locker& Other ) : mMutex( Other.mMutex ) {}
  public:
    Locker( pthread_mutex_t& Mutex ) : mMutex( Mutex ) { pthread_mutex_lock( &mMutex ); }
    ~Locker() { pthread_mutex_unlock( &mMutex ); }
  private:
    pthread_mutex_t& mMutex;
  }; // Locker
  static void* port_worker( void* This );
public:
  StarLighter( const std::string& PortName );
  ~StarLighter();

  uint8_t high() const { return High; }
  void high( uint8_t NewHigh ) { High = NewHigh; }
  uint8_t low() const { return Low; }
  void low( uint8_t NewLow ) { Low = NewLow; }
  uint8_t stars_for_note() const { return StarsForNote; }
  void stars_for_note( uint8_t NewStarsForNote ) { StarsForNote = NewStarsForNote; }
  
  void note_on( int Channel, int Note, int Velocity );
  void note_off( int Channel, int Note, int Velocity );
  void program_change( int Channel, int NewProgram );
private:
  Color color_for_note( int Channel, int Note, int Velocity );
  void send_command( const std::string& Command );
  void data_received( const std::string& Data );
  std::vector<Star> Stars;
  int Port;
  pthread_t mPortThread;
  pthread_mutex_t mPortMutex;
  char mBuffer[2048];
  int mCount;
  uint8_t High;
  uint8_t Low;
  uint8_t StarsForNote;
}; // StarLighter
#endif // STAR_LIGHTER_HPP
