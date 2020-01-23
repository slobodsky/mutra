#include "midi_events.hpp"
#include <iomanip>
using std::istream;
using std::ostream;
using std::hex;
using std::dec;
using std::setfill;
using std::setw;
using std::cout;
using std::endl;
using std::string;
using std::cerr;

namespace MuTraMIDI {
#if 0
  Event* MIDIEvent::parse( const unsigned char*& Pos, int& ToGo ) {
    unsigned char EventCode = *Pos & 0xF0;
    int Channel = *Pos & 0x0F;
    switch( EventCode ) // В зависимости от сообщения
    {
    case 0x80: // Note off
    case 0x90: // Note on
    case 0xA0: // After touch
      if( ToGo < 3 ) return nullptr;
      else {
	unsigned char Note = Pos[1];
	unsigned char Velocity = Pos[2];
	Pos += 3;
	ToGo -= 3;
	return new NoteEvent( MIDIEvent::MIDIStatus( EventCode ), Channel, Note, Velocity );
      }
    case 0xB0: // Control change
      if( ToGo < 3 ) return nullptr;
      else {
	unsigned char Control = Pos[1];
	unsigned char Value = Pos[2];
	Pos += 3;
	ToGo -= 3;
	return new ControlChangeEvent( Channel, Control, Value );
      }
    case 0xC0: // Program Change
      if( ToGo < 2 ) return nullptr;
      else {
	unsigned char Program = Pos[1];
	Pos += 2;
	ToGo -= 2;
	return new ProgramChangeEvent( Channel, Program );
      }
    case 0xD0: // Channel AfterTouch
      if( ToGo < 2 ) return nullptr;
      else {
	unsigned char Value = Pos[1];
	Pos += 2;
	ToGo -= 2;
	return new ChannelAfterTouchEvent( Channel, Value );
      }
    case 0xE0: // Pitch bend
      if( ToGo < 3 ) return nullptr;
      else {
	int Bend = Pos[1] | ( Pos[2] << 7 );
	Pos += 3;
	ToGo -= 3;
	return new PitchBendEvent( Channel, Bend );
      }
    }
    return nullptr;
  } // parse( unsigned char&*, int& )
#endif // 0
  Event* ChannelEvent::get( InStream& Str, size_t& Count ) {
    uint8_t Status = Str.status();
#if 0
    cout << "Get Channel event. Status " << hex << int(Status) << " in " << int(In) << " to go " << dec << ToGo << endl;
#endif
    uint8_t EventCode = Status & EventMask;
    int Channel = Status & ChannelMask;
    switch( EventCode ) { // В зависимости от сообщения
    case NoteOff: // Note off
    case NoteOn: // Note on
    case AfterTouch: { // After touch
      //! \todo Maybe check if note & velocity are in [0-127] (in other cases too)
      int Note = Str.get();
      int Velocity = Str.get();
      Count += 2;
      return new NoteEvent( ChannelEvent::StatusCode( EventCode ), Channel, Note, Velocity );
    }
    case ControlChange: {
      int Control = Str.get();
      int Value = Str.get();
      Count += 2;
      return new ControlChangeEvent( Channel, Control, Value );
    }
    case ProgramChange: {
      ++Count;
      return new ProgramChangeEvent( Channel, Str.get() );
    }
    case ChannelAfterTouch: {
      ++Count;
      return new ChannelAfterTouchEvent( Channel, Str.get() );
    }
    case PitchBend: {
	int Bend = Str.get();
	Bend |= Str.get() << 7;
	Count += 2;
	return new PitchBendEvent( Channel, Bend );
      }
    }
    cerr << "Unknown channel event " << hex << Status << endl;
    return nullptr;
  } // get( istream&, int&, unsigned char& )  
  void ChannelEvent::print( ostream& Stream ) { Stream << "MIDI сообщение " << hex << Status << ", канал " << dec << Channel; }
  
