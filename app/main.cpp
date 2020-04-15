#include "widgets.hpp"

int main( int argc, char** argv ) {
  MuTraWidgets::Application App( argc, argv );
  MuTraWidgets::MainWindow* Main = new MuTraWidgets::MainWindow();
  Main->show();
  if( argc > 1 ) Main->load_file( argv[ 1 ] );
  return App.exec();
} // main()
