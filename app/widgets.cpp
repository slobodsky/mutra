#include <QSettings>
#include <QFileDialog>
#include <QToolBar>
#include <QGraphicsView>
#include <QMessageBox>
#include "widgets.hpp"
#include <QDebug>
#include "forms/ui_main.h"
#include "forms/ui_settings_dialog.h"
#include "forms/ui_metronome_settings.h"
#include "forms/ui_devices_settings.h"
#include "forms/ui_exercise_settings.h"
#include "forms/ui_midi_mixer.h"
using MuTraMIDI::get_time_us;
using MuTraMIDI::InputDevice;
using MuTraMIDI::Sequencer;
using MuTraMIDI::MIDISequence;
using MuTraMIDI::NoteEvent;
using MuTraMIDI::InputConnector;
using MuTraTrain::MetronomeOptions;
using MuTraTrain::ExerciseSequence;
using MuTraTrain::Lesson;
using std::vector;
using std::string;

namespace MuTraWidgets {
  void load_from_settings( SystemOptions& Options ) {
    QSettings Set;
    Options.midi_input( Set.value( "System/MIDIInput", QString::fromStdString( Options.midi_input() ) ).toString().toStdString() )
      .midi_output( Set.value( "System/MIDIOutput", QString::fromStdString( Options.midi_output() ) ).toString().toStdString() )
      .midi_echo( Set.value( "System/MIDIEcho", Options.midi_echo() ).toBool() );
  } // load_from_settings( MetronomeOptions& 
  void save_to_settings( const SystemOptions& Options ) {
    QSettings Set;
    Set.setValue( "System/MIDIInput", QString::fromStdString( Options.midi_input() ) );
    Set.setValue( "System/MIDIOutput", QString::fromStdString( Options.midi_output() ) );
    Set.setValue( "System/MIDIEcho", Options.midi_echo() );
  } // save_to_settings( const SystemOptions& )