  void NoteEvent::print( ostream& Stream )
  { 
    switch( Status )
    {
    case NoteOff:
      Stream << "Note Off";
      break;
    case NoteOn:
      Stream << "Note On";
      break;
    case AfterTouch:
      Stream << "After Touch";
      break;
    default:
      Stream << "!!! Неизвестное сообщение: " << hex << Status << dec;
      break;
    }
    Stream << ", канал " << Channel << ", нота " << Note << ", сила " << Velocity;
  } // print( std::ostream& )
  void NoteEvent::play( Sequencer& S )
  {
    switch( Status )
    {
    case NoteOff:
      S.note_off( Channel, Note, Velocity );
      break;
    case NoteOn:
      S.note_on( Channel, Note, Velocity );
      break;
    case AfterTouch:
      S.after_touch( Channel, Note, Velocity );
      break;
    }
  } // play( Sequencer& )
  int NoteEvent::write( std::ostream& File ) const
  {
    File.put( Status | Channel );
    File.put( Note );
    File.put( Velocity );
    return 3;
  } // write( std::ostream& ) const
  void ControlChangeEvent::print( ostream& Stream )
  { Stream << "Control Change, канал " << Channel << ", контроллер " << Control << ", значение " << Value; }
  void ControlChangeEvent::play( Sequencer& S ) { S.control_change( Channel, Control, Value ); }
  int ControlChangeEvent::write( std::ostream& File ) const
  {
    File.put( Status | Channel );
    File.put( Control );
    File.put( Value );
    return 3;
  } // write( std::ostream& ) const
  void ProgramChangeEvent::print( ostream& Stream )
  { Stream << "Program Change, канал " << Channel << ", " << Instruments[ Program ] << " (" << Program << ")"; }
  void ProgramChangeEvent::play( Sequencer& S ) { S.program_change( Channel, Program ); }
  int ProgramChangeEvent::write( std::ostream& File ) const
  {
    File.put( Status | Channel );
    File.put( Program );
    return 2;
  } // write( std::ostream& ) const
  void ChannelAfterTouchEvent::print( ostream& Stream ) { Stream << "Channel After Touch, канал " << Channel << ", значение " << Velocity; }
  int ChannelAfterTouchEvent::write( std::ostream& File ) const
  {
    File.put( Status | Channel );
    File.put( Velocity );
    return 2;
  } // write( std::ostream& ) const
  void PitchBendEvent::print( ostream& Stream ) { Stream << "Pitch Bend, канал " << Channel << ", значение " << Bend; }
  void PitchBendEvent::play( Sequencer& S ) { S.pitch_bend( Channel, Bend ); }
  int PitchBendEvent::write( std::ostream& File ) const
  {
    File.put( Status | Channel );
    File.put( ( Bend >> 7 ) & 0x7F );
    File.put( Bend & 0x7F );
    return 3;
  } // write( std::ostream& ) const

#if 0
  Event* MetaEvent::parse( const unsigned char*& Pos, int& ToGo ) {
    int Count = ToGo;
    const unsigned char* P = Pos;
    if( Count < 3 ) return nullptr;
    unsigned char Type = P[1];
    P += 2;
    Count -= 2;
    int Length = get_var( P, Count );
    if( Count < 0 || Count < Length ) return nullptr;
    Pos = P + Length;
    ToGo = Count - Length;
    switch( Type )
    {
    case MetaEvent::SequenceNumber: // Sequence Number
      return new SequenceNumberEvent( get_int( P, Length ) );
    case MetaEvent::Text: // Text event
      return new TextEvent( string( (const char*)P, Length ) );
    case MetaEvent::Copyright: // Copyright
      return new CopyrightEvent( string( (const char*)P, Length ) );
    case MetaEvent::TrackName: // Sequence/track name
      return new TrackNameEvent( string( (const char*)P, Length ) );
    case MetaEvent::InstrumentName: // Instrument name
      return new InstrumentNameEvent( string( (const char*)P, Length ) );
    case MetaEvent::Lyric: // Lyric
      return new LyricEvent( string( (const char*)P, Length ) );
    case MetaEvent::Marker: // Marker
      return new MarkerEvent( string( (const char*)P, Length ) );
    case MetaEvent::CuePoint: // Cue point
      return new CuePointEvent( string( (const char*)P, Length ) );
    case MetaEvent::ChannelPrefix: // MIDI Channel Prefix
      //! \todo Check length
      return new ChannelPrefixEvent( P[0] );
      break;
    case MetaEvent::TrackEnd: // Track end
      //! \todo Check length
      return new TrackEndEvent();
    case MetaEvent::Tempo: // Tempo (us/quarter)
      return new TempoEvent( get_int( P, Length ) );
    case MetaEvent::SMTPEOffset: // SMTPE offset
      //! \todo Check length
      return new SMTPEOffsetEvent( P[0], P[1], P[2], P[3], P[4] );
    case MetaEvent::TimeSignature: // Time signature
      //! \todo Check length
      return new TimeSignatureEvent( P[0], P[1], P[2], P[3] );
    case MetaEvent::KeySignature: // Key signature
      //! \todo Check length
      return new KeySignatureEvent( P[0], P[1] != 0 );
    case MetaEvent::SequencerMeta: // Sequencer Meta-event
      { 
	unsigned char* Data = new unsigned char[ Length ];
	memcpy( Data, P, Length );
	return new SequencerMetaEvent( Length, Data );
      }
    default:
      {
	unsigned char* Data = new unsigned char[ Length ];
	memcpy( Data, P, Length );
	return new UnknownMetaEvent( Type, Length, Data );
      }
    }
    return nullptr;
  } // parse( const unsigned char*&, int& )
#endif
  Event* MetaEvent::get( InStream& Stream, size_t& Count ) {
    uint8_t Type = Stream.get();
    ++Count;
    int Length =  Stream.get_var();
    Count += Stream.count() + Length;
#if 0
    cout << "Get meta event " << int( Status ) << " type " << Type << " length " << Length << " to go " << ToGo << endl;
#endif
    switch( Type )
    {
    case MetaEvent::SequenceNumber: // Sequence Number
      return new SequenceNumberEvent( Stream.get_int( Length ) );
    // Different text events
    case MetaEvent::Text: // Text event
    case MetaEvent::Copyright: // Copyright
    case MetaEvent::TrackName: // Sequence/track name
    case MetaEvent::InstrumentName: // Instrument name
    case MetaEvent::Lyric: // Lyric
    case MetaEvent::Marker: // Marker
    case MetaEvent::CuePoint: // Cue point
      return TextEvent::get_text( Stream, Length, Type );
    case MetaEvent::ChannelPrefix: // MIDI Channel Prefix
      //! \note Length have to be 1. Maybe check it. (And other fixed size events below too.)
      return new ChannelPrefixEvent( Stream.get() );
    case MetaEvent::TrackEnd: // Track end
      return new TrackEndEvent();
    case MetaEvent::Tempo: // Tempo (us/quarter)
      return new TempoEvent( Stream.get_int( Length ) );
    case MetaEvent::SMTPEOffset: { // SMTPE offset
	int Hour	= Stream.get();
	int Min		= Stream.get();
	int Sec		= Stream.get();
	int Frames	= Stream.get();
	int Hundredths	= Stream.get();
	return new SMTPEOffsetEvent( Hour, Min, Sec, Frames, Hundredths );
      }
    case MetaEvent::TimeSignature: { // Time signature
	int Numerator		= Stream.get();
	int Denominator		= Stream.get();
	int ClickClocks		= Stream.get();
	int ThirtySeconds	= Stream.get();
	return new TimeSignatureEvent( Numerator, Denominator, ClickClocks, ThirtySeconds );
      }
    case MetaEvent::KeySignature: { // Key signature
        int Tonal	= Stream.get();
	int Minor	= Stream.get();
	return new KeySignatureEvent( Tonal, Minor != 0 );
      }
    default: {
      unsigned char* Data = new unsigned char[ Length ];
      Stream.read( Data, Length );
      if( Type == MetaEvent::SequencerMeta ) return new SequencerMetaEvent( Length, Data ); // Sequencer Meta-event
      else return new UnknownMetaEvent( Type, Length, Data );
    }
    }
    return nullptr;
  } // get( InStream&, size_t& )
  void MetaEvent::print( ostream& Stream ) { Stream << "!!! Неопознанное метасообщение"; }
  void SequenceNumberEvent::print( ostream& Stream ) { Stream << "Sequence Number " << Number; }
  int SequenceNumberEvent::write( std::ostream& File ) const
  {
    File.put( 0xFF );
    File.put( MetaEvent::SequenceNumber );
    File.put( 2 );
    put_int( File, 2, Number );
    return 1 + 1 + 1 + 2;
  } // write( std::ostream& ) const

