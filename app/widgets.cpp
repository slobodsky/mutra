#include <QSettings>
#include <QFileDialog>
#include <QToolBar>
#include <QMessageBox>
#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QDBusError>
#include <QAction>
#include <QActionGroup>
#include <QGraphicsSvgItem>
//#include <QGraphicsItemGroup>
#include <queue>
#include "widgets.hpp"
#include <QDebug>
#include "forms/ui_main.h"
#include "forms/ui_settings_dialog.h"
#include "forms/ui_metronome_settings.h"
#include "forms/ui_devices_settings.h"
#include "forms/ui_exercise_settings.h"
#include "forms/ui_notes_exercise_settings.h"
#include "forms/ui_midi_mixer.h"
using MuTraMIDI::get_time_us;
using MuTraMIDI::InputDevice;
using MuTraMIDI::Sequencer;
using MuTraMIDI::MIDISequence;
using MuTraMIDI::NoteEvent;
using MuTraMIDI::InputConnector;
using MuTraMIDI::Transposer;
using MuTraMIDI::Names;
using MuTraMIDI::MIDIBackend;
using MuTraTrain::MetronomeOptions;
using MuTraTrain::ExerciseSequence;
using MuTraTrain::NoteTrainer;
using MuTraTrain::Lesson;
using std::queue;
using std::vector;
using std::string;
using std::swap;

namespace MuTraWidgets {
  void load_from_settings( SystemOptions& Options ) {
    QSettings Set;
    Options.midi_input( Set.value( "System/MIDIInput", QString::fromStdString( Options.midi_input() ) ).toString().toStdString() )
      .midi_output( Set.value( "System/MIDIOutput", QString::fromStdString( Options.midi_output() ) ).toString().toStdString() )
      .midi_echo( Set.value( "System/MIDIEcho", Options.midi_echo() ).toBool() ).transpose( Set.value( "System/Transpose", Options.transpose() ).toBool() )
      .halftones( Set.value( "System/HalfTones", Options.halftones() ).toInt() );
  } // load_from_settings( MetronomeOptions& 
  void save_to_settings( const SystemOptions& Options ) {
    QSettings Set;
    Set.setValue( "System/MIDIInput", QString::fromStdString( Options.midi_input() ) );
    Set.setValue( "System/MIDIOutput", QString::fromStdString( Options.midi_output() ) );
    Set.setValue( "System/MIDIEcho", Options.midi_echo() );
    Set.setValue( "System/Transpose", Options.transpose() );
    Set.setValue( "System/HalfTones", Options.halftones() );
  } // save_to_settings( const SystemOptions& )

  SystemOptions::SystemOptions() : mMIDIEcho( false ), mTranspose( false ), mHalfTones( 0 ) {}
  
  void load_from_settings( MetronomeOptions& Options ) {
    QSettings Set;
    // Load metronome settings
    Options.tempo( Set.value( "Metronome/Tempo", Options.tempo() ).toInt() )
      .beat( Set.value( "Metronome/Beat", Options.beat() ).toInt() ).measure( Set.value( "Metronome/Measure", Options.measure() ).toInt() )
      .note( Set.value( "Metronome/Note", Options.note() ).toInt() ).velocity( Set.value( "Metronome/Velocity", Options.velocity() ).toInt() )
      .medium_note( Set.value( "Metronome/MediumNote", Options.medium_note() ).toInt() ).medium_velocity( Set.value( "Metronome/MediumVelocity", Options.medium_velocity() ).toInt() )
      .power_note( Set.value( "Metronome/PowerNote", Options.power_note() ).toInt() ).power_velocity( Set.value( "Metronome/PowerVelocity", Options.power_velocity() ).toInt() );
  } // load_from_settings( MetronomeOptions& Options )
  void save_to_settings( const MetronomeOptions& Options ) {
    QSettings Set;
    // Save metronome settings
    Set.setValue( "Metronome/Tempo", Options.tempo() );
    Set.setValue( "Metronome/Beat", Options.beat() );
    Set.setValue( "Metronome/Measure", Options.measure() );
    Set.setValue( "Metronome/Note", Options.note() );
    Set.setValue( "Metronome/Velocity", Options.velocity() );
    Set.setValue( "Metronome/MediumNote", Options.medium_note() );
    Set.setValue( "Metronome/MediumVelocity", Options.medium_velocity() );
    Set.setValue( "Metronome/PowerNote", Options.power_note() );
    Set.setValue( "Metronome/PowerVelocity", Options.power_velocity() );
  } // save_to_settings( const MetronomeOptions& )

  ExerciseOptions::ExerciseOptions() : StartThreshold( 45 ), StopThreshold( 60 ), VelocityThreshold( 30 ), Start( 0 ), Duration( 0 ), TempoSkew( 1 ), TracksNum( 1 ), Tracks( 1 ), Channels( 0xFFFF ) {}
  ExerciseOptions::ExerciseOptions( const ExerciseSequence& Ex )
    : StartThreshold( Ex.StartThreshold ), StopThreshold( Ex.StopThreshold ), VelocityThreshold( Ex.VelocityThreshold ), Start( Ex.StartPoint ),
      Duration( Ex.StopPoint < 0 ? -1 : Ex.StopPoint - Ex.StartPoint ), TempoSkew( Ex.TempoSkew ), TracksNum( Ex.play() ? Ex.play()->tracks_num() : 0 ), Tracks( Ex.tracks_filter() ),
      Channels( Ex.channels() ) {}
  ExerciseOptions& ExerciseOptions::track( int Index, bool NewValue ) {
    if( Index >= 0 && Index < tracks_count() ) {
      if( NewValue ) Tracks |= ( 1 << Index );
      else Tracks &= ~( 1 << Index );
    }
    return *this;
  } // track( int, bool )
  ExerciseOptions& ExerciseOptions::channel( int Index, bool NewValue ) {
    if( Index >= 0 && Index < ChannelsCount ) {
      if( NewValue ) Channels |= ( 1 << Index );
      else Channels &= ~( 1 << Index );
    }
    return *this;
  } // channel( int, bool )
  bool ExerciseOptions::set_to( MuTraTrain::ExerciseSequence& Exercise ) const {
    Exercise.StartThreshold = note_on_low();
    Exercise.StopThreshold = note_off_low();
    Exercise.VelocityThreshold = velocity_low();
    Exercise.TempoSkew = tempo_skew();
    int Finish = fragment_duration() < 0 ? -1 : fragment_start() + fragment_duration();
    if( Exercise.StartPoint != fragment_start() || Exercise.StopPoint != Finish || Exercise.tracks_filter() != tracks() || Exercise.channels() != channels() ) {
      Exercise.StartPoint = fragment_start();
      Exercise.StopPoint = Finish;
      Exercise.channels( channels() );
      Exercise.tracks_filter( tracks() );
      Exercise.rescan();
      return true;
    }
    return false;
  } // set_to( MuTraTrain::ExerciseSequence& ) const
  void load_from_settings( ExerciseOptions& Options ) {
    QSettings Set;
    // Load exercise settings
    Options.note_on_low( Set.value( "Exercise/StartThreshold", Options.note_on_low() ).toInt() );
    Options.note_off_low( Set.value( "Exercise/StopThreshold", Options.note_off_low() ).toInt() );
    Options.velocity_low( Set.value( "Exercise/VelocityThreshold", Options.velocity_low() ).toInt() );
    Options.fragment_duration( Set.value( "Exercise/Duration", -1 ).toInt() );
  } // load_from_settings()
  void save_to_settings( const ExerciseOptions& Options ) {
    QSettings Set;
    // Save exercise settings
    Set.setValue( "Exercise/StartThreshold", Options.note_on_low() );
    Set.setValue( "Exercise/StopThreshold", Options.note_off_low() );
    Set.setValue( "Exercise/VelocityThreshold", Options.velocity_low() );
    Set.setValue( "Exercise/Duration", Options.fragment_duration() );
  } // save_to_settings()

  struct Note {
    Note( int Value, int Velocity = 127, int Start = 0, int Stop = -1, int Channel = 0, int Track = 0 )
      : mValue( Value ), mVelocity( Velocity ), mStart( Start ), mStop( Stop ), mChannel( Channel ), mTrack( Track ) {}
    int mValue;
    int mStart;
    int mStop;
    int mChannel;
    int mTrack;
    int mVelocity;
    bool operator<( const Note& Other ) const {
      if( mStart < Other.mStart ) return true;
      if( mStart > Other.mStart ) return false;
      if( mStop < Other.mStop ) return true;
      if( mStop > Other.mStop ) return false;
      return mTrack < Other.mTrack;
    }
  }; // Note

