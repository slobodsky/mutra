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
    StatusCode Status; //!< \todo Use status without channel and move it to Event.
    ChannelEvent( const ChannelEvent& Other ) : Event( Other ), Status( Other.Status ), Channel( Other.Channel ) {}
  public:
    // static Event* parse( const unsigned char*& Pos, int& ToGo );
    //! Get the next event from the (character) stream.
    //! \param Str - input stream
    static Event* get( InStream& Str, size_t& Count );
    ChannelEvent( StatusCode Status0, int Channel0, TimeuS Time0 = -1 ) : Event( Time0 ), Status( Status0 ), Channel( Channel0 ) {}
    StatusCode status() const { return StatusCode( Status & EventMask ); }
    int channel() const { return Channel; }

    void print( std::ostream& Stream ) const;
    Event* clone() const { return new ChannelEvent( *this ); }
  }; // ChannelEvent

  class NoteEvent : public ChannelEvent
  {
    int Note;
    int Velocity;
  protected:
    NoteEvent( const NoteEvent& Other ) : ChannelEvent( Other ), Note( Other.Note ), Velocity( Other.Velocity ) {}
  public:
    enum { PercussionFirst = 35, PercussionLast = 81 };
    static const char* Percussion[ PercussionLast-PercussionFirst+1 ];
    NoteEvent( StatusCode Status0, int Channel0, int Note0, int Velocity0, TimeuS Time0 = -1 ) : ChannelEvent( Status0, Channel0, Time0 ), Note( Note0 ), Velocity( Velocity0 ) {}
    int note() const { return Note; }
    int velocity() const { return Velocity; }

    Event* clone() const { return new NoteEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const;
  }; // NoteEvent

  class ControlChangeEvent : public ChannelEvent
  {
    int Control;
    int Value;
  protected:
    ControlChangeEvent( const ControlChangeEvent& Other ) : ChannelEvent( Other ), Control( Other.Control ), Value( Other.Value ) {}
  public:
    ControlChangeEvent( int Channel0, int Control0, int Value0, TimeuS Time0 = -1 ) : ChannelEvent( ControlChange, Channel0, Time0 ), Control( Control0 ), Value( Value0 ) {}
    int control() const { return Control; }
    int value() const { return Value; }

    Event* clone() const { return new ControlChangeEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const;
  }; // ControlChangeEvent

  class ProgramChangeEvent : public ChannelEvent
  {
    int Program;
  protected:
    ProgramChangeEvent( const ProgramChangeEvent& Other ) : ChannelEvent( Other ), Program( Other.Program ) {}
  public:
    static const char* Instruments[ 128 ];
    static const char* Families[ 16 ];
    ProgramChangeEvent( int Channel0, int Program0, TimeuS Time0 = -1 ) : ChannelEvent( ProgramChange, Channel0, Time0 ), Program( Program0 ) {}
    int program() const { return Program; }

    Event* clone() const { return new ProgramChangeEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const;
  }; // ProgramChangeEvent

  class ChannelAfterTouchEvent : public ChannelEvent
  {
    int Velocity;
  protected:
    ChannelAfterTouchEvent( const ChannelAfterTouchEvent& Other ) : ChannelEvent( Other ), Velocity( Other.Velocity ) {}
  public:
    ChannelAfterTouchEvent( int Channel0, int Velocity0, TimeuS Time0 = -1 ) : ChannelEvent( ChannelAfterTouch, Channel0, Time0 ), Velocity( Velocity0 ) {}
    int velocity() const { return Velocity; }

    Event* clone() const { return new ChannelAfterTouchEvent( *this ); }
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // ChannelAfterTouchEvent

  class PitchBendEvent : public ChannelEvent
  {
    int Bend;
  protected:
    PitchBendEvent( const PitchBendEvent& Other ) : ChannelEvent( Other ), Bend( Other.Bend ) {}
  public:
    PitchBendEvent( int Channel0, int Bend0, TimeuS Time0 = -1 ) : ChannelEvent( PitchBend, Channel0, Time0 ), Bend( Bend0 ) {}
    int bend() const { return Bend; }

    Event* clone() const { return new PitchBendEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const;
  }; // PitchBendEvent

  class MetaEvent : public Event
  {
  protected:
    MetaEvent( const MetaEvent& Other ) : Event( Other ) {}
  public:
    MetaEvent( Event::TimeuS Time = -1 ) : Event( Time ) {}
    enum MetaType { SequenceNumber = 0x00, Text = 0x01, Copyright = 0x02, TrackName = 0x03, InstrumentName = 0x04,
		    Lyric = 0x05, Marker = 0x06, CuePoint = 0x07, ChannelPrefix = 0x20, TrackEnd = 0x2F, Tempo = 0x51,
		    SMTPEOffset = 0x54, TimeSignature = 0x58, KeySignature = 0x59, SequencerMeta = 0x7F };
    static Event* get( InStream& Str, size_t& Count );
    Event* clone() const { return new MetaEvent( *this ); }
    void print( std::ostream& Stream ) const;
  }; // MetaEvent

  //! \todo Here and below: consider sharing data and using copy on write.
  class UnknownMetaEvent : public MetaEvent
  {
    int Type;
    int Length;
    unsigned char* Data;
  protected:
    UnknownMetaEvent( const UnknownMetaEvent& Other );
  public:
    UnknownMetaEvent( int Type0, int Length0, unsigned char* Data0, TimeuS Time0 = -1 );
    ~UnknownMetaEvent();
    Event* clone() const { return new UnknownMetaEvent( *this ); }
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // UnknownMetaEvent

  class SequenceNumberEvent : public MetaEvent
  {
    int Number;
  protected:
    SequenceNumberEvent( const SequenceNumberEvent& Other ) : MetaEvent( Other ), Number( Other.Number ) {}
  public:
    SequenceNumberEvent( int Number0, TimeuS Time0 = -1 ) : MetaEvent( Time0 ), Number( Number0 ) {}
    Event* clone() const { return new SequenceNumberEvent( *this ); }
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // SequenceNumberEvent

  class TextEvent : public MetaEvent
  {
    std::string Text;
  protected:
    TextEvent( const TextEvent& Other ) : MetaEvent( Other ), Text( Other.Text ) {}
  public:
    static TextEvent* get_text( InStream& Str, size_t Length, uint8_t Type );
    TextEvent( const std::string& Text0, TimeuS Time0 = -1 ) : MetaEvent( Time0 ), Text( Text0 ) {}
    const std::string& text() const { return Text; }
    Event* clone() const { return new TextEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const { return write_text( File, MetaEvent::Text ); }
    int write_text( std::ostream& File, int Type ) const;
  }; // TextEvent
  class CopyrightEvent : public TextEvent
  {
    CopyrightEvent( const TextEvent& Other ) : TextEvent( Other ) {}
  public:
    CopyrightEvent( const std::string& Text0, TimeuS Time0 = -1 ) : TextEvent( Text0, Time0 ) {}
    Event* clone() const { return new CopyrightEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const { return write_text( File, MetaEvent::Copyright ); }
  }; // CopyrightEvent
  class TrackNameEvent : public TextEvent
  {
  protected:
    TrackNameEvent( const TrackNameEvent& Other ) : TextEvent( Other ) {}
  public:
    TrackNameEvent( const std::string& Text0, TimeuS Time0 = -1 ) : TextEvent( Text0, Time0 ) {}
    Event* clone() const { return new TrackNameEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const { return write_text( File, MetaEvent::TrackName ); }
  }; // TrackNameEvent
  class InstrumentNameEvent : public TextEvent
  {
  protected:
    InstrumentNameEvent( const InstrumentNameEvent& Other ) : TextEvent( Other ) {}
  public:
    InstrumentNameEvent( const std::string& Text0, TimeuS Time0 = -1 ) : TextEvent( Text0, Time0 ) {}
    Event* clone() const { return new InstrumentNameEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const { return write_text( File, MetaEvent::InstrumentName ); }
  }; // InstrumentNameEvent
  class LyricEvent : public TextEvent
  {
  protected:
    LyricEvent( const LyricEvent& Other ) : TextEvent( Other ) {}
  public:
    LyricEvent( const std::string& Text0, TimeuS Time0 = -1 ) : TextEvent( Text0, Time0 ) {}
    Event* clone() const { return new LyricEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const { return write_text( File, MetaEvent::Lyric ); }
  }; // LyricEvent
  class MarkerEvent : public TextEvent
  {
  protected:
    MarkerEvent( const MarkerEvent& Other ) : TextEvent( Other ) {}
  public:
    MarkerEvent( const std::string& Text0, TimeuS Time0 = -1 ) : TextEvent( Text0, Time0 ) {}
    Event* clone() const { return new MarkerEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const { return write_text( File, MetaEvent::Marker ); }
  }; // MarkerEvent
  class CuePointEvent : public TextEvent
  {
  protected:
    CuePointEvent( const CuePointEvent& Other ) : TextEvent( Other ) {}
  public:
    CuePointEvent( const std::string& Text0, TimeuS Time0 = -1 ) : TextEvent( Text0, Time0 ) {}
    Event* clone() const { return new CuePointEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const { return write_text( File, MetaEvent::CuePoint ); }
  }; // CuePointEvent

  class ChannelPrefixEvent : public MetaEvent
  {
    int Channel;
  protected:
    ChannelPrefixEvent( const ChannelPrefixEvent& Other ) : MetaEvent( Other ), Channel( Other.Channel ) {}
  public:
    ChannelPrefixEvent( int Channel0, TimeuS Time0 = -1 ) : MetaEvent( Time0 ), Channel( Channel0 ) {}
    int channel() const { return Channel; }
    Event* clone() const { return new ChannelPrefixEvent( *this ); }
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // ChannelPrefixEvent

  class TrackEndEvent : public MetaEvent
  {
  protected:
    TrackEndEvent( const TrackEndEvent& Other ) : MetaEvent( Other ) {}
  public:
    TrackEndEvent( TimeuS Time0 = -1 ) : MetaEvent( Time0 ) {}
    Event* clone() const { return new TrackEndEvent( *this ); }
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // TrackEndEvent

  class TempoEvent : public MetaEvent
  {
    int Tempo; // Микросекунд на четвертную ноту
  protected:
    TempoEvent( const TempoEvent& Other ) : MetaEvent( Other ), Tempo( Other.Tempo ) {}
  public:
    TempoEvent( int Tempo0, TimeuS Time0 = -1 ) : MetaEvent( Time0 ), Tempo( Tempo0 ) {}
    int tempo() const { return Tempo; }
    Event* clone() const { return new TempoEvent( *this ); }
    void print( std::ostream& Stream ) const;
    void play( Sequencer& S ) const;
    int write( std::ostream& File ) const;
  }; // TempoEvent

  class SMTPEOffsetEvent : public MetaEvent
  {
    int Hour;
    int Minute;
    int Second;
    int Frame;
    int Hundredths;
  protected:
    SMTPEOffsetEvent( const SMTPEOffsetEvent& Other ) : MetaEvent( Other ), Hour( Other.Hour ), Minute( Other.Minute ), Second( Other.Second ), Frame( Other.Frame ), Hundredths( Other.Hundredths ) {}
  public:
    SMTPEOffsetEvent( int Hour0 = 0, int Minute0 = 0, int Second0 = 0, int Frame0 = 0, int Hundredths0 = 0, TimeuS Time0 = -1 )
      : MetaEvent( Time0 ), Hour( Hour0 ), Minute( Minute0 ), Second( Second0 ), Frame( Frame0 ), Hundredths( Hundredths0 ) {}
    int hour() const { return Hour; }
    int minute() const { return Minute; }
    int second() const { return Second; }
    int frame() const { return Frame; }
    int hundredths() const { return Hundredths; }

    Event* clone() const { return new SMTPEOffsetEvent( *this ); }
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // SMTPEOffsetEvent

  class TimeSignatureEvent : public MetaEvent
  {
    int Numerator;	// Число долей (например 6 для 6/8)
    int Denominator;	// Размер доли, как степень 2 (например, 3 для 6/8)
    int ClickClocks;	// Число "тиков" на удар метронома 
    int ThirtySeconds;	// Число тридцатьвторых нот на 24 "тика", обычно 8. Не все программы позволяют менять эти значения, тем не менее, они могут встречаться.
  protected:
    TimeSignatureEvent( const TimeSignatureEvent& Other )
      : MetaEvent( Other ), Numerator( Other.Numerator ), Denominator( Other.Denominator ), ClickClocks( Other.ClickClocks ), ThirtySeconds( Other.ThirtySeconds ) {}
  public:
    TimeSignatureEvent( int Numerator0, int Denominator0, int ClickClocks0, int ThirtySeconds0 = 8, TimeuS Time0 = -1 )
      : MetaEvent( Time0 ), Numerator( Numerator0 ), Denominator( Denominator0 ), ClickClocks( ClickClocks0 ), ThirtySeconds( ThirtySeconds0 ) {}
    int numerator() const { return Numerator; }
    int denominator() const { return Denominator; }
    int click_clocks() const { return ClickClocks; }
    int thirtyseconds() const { return ThirtySeconds; }

    Event* clone() const { return new TimeSignatureEvent( *this ); }
    void play( Sequencer& S ) const;
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // TimeSignatureEvent

  class KeySignatureEvent : public MetaEvent
  {
    int Tonal;	// Число бемолей (<0) или диезов (>0) при ключе. 0 - C dur / A mol
    bool Minor;	// Признак минорной тональности
  protected:
    KeySignatureEvent( const KeySignatureEvent& Other ) : MetaEvent( Other ), Tonal( Other.Tonal ), Minor( Other.Minor ) {}
  public:
    KeySignatureEvent( int Tonal0, bool Minor0, TimeuS Time0 = -1 ) : MetaEvent( Time0 ), Tonal( Tonal0 ), Minor( Minor0 ) {}
    int tonal() const { return Tonal; }
    bool minor() const { return Minor; }

    Event* clone() const { return new KeySignatureEvent( *this ); }
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // KeySignatureEvent

  class SequencerMetaEvent : public MetaEvent
  {
    int Length;
    unsigned char* Data;
  protected:
    SequencerMetaEvent( const SequencerMetaEvent& Other );
  public:
    SequencerMetaEvent( int Length0 = 0, unsigned char* Data0 = 0, TimeuS Time0 = -1 ) : MetaEvent( Time0 ), Length( Length0 ), Data( Data0 ) {}
    ~SequencerMetaEvent() { if( Data )	delete [] Data; }
    int length() const { return Length; }
    const unsigned char* data() const { return Data; }
    Event* clone() const { return new SequencerMetaEvent( *this ); }
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // SequencerMetaEvent

  class SysExEvent : public Event
  {
    int Length;
    unsigned char* Data;
  protected:
    SysExEvent( const SysExEvent& Other );
  public:
    static Event* get( InStream& Str, size_t& Count );
    SysExEvent( int Length0 = 0, unsigned char* Data0 = 0, TimeuS Time0 = -1 ) : Event( Time0 ), Length( Length0 ), Data( Data0 ) {}
    ~SysExEvent() { if( Data ) delete [] Data; }
    int length() const { return Length; }
    const unsigned char* data() const { return Data; }
    Event* clone() const { return new SysExEvent( *this ); }
    void print( std::ostream& Stream ) const;
    int write( std::ostream& File ) const;
  }; // SysExEvent
} // MuTraMIDI
#endif // MUTRA_MIDI_EVENTS_HPP
