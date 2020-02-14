#ifndef MUTRA_WIDGETS_HPP
#define MUTRA_WIDGETS_HPP
#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <midi/midi_utility.hpp>
#include <training/metronome.hpp>
#include <training/exercise_sequence.hpp>

namespace Ui {
  class MainWindow;
  class MetronomeSettings;
  class SettingsDialog;
} // Ui

namespace MuTraWidgets {
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
    const MuTraTrain::MetronomeOptions& metronome() const { return Metronome; }
    void metronome( const MuTraTrain::MetronomeOptions& NewOptions ) { Metronome = NewOptions; }
  private:
    MuTraTrain::MetronomeOptions Metronome;
    ExerciseOptions Exercise;
  }; // Application

  class SettingsDialog : public QDialog {
    Q_OBJECT
  public:
    SettingsDialog( QWidget* Parent = nullptr );
    ~SettingsDialog();
    const MuTraTrain::MetronomeOptions& metronome() const { return mMetronome; }
    void metronome( const MuTraTrain::MetronomeOptions& NewOptions );
  public slots:
    void accept();
  private:
    MuTraTrain::MetronomeOptions mMetronome;
    Ui::SettingsDialog* mDlg;
    Ui::MetronomeSettings* mMetronomePage;
  }; // SettingsDialog

  class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    MainWindow( QWidget* Parent = nullptr );
    ~MainWindow();
    void stop_recording();
  public slots:
    void open_file();
    bool save_file();
    bool save_file_as();
    void edit_options();
    void toggle_metronome( bool On );
    void toggle_record( bool On );
    bool close_file();
  private:
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
