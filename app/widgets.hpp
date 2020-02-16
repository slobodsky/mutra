#ifndef MUTRA_WIDGETS_HPP
#define MUTRA_WIDGETS_HPP
#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <QTimer>
#include <midi/midi_utility.hpp>
#include <training/metronome.hpp>
#include <training/exercise_sequence.hpp>

namespace Ui {
  class MainWindow;
  class DevicesSettings;
  class MetronomeSettings;
  class SettingsDialog;
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
    void load_from_settings();
    void save_to_settings();
    int StartThreshold;
    int StopThreshold;
    int VelocityThreshold;
    int Duration;
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
  private:
    MuTraTrain::MetronomeOptions mMetronomeOptions;
    SystemOptions mSystemOptions;
    ExerciseOptions mExercise;
  }; // Application

  class SettingsDialog : public QDialog {
    Q_OBJECT
  public:
    SettingsDialog( QWidget* Parent = nullptr );
    ~SettingsDialog();
    const MuTraTrain::MetronomeOptions& metronome() const { return mMetronome; }
    void metronome( const MuTraTrain::MetronomeOptions& NewOptions );
    const SystemOptions& system() const { return mSystem; }
    void system( const SystemOptions& NewOptions );
  public slots:
    void accept();
  private:
    SystemOptions mSystem;
    MuTraTrain::MetronomeOptions mMetronome;
    Ui::SettingsDialog* mDlg;
    Ui::DevicesSettings* mDevicesPage;
    Ui::MetronomeSettings* mMetronomePage;
  }; // SettingsDialog

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
    void toggle_metronome( bool On );
    void toggle_record( bool On );
    void toggle_exercise( bool On );
    bool close_file();
    void timer();
  private:
    void start_exercise();
    bool complete_exercise();
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
    MuTraTrain::Metronome* mMetronome;
    Ui::MainWindow* mUI;
  }; // MainWindow
} // MuTraWidgets
#endif // MUTRA_WIDGETS_HPP