  //! \todo Create suitable format for MIDI-files & exercises
  class NotesListBuilder : public Sequencer {
  public:
    struct TrackInfo {
      int Delay;
      int High; //!< \todo We need this for every track/channel combination
      int Low;
      int Finish;
      TrackInfo( int Delay0 = -2000000, int High0 = -1, int Low0 = 128, int Finish0 = 0 ) : Delay( Delay0 ), High( High0 ), Low( Low0 ), Finish( Finish0 ) {}
    }; // TrackInfo
    NotesListBuilder( int TracksNum = 1 ) : Infos( nullptr ), Beats( 4 ), Measure( 2 ), Tonal( 0 ), Minor( false ) {
      Infos = new TrackInfo[ TracksNum ]; // The number must be correct or this will crush. Maybe use vecror & resize it?
    }
    ~NotesListBuilder() { if( Infos ) delete [] Infos; }
    void key_signature( int NewTonal, bool NewMinor ) override {
      Tonal = NewTonal;
      Minor = NewMinor;
    } // key_signature( int Tonal, bool )
    void note_on( int Channel, int Value, int Velocity ) {
      if( Velocity == 0 ) note_off( Channel, Value, Velocity );
      else {
#ifdef MUTRA_DEBUG
	qDebug() << "Add note" << Value << "in channel" << Channel << "of track" << Track << "@" << Clock;
#endif  //! \todo Use track & channel
	if( Infos[ Track ].Delay < -1000000 ) Infos[ Track ].Delay = Clock;
	if( Infos[ Track ].Low > Value ) Infos[ Track ].Low = Value;
	if( Infos[ Track ].High < Value ) Infos[ Track ].High = Value;
	if( Infos[ Track ].Finish < Clock ) Infos[ Track ].Finish = Clock;
	Notes.push_back( Note( Value, Velocity, Clock, -1, Channel, Track ) );
      }
    }
    void note_off( int Channel, int Value, int Velocity ) {
      if( Infos[ Track ].Finish < Clock ) Infos[ Track ].Finish = Clock;
      for( auto It = Notes.rbegin(); It != Notes.rend(); ++It )
	if( It->mValue == Value && It->mChannel == Channel && It->mTrack == Track && It->mStop < 0 && It->mStart <= Clock ) {
	  It->mStop = Clock;
	  return;
	}
      qDebug() << "Note" << Value << "in channel" << Channel << "not found for stop @" << Clock;
    }
    void meter( int Numerator, int Denominator ) {
#ifdef MUTRA_DEBUG
      qDebug() << "Metar: " << Numerator << "/" << Denominator << endl;
#endif
      Beats = Numerator;
      Measure = Denominator;
    } // meter( int, int )
    void start() { Time = 0; Clock = 0; }
    void wait_for( unsigned WaitClock ) { Clock = WaitClock; Time = Clock * tempo(); }
    vector<Note> Notes;
    TrackInfo* Infos;

    int Beats;
    int Measure;
    int Tonal;
    bool Minor;
  }; // NotesListBuilder

  //! \todo Move these globals into PianoRollView etc.
  const int ColorsNum = 7;
  QColor Colors[] = { QColor( 128, 128, 128, 64 ), QColor( 0, 255, 0, 64 ), QColor( 0, 255, 255, 64 ), QColor( 0, 0, 255, 64 ), QColor( 255, 0, 255, 64 ), QColor( 255, 0, 0, 64 ), QColor( 255, 255, 0, 64 ) };
  PianoRollView::PianoRollView( QWidget* Parent ) : QGraphicsView( Parent ), Keyboard( nullptr ) {} // PianoRollView( QWidget* )
  void PianoRollView::update_piano_roll( MIDISequence* Sequence ) {
    QGraphicsScene* Sc = new QGraphicsScene( this );
    Sc->setBackgroundBrush( QColor( 72, 72, 72 ) );
    int SceneLeft = 0;
    if( Sequence ) {
      int TracksCount = Sequence->tracks().size();
      int DrawTracks = TracksCount < 4 ? TracksCount : 4;
      NotesListBuilder NL( TracksCount );
      Sequence->play( NL );
      int Div = Sequence->division() * ( 4.0 / ( 1 << NL.Measure ) );
      qreal K = Div / 32.0;
      qreal H = DrawTracks < 5 ? 16 : DrawTracks * 4;
      qreal BarH = H / DrawTracks;
#ifdef MUTRA_DEBUG
      qDebug() << "We have" << NL.Notes.size() << "notes.";
      for( int I = 0; I < TracksCount; ++I )
	qDebug() << "Track" << I << "delay" << NL.Infos[ I ].Delay;
#endif
      int Finish = 0;
      int Low = 127;
      int High = 0;
      for( int I = 0; I < TracksCount; ++I ) {
	if( NL.Infos[ I ].Finish > Finish ) Finish = NL.Infos[ I ].Finish;
	if( NL.Infos[ I ].Low < Low ) Low = NL.Infos[ I ].Low;
	if( NL.Infos[ I ].High > High ) High = NL.Infos[ I ].High;
      }
      {
	int BarLength = Div * NL.Beats;
	int LengthAlign = Finish % BarLength;
	if( LengthAlign > 0 ) Finish += BarLength - LengthAlign;
      }	   
      int Bottom = 0;
      if( High > Low ) {
	int Top = ( 60-High ) * H;
	Bottom = ( 60-Low+1 ) * H;
	for( int I = Low; I <= High; ++I )
	  if( ( 1 << ( I % 12 ) ) & 0x54A )
	    Sc->addRect( 0, ( 60-I ) * H, Finish / K, H, QPen(), QBrush( QColor( 64, 64, 64 ) ) );
	int Beat = 0;
	for( int X = 0; X < Finish; X += Div ) {
	  QPen Pen;
	  if( NL.Beats != 0 && Beat % NL.Beats == 0 ) Pen = QPen( Qt::black, 2 );
	  ++Beat;
	  Sc->addLine( X / K, Top, X /K, Bottom, Pen );
	}
	for( int Y = Top; Y <= Bottom; Y += H )
	  Sc->addLine( 0, Y, Finish / K, Y );
	QPen BoldPen = QPen( Qt::black, 2 );
	QPen LinePen = QPen( QColor( 0, 0, 0, 128 ), 1 );
	QPen SemiBoldPen = QPen( QColor( 0, 0, 0, 128 ), 1.5 );
	int BarsTop = Bottom + 16;
	int BarsBottom = BarsTop + 128;
	Sc->addRect( 0, BarsTop, Finish / K, 128, BoldPen, QBrush( QColor( 64, 64, 64 ) ) );
	for( qreal Y = BarsBottom; Y > BarsTop; Y -= 12.8 ) Sc->addLine( 0, Y, Finish / K, Y );
	Sc->addLine( 0, BarsBottom - 64, Finish / K, BarsBottom - 64, SemiBoldPen );
      }
      for( Note N: NL.Notes ) {
#ifdef MUTRA_DEBUG
	qDebug() << "Note" << N.mValue << "in track" << N.mTrack << "[" << N.mStart << "-" << N.mStop;
#endif
	int TrackPlace = N.mTrack - (TracksCount-DrawTracks);
	if( N.mTrack == 0 ) TrackPlace = 0;
	else
	  if( TrackPlace <= 0 ) continue;
	QColor Clr = Colors[ TrackPlace % ColorsNum ];
	Clr.setAlphaF( N.mVelocity / 127.0 );
	int Start = N.mStart / K;
	int Stop = (N.mStop < 0 ? Div : N.mStop-N.mStart) / K;
	Sc->addRect( Start, (60-N.mValue) * H + (TrackPlace * BarH), Stop, BarH, QPen( N.mStop < 0 ? Qt::red : Qt::white ), QBrush( Clr ) );
	if( N.mTrack == 0 || N.mTrack == TracksCount-1 ) {
	  Sc->addRect( Start, Bottom + H + 127 - N.mVelocity, Stop, N.mVelocity, QPen( N.mTrack == 0 ? Qt::gray : Qt::cyan ),
		       QBrush( N.mTrack == 0 ? QColor( 0, 255, 80, 32 ) : QColor( 0, 80, 255, 32 ) ) );
	}
      }
      {
	int KeyLow = 60-24; // Draw 61-key keyboard
	int KeyHigh = 60+36;
	int WhiteWidth = 64;
	QPen WhitePen = QPen( Qt::gray );
	QBrush WhiteBrush = QBrush( Qt::white );
	int BlackWidth = 32;
	QPen BlackPen = QPen( Qt::gray );
	QBrush BlackBrush = QBrush( Qt::black );
	int KeysRight = -3;
	int KeysLeft = 0; //KeysRight - WhiteWidth;
	SceneLeft = KeysLeft;
	Keyboard = new QGraphicsItemGroup;
	Sc->addItem( Keyboard );
	Keyboard->setPos( KeysRight - WhiteWidth, 0 );
	for( int I = KeyLow; I <= KeyHigh; ++I ) {
	  int N = I % 12;
	  int KeyTop = ( 60-I ) * H;
	  qreal KeyHeight = H;
	  QGraphicsRectItem* Key = nullptr;
	  if( N == 4 || N == 11 || I == KeyHigh ) {
	    if( I != KeyLow && I != KeyHigh ) KeyHeight += H / 2;
	    Key = new QGraphicsRectItem( KeysLeft, KeyTop, WhiteWidth, KeyHeight, Keyboard );
	    Key->setPen( WhitePen );
	    Key->setBrush( WhiteBrush );
	  }
	  else {
	    KeyTop -= H / 2;
	    if( N == 0 || N == 5 || I == KeyLow ) KeyHeight += H / 2;
	    else KeyHeight += H;
	    Key = new QGraphicsRectItem( KeysLeft, KeyTop, WhiteWidth, KeyHeight, Keyboard );
	    Key->setPen( WhitePen );
	    Key->setBrush( WhiteBrush );
	    ++I;
	    Key = new QGraphicsRectItem( KeysLeft, ( 60-I ) * H, BlackWidth, H, Keyboard );
	    Key->setZValue( 1 );
	    Key->setPen( BlackPen );
	    Key->setBrush( BlackBrush );
	  }
	}
      }
    }
    setScene( Sc );
    setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    QPointF Origin = mapToScene( 0, 0 );
  } // update_piano_roll( MIDISequence* )  
  void PianoRollView::scrollContentsBy( int DX, int DY ) {
    QGraphicsView::scrollContentsBy( DX, DY );
    moveKeyboard();
  } // scrollContentsBy( int, int )
  void PianoRollView::resizeEvent( QResizeEvent* Ev ) {
    QGraphicsView::resizeEvent( Ev );
    moveKeyboard();
  } // resizeEvent( QResizeEvent* )
  void PianoRollView::moveKeyboard() {
    if( Keyboard ) {
      QPointF Origin = mapToScene( 0, 0 );
      QPointF Pos = Keyboard->pos();
      if( Pos.x() != Origin.x() )
	Keyboard->setPos( Origin.x(), Pos.y() );
    }
  } // moveKeyboard()
  
