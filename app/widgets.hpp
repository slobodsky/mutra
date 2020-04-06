#ifndef MUTRA_WIDGETS_HPP
#define MUTRA_WIDGETS_HPP
#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QThread>
#include <QAbstractListModel>
#include <QDBusPendingCallWatcher>
#include <midi/midi_utility.hpp>
#include <training/metronome.hpp>
#include <training/exercise_sequence.hpp>
#include <training/Lesson.hpp>

namespace Ui {
  class MainWindow;
  class DevicesSettings;
  class MetronomeSettings;
  class ExerciseSettings;
  class NotesExerciseSettings;
  class SettingsDialog;
  class MIDIMixer;
} // Ui

namespace MuTraWidgets {
  class SystemOptions {
  public:
    SystemOptions();
    const std::string& midi_input() const { return mMIDIInput; }
    SystemOptions& midi_input( const std::string& NewInput ) { mMIDIInput = NewInput; return *this; }
    const std::string& midi_output() const { return mMIDIOutput; }
    SystemOptions& midi_output( const std::string& NewOutput ) { mMIDIOutput = NewOutput; return *this; }
    bool midi_echo() const { return mMIDIEcho; }
    SystemOptions& midi_echo( bool NewEcho ) { mMIDIEcho = NewEcho; return *this; }
  private:
    std::string mMIDIInput;
    std::string mMIDIOutput;
    bool mMIDIEcho;
  }; // SystemOptions
  struct ExerciseOptions {
    ExerciseOptions();
    ExerciseOptions( const MuTraTrain::ExerciseSequence& Ex );
    int note_on_low() const { return StartThreshold; }
    ExerciseOptions& note_on_low( int NewLimit ) { StartThreshold = NewLimit; return *this; }
    int note_off_low() const { return StopThreshold; }
    ExerciseOptions& note_off_low( int NewLimit ) { StopThreshold = NewLimit; return *this; }
    int velocity_low() const { return VelocityThreshold; }
    ExerciseOptions& velocity_low( int NewLimit ) { VelocityThreshold = NewLimit; return *this; }
    int fragment_start() const { return Start; }
    ExerciseOptions& fragment_start( int NewStart ) { Start = NewStart; return *this; }
    int fragment_duration() const { return Duration; }
    ExerciseOptions& fragment_duration( int NewDuration ) { Duration = NewDuration; return *this; }
    double tempo_skew() const { return TempoSkew; }
    ExerciseOptions& tempo_skew( double NewSkew ) { TempoSkew = NewSkew; return *this; }
    int tracks_count() const { return TracksNum; }
    ExerciseOptions& tracks_count( int NewCount ) { TracksNum = NewCount; return *this; }
    unsigned tracks() const { return Tracks; }
    ExerciseOptions& tracks( unsigned NewValue ) { Tracks = NewValue; return *this; }
    bool track( int Index ) const { return Index < 0 || Index >= TracksNum ? 0 : ( Tracks & ( 1 << Index ) ); }
    ExerciseOptions& track( int Index, bool NewValue );
    unsigned channels() const { return Channels; }
    ExerciseOptions& channels( unsigned NewValue ) { Channels = NewValue; return *this; }
    bool channel( int Index ) const { return Index < 0 || Index >= ChannelsCount ? 0 : ( Channels & ( 1 << Channels ) ); }
    ExerciseOptions& channel( int Index, bool NewValue );
    bool set_to( MuTraTrain::ExerciseSequence& Exercise ) const;
  private:
    int StartThreshold;
    int StopThreshold;
    int VelocityThreshold;
    int Start;
    int Duration;
    double TempoSkew;
    int TracksNum;
    unsigned Tracks;
    enum { ChannelsCount = 16 };
    unsigned Channels;
  }; // ExerciseOptions
  class Application : public QApplication {
    Q_OBJECT
  public:
    static Application* get() { return qobject_cast<Application*>( qApp ); }
    Application( int& argc, char** argv );
    ~Application();
    const MuTraTrain::MetronomeOptions& metronome_options() const { return mMetronomeOptions; }
    void metronome_options( const MuTraTrain::MetronomeOptions& NewOptions ) { mMetronomeOptions = NewOptions; }
    const SystemOptions& system_options() const { return mSystemOptions; }
    void system_options( const SystemOptions& NewOptions ) { mSystemOptions = NewOptions; }
    const ExerciseOptions& exercise_options() const { return mExerciseOptions; }
    void exercise_options( const ExerciseOptions& NewOptions ) { mExerciseOptions = NewOptions; }
  private:
    MuTraTrain::MetronomeOptions mMetronomeOptions;
    SystemOptions mSystemOptions;
    ExerciseOptions mExerciseOptions;
  }; // Application