  UnknownMetaEvent::UnknownMetaEvent( int Type0, int Length0, unsigned char* Data0 ) : Type( Type0 ), Length( Length0 ), Data( Data0 ) {} // Конструктор
  UnknownMetaEvent::~UnknownMetaEvent() { if( Data ) delete Data; }
  void UnknownMetaEvent::print( ostream& Stream )
  {
    Stream << "Неопознанное метасообщение " << setfill( '0' ) << setw( 2 ) << hex << (int)Type << " длиной " << dec << setw( 1 ) << Length
	   << " байт:" << hex << setfill( '0' );
    for( int I = 0; I < Length; I++ )
      Stream << ' ' << setw( 2 ) << (int)Data[ I ];
    Stream << dec;
  }
  int UnknownMetaEvent::write( std::ostream& File ) const
  {
    File.put( 0xFF );
    File.put( Type );
    int Result = 1 + 1 + put_var( File, Length ) + Length;
    File.write( reinterpret_cast<char*>( Data ), Length );
    return Result;
  } // write( std::ostream& ) const

  TextEvent* TextEvent::get_text( InStream& Str, size_t Length, uint8_t Type ) {
    char* Data = new char[ Length+1 ];
    Str.read( (unsigned char*)Data, Length );
    Data[ Length ] = 0;
    string Text = Data;
    delete Data;
    switch( Type ) {
    case MetaEvent::Text: return new TextEvent( Text ); // Text event
    case MetaEvent::Copyright: return new CopyrightEvent( Text ); // Copyright
    case MetaEvent::TrackName: return new TrackNameEvent( Text ); // Sequence/track name
    case MetaEvent::InstrumentName: return new InstrumentNameEvent( Text ); // Instrument name
    case MetaEvent::Lyric: return new LyricEvent( Text ); // Lyric
    case MetaEvent::Marker: return new MarkerEvent( Text ); // Marker
    case MetaEvent::CuePoint: return new CuePointEvent( Text ); // Cue point
    }
    return nullptr;
  } // get( istream&, int&, unsigned char& )
  void TextEvent::print( ostream& Stream ) { Stream << "Text: \"" << text() << "\""; }
  void TextEvent::play( Sequencer& S ) { S.text( text() ); }
  int TextEvent::write_text( std::ostream& File, int Type ) const
  {
    int Length = Text.length();
    File.put( 0xFF );
    File.put( Type );
    int Result = 1 + 1 + put_var( File, Length ) + Length;
    File << Text;
    return Result;
  } // write_text( std::ostream&, int ) const
  void CopyrightEvent::print( ostream& Stream ) { Stream << "Copyright: \"" << text() << "\""; }
  void CopyrightEvent::play( Sequencer& S ) { S.copyright( text() ); }
  void TrackNameEvent::print( ostream& Stream ) { Stream << "Track Name: \"" << text() << "\""; }
  void TrackNameEvent::play( Sequencer& S ) { S.name( text() ); }
  void InstrumentNameEvent::print( ostream& Stream ) { Stream << "Инструмент: \"" << text() << "\""; }
  void InstrumentNameEvent::play( Sequencer& S ) { S.instrument( text() ); }
  void LyricEvent::print( ostream& Stream ) { Stream << "Стихи: \"" << text() << "\""; }
  void LyricEvent::play( Sequencer& S ) { S.lyric( text() ); }
  void MarkerEvent::print( ostream& Stream ) { Stream << "Маркер: \"" << text() << "\""; }
  void MarkerEvent::play( Sequencer& S ) { S.marker( text() ); }
  void CuePointEvent::print( ostream& Stream ) { Stream << "Точка подсказки: \"" << text() << "\""; }
  void CuePointEvent::play( Sequencer& S ) { S.cue( text() ); }