  struct BrokenNote {
    enum { MiddleC = 60, FirstLine = 5 };
    BrokenNote( int Value = MiddleC ) {
      mOctave = Value / 12;
      mNote = Value % 12;
      mSharp = mNote % 2;
      if( mNote > 4 ) {
	mNote = ( mNote+1 ) / 2;
	mSharp = !mSharp;
      }
      else
	mNote = mNote / 2;
    }
    int notes_to_c() const { return ( FirstLine - mOctave ) * 7 - mNote; }
    int mOctave;
    int mNote;
    bool mSharp;
  }; // BrokenNote

  // Лигованная нота (по длительности)
  struct TiedNote {
    TiedNote( int Note, int Start = 0, int Length = 0, const QPointF& TieFrom = QPointF() ) : mNote( Note ), mTieFrom( TieFrom ), mStart( Start ), mLength( Length ) {}
    bool operator==( int Other ) const { return mNote == Other; }
    int mNote;
    QPointF mTieFrom;
    int mStart;
    int mLength;
  }; // TiedNote

  ScoresView::ScoresView( QWidget* Parent ) : QGraphicsView( Parent ) { scale( 2, 2 ); }
  void ScoresView::update_scores( MIDISequence* Sequence ) {
    QGraphicsScene* Sc = new QGraphicsScene( this );
    Sc->setBackgroundBrush( QColor( 160, 160, 128 ) );
    if( Sequence ) {
      int TracksCount = Sequence->tracks_num();
      NotesListBuilder NL( TracksCount );
      Sequence->play( NL );
      int Div = Sequence->division();
      QPen LinesPen( QColor( 32, 32, 32, 160 ) );
      int StartX = 0;
      int NotesStartX = StartX;
      int FinishX = 0;
      // Here we draw only the first track & one scoreboard just for piano
      int LinesSpacing = 6;
      int Top = 0;
      int Bottom = Top + LinesSpacing * 10;
      Sc->addLine( StartX, Top, StartX, Bottom, LinesPen );
      int GClefOffsetX = 3;
      int GClefOffsetY = -5;
      int FClefOffsetY = 23;
      int ClefsWidth = 16;   //      F   C   G    D   A   E   B
      static int SignsOffsetsY[] = { -9,  0,  9,  -3,  6, -6,  3 };
      int SignsOffsetX = 3;
      QGraphicsSvgItem* Sign = new QGraphicsSvgItem( ":/images/g-clef.svg" );
      Sc->addItem( Sign );
      Sign->setPos( StartX + GClefOffsetX, Top + GClefOffsetY );
      Sign = new QGraphicsSvgItem( ":/images/f-clef.svg" );
      Sc->addItem( Sign );
      Sign->setPos( StartX + GClefOffsetX, Bottom - FClefOffsetY );
      NotesStartX += GClefOffsetX + ClefsWidth;
      if( NL.Tonal ) {
	int Start = 0;
	int Step = 1;
	int Stop = NL.Tonal;
	QString SignName = ":/images/sharp.svg";
	if( Stop < 0 ) {
	  Start = 6;
	  Step = -1;
	  Stop = -Stop;
	  SignName = ":/images/flat.svg";
	}
	for( int I = 0; I < Stop; ++I ) {
	  Sign = new QGraphicsSvgItem( SignName );
	  Sc->addItem( Sign );
	  int OffsetY = SignsOffsetsY[ Start+I*Step ];
	  Sign->setPos( NotesStartX, OffsetY );
	  Sign = new QGraphicsSvgItem( SignName );
	  Sc->addItem( Sign );
	  Sign->setPos( NotesStartX, OffsetY + LinesSpacing * 7 );
	  NotesStartX += SignsOffsetX;
	}
      }
      int TextOffsetY = -1.25 * LinesSpacing;
      QGraphicsTextItem* Text = Sc->addText( QString::number( NL.Beats ) );
      Text->setPos( NotesStartX, TextOffsetY );
      Text->setDefaultTextColor( Qt::black ); //!< \todo There definetly has to be a method to set all text to black. Maybe palette's color in scene. But not now.
      Text = Sc->addText( QString::number( 1 << NL.Measure ) );
      Text->setPos( NotesStartX, TextOffsetY + LinesSpacing * 2 );
      Text->setDefaultTextColor( Qt::black );
      Text = Sc->addText( QString::number( NL.Beats ) );
      TextOffsetY += 6 * LinesSpacing;
      Text->setPos( NotesStartX, TextOffsetY );
      Text->setDefaultTextColor( Qt::black ); //!< \todo There definetly has to be a method to set all text to black. Maybe palette's color in scene. But not now.
      Text = Sc->addText( QString::number( 1 << NL.Measure ) );
      Text->setPos( NotesStartX, TextOffsetY + LinesSpacing * 2 );
      Text->setDefaultTextColor( Qt::black );
      NotesStartX += Text->boundingRect().width() + SignsOffsetX * 2;
      int LastBar = 0;
      int BarLength = ( NL.Beats * Div ) / (( 1 << NL.Measure ) / 4.0 );
      int End = LastBar;
      int Thresh = Div / 16; // 1/64 note
      for( int I = 0; I < NL.Notes.size(); ++I )
	if( NL.Notes[ I ].mTrack == 0 ) { //! \todo Allow tracks selection.
	  if( NL.Notes[ I ].mStart > End+Thresh ) { // This stands for the rest
	    NL.Notes.insert( NL.Notes.begin() + I, Note( 60, -1, End, NL.Notes[ I ].mStart-1, NL.Notes[ I ].mChannel, NL.Notes[ I ].mTrack ) );
	    ++I;
	  }
	  if( NL.Notes[ I ].mStop > End ) End = NL.Notes[ I ].mStop;
	}
      if( int Tail = End % BarLength ) {
	Note& N = NL.Notes.back();
	NL.Notes.push_back( Note( 60, -1, End, End + BarLength - Tail, 0, 0 ) );
	End += BarLength - Tail;
      }
      int NoteWidth = 10;
      double K = NoteWidth * 4.0 / Div;
      double NoteOffset = LinesSpacing * 1.5;
      double SignsOffset = LinesSpacing * 3.5;
      queue<TiedNote> Ties;
      int NoteIndex = 0;
      bool Rest = true;
      while( LastBar < End ) {
	while( NoteIndex < NL.Notes.size() && NL.Notes[ NoteIndex ].mTrack != 0 ) ++NoteIndex; // ATM draw only the first track
	int NoteStart = 0;
	int NoteLength = 0;
	int NoteValue = 60;
	QPointF TieFrom;
	if( NoteIndex < NL.Notes.size() && ( Ties.empty() || NL.Notes[ NoteIndex ].mStart < Ties.front().mStart ) ) {
	  NoteStart = NL.Notes[ NoteIndex ].mStart;
	  NoteLength = NL.Notes[ NoteIndex ].mStop - NL.Notes[ NoteIndex ].mStart;
	  NoteValue = NL.Notes[ NoteIndex ].mValue;
	  Rest = NL.Notes[ NoteIndex ].mVelocity < 0;
	  ++NoteIndex;
	}
	else if( Ties.empty() ) NoteStart = End;
	else {
	  NoteStart = Ties.front().mStart;
	  NoteLength = Ties.front().mLength;
	  NoteValue = Ties.front().mNote;
	  TieFrom = Ties.front().mTieFrom;
	  Ties.pop();
	}
	int InBar = NoteStart - LastBar;
	while( InBar >= BarLength ) {
	  InBar -= BarLength;
	  NotesStartX += BarLength * K;
	  NotesStartX += SignsOffsetX;
	  Sc->addLine( NotesStartX, Top, NotesStartX, Bottom, LinesPen );
	  NotesStartX += SignsOffsetX;
	  LastBar += BarLength;
	}
	if( NoteLength > 0 ) {
	  BrokenNote B( NoteValue );
	  if( B.mSharp && !Rest ) {
	    Sign = new QGraphicsSvgItem( ":/images/sharp.svg" );
	    Sc->addItem( Sign );
	    Sign->setPos( NotesStartX + InBar * K, SignsOffset + B.notes_to_c() * LinesSpacing / 2.0 );
	    InBar += ( Sign->boundingRect().width() + 1 ) / K;
	  }
	  if( NoteValue == BrokenNote::MiddleC ) Sc->addLine( NotesStartX + InBar*K - SignsOffsetX, LinesSpacing * 5, NotesStartX + InBar*K + NoteWidth, LinesSpacing * 5, LinesPen );
	  else if( NoteValue < BrokenNote::MiddleC - 19 )
	    for( int I = 12; I <= B.notes_to_c(); I += 2 )
	      Sc->addLine( NotesStartX + InBar*K - SignsOffsetX, LinesSpacing * ( I/2+5 ), NotesStartX + InBar*K + NoteWidth, LinesSpacing * ( I/2+5 ), LinesPen );
	  else if( NoteValue > BrokenNote::MiddleC + 20 )
	    for( int I = -12; I >= B.notes_to_c(); I -= 2 )
	      Sc->addLine( NotesStartX + InBar*K - SignsOffsetX, LinesSpacing * ( I/2+5 ), NotesStartX + InBar*K + NoteWidth, LinesSpacing * ( I/2+5 ), LinesPen );
	  QString NoteFileName = ":/images/";
	  int CutOff = ( NoteStart + NoteLength ) - ( LastBar + BarLength );
	  int NoteInBar = CutOff > 0 ? NoteLength - CutOff : NoteLength;
	  if( NoteInBar > 0 ) {
	    int Duration = 0;
	    int DurTime = Div * 4;
	    int ToGo = NoteInBar;
	    int DurStop = 5;
	    static const char* DurNames[] = { "whole", "half", "quarter", "eighth", "sixteenth", "thirtysecond", "sixtyfourth" };
	    while( Duration < DurStop ) {
	      int Diff = DurTime / ( 1 << Duration ) - NoteInBar; //! \todo Use an array of names & a loop for this.
#ifdef MUTRA_DEBUG
	      qDebug() << "Thresh" << Thresh << "Diff" << Diff << "NoteInBar" << NoteInBar << "Div" << Div;
#endif // MUTRA_DEBUG
	      if( Diff < Thresh ) break;
	      ++Duration;
	    }
	    DurTime /= ( 1 << Duration );
	    NoteFileName += ( QString( Rest ? "rest-" : "note-" ) + DurNames[ Duration ] ) + ".svg";
	    qreal NoteY = NoteOffset + B.notes_to_c() * LinesSpacing / 2.0;
	    if( Rest )  NoteY = LinesSpacing * 3;
	    Sign = new QGraphicsSvgItem( NoteFileName );
	    Sign->setPos( NotesStartX + InBar * K, NoteY );
	    Sc->addItem( Sign );
	    int Dots = 0;
	    int CutOffInBar = NoteInBar - DurTime;
	    while( CutOffInBar + Thresh > DurTime / 2 && CutOffInBar < DurTime && DurTime > Thresh ) {
	      ++Dots;
	      DurTime /= 2;
	      CutOffInBar -= DurTime;
	    }
	    NoteY = ( B.notes_to_c()+10 ) * LinesSpacing / 2;
	    qreal NoteX = NotesStartX + InBar*K;
	    QPointF TieTo( NoteX + SignsOffsetX, NoteY + LinesSpacing * 0.75 );
	    qreal DotX = NoteX + NoteWidth + SignsOffsetX;
	    for( int I = 0; I < Dots; ++I ) {
	      Sc->addEllipse( DotX - 1, NoteY - 1, 2, 2, QPen(), QBrush( Qt::black ) );
	      DotX += SignsOffsetX * 2;
	    }
	    if( CutOffInBar > Thresh )
	      if( CutOff > 0 ) CutOff += CutOffInBar;
	      else CutOff = CutOffInBar;
	    if( CutOff > Thresh ) Ties.push( TiedNote( NoteValue, NoteStart + NoteLength - CutOff, CutOff, TieTo ) );
	    if( !Rest && !TieFrom.isNull() ) {
	      QPainterPath Tie;
	      Tie.moveTo( TieFrom );
	      qreal Third = ( TieTo.x() - TieFrom.x() ) / 3;
	      Tie.cubicTo( QPoint( TieFrom.x() + Third, TieFrom.y()+LinesSpacing ), QPointF( TieTo.x() - Third, TieTo.y()+LinesSpacing ), TieTo );
	      Sc->addPath( Tie );
	    }
	  } // NoteInBar > 0
	} // NoteLength > 0
      } // while( LastBar < End )
      FinishX = NotesStartX;
      if( End % BarLength > 0 ) FinishX += BarLength * K + SignsOffsetX;
      Sc->addLine( FinishX, Top, FinishX, Bottom, LinesPen );
      int Y = Top;
      for( int I = 0; I < 5; ++I ) {
	Sc->addLine( StartX, Y, FinishX, Y, LinesPen );
	Y += LinesSpacing;
      }
      Y += LinesSpacing;
      for( int I = 0; I < 5; ++I ) {
	Sc->addLine( StartX, Y, FinishX, Y, LinesPen );
	Y += LinesSpacing;
      }
    } // Have sequence
    setScene( Sc );
  } // update_scores( MIDISequence* )
  
