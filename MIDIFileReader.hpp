// -*- coding: utf-8; -*-
#ifndef MIDI_FILE_READER_HPP
#define MIDI_FILE_READER_HPP
#include <string>
#include <iostream>
#include <vector>

#ifdef minor
#undef minor
#endif

class Sequencer
{
protected:
  double Start;		// Реальное время начала в микросекундах (все прочие - с начала пьесы)
  unsigned Time;	// Текущее время в микросекундах
  unsigned Clock;	// Текущее время в MIDI-клоках
  unsigned TempoTime;	// Время смены темпа в микросекундах
  unsigned TempoClock;	// Время смены темпа в MIDI-клоках
  double Division;
  unsigned Tempo;
  int Track;		// Текущая дорожка
public:
  Sequencer() : Start( 0 ), Time( 0 ), Clock( 0 ), Division( 120 ), Tempo( 500000 ), TempoTime( 0 ), TempoClock( 0 ), Track( 0 ) {}
  virtual ~Sequencer() {}

  virtual void track( int NewTrack ) { Track = NewTrack; }

  virtual void name( std::string Text ) {}
  virtual void copyright( std::string Text ) {}
  virtual void text( std::string Text ) {}
  virtual void lyric( std::string Text ) {}
  virtual void instrument( std::string Text ) {}
  virtual void marker( std::string Text ) {}
  virtual void cue( std::string Text ) {}

  virtual void meter( int Numerator, int Denominator ) {}

  virtual void note_on( int Channel, int Note, int Velocity ) {}
  virtual void note_off( int Channel, int Note, int Velocity ) {}
  virtual void after_touch( int Channel, int Note, int Velocity ) {}
  virtual void program_change( int Channel, int NewProgram ) {}
  virtual void control_change( int Channel, int Control, int Value ) {}
  virtual void pitch_bend( int Channel, int Bend ) {}

  virtual void reset()
  { 
    Time = 0;
    Clock = 0;
    Start = 0;
    TempoTime = 0;
    TempoClock = 0;
  }
  virtual void start() {}
  virtual void wait_for( unsigned WaitClock )
  {
    Clock = WaitClock;
    Time = static_cast<int>( TempoTime + ( Clock-TempoClock ) / Division * Tempo );
    wait_for_usec( Start + Time );
  }
  virtual void division( unsigned MIDIClockForQuarter ) { Division = MIDIClockForQuarter; }
  unsigned tempo() const { return Tempo; }
  virtual void tempo( unsigned uSecForQuarter )
  {
    TempoTime += static_cast<int>( ( Clock-TempoClock ) / Division * Tempo ); // Сколько времени в микросекундах прошло с тех пор, как мы меняли темп
    TempoClock = Clock;
    Tempo = uSecForQuarter;
  }
protected:
  virtual void wait_for_usec( double WaitMicroSecs ) {}
}; // Sequencer

class Event
{
public:
  typedef int64_t TimeMS; //!< Time in milliseconds from the start of the file or session. If < 0 - right now (to play).
  static int get_int( const unsigned char* Pos, int Length );
  static int get_var( const unsigned char*& Pos, int& ToGo );
  static Event* parse( const unsigned char*& Pos, int& ToGo );
  Event( TimeMS Time = -1 ) : mTimeMS( Time ) {}
  virtual ~Event() {}
  virtual void print( std::ostream& Stream );
  virtual void play( Sequencer& S ) {}
  virtual int write( std::ostream& File ) const { return File.good(); }
  TimeMS time() const { return mTimeMS; }
private:
  TimeMS mTimeMS;
}; // Event

class MIDIInputDevice
{
public:
  class Client
  {
  public:
    virtual ~Client() {}
    virtual void event_received( Event& Ev );
    virtual void connected( MIDIInputDevice& Dev );
    virtual void disconnected( MIDIInputDevice& Dev );
  }; // CLient
  virtual ~MIDIInputDevice();
  void add_client( Client& Cli );
  void remove_client( Client& Cli );
  const std::vector<Client*> clients() const { return Clients; }
protected:
  void event_received( Event& Ev );
  //! Parse the incoming buffer and fire events
  //! \return Number of bytes rest at the end because they don't form a MIDI event.
  int parse( const unsigned char* Buffer, size_t Count );
private:
  std::vector<Client*> Clients;
}; // MIDIInputDevice