  void ChannelPrefixEvent::print( ostream& Stream ) { Stream << "Префикс канала: " << channel(); }
  int ChannelPrefixEvent::write( std::ostream& File ) const
  {
    File.put( 0xFF );
    File.put( MetaEvent::ChannelPrefix );
    File.put( 1 );
    File.put( Channel );
    return 1 + 1 + 1 + 1;
  } // write( std::ostream& ) const
  void TrackEndEvent::print( ostream& Stream ) { Stream << "Конец дорожки."; }
  int TrackEndEvent::write( std::ostream& File ) const
  {
    File.put( 0xFF );
    File.put( MetaEvent::TrackEnd );
    File.put( 0 );
    return 1 + 1 + 1;
  } // write( std::ostream& ) const
  void TempoEvent::print( ostream& Stream ) { Stream << "Длительность четвертной ноты " << tempo() << " микросекунд"; }
  void TempoEvent::play( Sequencer& S ) { S.tempo( tempo() ); }
  int TempoEvent::write( std::ostream& File ) const
  {
    File.put( 0xFF );
    File.put( MetaEvent::Tempo );
    File.put( 3 );
    put_int( File, 3, Tempo );
    return 1 + 1 + 1 + 3;
  } // write( std::ostream& ) const
  void SMTPEOffsetEvent::print( ostream& Stream )
  { Stream << "Задержка дорожки: " << setfill( '0' ) << setw( 2 ) << hour() << ':' << minute() << ':' << second() << ':' << frame() << '.' << hundredths(); }
  int SMTPEOffsetEvent::write( std::ostream& File ) const
  {
    File.put( 0xFF );
    File.put( MetaEvent::SMTPEOffset );
    File.put( 5 );
    File.put( Hour );
    File.put( Minute );
    File.put( Second );
    File.put( Frame );
    File.put( Hundredths );
    return 1 + 1 + 1 + 5;
  } // write( std::ostream& ) const
  void TimeSignatureEvent::play( Sequencer& S ) { S.meter( Numerator, Denominator ); }
  void TimeSignatureEvent::print( ostream& Stream )
  { 
    Stream << "Time Signature: " << numerator() << '/' << ( 1 << denominator() ) << ", метроном: " << click_clocks() << ", в одной четверти "
	   << thirtyseconds() << " тридцатьвторых";
  }
  int TimeSignatureEvent::write( std::ostream& File ) const
  {
    File.put( 0xFF );
    File.put( MetaEvent::TimeSignature );
    File.put( 4 );
    File.put( Numerator );
    File.put( Denominator );
    File.put( ClickClocks );
    File.put( ThirtySeconds );
    return 1 + 1 + 1 + 4;
  } // write( std::ostream& ) const
  void KeySignatureEvent::print( ostream& Stream )
  {
    Stream << "Тональность " << ( minor() ? "минорная" : "мажорная" ) << ", при ключе ";
    if( tonal() < 0 )
      Stream << -tonal() << " бемолей";
    else
      Stream << tonal() << " диезов";
  }
  int KeySignatureEvent::write( std::ostream& File ) const
  {
    File.put( 0xFF );
    File.put( MetaEvent::KeySignature );
    File.put( 2 );
    File.put( Tonal );
    File.put( Minor );
    return 1 + 1 + 1 + 2;
  } // write( std::ostream& ) const
  void SequencerMetaEvent::print( ostream& Stream )
  {
    Stream << "Metaсообщение секвенсору (" << length() << " байт):" << hex << setfill( '0' ) << setw( 2 );
    for( int I = 0; I < length(); I++ )
      Stream << ' ' << int( Data[ I ] );
    Stream << dec;
  }
  int SequencerMetaEvent::write( std::ostream& File ) const
  {
    File.put( 0xFF );
    File.put( MetaEvent::SequencerMeta );
    int Result = 1 + 1 + put_var( File, Length ) + Length;
    File.write( reinterpret_cast<char*>( Data ), Length );
    return Result;
  } // write( std::ostream& ) const

#if 0
  Event* SysExEvent::parse( const unsigned char*& Pos, int& ToGo ) {
    if( ToGo < 2 ) return nullptr;
    const unsigned char* P = Pos+1;
    int Count = ToGo;
    int Length = get_var( P, Count );
    if( Count < 0 || Count < Length ) return nullptr;
    unsigned char* Data = new unsigned char[ *Pos == 0xF0 ? Length+1 : Length ];
    if( *Pos == 0xF0 )
    {
      Data[ 0 ] = 0xF0;
      memcpy( Data+1, P, Length );
    }
    else
      memcpy( Data, P, Length );
    Pos = P + Length;
    ToGo = Count - Length;
    return new SysExEvent( Length, Data );
  } // parse( const unsigned char*&, int& )
#endif
  Event* SysExEvent::get( InStream& Stream, size_t& Count ) {
    int Length = Stream.get_var();
    Count += Stream.count();
#if 0
    cout << "Get sysex event " << hex << int( Status ) << " length " << dec << Length << " to go " << ToGo << endl;
#endif
    unsigned char* Data = 0;
    int Offset = 0;
    if( Stream.status() == 0xF0 )
    {
      Data = new unsigned char[ Length+1 ];
      Data[ 0 ] = 0xF0;
      Offset = 1;
    }
    else
      Data = new unsigned char[ Length ];
    Stream.read( Data+Offset, Length );
    Count += Length;
    return new SysExEvent( Length, Data );
  } // get( InStream&, size_t& )
  void SysExEvent::print( ostream& Stream )
  {
    Stream << "SysEx (" << length() << " байт):" << hex << setfill( '0' ) << setw( 2 );
    for( int I = 0; I < length(); I++ )
      Stream << ' ' << int( Data[ I ] );
    Stream << dec;
  }
  int SysExEvent::write( std::ostream& File ) const
  {
    int Result = 1;
    if( *Data == 0xF0 )
    {
      File.put( 0xF0 );
      Result += put_var( File, Length-1 ) + Length-1;
      File.write( reinterpret_cast<char*>( Data+1 ), Length-1 );
    }
    else
    {
      File.put( 0xF7 );
      Result += put_var( File, Length ) + Length;
      File.write( reinterpret_cast<char*>( Data ), Length );
    }
    return Result;
  } // write( std::ostream& ) const

