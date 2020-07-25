#ifndef MUTRA_ALSA_BACKEND_HPP
#define MUTRA_ALSA_BACKEND_HPP
#include "linux_backend.hpp"
#include <alsa/asoundlib.h>

namespace MuTraMIDI {
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
} // MuTraMIDI
#endif // MUTRA_ALSA_BACKEND_HPP
