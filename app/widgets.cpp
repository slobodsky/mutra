#include <QSettings>
#include <QFileDialog>
#include <QToolBar>
#include <QGraphicsView>
#include "widgets.hpp"
#include <QDebug>
#include "forms/ui_main.h"
#include "forms/ui_settings_dialog.h"
#include "forms/ui_metronome_settings.h"
using MuTraMIDI::Sequencer;
using MuTraMIDI::MIDISequence;
using MuTraMIDI::NoteEvent;
using MuTraTrain::MetronomeOptions;
using std::vector;

namespace MuTraWidgets {
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

  SettingsDialog::SettingsDialog( QWidget* Parent ) : QDialog( Parent ) {
    mDlg = new Ui::SettingsDialog;
    mDlg->setupUi( this );
    QWidget* Page = new QWidget( this );
    mMetronomePage = new Ui::MetronomeSettings;
    mMetronomePage->setupUi( Page );
    for( int I = NoteEvent::PercussionFirst; I <= NoteEvent::PercussionLast; ++I ) {
      mMetronomePage->PowerNote->addItem( NoteEvent::Percussion[ I-NoteEvent::PercussionFirst ], I );
      mMetronomePage->WeakNote->addItem( NoteEvent::Percussion[ I-NoteEvent::PercussionFirst ], I );
      mMetronomePage->MediumNote->addItem( NoteEvent::Percussion[ I-NoteEvent::PercussionFirst ], I );
    }
    mDlg->SettingsTabs->addTab( Page, tr( "Metronome" ) );
  } // SettingsDialog( QWidget* )
  SettingsDialog::~SettingsDialog() {
    if( mMetronomePage ) delete mMetronomePage;
    if( mDlg ) delete mDlg;
  } // ~SettingsDialog()
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
  void SettingsDialog::accept() {
    mMetronome.beat( mMetronomePage->Beats->value() ).measure( mMetronomePage->Measure->currentIndex() ) //!< \todo Better set data in the setup
      .tempo( mMetronomePage->Tempo->value() ).note( mMetronomePage->WeakNote->currentData().toInt() ).velocity( mMetronomePage->WeakVelocity->value() )
      .medium_note( mMetronomePage->MediumNote->currentData().toInt() ).medium_velocity( mMetronomePage->MediumVelocity->value() )
      .power_note( mMetronomePage->PowerNote->currentData().toInt() ).power_velocity( mMetronomePage->PowerVelocity->value() );
    qDebug() << "Settings accepted:\nMetronome\n\tMeter:" << mMetronomePage->Beats->value() << "/" << mMetronomePage->Measure->currentText() << "\tTempo" << mMetronomePage->Tempo->value()
	     << "Power beat" << mMetronomePage->PowerNote->currentText() << "(" << mMetronomePage->PowerNote->currentData() << ") @" << mMetronomePage->PowerVelocity->value()
	     << "Weak beat" << mMetronomePage->WeakNote->currentText() << "(" << mMetronomePage->WeakNote->currentData() << ") @" << mMetronomePage->WeakVelocity->value()
	     << "Medium beat" << mMetronomePage->MediumNote->currentText() << "(" << mMetronomePage->MediumNote->currentData() << ") @" << mMetronomePage->MediumVelocity->value() << endl;
    QDialog::accept();
  } // accept()

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
	qDebug() << "Add note" << Value << "in channel" << Channel << "of track" << Track << "@" << Clock;
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
      qDebug() << "Note" << Value << "in channel" << Channel << "not found for stop @" << Clock;
    }
    void meter( int Numerator, int Denominator ) {
      qDebug() << "Metar: " << Numerator << "/" << Denominator << endl;
      Beats = Numerator;
      Measure = Denominator;
    } // meter( int, int )
    void start() { Time = 0; Clock = 0; }
    void wait_for( unsigned WaitClock ) { Clock = WaitClock; Time = Clock  * tempo(); }
    vector<Note> Notes;
    int* Delays;

    int Beats;
    int Measure;
  }; // NotesListBuilder
  
  MainWindow::MainWindow( QWidget* Parent ) : QMainWindow( Parent ), mMIDI( nullptr ), mInput( nullptr ), mSequencer( nullptr ), mMetronome( nullptr ) {
    mUI = new Ui::MainWindow;
    mUI->setupUi( this );
    mUI->ActionOpen->setShortcut( QKeySequence::Open );
    connect( mUI->ActionOpen, SIGNAL( triggered() ), SLOT( open_file() ) );
    connect( mUI->ActionOptions, SIGNAL( triggered() ), SLOT( edit_options() ) );
    mUI->ActionQuit->setShortcut( QKeySequence::Quit );
    qApp->connect( mUI->ActionQuit, SIGNAL( triggered() ), SLOT( quit() ) );
    setCentralWidget( new QGraphicsView( this ) );
    QToolBar* ExerciseBar = addToolBar( tr( "Exercise" ) );
    QAction* ActionMetronome = ExerciseBar->addAction( tr( "Metronome" ) );
    ActionMetronome->setCheckable( true );
    connect( ActionMetronome, SIGNAL( triggered( bool ) ), SLOT( toggle_metronome( bool ) ) );
    if( mSequencer = Sequencer::get_instance( "alsa://24" ) ) {
      qDebug() << "Sequencer created.";
      mSequencer->start();
      mMetronome = new MuTraTrain::Metronome( mSequencer );
    } else statusBar()->showMessage( "Can't get sequencer" );
  } // MainWindow( QWidget* )
  MainWindow::~MainWindow() {
    if( mMetronome ) delete mMetronome;
    if( mSequencer ) delete mSequencer;
    if( mInput ) delete mInput;
    if( mMIDI ) delete mMIDI;
    if( mUI ) delete mUI;
  } // ~MainWindow()

  const int ColorsNum = 7;
  QColor Colors[] = { QColor( 255, 255, 255, 64 ), QColor( 0, 255, 0, 64 ), QColor( 0, 255, 255, 64 ), QColor( 0, 0, 255, 64 ), QColor( 255, 0, 255, 64 ), QColor( 255, 0, 0, 64 ), QColor( 255, 255, 0, 64 ) };
  
  void MainWindow::open_file() {
    QString SelFilter;
    QString FileName = QFileDialog::getOpenFileName( this, tr( "Open File" ), QString(), tr( "Lessons (*.mles);;Exercises (*.mex);;MIDI files (*.mid);;All files (*.*)" ), &SelFilter );
    if( !FileName.isEmpty() ) {
      qDebug() << "Selected:" << FileName << "of type" << SelFilter;
      if( FileName.endsWith( ".mid" ) ) {
	if( mMIDI ) delete mMIDI;
	mMIDI = new MIDISequence( FileName.toStdString() );
	if( QGraphicsView* View = qobject_cast<QGraphicsView*>( centralWidget() ) ) {
	  QGraphicsScene* Sc = new QGraphicsScene( View );
	  int Div = mMIDI->division();
	  NotesListBuilder NL( mMIDI->tracks().size() );
	  int TracksCount = mMIDI->tracks().size();
	  mMIDI->play( NL );
	  qreal K = Div / 32;
	  qreal H = TracksCount < 5 ? 16 : TracksCount * 4;
	  qreal BarH = H / TracksCount;
	  qDebug() << "We have" << NL.Notes.size() << "notes.";
	  for( int I = 0; I < TracksCount; ++I )
	    qDebug() << "Track" << I << "delay" << NL.Delays[ I ];
	  int Finish = 0;
	  int Low = 127;
	  int High = 0;
	  for( Note N: NL.Notes ) {
	    if( N.mStop - NL.Delays[ N.mTrack ] + NL.Delays[ 0 ] > Finish ) Finish = N.mStop - NL.Delays[ N.mTrack ] + NL.Delays[ 0 ];
	    if( N.mValue < Low ) Low = N.mValue;
	    if( N.mValue > High ) High = N.mValue;
	  }
	  if( High > Low ) {
	    int Top = ( 60-High ) * H;
	    int Bottom = ( 60-Low+1 ) * H;
	    int Beat = 0;
	    for( int X = 0; X < Finish; X += Div ) {
	      QPen Pen;
	      if( NL.Beats != 0 && Beat % NL.Beats == 0 ) Pen = QPen( Qt::black, 2 );
	      ++Beat;
	      Sc->addLine( X / K, Top, X /K, Bottom, Pen );
	    }
	    for( int Y = Top; Y <= Bottom; Y += H )
	      Sc->addLine( 0, Y, Finish / K, Y );
	  }
	  for( Note N: NL.Notes ) {
	    qDebug() << "Note" << N.mValue << "in track" << N.mTrack << "[" << N.mStart << "-" << N.mStop;
	    Sc->addRect( (N.mStart - NL.Delays[ N.mTrack ] + NL.Delays[ 0 ]) / K, (60-N.mValue) * H + (N.mTrack * BarH), (N.mStop < 0 ? Div : N.mStop-N.mStart) / K, BarH,
			 QPen( N.mStop < 0 ? Qt::red : Qt::white ), QBrush( Colors[ N.mTrack % ColorsNum ] ) );
	  }
	  View->setScene( Sc );
	}
      }
    }
  } // open_file()

  void MainWindow::edit_options() {
    SettingsDialog Dlg( this );
    if( mMetronome ) Dlg.metronome( mMetronome->options() );
    else Dlg.metronome( Application::get()->metronome() );
    if( Dlg.exec() ) {
      if( mMetronome ) mMetronome->options( Dlg.metronome() );
      else Application::get()->metronome( Dlg.metronome() );
    }
  } // edit_options()

  void MainWindow::toggle_metronome( bool On ) {
    if( mMetronome && mSequencer ) {
      if( On ) { mMetronome->start(); qDebug() << "Start metronome."; }
      else { mMetronome->stop(); qDebug() << "Stop metronome."; }
    }
  } // toggle_metronome( bool )
  
  Application::Application( int& argc, char** argv ) : QApplication( argc, argv ) {
    setOrganizationName( "Nick Slobodsky" );
    setOrganizationDomain( "software.slobodsky.ru" );
    setApplicationName( "Music Trainer" );
    setApplicationVersion( "0.0.1~preview1" );
    // was: Load standard settings (do I need it?)
    load_from_settings( Metronome );
    Exercise.load_from_settings();
    // was: Create main (MDI) window.
  } // Application()
  Application::~Application() {
    Exercise.save_to_settings();
    save_to_settings( Metronome );
  } // ~Application()
} // MuTraWidgets