  SystemOptions::SystemOptions() : mMIDIEcho( false ) {}
  
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
    qDebug() << "Set data for item's" << Index << "role" << Role << "to" << Value;
    return setData( Index, Value, Role );
  } // setData( const QModelIndex&, const QVariant&, int )
  
  SettingsDialog::SettingsDialog( QWidget* Parent ) : QDialog( Parent ), mOriginalTempo( 120 ), mMetronomePage( nullptr ), mDevicesPage( nullptr ), mExercisePage( nullptr ) {
    mDlg = new Ui::SettingsDialog;
    mDlg->setupUi( this );
    //! \todo Move the dialogs to separate objects.
    QWidget* Page = new QWidget( this );
    mDevicesPage = new Ui::DevicesSettings;
    mDevicesPage->setupUi( Page );
    for( const InputDevice::Info& Dev : InputDevice::get_available_devices() ) mDevicesPage->MIDIInput->addItem( QString::fromStdString( Dev.name() ), QString::fromStdString( Dev.uri() ) );
    for( const Sequencer::Info& Dev : Sequencer::get_available_devices() ) mDevicesPage->MIDIOutput->addItem( QString::fromStdString( Dev.name() ), QString::fromStdString( Dev.uri() ) );
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
  } // SettingsDialog( QWidget* )
  SettingsDialog::~SettingsDialog() {
    if( mMetronomePage ) delete mMetronomePage;
    if( mDlg ) delete mDlg;
  } // ~SettingsDialog()
  void SettingsDialog::system( const SystemOptions& NewOptions ) {
    mDevicesPage->MIDIInput->setCurrentIndex( mDevicesPage->MIDIInput->findData( QString::fromStdString( NewOptions.midi_input() ) ) );
    mDevicesPage->MIDIOutput->setCurrentIndex( mDevicesPage->MIDIOutput->findData( QString::fromStdString( NewOptions.midi_output() ) ) );
    mDevicesPage->MIDIEcho->setChecked( NewOptions.midi_echo() );
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
  void SettingsDialog::tempo_skew_changed( int Value ) {
    mMetronomePage->Tempo->setValue( mOriginalTempo * ( Value / 100.0 ) );
  } // tempo_skew_changed( int )
  void SettingsDialog::accept() {
    mMetronome.beat( mMetronomePage->Beats->value() ).measure( mMetronomePage->Measure->currentIndex() ) //!< \todo Better set data in the setup
      .tempo( mMetronomePage->Tempo->value() ).note( mMetronomePage->WeakNote->currentData().toInt() ).velocity( mMetronomePage->WeakVelocity->value() )
      .medium_note( mMetronomePage->MediumNote->currentData().toInt() ).medium_velocity( mMetronomePage->MediumVelocity->value() )
      .power_note( mMetronomePage->PowerNote->currentData().toInt() ).power_velocity( mMetronomePage->PowerVelocity->value() );
    mSystem.midi_input( mDevicesPage->MIDIInput->currentData().toString().toStdString() ).midi_output( mDevicesPage->MIDIOutput->currentData().toString().toStdString() )
      .midi_echo( mDevicesPage->MIDIEcho->isChecked() );
    int Start = mExercisePage->FragmentStart->value();
    int Finish = mExercisePage->FragmentFinish->value();
    mExercise.fragment_start( Start ).fragment_duration( Finish < 0 ? -1 : Finish >= Start ? Finish - Start : 0 ).tempo_skew( mExercisePage->TempoPercent->value() / 100.0 )
      .note_on_low( mExercisePage->NoteOnLow->value() ).note_off_low( mExercisePage->NoteOffLow->value() ).velocity_low( mExercisePage->VelocityLow->value() );
    if( TracksChannelsModel* Mod = qobject_cast<TracksChannelsModel*>( mExercisePage->Tracks->model() ) )
      mExercise.tracks_count( Mod->tracks_count() ).tracks( Mod->tracks() ).channels( Mod->channels() );
#ifdef MUTRA_DEBUG
    qDebug() << "Settings accepted:\nMetronome\n\tMeter:" << mMetronomePage->Beats->value() << "/" << mMetronomePage->Measure->currentText() << "\tTempo" << mMetronomePage->Tempo->value()
	     << "Power beat" << mMetronomePage->PowerNote->currentText() << "(" << mMetronomePage->PowerNote->currentData() << ") @" << mMetronomePage->PowerVelocity->value()
	     << "Weak beat" << mMetronomePage->WeakNote->currentText() << "(" << mMetronomePage->WeakNote->currentData() << ") @" << mMetronomePage->WeakVelocity->value()
	     << "Medium beat" << mMetronomePage->MediumNote->currentText() << "(" << mMetronomePage->MediumNote->currentData() << ") @" << mMetronomePage->MediumVelocity->value() << endl;
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
#ifdef MUTRA_DEBUG
	qDebug() << "Add note" << Value << "in channel" << Channel << "of track" << Track << "@" << Clock;
#endif
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
#ifdef MUTRA_DEBUG
      qDebug() << "Metar: " << Numerator << "/" << Denominator << endl;
#endif
      Beats = Numerator;
      Measure = Denominator;
    } // meter( int, int )
    void start() { Time = 0; Clock = 0; }
    void wait_for( unsigned WaitClock ) { Clock = WaitClock; Time = Clock * tempo(); }
    vector<Note> Notes;
    int* Delays;

    int Beats;
    int Measure;
  }; // NotesListBuilder

  class StatisticsModel : public QAbstractItemModel {
  public:
    StatisticsModel( QObject* Parent = nullptr ) : QAbstractItemModel( Parent ), mLesson( nullptr ) {} // StatisticsModel( QObject* )
    Lesson* lesson() const { return mLesson; }
    void lesson( Lesson* NewLesson );
    void add_stat( const ExerciseSequence::NotesStat& NewStat );
    // Model overrides
    QModelIndex index( int Row, int Column, const QModelIndex& Parent ) const;
    QModelIndex parent( const QModelIndex& Index ) const;
    int rowCount( const QModelIndex& Index ) const;
    int columnCount( const QModelIndex& Index ) const;
    QVariant headerData( int Section, Qt::Orientation Orient, int Role ) const;
    QVariant data( const QModelIndex& Index, int Role ) const;
  private:
    enum { EmptyType, LessonType, ExerciseType, StatsType };
    static unsigned type( quintptr ID ) { return ID & 0xF; }
    static unsigned exercise( quintptr ID ) { return ( ID >> 4 ) & 0xFFF; }
    static unsigned stats( quintptr ID ) { return ( ID >> 16 ) & 0xFFFF; }
    static quintptr pack( unsigned Type, unsigned Exercise = 0, unsigned Stats = 0 ) { return ( Type & 0xF ) | ( ( Exercise & 0xFFF ) << 4 ) | ( ( Stats & 0xFFFF ) << 16 ); }
    Lesson* mLesson;
    vector<ExerciseSequence::NotesStat> mStats;
  }; // StatisticsModel
  void StatisticsModel::lesson( Lesson* NewLesson ) {
    beginResetModel();
    mLesson = NewLesson;
    mStats.clear();
    endResetModel();
  } // lesson( Lesson* )
  void StatisticsModel::add_stat( const ExerciseSequence::NotesStat& NewStat ) {
    beginInsertRows( QModelIndex(), mStats.size(), mStats.size() );
    mStats.push_back( NewStat );
    endInsertRows();
  } // add_stat( const ExerciseSequence::NotesStat& )
  QModelIndex StatisticsModel::index( int Row, int Column, const QModelIndex& Parent ) const {
    if( Parent.isValid() ) {
      switch( type( Parent.internalId() ) ) {
      case LessonType: return createIndex( Row, Column, pack( ExerciseType, Row ) );
      case ExerciseType: return createIndex( Row, Column, pack( StatsType, Parent.row(), Row ) );
      }
    }
    else if( mLesson ) { if( Row == 0 ) return createIndex( Row, Column, pack( LessonType, Row ) ); }
    else if( Row < mStats.size() ) return createIndex( Row, Column, pack( StatsType, 0, Row ) );
    return QModelIndex();
  } // index( int, int, const QModelIndex& ) const
  QModelIndex StatisticsModel::parent( const QModelIndex& Index ) const {
    if( Index.isValid() ) {
      switch( type( Index.internalId() ) ) {
      case LessonType: return QModelIndex();
      case ExerciseType: return createIndex( 0, 0, pack( LessonType ) );
      case StatsType: if( mLesson ) {
	  int Row = exercise( Index.internalId() );
	  return createIndex( Row, 0, pack( ExerciseType, Row ) );
	}
	break;
      }
    }
    return QModelIndex();
  } // parent( const QModelIndex& )
  int StatisticsModel::rowCount( const QModelIndex& Index ) const {
    //! \todo Make stats for exercises without lesson.
    //qDebug() << "Get rows for index" << Index;
    if( !mLesson ) return mStats.size();
    if( !Index.isValid() ) { /*qDebug() << "We have lesson";*/ return 1; }
    if( Index.column() != 0 ) return 0;
    switch( type( Index.internalId() ) ) {
    case LessonType: /*qDebug() << "Lesson have" << mLesson->exercises().size() << "exercises.";*/ return mLesson->exercises().size();
    case ExerciseType: /*qDebug() << "Exercise have" << mLesson->exercise( Index.row() ).statistics().size() << "statistics.";*/ return mLesson->exercise( Index.row() ).statistics().size();
    }
    return 0;
  } // rowCount( const QModelIndex& ) const
  int StatisticsModel::columnCount( const QModelIndex& Index ) const {
    if( Index.isValid() )
      switch( type( Index.internalId() ) ) {
      case LessonType:
      case ExerciseType:
	return 1;
      case StatsType:
	return 7;
      }
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
    if( Role == Qt::DisplayRole )
      switch( type( Index.internalId() ) ) {
      case LessonType:
	if( Index.column() == 0 ) return QFileInfo( QString::fromStdString( mLesson->lesson_name() ) ).baseName();
	return QVariant();
      case ExerciseType: {
	const Lesson::Exercise& Ex = mLesson->exercise( exercise( Index.internalId() ) );
	switch( Index.column() ) { //! \todo Add exercise limits here
	case 0: return QFileInfo( QString::fromStdString( Ex.file_name() ) ).baseName();
	}
      }	break;
      case StatsType:
	if( Index.column() == 0 ) return QVariant( Index.row()+1 );
	else {
	  ExerciseSequence::NotesStat Stats = mLesson ? mLesson->exercise( exercise( Index.internalId() ) ).stats( stats( Index.internalId() ) ) : mStats[ stats( Index.internalId() ) ];
	  if( Stats.Result < ExerciseSequence::NoteError )
	    switch( Index.column() ) {
	    case 1: return QVariant( Stats.StartMin );
	    case 2: return QVariant( Stats.StartMax );
	    case 3: return QVariant( Stats.StopMin );
	    case 4: return QVariant( Stats.StopMax );
	    case 5: return QVariant( Stats.VelocityMin );
	    case 6: return QVariant( Stats.VelocityMax );
	    }
	}
	break;
      }
    else if( Role == Qt::BackgroundRole ) {
      switch( type( Index.internalId() ) ) {
      case LessonType: return QBrush( QColor( 128, 128, 128 ) );
      case ExerciseType: return QBrush( QColor( 128, 128, 128, 128 ) );
      }
    }
    else if( Role == Qt::DecorationRole ) {
      if( Index.column() == 0 && type( Index.internalId() ) == StatsType )
	switch( mLesson ? mLesson->exercise( exercise( Index.internalId() ) ).stats( stats( Index.internalId() ) ).Result : mStats[ stats( Index.internalId() ) ].Result ) {
	case ExerciseSequence::NoError: return QIcon::fromTheme( "gtk-yes" );
	case ExerciseSequence::NoteError: return QIcon::fromTheme( "gtk-no" );
	case ExerciseSequence::RythmError: return QIcon::fromTheme( "preferences-desktop-thunderbolt" );
	case ExerciseSequence::VelocityError: return QIcon::fromTheme( "preferences-desktop-notification-bell" );
	case ExerciseSequence::EmptyPlay: return QIcon::fromTheme( "user-offline" );
	}
    }
    return QVariant();
  } // data( const QModelIndex&, int )
  
  MainWindow::MainWindow( QWidget* Parent )
    : QMainWindow( Parent ), State( StateIdle ), mLesson( nullptr ), mStats( nullptr ), mStrike( 0 ), mExercise( nullptr ), mRetries( 3 ), mToGo( 3 ), mRec( nullptr ), mMIDI( nullptr ),
      mInput( nullptr ), mSequencer( nullptr ), mEchoConnector( nullptr ), mMetronome( nullptr )
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
    setCentralWidget( new QGraphicsView( this ) );
    QToolBar* ExerciseBar = addToolBar( tr( "Exercise" ) );
    ExerciseBar->addAction( mUI->ActionMetronome );
    connect( mUI->ActionMetronome, SIGNAL( triggered( bool ) ), SLOT( toggle_metronome( bool ) ) );
    ExerciseBar->addAction( mUI->ActionRecord );
    connect( mUI->ActionRecord, SIGNAL( triggered( bool ) ), SLOT( toggle_record( bool ) ) );
    ExerciseBar->addAction( mUI->ActionExercise );
    connect( mUI->ActionExercise, SIGNAL( triggered( bool ) ), SLOT( toggle_exercise( bool ) ) );
    SystemOptions SysOp = Application::get()->system_options();
    if( mSequencer = Sequencer::get_instance( SysOp.midi_output() ) ) {
      qDebug() << "Sequencer created.";
      mSequencer->start();
      mMetronome = new MuTraTrain::Metronome( mSequencer );
      mMetronome->options( Application::get()->metronome_options() );
    } else statusBar()->showMessage( "Can't get sequencer" );
    mInput = InputDevice::get_instance( SysOp.midi_input() );
    if( !mInput ) QMessageBox::warning( this, tr( "Device problem" ), tr( "Can't open input device." ) );
    else if( mSequencer && SysOp.midi_echo() ) {
      mEchoConnector = new InputConnector( mSequencer );
      mInput->add_client( *mEchoConnector );
      mInput->start();
    }
    connect( &mTimer, SIGNAL( timeout() ), SLOT( timer() ) );
  } // MainWindow( QWidget* )
  MainWindow::~MainWindow() {
    if( mMetronome ) delete mMetronome;
    if( mSequencer ) delete mSequencer;
    if( mInput ) delete mInput;
    if( mEchoConnector ) delete mEchoConnector;
    if( mMIDI ) delete mMIDI;
    if( mUI ) delete mUI;
  } // ~MainWindow()

  const int ColorsNum = 7;
  QColor Colors[] = { QColor( 255, 255, 255, 64 ), QColor( 0, 255, 0, 64 ), QColor( 0, 255, 255, 64 ), QColor( 0, 0, 255, 64 ), QColor( 255, 0, 255, 64 ), QColor( 255, 0, 0, 64 ), QColor( 255, 255, 0, 64 ) };

  void MainWindow::update_buttons() { //!< \todo Implement this.
    mUI->ActionRecord->setChecked( mRec );
    mUI->ActionExercise->setEnabled( mExercise );
  } // update_buttons()
  void MainWindow::update_piano_roll() {
    if( QGraphicsView* View = qobject_cast<QGraphicsView*>( centralWidget() ) ) {
      QGraphicsScene* Sc = new QGraphicsScene( View );
      if( mMIDI ) {
	int Div = mMIDI->division();
	int TracksCount = mMIDI->tracks().size();
	int DrawTracks = TracksCount < 4 ? TracksCount : 4;
	NotesListBuilder NL( TracksCount );
	mMIDI->play( NL );
	qreal K = Div / 32;
	qreal H = DrawTracks < 5 ? 16 : DrawTracks * 4;
	qreal BarH = H / DrawTracks;
#ifdef MUTRA_DEBUG
	qDebug() << "We have" << NL.Notes.size() << "notes.";
	for( int I = 0; I < TracksCount; ++I )
	  qDebug() << "Track" << I << "delay" << NL.Delays[ I ];
#endif
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
#ifdef MUTRA_DEBUG
	  qDebug() << "Note" << N.mValue << "in track" << N.mTrack << "[" << N.mStart << "-" << N.mStop;
#endif
	  int TrackPlace = N.mTrack - (TracksCount-DrawTracks);
	  if( N.mTrack == 0 ) TrackPlace = 0;
	  else
	    if( TrackPlace <= 0 ) continue;
	  QColor Clr = Colors[ TrackPlace % ColorsNum ];
	  Clr.setAlphaF( N.mVelocity / 127.0 );
	  Sc->addRect( (N.mStart - NL.Delays[ N.mTrack ] + NL.Delays[ 0 ]) / K, (60-N.mValue) * H + (TrackPlace * BarH), (N.mStop < 0 ? Div : N.mStop-N.mStart) / K, BarH,
		       QPen( N.mStop < 0 ? Qt::red : Qt::white ), QBrush( Clr ) );
	}
      }
      View->setScene( Sc );
    }
  } // update_piano_roll()
  
  void MainWindow::open_file() {
    QString SelFilter = "MIDI files (*.mid)";
    QString FileName = QFileDialog::getOpenFileName( this, tr( "Open File" ), QString::fromStdString( mMIDIFileName ), tr( "Lessons (*.mles);;Exercises (*.mex);;MIDI files (*.mid);;All files (*.*)" ),
						     &SelFilter );
    if( !FileName.isEmpty() ) {
      //! \todo Analyze the file's content instead of it's name.
      if( FileName.endsWith( ".mid" ) ) {
	if( !close_file() ) return;
	mMIDIFileName = FileName.toStdString();
	mMIDI = new MIDISequence( mMIDIFileName );
	setWindowTitle( "MIDI: " + QString::fromStdString( mMIDIFileName ) + " - " + qApp->applicationDisplayName() );
	update_piano_roll();
      }
      else if( FileName.endsWith( ".mex" ) ) load_exercise( FileName.toStdString() );
      else if( FileName.endsWith( ".mles" ) ) load_lesson( FileName.toStdString() );
    }
  } // open_file()
  bool MainWindow::load_lesson( const string& FileName ) {
    mLesson = new Lesson( FileName );
    if( mStats ) mStats->lesson( mLesson );
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
    if( mStats ) mStats->lesson( nullptr );
    if( mLesson ) { //! \todo If the file is modified then ask to save it.
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
    if( Dlg.exec() ) {
      if( mMetronome ) //!< \todo restart metronome if it was active before changing settings
	mMetronome->options( Dlg.metronome() );
      //! \todo Change current midi file metronome settings.
      if( !mMIDI ) Application::get()->metronome_options( Dlg.metronome() );
      if( mExercise && Dlg.exercise().set_to( *mExercise ) ) update_piano_roll();
      Application::get()->system_options( Dlg.system() ); //!< \todo Connect to other devices & set echo if something of these parameters has been changed
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
      mInput->remove_client( *mRec );
    }
    mRec = nullptr;
    update_buttons();
    update_piano_roll();
  } // stop_recording()
  void MainWindow::toggle_playback( bool On ) {
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
      mInput->add_client( *mRec );
      if( !mEchoConnector ) mInput->start();
      mMetronome->start();
    }
    else stop_recording();
  } // toggle_record( bool )
  void MainWindow::toggle_exercise( bool On ) {
    if( mExercise && mInput ) {
      if( On ) {
	if( mToGo == 0 ) mToGo = mRetries;
	start_exercise();
      }
      else complete_exercise();
    }
    update_buttons();
  } // toggle_exercise( bool )
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
	}
      }
    }
    else mToGo = mRetries;
    update_piano_roll();
    if( mExercise && mToGo > 0 ) start_exercise();
  } // timer()

  void MainWindow::start_exercise() {
    if( !mExercise ) return;
    mInput->add_client( *mExercise );
    mExercise->new_take();
    mExercise->start();
    if( !mEchoConnector ) mInput->start();
    if( mMetronome ) {
      qDebug() << "Start metronome" << mMetronome->options().beat() << "/" << (1<<mMetronome->options().measure()) << "tempo:" << (60000000/mMetronome->tempo()) << "bpm" << mMetronome->tempo() << "uspq";
      mMetronome->start();
    }
    mTimer.start( (mExercise->tempo() * mMetronome->options().beat()) / 1000 );
  } // start_exercise()
  bool MainWindow::complete_exercise() {
    if( !mExercise ) return false;
    mTimer.stop();
    mMetronome->stop();
    if( !mEchoConnector ) mInput->stop();
    mInput->remove_client( *mExercise );
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
    else if( mStats ) mStats->add_stat( Stats );
    qDebug() << "Statistics: start:" << Stats.StartMin << "-" << Stats.StartMax << "stop:" << Stats.StopMin << "-" << Stats.StopMax << "velocity:" << Stats.VelocityMin << "-" << Stats.VelocityMax;
    mUI->ActionExercise->setChecked( false );
    update_piano_roll();
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
