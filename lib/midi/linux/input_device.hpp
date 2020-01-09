#include "../midi_core.hpp"
#include <thread>
#include <fstream>

namespace MuTraMIDI {
  class FileInputDevice : public InputDevice
  {
  public:
    FileInputDevice( std::string FileName );
#if 0
    void process();
#endif
    void start();
  private:
    std::ifstream Str;
  }; // FileInputDevice
} // MuTraMIDI