  class TracksChannelsModel : public QAbstractListModel {
    Q_OBJECT
  public:
    TracksChannelsModel( QObject* Parent = nullptr ) : mTracksCount( 0 ), mTracks( 0xFFFF ), mChannels( 0xFFFF ) {}
    int tracks_count() const { return mTracksCount; }
    TracksChannelsModel& tracks_count( int NewCount );
    bool track( int Index ) const { return Index >= 0 && Index < mTracksCount && ( mTracks & ( 1 << Index ) ) != 0; }
    TracksChannelsModel& track( int Index, bool Value );
    uint16_t tracks() const { return mTracks; }
    TracksChannelsModel& tracks( uint16_t NewTracks );
    bool channel( int Index ) const { return Index >= 0 && Index < 16 && ( mChannels & ( 1 << Index ) ) != 0; }
    TracksChannelsModel& channel( int Index, bool Value );
    uint16_t channels() const { return mChannels; }
    TracksChannelsModel& channels( uint16_t NewChannels );
    // Model overrides
    int rowCount( const QModelIndex& Parent = QModelIndex() ) const;
    Qt::ItemFlags flags( const QModelIndex& Index ) const;
    QVariant data( const QModelIndex& Index, int Role = Qt::DisplayRole ) const;
    bool setData( const QModelIndex& Index, const QVariant& Value, int Role = Qt::EditRole );
  private:
    int mTracksCount;
    uint16_t mTracks;
    uint16_t mChannels;
  }; // TracksChannelsModel
  class SettingsDialog : public QDialog {
    Q_OBJECT
  public:
    SettingsDialog( QWidget* Parent = nullptr );
    ~SettingsDialog();
    const MuTraTrain::MetronomeOptions& metronome() const { return mMetronome; }
    void metronome( const MuTraTrain::MetronomeOptions& NewOptions );
    const SystemOptions& system() const { return mSystem; }
    void system( const SystemOptions& NewOptions );
    const ExerciseOptions& exercise() const { return mExercise; }
    void exercise( const ExerciseOptions& Ex );
    int opriginal_tempo() const { return mOriginalTempo; }
    void original_tempo( int NewTempo );
    void notes_options( int NoteLow, int NoteHigh );
    int note_low() const { return mNoteLow; }
    int note_high() const { return mNoteHigh; }
  public slots:
    void tempo_skew_changed( int Value );
    void accept();
  private:
    SystemOptions mSystem;
    MuTraTrain::MetronomeOptions mMetronome;
    ExerciseOptions mExercise;
    int mOriginalTempo;
    int mNoteLow;
    int mNoteHigh;
    Ui::SettingsDialog* mDlg;
    Ui::DevicesSettings* mDevicesPage;
    Ui::MetronomeSettings* mMetronomePage;
    Ui::ExerciseSettings* mExercisePage;
    Ui::NotesExerciseSettings* mNotesPage;
  }; // SettingsDialog
  //! \todo Consider make it just a widget.
  class MIDIMixer : public QDialog {
    Q_OBJECT
    struct ChannelObjects {
      ChannelObjects() : mVolSlider( nullptr ), mVolBox( nullptr ), mPanSlider( nullptr ), mPanBox( nullptr ) {}
      QSlider* mVolSlider;
      QSpinBox* mVolBox;
      QSlider* mPanSlider;
      QSpinBox* mPanBox;
    }; // ChannelObjects
    enum { ChannelsNum = 16 };
  public:
    MIDIMixer( QWidget* Parent = nullptr );
    void sequencer( MuTraMIDI::Sequencer* NewSequencer ) { mSequencer = NewSequencer; }
  public slots:
    void volume_changed( int Channel, int Value );
    void pan_changed( int Channel, int Value );
  private:
    ChannelObjects mChannels[ ChannelsNum ];
    Ui::MIDIMixer* mUI;
    MuTraMIDI::Sequencer* mSequencer;
  }; // MIDIMixer

  class StatisticsModel;
  //! \todo Move this to MuTraMIDI with std::threads instad of QThread.
  class Player : public QThread {
    Q_OBJECT
  public:
    Player( MuTraMIDI::MIDISequence* Play, MuTraMIDI::Sequencer* Seq, QObject* Parent = nullptr );
    Player( const std::vector<std::string>& PlayList, MuTraMIDI::Sequencer* Seq, QObject* Parent = nullptr );
  signals:
    void playback_complete();
  private:
    void run() override;
    MuTraMIDI::Sequencer* mSequencer;
    MuTraMIDI::MIDISequence* mPlay;
    std::vector<std::string> mPlayList;
  }; // Player
  class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    MainWindow( QWidget* Parent = nullptr );
    ~MainWindow();
    void stop_recording();
    void update_buttons();
  public slots:
    void update_piano_roll();
    void open_file();
    bool save_file();
    bool save_file_as();
    void edit_options();
    void midi_mixer();
    void toggle_metronome( bool On );
    void toggle_playback( bool On );
    void toggle_record( bool On );
    void toggle_exercise( bool On );
    void toggle_notes_training( bool On );
    bool close_file();
    void timer();
  private slots:
    void playback_complete();
  private:
    bool load_lesson( const std::string& FileName );
    bool load_exercise( const std::string& FileName );
    void start_exercise();
    bool complete_exercise();
    enum { StateIdle, StatePlayback, StateRecording, StateMetronome, StateExercise } State;
    MuTraTrain::Lesson* mLesson;
    StatisticsModel* mStats;
    int mStrike;
    //! \todo Move the exercise mechanism to a separate object outside of the GUI app, sync with metronome & provide callbacks on exercise start & finish (with result).
    std::string mExerciseName;
    MuTraTrain::ExerciseSequence* mExercise;
    int mRetries;
    int mToGo;
    QTimer mTimer;
    std::string mMIDIFileName;
    MuTraMIDI::MIDISequence* mMIDI;
    MuTraMIDI::Recorder* mRec;
    MuTraMIDI::InputDevice* mInput;
    MuTraMIDI::Sequencer* mSequencer;
    MuTraMIDI::InputConnector* mEchoConnector;
    MuTraTrain::Metronome* mMetronome;
    MuTraTrain::NoteTrainer* mNoteTrainer;
    int mNoteHigh;
    int mNoteLow;
    Player* mPlayer;
    Ui::MainWindow* mUI;
    QDBusPendingCallWatcher* mDBusReply;
    int mScreenSaverCookie;
  }; // MainWindow
} // MuTraWidgets
#endif // MUTRA_WIDGETS_HPP
