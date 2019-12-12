#include "../midi_core.hpp"
#include <thread>
#include <fstream>

class FileInputDevice : public MIDIInputDevice
{
public:
  FileInputDevice( std::string FileName );
  void process();
private:
  std::ifstream Str;
}; // FileInputDevice
