#ifndef MUTRA_MIDI_EVENTS_HPP
#define MUTRA_MIDI_EVENTS_HPP
#include "midi_core.hpp"

/** \file
 * Различные события интерфейса MIDI
 */
namespace MuTraMIDI {
  class ChannelEvent : public Event
  {
  protected:
    int Channel;
    StatusCode Status;
  public:
    // static Event* parse( const unsigned char*& Pos, int& ToGo );
    //! Get the next event from the (character) stream.
    //! \param Str - input stream
    static Event* get( InStream& Str, size_t& Count );
    ChannelEvent( StatusCode Status0, int Channel0 ) : Status( Status0 ), Channel( Channel0 ) {}
    StatusCode status() const { return Status; }
    int channel() const { return Channel; }

    void print( std::ostream& Stream );
  }; // ChannelEvent

  class NoteEvent : public ChannelEvent
  {
    int Note;
    int Velocity;
  public:
    enum { PercussionFirst = 35, PercussionLast = 81 };
    static const char* Percussion[ PercussionLast-PercussionFirst+1 ];
    NoteEvent( StatusCode Status0, int Channel0, int Note0, int Velocity0 ) : ChannelEvent( Status0, Channel0 ), Note( Note0 ), Velocity( Velocity0 ) {}
    int note() const { return Note; }
    int velocity() const { return Velocity; }

    void print( std::ostream& Stream );
    void play( Sequencer& S );
    int write( std::ostream& File ) const;
  }; // NoteEvent

  class ControlChangeEvent : public ChannelEvent
  {
    int Control;
    int Value;
  public:
    ControlChangeEvent( int Channel0, int Control0, int Value0 ) : ChannelEvent( ControlChange, Channel0 ), Control( Control0 ), Value( Value0 ) {}
    int control() const { return Control; }
    int value() const { return Value; }

    void print( std::ostream& Stream );
    void play( Sequencer& S );
    int write( std::ostream& File ) const;
  }; // ControlChangeEvent

  class ProgramChangeEvent : public ChannelEvent
  {
    int Program;
  public:
    static const char* Instruments[ 128 ];
    static const char* Families[ 16 ];
    ProgramChangeEvent( int Channel0, int Program0 ) : ChannelEvent( ProgramChange, Channel0 ), Program( Program0 ) {}
    int program() const { return Program; }

    void print( std::ostream& Stream );
    void play( Sequencer& S );
    int write( std::ostream& File ) const;
  }; // ProgramChangeEvent

  class ChannelAfterTouchEvent : public ChannelEvent
  {
    int Velocity;
  public:
    ChannelAfterTouchEvent( int Channel0, int Velocity0 ) : ChannelEvent( ChannelAfterTouch, Channel0 ), Velocity( Velocity0 ) {}
    int velocity() const { return Velocity; }

    void print( std::ostream& Stream );
    int write( std::ostream& File ) const;
  }; // ChannelAfterTouchEvent

  class PitchBendEvent : public ChannelEvent
  {
    int Bend;
  public:
    PitchBendEvent( int Channel0, int Bend0 ) : ChannelEvent( PitchBend, Channel0 ), Bend( Bend0 ) {}
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
#if 0
    static Event* parse( const unsigned char*& Pos, int& ToGo );
#endif
    static Event* get( InStream& Str, size_t& Count );
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
    static TextEvent* get_text( InStream& Str, size_t Length, uint8_t Type );
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
    int Numerator;	// Число долей (например 6 для 6/8)
    int Denominator;	// Размер доли, как степень 2 (например, 3 для 6/8)
    int ClickClocks;	// Число "тиков" на удар метронома 
    int ThirtySeconds;	// Число тридцатьвторых нот на 24 "тика", обычно 8. Не все программы позволяют менять эти значения, тем не менее, они могут встречаться.
  public:
    TimeSignatureEvent( int Numerator0, int Denominator0, int ClickClocks0, int ThirtySeconds0 = 8 )
      : Numerator( Numerator0 ), Denominator( Denominator0 ), ClickClocks( ClickClocks0 ), ThirtySeconds( ThirtySeconds0 ) {}
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
    int Tonal;	// Число бемолей (<0) или диезов (>0) при ключе. 0 - C dur / A mol
    bool Minor;	// Признак минорной тональности
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
    ~SequencerMetaEvent() { if( Data )	delete [] Data; }
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
#if 0
    static Event* parse( const unsigned char*& Pos, int& ToGo );
#endif
    static Event* get( InStream& Str, size_t& Count );
    SysExEvent( int Length0 = 0, unsigned char* Data0 = 0 ) : Length( Length0 ), Data( Data0 ) {}
    ~SysExEvent() { if( Data ) delete [] Data; }
    int length() const { return Length; }
    const unsigned char* data() const { return Data; }
    void print( std::ostream& Stream );
    int write( std::ostream& File ) const;
  }; // SysExEvent
} // MuTraMIDI
#endif // MUTRA_MIDI_EVENTS_HPP
