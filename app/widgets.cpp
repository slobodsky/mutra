#include <QSettings>
#include <QFileDialog>
#include <QGraphicsView>
#include "widgets.hpp"
#include <QDebug>
using MuTraMIDI::Sequencer;
using MuTraMIDI::MIDISequence;
using std::vector;

namespace MuTraWidgets {
  MetronomeOptions::MetronomeOptions() : Beat( 4 ), Note( 42 ), Velocity( 80 ), PowerNote( 35 ), PowerVelocity( 100 ) {
  } // MetronomeOptions()
  void MetronomeOptions::load_from_settings() {
    QSettings Set;
    // Load metronome settings
    Beat = Set.value( "Metronome/Beat", Beat ).toInt();
    Note = Set.value( "Metronome/Note", Note ).toInt();
    Velocity = Set.value( "Metronome/Velocity", Velocity ).toInt();
    PowerNote = Set.value( "Metronome/PowerNote", PowerNote ).toInt();
    PowerVelocity = Set.value( "Metronome/PowerVelocity", PowerVelocity ).toInt();
  } // load_from_settings()
  void MetronomeOptions::save_to_settings() {
    QSettings Set;
    // Save metronome settings
    Set.setValue( "Metronome/Beat", Beat );
    Set.setValue( "Metronome/Note", Note );
    Set.setValue( "Metronome/Velocity", Velocity );
    Set.setValue( "Metronome/PowerNote", PowerNote );
    Set.setValue( "Metronome/PowerVelocity", PowerVelocity );
  } // save_to_settings()

