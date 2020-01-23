#include "../midi_core.hpp"
#include <thread>
#include <fstream>

namespace MuTraMIDI {
  class FileInputDevice : public InputDevice
  {
  public:
    FileInputDevice( const std::string& FileName0 );
    void start();
  private:
    std::string FileName;
    std::ifstream Str;
    FileInStream Stream;
  }; // FileInputDevice
} // MuTraMIDI