  TracksChannelsModel& TracksChannelsModel::tracks_count( int NewCount ) {
    mTracksCount = NewCount < 0 ? 0 : NewCount > 16 ? 16 : NewCount;
    return *this;
  } // tracks_count( int )
  TracksChannelsModel& TracksChannelsModel::track( int Index, bool Value ) {
    if( Index >= 0 && Index < mTracksCount ) {
      if( Value ) mTracks |= 1 << Index;
      else mTracks &= ~( 1 << Index );
    }
    return *this;
  } // track( int, bool )
  TracksChannelsModel& TracksChannelsModel::tracks( uint16_t NewTracks ) {
    mTracks = NewTracks;
    return *this;
  } // tracks( uint16_t )
  TracksChannelsModel& TracksChannelsModel::channel( int Index, bool Value ) {
    if( Index >= 0 && Index < 16 ) {
      if( Value ) mChannels |= 1 << Index;
      else mChannels &= ~( 1 << Index );
    }
    return *this;
  } // channel( int, bool )
  TracksChannelsModel& TracksChannelsModel::channels( uint16_t NewChannels ) {
    mChannels = NewChannels;
    return *this;
  } // channels( uint16_t )
  int TracksChannelsModel::rowCount( const QModelIndex& Parent ) const { return Parent.isValid() ? 0 : mTracksCount + 18; }
  Qt::ItemFlags TracksChannelsModel::flags( const QModelIndex& Index ) const {
    if( Index.isValid() && Index.row() != 0 && Index.row() != mTracksCount + 1 ) return QAbstractListModel::flags( Index ) | Qt::ItemIsUserCheckable;
    return QAbstractListModel::flags( Index );
  } // flags( const QModelIndex& ) const
  QVariant TracksChannelsModel::data( const QModelIndex& Index, int Role ) const {
    switch( Role ) {
    case Qt::DisplayRole:
      if( Index.row() == 0 ) return tr( "Tracks" );
      if( Index.row() <= mTracksCount ) return tr( "Track " ) + QString::number( Index.row() );
      if( Index.row() == mTracksCount + 1 ) return tr( "Channels" );
      {
	int Channel = Index.row() - mTracksCount - 1;
	if( Channel > 0 && Channel <= 16 ) return tr( "Channel " ) + QString::number( Channel );
      }
      break;
    case Qt::BackgroundRole:
      if( Index.row() == 0 || Index.row() == mTracksCount + 1 ) return QBrush( QColor( 64, 64, 64 ) );
      break;
    case Qt::CheckStateRole:
      if( Index.row() > 0 && Index.row() <= mTracksCount ) return track( Index.row() - 1 ) ? Qt::Checked : Qt::Unchecked;
      if( Index.row() > mTracksCount + 1 && Index.row() <= mTracksCount + 17 ) return channel( Index.row() - mTracksCount - 2 ) ? Qt::Checked : Qt::Unchecked;
      break;
    }
    return QVariant();
  } // data( const QModelIndex&, int )
  bool TracksChannelsModel::setData( const QModelIndex& Index, const QVariant& Value, int Role ) {
    if( Index.isValid() && Role == Qt::CheckStateRole ) {
      if( Index.row() <= 0 || Index.row() == mTracksCount + 1 || Index.row() > mTracksCount + 1 + 16 ) return false;
      if( Index.row() <= mTracksCount ) track( Index.row()-1, Value == Qt::Checked );
      else channel( Index.row()-mTracksCount-2, Value == Qt::Checked );
      return true;
    }
    return setData( Index, Value, Role );
  } // setData( const QModelIndex&, const QVariant&, int )
  