class EventsList
{
  unsigned Time; // Время (в единицах MIDI)
  std::vector<Event*> Events;
public:
  EventsList( unsigned Time0 ) : Time( Time0 ) {}
  ~EventsList()
  {
    while( !Events.empty() )
    {
      delete Events.back();
      Events.pop_back();
    }
  } // Деструктор (не полиморфный)
  void add( Event* NewEvent ) { Events.push_back( NewEvent ); }
  unsigned time() const { return Time; }
  std::vector<Event*>& events() { return Events; }

  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File, unsigned Clock ) const;
}; // EventsList

class MIDIEvent : public Event
{
public:
  enum MIDIStatus { NoteOff = 0x80, NoteOn = 0x90, AfterTouch = 0xA0, ControlChange = 0xB0, ProgramChange = 0xC0, ChannelAfterTouch = 0xD0,
		    PitchBend = 0xE0 };
protected:
  int Channel;
  MIDIStatus Status;
public:
  static Event* parse( const unsigned char*& Pos, int& ToGo );
  MIDIEvent( MIDIStatus Status0, int Channel0 ) : Status( Status0 ), Channel( Channel0 ) {}
  MIDIStatus status() const { return Status; }
  int channel() const { return Channel; }

  void print( std::ostream& Stream );
}; // MIDIEvent

class NoteEvent : public MIDIEvent
{
  int Note;
  int Velocity;
public:
  enum { PercussionFirst = 35, PercussionLast = 81 };
  static const char* Percussion[ PercussionLast-PercussionFirst+1 ];
  NoteEvent( MIDIStatus Status0, int Channel0, int Note0, int Velocity0 )
    : MIDIEvent( Status0, Channel0 ), Note( Note0 ), Velocity( Velocity0 ) {}
  int note() const { return Note; }
  int velocity() const { return Velocity; }

  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const;
}; // NoteEvent

class ControlChangeEvent : public MIDIEvent
{
  int Control;
  int Value;
public:
  ControlChangeEvent( int Channel0, int Control0, int Value0 )
    : MIDIEvent( ControlChange, Channel0 ), Control( Control0 ), Value( Value0 ) {}
  int control() const { return Control; }
  int value() const { return Value; }

  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const;
}; // ControlChangeEvent

class ProgramChangeEvent : public MIDIEvent
{
  int Program;
public:
  static const char* Instruments[ 128 ];
  static const char* Families[ 16 ];
  ProgramChangeEvent( int Channel0, int Program0 ) : MIDIEvent( ProgramChange, Channel0 ), Program( Program0 ) {}
  int program() const { return Program; }

  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const;
}; // ProgramChangeEvent

