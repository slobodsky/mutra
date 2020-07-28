#ifndef LESSON_HPP
#define LESSON_HPP
#include "exercise_sequence.hpp"
namespace MuTraTrain {
  class Lesson
  {
  public:
    class Exercise
    {
      std::string FileName; // Name of the file with the exercise. As set in the lesson (usualy relative to the lesson).
      std::string AbsoluteFileName; // Absolute file name. Q&D fix for relative files. Very ugly, but we must have it here, to set right path for statistics & played .mid files on save.
      int Retries;
      int Strike;
      std::vector<ExerciseSequence::NotesStat> Stats;
    public:
      Exercise( const std::string& FileName0, int Retries0 = 3, int Strike0 = 0 )
	: FileName( FileName0 ), AbsoluteFileName( FileName0 ), Retries( Retries0 ), Strike( Strike0 ) {}
      const std::string& file_name() const { return FileName; }
      const std::string& absolute_file_name() const { return AbsoluteFileName; }
      int retries() const { return Retries; }
      int strike() const { return Strike; }
      const std::vector<ExerciseSequence::NotesStat>& statistics() const { return Stats; }
      const ExerciseSequence::NotesStat& stats( int Index ) const { return Stats[ Index ]; }
      void new_stat( const ExerciseSequence::NotesStat& NewStat );
      void absolute_file_name( const std::string& NewName ) { AbsoluteFileName = NewName; }
      void save();
    }; // Exercise
  private:
    std::string FileName;
    std::vector<Exercise> Exercises;
    size_t Current;
  public:
    Lesson( const std::string& FileName0 );

    const std::string& lesson_name() const { return FileName; }
    const std::string& file_name() const { return Exercises[ Current ].file_name(); }
    const std::string& absolute_file_name() const { return Exercises[ Current ].absolute_file_name(); }
    const std::vector<Exercise>& exercises() const { return Exercises; }
    const Exercise& exercise( int Index ) const { return Exercises[ Index ]; }
    int retries() const { return Exercises[ Current ].retries(); }
    int strike() const { return Exercises[ Current ].strike(); }
    void new_stat( const ExerciseSequence::NotesStat& NewStat ) { Exercises[ Current ].new_stat( NewStat ); }

    int current() const { return Current; }
    bool next();
    bool save( const std::string& ToFile );
    bool save();
  }; // Lesson
} // MuTraTrain
#endif // LESSON_HPP