  SettingsDialog::SettingsDialog( QWidget* Parent )
    : QDialog( Parent ), mOriginalTempo( 120 ), mNoteLow( 36 ), mNoteHigh( 96 ), mMetronomePage( nullptr ), mDevicesPage( nullptr ), mExercisePage( nullptr ) {
    mDlg = new Ui::SettingsDialog;
    mDlg->setupUi( this );
    //! \todo Move the dialogs to separate objects.
    QWidget* Page = new QWidget( this );
    mDevicesPage = new Ui::DevicesSettings;
    mDevicesPage->setupUi( Page );
    for( const InputDevice::Info& Dev : MIDIBackend::get_manager().list_devices( MIDIBackend::Input ) )
      mDevicesPage->MIDIInput->addItem( QString::fromStdString( Dev.name() ), QString::fromStdString( Dev.uri() ) );
    for( const Sequencer::Info& Dev : MIDIBackend::get_manager().list_devices( MIDIBackend::Output ) )
      mDevicesPage->MIDIOutput->addItem( QString::fromStdString( Dev.name() ), QString::fromStdString( Dev.uri() ) );
    mDlg->SettingsTabs->addTab( Page, tr( "Devices" ) );
    Page = new QWidget( this );
    mMetronomePage = new Ui::MetronomeSettings;
    mMetronomePage->setupUi( Page );
    for( int I = NoteEvent::PercussionFirst; I <= NoteEvent::PercussionLast; ++I ) {
      mMetronomePage->PowerNote->addItem( NoteEvent::Percussion[ I-NoteEvent::PercussionFirst ], I );
      mMetronomePage->WeakNote->addItem( NoteEvent::Percussion[ I-NoteEvent::PercussionFirst ], I );
      mMetronomePage->MediumNote->addItem( NoteEvent::Percussion[ I-NoteEvent::PercussionFirst ], I );
    }
    mDlg->SettingsTabs->addTab( Page, tr( "Metronome" ) );
    Page = new QWidget( this );
    mExercisePage = new Ui::ExerciseSettings;
    mExercisePage->setupUi( Page );
    mExercisePage->Tracks->setModel( new TracksChannelsModel( mExercisePage->Tracks ) );
    mDlg->SettingsTabs->addTab( Page, tr( "Exercise" ) );
    connect( mExercisePage->TempoPercent, SIGNAL( valueChanged( int ) ), SLOT( tempo_skew_changed( int ) ) );
    Page = new QWidget( this );
    mNotesPage = new Ui::NotesExerciseSettings;
    mNotesPage->setupUi( Page );
    int MIDINote = 0;
    for( int I = 0; I < 7; ++I ) {
      mNotesPage->LowNote->addItem( QString::fromStdString( Names::note_name( I ) ), MIDINote );
      mNotesPage->HighNote->addItem( QString::fromStdString( Names::note_name( I ) ), MIDINote );
      ++MIDINote;
      if( I != 2 && I != 6 ) {
	mNotesPage->LowNote->addItem( QString::fromStdString( Names::note_name( I )+"#" ), MIDINote );
	mNotesPage->HighNote->addItem( QString::fromStdString( Names::note_name( I )+"#" ), MIDINote );
	++MIDINote;
      }
    }
    for( int I = 0; I < 9; ++I ) {
      mNotesPage->LowOctave->addItem( QString::fromStdString( Names::octave_name( I ) ), I );
      mNotesPage->HighOctave->addItem( QString::fromStdString( Names::octave_name( I ) ), I );
    }
    notes_options( mNoteLow, mNoteHigh );
    mDlg->SettingsTabs->addTab( Page, tr( "Notes" ) );
  } // SettingsDialog( QWidget* )
  SettingsDialog::~SettingsDialog() {
    if( mMetronomePage ) delete mMetronomePage;
    if( mDlg ) delete mDlg;
  } // ~SettingsDialog()
  void SettingsDialog::system( const SystemOptions& NewOptions ) {
    mDevicesPage->MIDIInput->setCurrentIndex( mDevicesPage->MIDIInput->findData( QString::fromStdString( NewOptions.midi_input() ) ) );
    mDevicesPage->MIDIOutput->setCurrentIndex( mDevicesPage->MIDIOutput->findData( QString::fromStdString( NewOptions.midi_output() ) ) );
    mDevicesPage->MIDIEcho->setChecked( NewOptions.midi_echo() );
    mDevicesPage->Transpose->setChecked( NewOptions.transpose() );
    mDevicesPage->Halftones->setValue( NewOptions.halftones() );
    mSystem = NewOptions;
  } // system( const SystemOptions& )
  void SettingsDialog::metronome( const MetronomeOptions& NewOptions ) {
    mMetronomePage->Beats->setValue( NewOptions.beat() );
    mMetronomePage->Measure->setCurrentIndex( NewOptions.measure() );
    mMetronomePage->Tempo->setValue( NewOptions.tempo() );
    mMetronomePage->WeakNote->setCurrentIndex( mMetronomePage->WeakNote->findData( NewOptions.note() ) );
    mMetronomePage->WeakVelocity->setValue( NewOptions.velocity() );
    mMetronomePage->MediumNote->setCurrentIndex( mMetronomePage->MediumNote->findData( NewOptions.medium_note() ) );
    mMetronomePage->MediumVelocity->setValue( NewOptions.medium_velocity() );
    mMetronomePage->PowerNote->setCurrentIndex( mMetronomePage->PowerNote->findData( NewOptions.power_note() ) );
    mMetronomePage->PowerVelocity->setValue( NewOptions.power_velocity() );
    mMetronome = NewOptions;
  } // metronome( const MetronomeOptions& )
  void SettingsDialog::exercise( const ExerciseOptions& NewOptions ) {
    mExercisePage->FragmentStart->setValue( NewOptions.fragment_start() );
    mExercisePage->FragmentFinish->setValue( NewOptions.fragment_duration() < 0 ? -1 : NewOptions.fragment_start() + NewOptions.fragment_duration() );
    mExercisePage->TempoPercent->setValue( NewOptions.tempo_skew() * 100 );
    mExercisePage->NoteOnLow->setValue( NewOptions.note_on_low() );
    mExercisePage->NoteOffLow->setValue( NewOptions.note_off_low() );
    mExercisePage->VelocityLow->setValue( NewOptions.velocity_low() );
    if( TracksChannelsModel* Mod = qobject_cast<TracksChannelsModel*>( mExercisePage->Tracks->model() ) ) {
      Mod->tracks_count( NewOptions.tracks_count() );
      Mod->tracks( NewOptions.tracks() );
      Mod->channels( NewOptions.channels() );
    }
    if( mMetronomePage ) mMetronomePage->Tempo->setValue( mOriginalTempo * NewOptions.tempo_skew() );
    mExercise = NewOptions;
  } // exercise_settings( const ExerciseOptions& )
  void SettingsDialog::original_tempo( int NewTempo ) {
    mOriginalTempo = NewTempo;
    if( mMetronomePage ) mMetronomePage->Tempo->setValue( mOriginalTempo * ( mExercisePage ? mExercisePage->TempoPercent->value() / 100.0 : mExercise.tempo_skew() ) );
  } // original_tempo( int )
  void SettingsDialog::notes_options( int NoteLow, int NoteHigh ) {
    mNoteLow = NoteLow;
    mNoteHigh = NoteHigh;
    mNotesPage->LowNote->setCurrentIndex( mNotesPage->LowNote->findData( NoteLow % 12 ) );
    mNotesPage->LowOctave->setCurrentIndex( NoteLow / 12 - 1 );
    mNotesPage->HighNote->setCurrentIndex( mNotesPage->HighNote->findData( NoteHigh % 12 ) );
    mNotesPage->HighOctave->setCurrentIndex( NoteHigh / 12 - 1 );
  } // notes_options( int, int )
  void SettingsDialog::tempo_skew_changed( int Value ) {
    mMetronomePage->Tempo->setValue( mOriginalTempo * ( Value / 100.0 ) );
  } // tempo_skew_changed( int )
  void SettingsDialog::accept() {
    mMetronome.beat( mMetronomePage->Beats->value() ).measure( mMetronomePage->Measure->currentIndex() ) //!< \todo Better set data in the setup
      .tempo( mMetronomePage->Tempo->value() ).note( mMetronomePage->WeakNote->currentData().toInt() ).velocity( mMetronomePage->WeakVelocity->value() )
      .medium_note( mMetronomePage->MediumNote->currentData().toInt() ).medium_velocity( mMetronomePage->MediumVelocity->value() )
      .power_note( mMetronomePage->PowerNote->currentData().toInt() ).power_velocity( mMetronomePage->PowerVelocity->value() );
    mSystem.midi_input( mDevicesPage->MIDIInput->currentData().toString().toStdString() ).midi_output( mDevicesPage->MIDIOutput->currentData().toString().toStdString() )
      .midi_echo( mDevicesPage->MIDIEcho->isChecked() ).transpose( mDevicesPage->Transpose->isChecked() ).halftones( mDevicesPage->Halftones->value() );
    int Start = mExercisePage->FragmentStart->value();
    int Finish = mExercisePage->FragmentFinish->value();
    mExercise.fragment_start( Start ).fragment_duration( Finish < 0 ? -1 : Finish >= Start ? Finish - Start : 0 ).tempo_skew( mExercisePage->TempoPercent->value() / 100.0 )
      .note_on_low( mExercisePage->NoteOnLow->value() ).note_off_low( mExercisePage->NoteOffLow->value() ).velocity_low( mExercisePage->VelocityLow->value() );
    if( TracksChannelsModel* Mod = qobject_cast<TracksChannelsModel*>( mExercisePage->Tracks->model() ) )
      mExercise.tracks_count( Mod->tracks_count() ).tracks( Mod->tracks() ).channels( Mod->channels() );
    mNoteLow = mNotesPage->LowNote->currentData().toInt() + (mNotesPage->LowOctave->currentData().toInt()+1) * 12;
    mNoteHigh = mNotesPage->HighNote->currentData().toInt() + (mNotesPage->HighOctave->currentData().toInt()+1) * 12;
    if( mNoteHigh < mNoteLow ) swap( mNoteHigh, mNoteLow );
#ifdef MUTRA_DEBUG
    qDebug() << "Settings accepted:\nMetronome\n\tMeter:" << mMetronomePage->Beats->value() << "/" << mMetronomePage->Measure->currentText() << "\tTempo" << mMetronomePage->Tempo->value()
	     << "Power beat" << mMetronomePage->PowerNote->currentText() << "(" << mMetronomePage->PowerNote->currentData() << ") @" << mMetronomePage->PowerVelocity->value()
	     << "Weak beat" << mMetronomePage->WeakNote->currentText() << "(" << mMetronomePage->WeakNote->currentData() << ") @" << mMetronomePage->WeakVelocity->value()
	     << "Medium beat" << mMetronomePage->MediumNote->currentText() << "(" << mMetronomePage->MediumNote->currentData() << ") @" << mMetronomePage->MediumVelocity->value()
	     << "Notes exercise" << mNoteLow << "-" << mNoteHigh << endl;
#endif
    QDialog::accept();
  } // accept()

  void prepare_pair( QSlider* Slider, QSpinBox* Box ) {
    Slider->connect( Box, SIGNAL( valueChanged( int ) ), SLOT( setValue( int ) ) );
    Slider->setMaximum( 127 );
    Box->connect( Slider, SIGNAL( valueChanged( int ) ), SLOT( setValue( int ) ) );
    Box->setMaximum( 127 );
    Box->setValue( 63 );
  } // prepare_pair( QSlider*, QSpinBox* )
  
  MIDIMixer::MIDIMixer( QWidget* Parent ) : QDialog( Parent ), mSequencer( nullptr ) {
    mUI = new Ui::MIDIMixer;
    mUI->setupUi( this );
    for( int I = 0; I < ChannelsNum; ++I ) {
      mChannels[ I ].mVolSlider = new QSlider( Qt::Horizontal, this );
      mChannels[ I ].mVolBox = new QSpinBox( this );
      prepare_pair( mChannels[ I ].mVolSlider, mChannels[ I ].mVolBox );
      QObject::connect( mChannels[ I ].mVolBox, QOverload<int>::of( &QSpinBox::valueChanged ), [this, I]( int Volume ) { this->volume_changed( I, Volume ); } );
      mChannels[ I ].mPanSlider = new QSlider( Qt::Horizontal, this );
      mChannels[ I ].mPanBox = new QSpinBox( this );
      prepare_pair( mChannels[ I ].mPanSlider, mChannels[ I ].mPanBox );
      QObject::connect( mChannels[ I ].mPanBox, QOverload<int>::of( &QSpinBox::valueChanged ), [this, I]( int Pan ) { this->pan_changed( I, Pan ); } );
      mUI->Mixer->addWidget( new QLabel( QString::number( I+1 ), this ), I, 0 );
      mUI->Mixer->addWidget( mChannels[ I ].mVolSlider, I, 1 );
      mUI->Mixer->addWidget( mChannels[ I ].mVolBox, I, 2 );
      mUI->Mixer->addWidget( mChannels[ I ].mPanSlider, I, 3 );
      mUI->Mixer->addWidget( mChannels[ I ].mPanBox, I, 4 );
    }
  } // MIDIMixer( QWidget* )
  void MIDIMixer::volume_changed( int Channel, int Value ) {
    qDebug() << "Channel" << Channel << "volume" << Value << endl;
    if( mSequencer ) mSequencer->control_change( Channel, 7, Value );
  } // volume_changed( int, int )
  void MIDIMixer::pan_changed( int Channel, int Value ) {
    qDebug() << "Channel" << Channel << "pan" << Value << endl;
    if( mSequencer ) mSequencer->control_change( Channel, 10, Value );
  } // pan_changed( int, int )
  