class ChannelAfterTouchEvent : public MIDIEvent
{
  int Velocity;
public:
  ChannelAfterTouchEvent( int Channel0, int Velocity0 ) : MIDIEvent( ChannelAfterTouch, Channel0 ), Velocity( Velocity0 )
  {}
  int velocity() const { return Velocity; }

  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // ChannelAfterTouchEvent

class PitchBendEvent : public MIDIEvent
{
  int Bend;
public:
  PitchBendEvent( int Channel0, int Bend0 ) : MIDIEvent( PitchBend, Channel0 ), Bend( Bend0 ) {}
  int bend() const { return Bend; }

  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const;
}; // PitchBendEvent

class MetaEvent : public Event
{
public:
  enum MetaType { SequenceNumber = 0x00, Text = 0x01, Copyright = 0x02, TrackName = 0x03, InstrumentName = 0x04,
		  Lyric = 0x05, Marker = 0x06, CuePoint = 0x07, ChannelPrefix = 0x20, TrackEnd = 0x2F, Tempo = 0x51,
		  SMTPEOffset = 0x54, TimeSignature = 0x58, KeySignature = 0x59, SequencerMeta = 0x7F };
  static Event* parse( const unsigned char*& Pos, int& ToGo );
  void print( std::ostream& Stream );
}; // MetaEvent

class UnknownMetaEvent : public MetaEvent
{
  int Type;
  int Length;
  unsigned char* Data;
public:
  UnknownMetaEvent( int Type0, int Length0, unsigned char* Data0 );
  ~UnknownMetaEvent();
  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // UnknownMetaEvent

class SequenceNumberEvent : public MetaEvent
{
  int Number;
public:
  SequenceNumberEvent( int Number0 ) : Number( Number0 ) {}
  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // SequenceNumberEvent

class TextEvent : public MetaEvent
{
  std::string Text;
public:
  TextEvent( std::string Text0 ) : Text( Text0 ) {}
  std::string text() const { return Text; }
  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const { return write_text( File, MetaEvent::Text ); }
  int write_text( std::ostream& File, int Type ) const;
}; // TextEvent
class CopyrightEvent : public TextEvent
{
public:
  CopyrightEvent( std::string Text0 ) : TextEvent( Text0 ) {}
  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const { return write_text( File, MetaEvent::Copyright ); }
}; // CopyrightEvent
class TrackNameEvent : public TextEvent
{
public:
  TrackNameEvent( std::string Text0 ) : TextEvent( Text0 ) {}
  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const { return write_text( File, MetaEvent::TrackName ); }
}; // TrackNameEvent
class InstrumentNameEvent : public TextEvent
{
public:
  InstrumentNameEvent( std::string Text0 ) : TextEvent( Text0 ) {}
  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const { return write_text( File, MetaEvent::InstrumentName ); }
}; // InstrumentNameEvent
class LyricEvent : public TextEvent
{
public:
  LyricEvent( std::string Text0 ) : TextEvent( Text0 ) {}
  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const { return write_text( File, MetaEvent::Lyric ); }
}; // LyricEvent
class MarkerEvent : public TextEvent
{
public:
  MarkerEvent( std::string Text0 ) : TextEvent( Text0 ) {}
  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const { return write_text( File, MetaEvent::Marker ); }
}; // MarkerEvent
class CuePointEvent : public TextEvent
{
public:
  CuePointEvent( std::string Text0 ) : TextEvent( Text0 ) {}
  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const { return write_text( File, MetaEvent::CuePoint ); }
}; // CuePointEvent

class ChannelPrefixEvent : public MetaEvent
{
  int Channel;
public:
  ChannelPrefixEvent( int Channel0 ) : Channel( Channel0 ) {}
  int channel() const { return Channel; }
  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // ChannelPrefixEvent

class TrackEndEvent : public MetaEvent
{
public:
  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // TrackEndEvent

class TempoEvent : public MetaEvent
{
  int Tempo; // Микросекунд на четвертную ноту
public:
  TempoEvent( int Tempo0 ) : Tempo( Tempo0 ) {}
  int tempo() const { return Tempo; }
  void print( std::ostream& Stream );
  void play( Sequencer& S );
  int write( std::ostream& File ) const;
}; // TempoEvent

class SMTPEOffsetEvent : public MetaEvent
{
  int Hour;
  int Minute;
  int Second;
  int Frame;
  int Hundredths;
public:
  SMTPEOffsetEvent( int Hour0 = 0, int Minute0 = 0, int Second0 = 0, int Frame0 = 0, int Hundredths0 = 0 )
    : Hour( Hour0 ), Minute( Minute0 ), Second( Second0 ), Frame( Frame0 ), Hundredths( Hundredths0 ) {}
  int hour() const { return Hour; }
  int minute() const { return Minute; }
  int second() const { return Second; }
  int frame() const { return Frame; }
  int hundredths() const { return Hundredths; }

  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // SMTPEOffsetEvent

class TimeSignatureEvent : public MetaEvent
{
  int Numerator;
  int Denominator;
  int ClickClocks;
  int ThirtySeconds;
public:
  TimeSignatureEvent( int Numerator0, int Denominator0, int ClickClocks0, int ThirtySeconds0 = 8 )
    : Numerator( Numerator0 ), Denominator( Denominator0 ), ClickClocks( ClickClocks0 ), ThirtySeconds( ThirtySeconds0 )
  {}
  int numerator() const { return Numerator; }
  int denominator() const { return Denominator; }
  int click_clocks() const { return ClickClocks; }
  int thirtyseconds() const { return ThirtySeconds; }