  const char* NoteEvent::Percussion[ PercussionLast-PercussionFirst+1 ] =
    { "Bass Drum", "Bass Drum 1", "Side Stick", "Acoustic Snare", "Hand Clap", "Electric Snare", "Low Floor Tom", "Closed Hi-Hat", "High Floor Tom",
      "Pedal Hi-Hat", "Low Tom", "Open Hi-Hat", "Low-Mid Tom", "Hi-Mid Tom", "Crash Cymbal 1", "High Tom", "Ride Cymbal 1", "Chinese Cymbal",
      "Ride Bell", "Tambourine", "Splash Cymbal", "Cowbell", "Crash Cymbal 2", "Vibraslap", "Ride Cymbal 2", "Hi Bongo", "Low Bongo", "Mute Hi Conga",
      "Open Hi Conga", "Low Conga", "High Timbale", "Low Timbale", "High Agogo", "Low Agogo", "Cabasa", "Maracas", "Short Whistle", "Long Whistle",
      "Short Guiro", "Long Guiro", "Claves", "Hi Wood Block", "Low Wood Block", "Mute Cuica", "Open Cuica", "Mute Triangle", "Open Triangle" };

  const char* ProgramChangeEvent::Instruments[ 128 ] =
    { "Acoustic Grand", "Bright Acoustic", "Electric Grand", "Honky-Tonk", "Electric Piano1", "Electric Piano2", "Harpsichord", "Clav", "Celesta",
      "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer", "Drawbar Organ", "Percussive Organ",
      "Rock Organ", "Church Organ", "Reed Organ", "Accoridan", "Harmonica", "Tango Accordian", "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)",
      "Electric Guitar (jazz)", "Electric Guitar (clean)", "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar Harmonics",
      "Acoustic Bass", "Electric Bass (finger)	", "Electric Bass (pick)", "Fretless Bass", "Slap Bass 1", "Slap Bass 2", "Synth Bass 1",
      "Synth Bass 2", "Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings", "Pizzicato Strings", "Orchestral Strings", "Timpani",
      "String Ensemble 1", "String Ensemble 2", "SynthStrings 1", "SynthStrings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
      "Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "SynthBrass 1", "SynthBrass 2", "Soprano Sax", "Alto Sax",
      "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet", "Piccolo", "Flute", "Recorder", "Pan Flute", "Blown Bottle",
      "Skakuhachi", "Whistle", "Ocarina", "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)", "Lead 5 (charang)",
      "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass+lead)", "Pad 1 (new age)	", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)",
      "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)", "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)",
      "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)", "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba", "Bagpipe", "Fiddle",
      "Shanai", "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal", "Guitar Fret Noise",
      "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter", "Applause", "Gunshot" };
  const char* ProgramChangeEvent::Families[ 16 ] = { "PIANO", "CHROM PERCUSSION", "ORGAN", "GUITAR", "BASS", "STRINGS", "ENSEMBLE", "BRASS",
						     "REED", "PIPE", "SYNTH LEAD", "SYNTH PAD", "SYNTH EFFECTS", "ETHNIC", "PERCUSSIVE", "SOUND EFFECTS" };
} // MuTraMIDI