  StatisticsModel::Item::~Item() { for( Item* It : mSubItems ) delete It; } // ~Item()
  QVariant StatisticsModel::Item::data( int Col ) const { return Col == 0 ? QVariant( mName ) : QVariant(); }
  QVariant StatisticsModel::Item::icon() const { return QVariant(); }
  QVariant StatisticsModel::StatsItem::data( int Col ) const {
    if( mStat.Result < ExerciseSequence::NoteError )
      switch( Col ) {
      case 1: return QVariant( mStat.StartMin );
      case 2: return QVariant( mStat.StartMax );
      case 3: return QVariant( mStat.StopMin );
      case 4: return QVariant( mStat.StopMax );
      case 5: return QVariant( mStat.VelocityMin );
      case 6: return QVariant( mStat.VelocityMax );
      }
    return Item::data( Col );
  } // data( int ) const
  QVariant StatisticsModel::StatsItem::icon() const {
    switch( mStat.Result ) {
    case ExerciseSequence::NoError: return QIcon::fromTheme( "gtk-yes" );
    case ExerciseSequence::NoteError: return QIcon::fromTheme( "gtk-no" );
    case ExerciseSequence::RythmError: return QIcon::fromTheme( "preferences-desktop-thunderbolt" );
    case ExerciseSequence::VelocityError: return QIcon::fromTheme( "preferences-desktop-notification-bell" );
    case ExerciseSequence::EmptyPlay: return QIcon::fromTheme( "user-offline" );
    }
    return Item::icon();
  } // icon() const
  
  void StatisticsModel::clear() {
    beginResetModel();
    mActivity = EmptyType;
    for( Item* It : mItems ) delete It;
    mItems.clear();
    endResetModel();
  } // clear()
  void StatisticsModel::start_lesson( const Lesson& NewLesson ) {
    mActivity = LessonType;
    beginInsertRows( QModelIndex(), mItems.size(), mItems.size() );
    mItems.push_back( new Item( QFileInfo( QString::fromStdString( NewLesson.lesson_name() ) ).baseName() ) );
    endInsertRows();
  } // lesson( Lesson* )
  void StatisticsModel::finish_lesson( const Lesson& NewLesson ) { mActivity = EmptyType; } // lesson( Lesson& )
  void StatisticsModel::start_exercise( const Lesson::Exercise& NewExercise ) {
    if( mActivity == EmptyType ) {
      beginInsertRows( QModelIndex(), mItems.size(), mItems.size() );
      mItems.push_back( new Item( tr( "Free exercises" ) ) );
      endInsertRows();
      mActivity = ExerciseType;
    }
    int NewRow = mItems.back()->mSubItems.size();
    beginInsertRows( index( mItems.size()-1, 0, QModelIndex() ), NewRow, NewRow );
    new Item( QFileInfo( QString::fromStdString( NewExercise.file_name() ) ).baseName(), mItems.back() );
    endInsertRows();
  } // exercise( Lesson::Exercise& )
  void StatisticsModel::finish_exercise( const Lesson::Exercise& NewExercise ) {} // exercise( Lesson::Exercise& )
  void StatisticsModel::add_stat( const ExerciseSequence::NotesStat& NewStat ) {
    if( mActivity == EmptyType || mItems.empty() || mItems.back()->mSubItems.empty() ) return;
    int NewRow = mItems.back()->mSubItems.back()->mSubItems.size();
    beginInsertRows( index( mItems.back()->mSubItems.size()-1, 0, index( mItems.size()-1, 0, QModelIndex() ) ), NewRow, NewRow );
    new StatsItem( NewStat, QString::number( NewRow ), mItems.back()->mSubItems.back() );
    endInsertRows();
  } // add_stat( const ExerciseSequence::NotesStat& )
  QModelIndex StatisticsModel::index( int Row, int Column, const QModelIndex& Parent ) const {
    if( Parent.isValid() ) {
      if( Item* It = static_cast<Item*>( Parent.internalPointer() ) )		
	return createIndex( Row, Column, It->mSubItems[ Row ] );
    }
    else return createIndex( Row, Column, mItems[ Row ] );
    return QModelIndex();
  } // index( int, int, const QModelIndex& ) const
  QModelIndex StatisticsModel::parent( const QModelIndex& Index ) const {
    if( Index.isValid() ) {
      if( Item* It = static_cast<Item*>( Index.internalPointer() ) )
	if( Item* Parent = It->mParent ) {
	  if( Item* GranParent = Parent->mParent ) {
	    for( int Row = 0; Row < GranParent->mSubItems.size(); ++Row )
	      if( GranParent->mSubItems[ Row ] == Parent )
		return createIndex( Row, 0, Parent );
	  }
	  else {
	    for( int Row = 0; Row < mItems.size(); ++Row )
	      if( mItems[ Row ] == Parent )
		return createIndex( Row, 0, Parent );
	  }
	}
    }
    return QModelIndex();
  } // parent( const QModelIndex& )
  int StatisticsModel::rowCount( const QModelIndex& Index ) const {
    if( !Index.isValid() ) return mItems.size();
    if( Item* It = static_cast<Item*>( Index.internalPointer() ) )
      return It->mSubItems.size();
    return 0;
  } // rowCount( const QModelIndex& ) const
  int StatisticsModel::columnCount( const QModelIndex& Index ) const {
    if( Index.isValid() )
      if( Item* It = static_cast<Item*>( Index.internalPointer() ) )
	return It->colsnum();
    return 7;
  } // columnCount( const QModelIndex& ) const
  QVariant StatisticsModel::headerData( int Section, Qt::Orientation Orient, int Role ) const {
    if( Role == Qt::DisplayRole && Orient == Qt::Horizontal )
      switch( Section ) {
      case 0: return tr( "Name/Try" );
      case 1: return tr( "On min" );
      case 2: return tr( "On max" );
      case 3: return tr( "Off min" );
      case 4: return tr( "Off max" );
      case 5: return tr( "Vel min" );
      case 6: return tr( "Vel max" );
      }
    return QAbstractItemModel::headerData( Section, Orient, Role );
  } // headerData( int, int ) const
  QVariant StatisticsModel::data( const QModelIndex& Index, int Role ) const {
    //!  \todo implement this
    if( !Index.isValid() ) return QVariant();
    if( Item* It = static_cast<Item*>( Index.internalPointer() ) ) {
      if( Role == Qt::DisplayRole ) return It->data( Index.column() );
      else if( Role == Qt::BackgroundRole ) return It->brush();
      else if( Role == Qt::DecorationRole && Index.column() == 0 ) return It->icon();
    }
    return QVariant();
  } // data( const QModelIndex&, int )
  
  Player::Player( MIDISequence* Play, Sequencer* Seq, QObject* Parent ) : QThread( Parent ), mSequencer( Seq ), mPlay( Play ) {}
  Player::Player( const vector<string>& PlayList, Sequencer* Seq, QObject* Parent ) : QThread( Parent ), mSequencer( Seq ), mPlayList( PlayList ) {}
  void Player::run() {
    if( !mSequencer ) return; //!< \todo Report error
    if( mPlay ) {
      mPlay->play( *mSequencer );
      emit playback_complete();
    }
    else {
      for( auto FileName : mPlayList ) {
	MIDISequence Play( FileName );
	Play.play( *mSequencer );
      }
      emit playback_complete();
    }
  } // run() override
  
