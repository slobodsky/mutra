#include "../midi_core.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <fstream>
#include <alsa/asoundlib.h>

namespace MuTraMIDI {
  class FileBackend : public MIDIBackend {
  public:
    FileBackend() : MIDIBackend( "file://", "Device file" ) {}
    Sequencer* get_sequencer( const std::string& URI = std::string() );
    InputDevice* get_input( const std::string& URI = std::string() );
  }; // FileBackend

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

#ifdef USE_ALSA_BACKEND
  class ALSABackend : public MIDIBackend {
  public:
    ALSABackend();
    ~ALSABackend();
    std::vector<Sequencer::Info> list_devices( DeviceType Filter = All );
    Sequencer* get_sequencer( const std::string& URI = std::string() );
    InputDevice* get_input( const std::string& URI = std::string() );
  private:
    snd_seq_t* mSeq;
  }; // ALSABackend

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
    snd_seq_t* mSeq; //! \todo Reuse the backend's sequencer.
    int mClient;
    int mPort;
    int mInClient;
    int mInPort;
    Event::TimeuS mTime; // Реальное время старта устройства. Все сообщения считаются от него. Также это признак остановки, поэтому для сообщений, обработанных после остановки, оно будет неверным.
    std::thread mInputThread;
    std::thread mQueueThread;
    std::queue<snd_seq_event_t*> mQueue;
    std::mutex mQueueMutex;
    std::condition_variable mQueueCondition;
  }; // ALSAInputDevice
#endif // USE_ALSA_BACKEND
} // MuTraMIDI
