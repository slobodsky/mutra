#include "widgets.hpp"

int main( int argc, char** argv ) {
  MuTraWidgets::Application App( argc, argv );
  MuTraWidgets::MainWindow* Main = new MuTraWidgets::MainWindow();
  Main->show();
  return App.exec();
} // main()