  MainWindow::MainWindow( QWidget* Parent )
    : QMainWindow( Parent ), State( StateIdle ), mLesson( nullptr ), mStats( nullptr ), mStrike( 0 ), mExercise( nullptr ), mRetries( 3 ), mToGo( 3 ), mRec( nullptr ), mMIDI( nullptr ),
      mInput( nullptr ), mSequencer( nullptr ), mTransposer( nullptr ), mEchoConnector( nullptr ), mMetronome( nullptr ), mNoteTrainer( nullptr ), mNoteLow( 36 ), mNoteHigh( 96 ), mPlayer( nullptr ),
      mDBusReply( nullptr ), mScreenSaverCookie( -1 ), mViewStack( nullptr ), mScores( nullptr ), mPianoRoll( nullptr )
  {
    mUI = new Ui::MainWindow;
    mUI->setupUi( this );
    QTreeView* TV = new QTreeView( mUI->StatisticsDock );
    TV->setIndentation( 0 );
    mStats = new StatisticsModel( TV );
    TV->setModel( mStats );
    mUI->StatisticsDock->setWidget( TV );
    mUI->ActionOpen->setShortcut( QKeySequence::Open );
    connect( mUI->ActionOpen, SIGNAL( triggered() ), SLOT( open_file() ) );
    mUI->ActionSave->setShortcut( QKeySequence::Save );
    connect( mUI->ActionSave, SIGNAL( triggered() ), SLOT( save_file() ) );
    mUI->ActionSaveAs->setShortcut( QKeySequence::SaveAs );
    connect( mUI->ActionSaveAs, SIGNAL( triggered() ), SLOT( save_file_as() ) );
    connect( mUI->ActionOptions, SIGNAL( triggered() ), SLOT( edit_options() ) );
    connect( mUI->ActionMIDImixer, SIGNAL( triggered() ), SLOT( midi_mixer() ) );
    mUI->ActionQuit->setShortcut( QKeySequence::Quit );
    qApp->connect( mUI->ActionQuit, SIGNAL( triggered() ), SLOT( quit() ) );
    mViewStack = new QStackedWidget( this );
    setCentralWidget( mViewStack );
    mPianoRoll = new PianoRollView( mViewStack );
    mViewStack->addWidget( mPianoRoll );
    mUI->PianoBoard->hide();
#ifndef MUTRA_STACKED_SCORES
    mScores = new ScoresView( mUI->ScoresDock );
    mUI->ScoresDock->setWidget( mScores );
#else // MUTRA_STACKED_SCORES
    mScores = new ScoresView( mViewStack );
    mViewStack->addWidget( mScores );
    mViewStack->setCurrentWidget( mScores );
    QActionGroup* Group = new QActionGroup( mUI->ViewMenu );
    QAction* Act = new QAction( tr( "&Scores" ), Group );
    Act->setCheckable( true );
    connect( Act, &QAction::triggered, [this](){ mViewStack->setCurrentWidget( mScores ); } );
    Group->addAction( Act );
    mUI->ViewMenu->addAction( Act );
    Act = new QAction( tr( "&Piano roll" ), Group );
    Act->setCheckable( true );
    connect( Act, &QAction::triggered, [this](){ mViewStack->setCurrentWidget( mPianoRoll ); } );
    Group->addAction( Act );
    mUI->ViewMenu->addAction( Act );
    Act->setChecked( true );
#endif // MUTRA_STACKED_SCORES
    QToolBar* ExerciseBar = addToolBar( tr( "Exercise" ) );
    ExerciseBar->addAction( mUI->ActionPlay );
    connect( mUI->ActionPlay, SIGNAL( triggered( bool ) ), SLOT( toggle_playback( bool ) ) );
    ExerciseBar->addAction( mUI->ActionRecord );
    connect( mUI->ActionRecord, SIGNAL( triggered( bool ) ), SLOT( toggle_record( bool ) ) );
    ExerciseBar->addAction( mUI->ActionMetronome );
    connect( mUI->ActionMetronome, SIGNAL( triggered( bool ) ), SLOT( toggle_metronome( bool ) ) );
    ExerciseBar->addAction( mUI->ActionExercise );
    connect( mUI->ActionExercise, SIGNAL( triggered( bool ) ), SLOT( toggle_exercise( bool ) ) );
    connect( mUI->ActionPlayNotes, SIGNAL( triggered( bool ) ), SLOT( toggle_notes_training( bool ) ) );
    SystemOptions SysOp = Application::get()->system_options();
    if( mSequencer = MIDIBackend::get_manager().get_sequencer( SysOp.midi_output() ) ) {
      qDebug() << "Sequencer created.";
      mSequencer->start();
      mMetronome = new MuTraTrain::Metronome( mSequencer );
      mMetronome->options( Application::get()->metronome_options() );
    } else statusBar()->showMessage( "Can't get sequencer" );
    mInput = MIDIBackend::get_manager().get_input( SysOp.midi_input() );
    if( !mInput ) QMessageBox::warning( this, tr( "Device problem" ), tr( "Can't open input device." ) );
    else if( mSequencer ) {
      if( SysOp.transpose() ) {
	mTransposer = new Transposer( SysOp.halftones(), mInput );
      }
      if( SysOp.midi_echo() ) {
	mEchoConnector = new InputConnector( mSequencer );
	add_input_client( *mEchoConnector );
	mInput->start();
      }
    }
    connect( &mTimer, SIGNAL( timeout() ), SLOT( timer() ) );
    setWindowIcon( QIcon( ":/images/music_trainer.svg" ) );
  } // MainWindow( QWidget* )
  MainWindow::~MainWindow() {
    if( mPlayer ) delete mPlayer;
    if( mMetronome ) delete mMetronome;
    if( mSequencer ) delete mSequencer;
    if( mInput ) delete mInput;
    if( mTransposer ) delete mTransposer;
    if( mEchoConnector ) delete mEchoConnector;
    if( mMIDI ) delete mMIDI;
    if( mUI ) delete mUI;
  } // ~MainWindow()
  void MainWindow::add_input_client( InputDevice::Client& Cli ) {
    if( mTransposer ) mTransposer->add_client( Cli );
    else if( mInput ) mInput->add_client( Cli );
  } // add_input_client( Input::Client& )
  void MainWindow::remove_input_client( InputDevice::Client& Cli ) {
    if( mTransposer ) mTransposer->remove_client( Cli );
    else if( mInput ) mInput->remove_client( Cli );
  } // remove_input_client( Input::Client& )

  void MainWindow::update_buttons() { //!< \todo Implement this.
    mUI->ActionRecord->setChecked( mRec );
    mUI->ActionExercise->setEnabled( mExercise );
  } // update_buttons()
  void MainWindow::update_piano_roll() {
    if( mPianoRoll ) mPianoRoll->update_piano_roll( mMIDI );
    if( mScores ) mScores->update_scores( mMIDI );
  } // update_piano_roll()

  void MainWindow::load_file( const QString& FileName ) {
    //! \todo Analyze the file's content instead of it's name.
    if( FileName.toLower().endsWith( ".mid" ) ) {
      if( !close_file() ) return;
      mMIDIFileName = FileName.toStdString();
      mMIDI = new MIDISequence( mMIDIFileName );
      setWindowTitle( "MIDI: " + QString::fromStdString( mMIDIFileName ) + " - " + qApp->applicationDisplayName() );
      update_piano_roll();
    }
    else if( FileName.toLower().endsWith( ".mex" ) ) load_exercise( FileName.toStdString() );
    else if( FileName.toLower().endsWith( ".mles" ) ) load_lesson( FileName.toStdString() );
  } // load_file( const QString& )
  void MainWindow::open_file() {
    QString SelFilter = "Exercises (*.mex)";
    QString FileName = QFileDialog::getOpenFileName( this, tr( "Open File" ), QString::fromStdString( mMIDIFileName ), tr( "Lessons (*.mles);;Exercises (*.mex);;MIDI files (*.mid);;All files (*.*)" ),
						     &SelFilter );
    if( !FileName.isEmpty() ) load_file( FileName );
  } // open_file()
  void MainWindow::playback_complete() {
    if( mPlayer ) {
      delete mPlayer;
      mPlayer = nullptr;
    }
  } // playback_complete()
  bool MainWindow::load_lesson( const string& FileName ) {
    mLesson = new Lesson( FileName );
    if( mLesson->exercises().empty() ) return false;
    if( load_exercise( mLesson->file_name() ) ) {
      mStrike = mLesson->strike();
      mRetries = mLesson->retries();
      mToGo = mRetries;
    }
    return false;
  } // load_lesson( const string& )
  bool MainWindow::load_exercise( const string& FileName ) {
    mExerciseName = FileName;
    if( !mExercise ) mExercise = new ExerciseSequence();
    else mExercise->clear();
    qDebug() << "exercise name is: " << QString::fromStdString( mExerciseName );
    if( !mExercise->load( mExerciseName ) ) {
      delete mExercise;
      mExercise = nullptr;
      mMIDI = nullptr;
      mExerciseName.clear();
      return false;
    }
    qDebug() << "Exercise" << mExerciseName.c_str() << "loaded division" << mExercise->division()
	     << "meter" << mExercise->Numerator << "/" << (1<<mExercise->Denominator) << "tempo" << (60000000/mExercise->tempo());
    mMIDI = mExercise;
    //! \todo Ensure metronome, sequencer & input are ready
    if( mMetronome ) {
      mMetronome->division( mExercise->division() );
      mMetronome->meter( mExercise->Numerator, mExercise->Denominator );
      mMetronome->tempo( mExercise->tempo() );
      qDebug() << "Set metronome to" << mMetronome->options().beat() << "/" << (1<<mMetronome->options().measure()) << "tempo:" << (60000000/mMetronome->tempo()) << "bpm" << mMetronome->tempo() << "uspq";
    }
    update_buttons();
    update_piano_roll();
    return true;
  } // load_exercise( const string& )
  bool MainWindow::save_file() {
    if( !mMIDI ) return true;
    if( mMIDIFileName.empty() ) return save_file_as();
    return mMIDI->close_and_write( mMIDIFileName );
  } // save_file()
  bool MainWindow::save_file_as() {
    if( !mMIDI ) return true;
    QString NewName = QFileDialog::getSaveFileName( this, tr( "Save File" ), QString::fromStdString( mMIDIFileName ), tr( "MIDI files (*.mid);;All files (*.*)" ) );
    if( NewName.isEmpty() ) return false;
    if( !mMIDI->close_and_write( NewName.toStdString() ) ) {
      QMessageBox::warning( this, tr( "Save file" ), tr( "Can't write the MIDI file." ) );
      return false;
    }
    mMIDIFileName = NewName.toStdString();
    return true;
  } // save_file_as()

  bool MainWindow::close_file() {
    stop_recording();
    complete_exercise();
    if( mLesson ) { //! \todo If the file is modified then ask to save it.
      if( mStats ) mStats->finish_lesson( *mLesson );
      delete mLesson;
      mLesson = nullptr;
    }
    mExerciseName.clear();
    mExercise = nullptr; // It's the same as mMIDI
    if( mMIDI ) { //! \todo If the file is modified then ask to save it.
      delete mMIDI;
      mMIDI = nullptr;
    }
    mMIDIFileName.clear();
    update_buttons();
    update_piano_roll();
    return true;
  } // close_file()
  