  ExerciseOptions::ExerciseOptions() : StartThreshold( 120 ), StopThreshold( 120 ), VelocityThreshold( 64 ), Duration( 0 ) {}
  void ExerciseOptions::load_from_settings() {
    QSettings Set;
    // Load exercise settings
    StartThreshold = Set.value( "Exercise/StartThreshold", StartThreshold ).toInt();
    StopThreshold = Set.value( "Exercise/StopThreshold", StopThreshold ).toInt();
    VelocityThreshold = Set.value( "Exercise/VelocityThreshold", VelocityThreshold ).toInt();
    Duration = Set.value( "Exercise/Duration", 0 ).toInt();
  } // load_from_settings()
  void ExerciseOptions::save_to_settings() {
    QSettings Set;
    // Save exercise settings
    Set.setValue( "Exercise/StartThreshold", StartThreshold );
    Set.setValue( "Exercise/StopThreshold", StopThreshold );
    Set.setValue( "Exercise/VelocityThreshold", VelocityThreshold );
    Set.setValue( "Exercise/Duration", Duration );
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

  class NotesListBuilder : public Sequencer {
  public:
    NotesListBuilder( int TracksNum = 1 ) : Delays( nullptr ) {
      Delays = new int[ TracksNum ]; // The number must be correct or this will crush. Maybe use vecror & resize it?
      for( int I = 0; I < TracksNum; ++I ) Delays[ I ] = -1;
    }
    ~NotesListBuilder() { if( Delays ) delete [] Delays; }
    void note_on( int Channel, int Value, int Velocity ) {
      if( Velocity == 0 ) note_off( Channel, Value, Velocity );
      else {
	qDebug() << "Add note" << Value << "in channel" << Channel;
	if( Delays[ Track ] < 0 ) Delays[ Track ] = Clock;
	Notes.push_back( Note( Value, Velocity, Clock, -1, Channel, Track ) );
      }
    }
    void note_off( int Channel, int Value, int Velocity ) {
      for( auto It = Notes.rbegin(); It != Notes.rend(); ++It )
	if( It->mValue == Value && It->mChannel == Channel && It->mTrack == Track && It->mStop < 0 && It->mStart <= Clock ) {
	  It->mStop = Clock;
	  return;
	}
    }
    void start() { Time = 0; Clock = 0; }
    void wait_for( unsigned WaitClock ) { Clock = WaitClock; Time = Clock  * tempo(); }
    vector<Note> Notes;
    int* Delays;
  }; // NotesListBuilder
  
  MainWindow::MainWindow( QWidget* Parent ) : QMainWindow( Parent ), MIDI( nullptr ) {
    UI.setupUi( this );
    connect( UI.ActionOpen, SIGNAL( triggered() ), SLOT( open_file() ) );
    qApp->connect( UI.ActionQuit, SIGNAL( triggered() ), SLOT( quit() ) );
    setCentralWidget( new QGraphicsView( this ) );
  } // MainWindow( QWidget* )
  MainWindow::~MainWindow() {
    if( MIDI ) delete MIDI;
  } // ~MainWindow()

  const int ColorsNum = 7;
  QColor Colors[] = { QColor( 255, 255, 255, 64 ), QColor( 0, 255, 0, 64 ), QColor( 0, 255, 255, 64 ), QColor( 0, 0, 255, 64 ), QColor( 255, 0, 255, 64 ), QColor( 255, 0, 0, 64 ), QColor( 255, 255, 0, 64 ) };
  
  void MainWindow::open_file() {
    QString SelFilter;
    QString FileName = QFileDialog::getOpenFileName( this, tr( "Open File" ), QString(), tr( "Lessons (*.mles);;Exercises (*.mex);;MIDI files (*.mid);;All files (*.*)" ), &SelFilter );
    if( !FileName.isEmpty() ) {
      qDebug() << "Selected:" << FileName << "of type" << SelFilter;
      if( FileName.endsWith( ".mid" ) ) {
	if( MIDI ) delete MIDI;
	MIDI = new MIDISequence( FileName.toStdString() );
	if( QGraphicsView* View = qobject_cast<QGraphicsView*>( centralWidget() ) ) {
	  QGraphicsScene* Sc = new QGraphicsScene( View );
	  int Div = MIDI->division();
	  NotesListBuilder NL( MIDI->tracks().size() );
	  MIDI->play( NL );
	  qreal K = Div / 32;
	  qreal H = 16;
	  qDebug() << "We have" << NL.Notes.size() << "notes.";
	  for( int I = 0; I < MIDI->tracks().size(); ++I )
	    qDebug() << "Track" << I << "delay" << NL.Delays[ I ];
	  int Finish = 0;
	  int Low = 127;
	  int High = 0;
	  for( Note N: NL.Notes ) {
	    if( N.mStop - NL.Delays[ N.mTrack ] + NL.Delays[ 0 ] > Finish ) Finish = N.mStop - NL.Delays[ N.mTrack ] + NL.Delays[ 0 ];
	    if( N.mValue < Low ) Low = N.mValue;
	    if( N.mValue > High ) High = N.mValue;
	    qDebug() << "Note" << N.mValue << "in track" << N.mTrack << "[" << N.mStart << "-" << N.mStop;
	    if( N.mStop < 0 ) continue;
	    Sc->addRect( (N.mStart - NL.Delays[ N.mTrack ] + NL.Delays[ 0 ]) / K, (60-N.mValue) * H, (N.mStop-N.mStart) / K, H, QPen( Qt::white, 3 ), QBrush( Colors[ N.mTrack % ColorsNum ] ) );
	  }
	  if( High > Low ) {
	    int Top = ( 60-High ) * H;
	    int Bottom = ( 60-Low ) * H;
	    for( int X = 0; X < Finish; X += Div )
	      Sc->addLine( X / K, Top, X /K, Bottom );
	  }
	  View->setScene( Sc );
	}
      }
    }
  } // open_file()
  
  Application::Application( int& argc, char** argv ) : QApplication( argc, argv ) {
    setOrganizationName( "Nick Slobodsky" );
    setOrganizationDomain( "software.slobodsky.ru" );
    setApplicationName( "Music Trainer" );
    setApplicationVersion( "0.0.1~preview1" );
    // was: Load standard settings (do I need it?)
    Metronome.load_from_settings();
    Exercise.load_from_settings();
    // was: Create main (MDI) window.
  } // Application()
  Application::~Application() {
    Exercise.save_to_settings();
    Metronome.save_to_settings();
  } // ~Application()
} // MuTraWidgets