  void play( Sequencer& S );
  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // TimeSignatureEvent

class KeySignatureEvent : public MetaEvent
{
  int Tonal;
  bool Minor;
public:
  KeySignatureEvent( int Tonal0, bool Minor0 ) : Tonal( Tonal0 ), Minor( Minor0 ) {}
  int tonal() const { return Tonal; }
  bool minor() const { return Minor; }

  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // KeySignatureEvent

class SequencerMetaEvent : public MetaEvent
{
  int Length;
  unsigned char* Data;
public:
  SequencerMetaEvent( int Length0 = 0, unsigned char* Data0 = 0 ) : Length( Length0 ), Data( Data0 ) {}
  ~SequencerMetaEvent()
  { 
    if( Data )
      delete Data;
  }
  int length() const { return Length; }
  const unsigned char* data() const { return Data; }
  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // SequencerMetaEvent

class SysExEvent : public Event
{
  int Length;
  unsigned char* Data;
public:
  static Event* parse( const unsigned char*& Pos, int& ToGo );
  SysExEvent( int Length0 = 0, unsigned char* Data0 = 0 ) : Length( Length0 ), Data( Data0 ) {}
  ~SysExEvent()
  { 
    if( Data )
      delete Data;
  }
  int length() const { return Length; }
  const unsigned char* data() const { return Data; }
  void print( std::ostream& Stream );
  int write( std::ostream& File ) const;
}; // SysExEvent

class MIDIException
{
  std::string Description;
public:
  MIDIException( std::string Description0 ) : Description( Description0 ) {}
  std::string description() const { return Description; }
}; // MIDIException

class MIDITrack
{
  std::vector<EventsList*> Events;
public:
  ~MIDITrack()
  {
    while( !Events.empty() )
    {
      delete Events.back();
      Events.pop_back();
    }
  }
  std::vector<EventsList*>& events() { return Events; }
  void print( std::ostream& Stream );
  void play( Sequencer& Seq );
  bool write( std::ostream& File ) const;
}; // MIDITrack

class MIDISequence
{
public:
  static int put_int( std::ostream& File, int Size, int Number );
  static int put_var( std::ostream& File, int Number );
  static int get_int( std::istream& Stream, int Size );
  static int get_var( std::istream& Stream, int& ToGo );
protected:
  std::vector<MIDITrack*> Tracks;

  short Type;
  short TracksNum;
  short Division;

  void get_meta( std::istream& Stream, int& ToGo );
  void get_sysex( std::istream& Stream, int& ToGo, bool AddF0 = true );
  void get_event( std::istream& Stream, int& ToGo, unsigned char& Status, unsigned char FirstByte );
  void add( Event* NewEvent ) { Tracks.back()->events().back()->add( NewEvent ); }
public:
  MIDISequence( std::string FileName );
  MIDISequence() : Type( 0 ), TracksNum( 0 ), Division( 120 ) {}
  virtual ~MIDISequence()
  {
    while( !Tracks.empty() )
    {
      delete Tracks.back();
      Tracks.pop_back();
    }
  }
  int division() const { return Division; }
  int tracks_num() const { return TracksNum; }
  std::vector<MIDITrack*>& tracks() { return Tracks; }
  void print( std::ostream& Stream );
  virtual void parse( unsigned Data, int Clock = -1, int TrackNum = -1 );
  void play( Sequencer& S );
  bool write( std::ostream& File ) const;
}; // MIDISequence
#endif // MIDI_FILE_READER_HPP
