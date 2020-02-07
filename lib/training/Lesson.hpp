#ifndef LESSON_HPP
#define LESSON_HPP
#include "exercise_sequence.hpp"
namespace MuTraTrain {
  class Lesson
  {
  public:
    class Excercise
    {
      std::string FileName;
      int Retries;
      int Strike;
      std::vector<ExerciseSequence::NotesStat> Stats;
    public:
      Excercise( const std::string& FileName0, int Retries0 = 3, int Strike0 = 0 )
	: FileName( FileName0 ), Retries( Retries0 ), Strike( Strike0 ) {}
      const std::string& file_name() const { return FileName; }
      int retries() const { return Retries; }
      int strike() const { return Strike; }
      void new_stat( ExerciseSequence::NotesStat& NewStat );
      void save();
    }; // Excercise
  private:
    std::string FileName;
    std::vector<Excercise> Excercises;
    std::vector<Excercise>::iterator Current;
  public:
    Lesson( const std::string& FileName0 );

    const std::string& lesson_name() const { return FileName; }
    const std::string& file_name() const { return Current->file_name(); }
    int retries() const { return Current->retries(); }
    int strike() const { return Current->strike(); }
    void new_stat( ExerciseSequence::NotesStat& NewStat ) { Current->new_stat( NewStat ); }

    bool next();
    bool save( const std::string& ToFile );
    bool save();
  }; // Lesson
} // MuTraTrain
#endif // LESSON_HPP
