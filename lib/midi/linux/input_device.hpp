#include "../midi_core.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <fstream>
#include <alsa/asoundlib.h>

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

#ifdef USE_ALSA_INPUT
  //! \todo Use separate thread & midi_input blocking function.
  class ALSAInputDevice : public InputDevice {
  public:
    ALSAInputDevice( int InClient = 24, int InPort = 0 );
    ~ALSAInputDevice();
    void start();
    void stop();
    bool active() const { return mTime != 0; }
    Event::TimeuS time() const { return mTime; }
  private:
    snd_seq_event_t* get_event();
    void put_event( snd_seq_event_t* Event );
    snd_seq_t* mSeq;
    int mClient;
    int mPort;
    int mInClient;
    int mInPort;
    Event::TimeuS mTime; //! \todo Remove this. We don't need realtime here. It's copypaste from the RTMIDI. Boolean flag will be enough.
    std::thread mInputThread;
    std::thread mQueueThread;
    std::queue<snd_seq_event_t*> mQueue;
    std::mutex mQueueMutex;
    std::condition_variable mQueueCondition;
  }; // RtMIDIInputDevice
#endif // USE_ALSA_INPUT
} // MuTraMIDI
