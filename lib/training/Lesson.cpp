#include "Lesson.hpp"
using std::string;
using std::ifstream;
using std::ofstream;
using std::ios;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
namespace MuTraTrain {
  void Lesson::Excercise::new_stat( ExerciseSequence::NotesStat& NewStat )
  {
    // Добавляем новую статистику к результатам и, если была ошибка, сбрасываем параметры
    Stats.push_back( NewStat );
    if( NewStat.Result != ExerciseSequence::NoError )
    {
      Retries = 3;
      Strike = 0;
    }
  } // new_stat( ExerciseSequence::NotesStat& )

  void Lesson::Excercise::save()
  {
    string Name = FileName.substr( 0, FileName.length()-3 )+"stat";
    ofstream File( Name.c_str(), ios::app | ios::out | ios::binary );
    int ToGo = Retries;
    for( vector<ExerciseSequence::NotesStat>::const_iterator It = Stats.begin(); It != Stats.end(); It++ )
    {
      ExerciseSequence::NotesStat St = *It;
      File.write( (char*)&St, sizeof( St ) );
      if( ToGo > 0 )
	if( St.Result == ExerciseSequence::NoError )
	  ToGo--;
	else
	  ToGo = -1;
    }
    if( ToGo < 0 )
    {
      Retries = 3;
      Strike = 0;
    }
    else if( ToGo == 0 )
    {
      if( Retries == 0 )
      {
	if( ++Strike >= 7 )
	  Retries = 1;
      }
      else if( Retries == 1 )
      {
	if( ++Strike >= 7 )
	{
	  Retries = 0;
	  Strike = 0;
	}
      }
      else
      {
	Retries = 1;
	Strike = 0;
      }
    }
  } // save()

  Lesson::Lesson( const string& FileName0 ) : FileName( FileName0 )
  {
    ifstream Les( FileName );
    if( Les.good() )
    {
      while( !Les.eof() )
      {
	string Name;
	Les >> Name;
	if( Name.size() > 0 )
	  if( Name[ 0 ] != '#' )
	  {
	    int Retries = 0;
	    int Strike = 0;
	    Les >> Retries >> Strike;
	    Excercises.push_back( Excercise( Name, Retries, Strike ) );
	  }
	  else
	    Les.ignore( 1024 * 1024 * 1024, '\n' );
      }
      Current = Excercises.begin();
    }
  } // Конструктор урока

  bool Lesson::next()
  {
    if( Current != Excercises.end() )
    {
      Current->save();
      Current++;
      string Skip;
      while( Current != Excercises.end() && Current->retries() == 0 )
      {
	Skip += Current->file_name() + '\n';
	Current->save();
	save( FileName + ".tmp" );
	Current++;
      }
      if( !Skip.empty() )
	cout << "Упражнения\n" + Skip + "пропущены." << endl;
    }
    return Current != Excercises.end();
  } // next()

  bool Lesson::save( const string& ToName )
  {
    ofstream File( ToName );
    for( vector<Excercise>::const_iterator It = Excercises.begin(); It != Excercises.end(); It++ )
      File << It->file_name() << " " << It->retries() << " " << It->strike() << endl;
    return File.good();
  } // save( const string& )

  bool Lesson::save()
  {
    bool Result = true;
    string BakName = FileName + ".bak";
    remove( BakName.c_str() );
    if( rename( FileName.c_str(), BakName.c_str() ) < 0 ) {
      //! \todo Inform user and ask weather to continue
      cerr << "Can't rename the file " << FileName << " to .bak" << endl;
    }
    if( Result )
      Result = save( FileName );
    return Result;
  } // save()
} // MuTraTrain