  void MainWindow::edit_options() {
    SettingsDialog Dlg( this );
    Dlg.system( Application::get()->system_options() );
    if( mMetronome ) Dlg.metronome( mMetronome->options() );
    else Dlg.metronome( Application::get()->metronome_options() );
    if( mExercise ) {
      Dlg.exercise( *mExercise );
      Dlg.original_tempo( 60000000 / mExercise->original_tempo() );
    }
    Dlg.notes_options( mNoteLow, mNoteHigh );
    if( Dlg.exec() ) {
      if( mMetronome ) //!< \todo restart metronome if it was active before changing settings
	mMetronome->options( Dlg.metronome() );
      //! \todo Change current midi file metronome settings.
      if( !mMIDI ) Application::get()->metronome_options( Dlg.metronome() );
      if( mExercise && Dlg.exercise().set_to( *mExercise ) ) update_piano_roll();
      Application::get()->system_options( Dlg.system() ); //!< \todo Connect to other devices & set echo if something of these parameters has been changed
      mNoteLow = Dlg.note_low();
      mNoteHigh = Dlg.note_high();
    }
  } // edit_options()
  void MainWindow::midi_mixer() {
    MIDIMixer Dlg( this );
    Dlg.sequencer( mSequencer );
    Dlg.exec();
  } // midi_mixer()
  void MainWindow::toggle_metronome( bool On ) {
    if( mMetronome && mSequencer ) {
      if( On ) { mMetronome->start(); qDebug() << "Start metronome."; }
      else { mMetronome->stop(); qDebug() << "Stop metronome."; }
    }
  } // toggle_metronome( bool )
  void MainWindow::stop_recording() {
    if( !mRec ) return;
    if( mMetronome ) mMetronome->stop();
    if( mInput ) {
      if( !mEchoConnector ) mInput->stop();
      remove_input_client( *mRec );
    }
    mRec = nullptr;
    update_buttons();
    update_piano_roll();
  } // stop_recording()
  void MainWindow::toggle_playback( bool On ) {
    if( mPlayer ) {
      delete mPlayer;
      mPlayer = nullptr;
    }
    else if( mMIDI && mSequencer ) {
      mPlayer = new Player( mMIDI, mSequencer, this );
      connect( mPlayer, SIGNAL( playback_complete() ), SLOT( playback_complete() ) );
      mPlayer->start();
    }
  } // toggle_playback( bool )
  void MainWindow::toggle_record( bool On ) {
    if( On ) {
      if( mRec ) return;
      if( !close_file() || !mMetronome || !mInput ) {
	mUI->ActionRecord->setChecked( false );
	return;
      }
      //! \todo Check if we have a metronome & create one if needed
      mRec = new MuTraMIDI::Recorder( mMetronome->options().tempo() );
      mRec->meter( mMetronome->options().beat(), mMetronome->options().measure() );
      mMIDI = mRec;
      //! \todo Ensure we have an input device
      add_input_client( *mRec );
      if( !mEchoConnector ) mInput->start();
      mMetronome->start();
    }
    else stop_recording();
  } // toggle_record( bool )
  void MainWindow::toggle_exercise( bool On ) {
    if( mExercise && mInput ) {
      if( On ) {
	if( mToGo == 0 ) mToGo = mRetries;
	if( mStats ) {
	  if( mLesson ) {
	    if( mLesson->current() == 0 ) mStats->start_lesson( *mLesson );
	    mStats->start_exercise( mLesson->exercise( mLesson->current() ) );
	  }
	  else
	    mStats->start_exercise( Lesson::Exercise( mExerciseName ) );
	}
	start_exercise();
      }
      else complete_exercise();
    }
    update_buttons();
  } // toggle_exercise( bool )
  void MainWindow::toggle_notes_training( bool On ) {
    if( On ) {
      if( mNoteTrainer || !mInput || !mSequencer ) return;
      mNoteTrainer = new NoteTrainer( *mSequencer, mNoteLow, mNoteHigh );
      add_input_client( *mNoteTrainer );
      if( !mEchoConnector ) mInput->start();
    }
    else {
      if( !mNoteTrainer ) return;
      if( mInput ) {
	remove_input_client( *mNoteTrainer );
	if( !mEchoConnector ) mInput->stop();
      }
      delete mNoteTrainer;
      mNoteTrainer = nullptr;
    }
  } // toggle_notes_training( bool )
  void MainWindow::timer() { //! \todo Sync with metronome, not just a timer.
    if( !mExercise ) {
      mTimer.stop();
      return;
    }
    if( !mExercise->beat() ) return;
    if( complete_exercise() ) {
      if( --mToGo == 0 ) {
	mSequencer->note_on( 9, 52, 100 );
	if( mLesson && mLesson->next() && load_exercise( mLesson->file_name() ) ) {
	  mStrike = mLesson->strike();
	  mRetries = mLesson->retries();
	  mToGo = mRetries;
	  if( mStats )  mStats->start_exercise( mLesson->exercise( mLesson->current() ) );
	}
      }
    }
    else mToGo = mRetries;
    update_piano_roll();
    if( mExercise && mToGo > 0 ) start_exercise();
    else if( mStats && mLesson ) mStats->finish_lesson( *mLesson );
  } // timer()

  void MainWindow::start_exercise() {
    if( !mExercise ) return;
    //! \todo Move to some special object with platform independent call like enableScreenSaver( true/false )
    QDBusMessage Call = QDBusMessage::createMethodCall( "org.freedesktop.ScreenSaver", "/ScreenSaver", QString(), "Inhibit" );
    Call.setArguments( QList<QVariant>() << QString( "MusicTrainer" ) << QString( "Exercise" ) );
    mDBusReply = new QDBusPendingCallWatcher( QDBusConnection::sessionBus().asyncCall( Call ), this );
    connect( mDBusReply, &QDBusPendingCallWatcher::finished, [this]( QDBusPendingCallWatcher* Watcher ) {
							       QDBusPendingReply<unsigned> Reply( *Watcher );
							       if( Reply.isError() ) {
								 qDebug() << "Can't disable screensaver:" << Reply.error().message();
								 mScreenSaverCookie = -1;
							       }
							       else
								 mScreenSaverCookie = Reply.value();
							       if( Watcher == mDBusReply ) {
								 delete mDBusReply;
								 mDBusReply = nullptr;
							       }
							     } );
    add_input_client( *mExercise );
    mExercise->new_take();
    mExercise->start();
    if( mMetronome ) {
#ifdef MUTRA_DEBUG
      qDebug() << "Start metronome" << mMetronome->options().beat() << "/" << (1<<mMetronome->options().measure()) << "tempo:" << (60000000/mMetronome->tempo()) << "bpm" << mMetronome->tempo() << "uspq";
#endif // MUTRA_DEBUG
      mMetronome->start();
      mExercise->align_start( mMetronome->start_time() );
    }
    if( !mEchoConnector ) mInput->start();
    mTimer.start( (mExercise->tempo() * mMetronome->options().beat()) / 1000 );
  } // start_exercise()
  bool MainWindow::complete_exercise() {
    if( !mExercise ) return false;
    mTimer.stop();
    mMetronome->stop();
    if( !mEchoConnector ) mInput->stop();
    remove_input_client( *mExercise );
    ExerciseSequence::NotesStat Stats;
    switch( mExercise->compare( Stats ) ) {
    case ExerciseSequence::NoError: mSequencer->note_on( 9, 74, 100 ); break;
    case ExerciseSequence::NoteError: mSequencer->note_on( 9, 78, 100 ); break;
    case ExerciseSequence::RythmError: mSequencer->note_on( 9, 84, 100 ); break;
    case ExerciseSequence::VelocityError: mSequencer->note_on( 9, 81, 100 ); break;
    case ExerciseSequence::EmptyPlay:
    default:
      break;
    }
    if( mLesson ) mLesson->new_stat( Stats );
    if( mStats ) mStats->add_stat( Stats );
    qDebug() << "Statistics: start:" << Stats.StartMin << "-" << Stats.StartMax << "stop:" << Stats.StopMin << "-" << Stats.StopMax << "velocity:" << Stats.VelocityMin << "-" << Stats.VelocityMax;
    mUI->ActionExercise->setChecked( false );
    update_piano_roll();
    if( mScreenSaverCookie >= 0 ) {
      QDBusMessage Call = QDBusMessage::createMethodCall( "org.freedesktop.ScreenSaver", "/ScreenSaver", QString(), "UnInhibit" );
      Call.setArguments( QList<QVariant>() << unsigned( mScreenSaverCookie ) );
      mDBusReply = new QDBusPendingCallWatcher( QDBusConnection::sessionBus().asyncCall( Call ), this );
      connect( mDBusReply, &QDBusPendingCallWatcher::finished, [this]( QDBusPendingCallWatcher* Watcher ) {
								 QDBusPendingReply<> Reply( *Watcher );
								 if( Reply.isError() )
								   qDebug() << "Can't reenable screensaver for cookie" << mScreenSaverCookie << ":" << Reply.error().message();
								 if( Watcher == mDBusReply ) {
								   delete mDBusReply;
								   mDBusReply = nullptr;
								 }
								 mScreenSaverCookie = -1;
							       } );
    }
    return Stats.Result == ExerciseSequence::NoError;
  } // complete_exercise()
  
  Application::Application( int& argc, char** argv ) : QApplication( argc, argv ) {
    setOrganizationName( "Nick Slobodsky" );
    setOrganizationDomain( "software.slobodsky.ru" );
    setApplicationName( "Music Trainer" );
    setApplicationVersion( "0.0.1~preview1" );
    // was: Load standard settings (do I need it?)
    load_from_settings( mSystemOptions );
    load_from_settings( mMetronomeOptions );
    load_from_settings( mExerciseOptions );
    // was: Create main (MDI) window.
  } // Application()
  Application::~Application() {
    save_to_settings( mExerciseOptions );
    save_to_settings( mMetronomeOptions );
    save_to_settings( mSystemOptions );
  } // ~Application()
} // MuTraWidgets
