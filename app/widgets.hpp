#ifndef MUTRA_WIDGETS_HPP
#define MUTRA_WIDGETS_HPP
#include <QApplication>
#include <midi/midi_files.hpp>
#include "forms/ui_main.h"

namespace MuTraWidgets {
  struct MetronomeOptions {
    MetronomeOptions();
    void load_from_settings();
    void save_to_settings();
    int Beat;
    int Note;
    int Velocity;
    int PowerNote;
    int PowerVelocity;
  }; // MetronomeOptions
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
  public:
    Application( int& argc, char** argv );
    ~Application();
  private:
    MetronomeOptions Metronome;
    ExerciseOptions Exercise;
  }; // Application

  class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    MainWindow( QWidget* Parent = nullptr );
    ~MainWindow();
  public slots:
    void open_file();
  private:
    MuTraMIDI::MIDISequence* MIDI;
    Ui::MainWindow UI;
  }; // MainWindow
} // MuTraWidgets
#endif // MUTRA_WIDGETS_HPP
