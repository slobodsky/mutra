#ifndef MUTRA_MIDI_FILES_HPP
#define MUTRA_MIDI_FILES_HPP
#include "midi_events.hpp"

/** \file
 * Объекты для работы с MIDI-файлами (чтение/запись).
 */
namespace MuTraMIDI {
  //! Список событий, происходящих в определённое время.
  class EventsList
  {
    unsigned Time; // Время (в единицах MIDI)
    std::vector<Event*> Events;
  public:
    EventsList( unsigned Time0 ) : Time( Time0 ) {}
    ~EventsList()
    {
      while( !Events.empty() )
      {
	delete Events.back();
	Events.pop_back();
      }
    } // Деструктор (не полиморфный)
    void add( Event* NewEvent ) { Events.push_back( NewEvent ); }
    unsigned time() const { return Time; }
    std::vector<Event*>& events() { return Events; }

    void print( std::ostream& Stream );
    void play( Sequencer& S );
    int write( std::ostream& File, unsigned Clock ) const;
  }; // EventsList

  //! Дорожка в MIDI-файле
  class MIDITrack
  {
    std::vector<EventsList*> Events;
  public:
    ~MIDITrack()
    {
      while( !Events.empty() )
      {
	delete Events.back();
	Events.pop_back();
      }
    }
    std::vector<EventsList*>& events() { return Events; }
    void close();
    void print( std::ostream& Stream );
    void play( Sequencer& Seq );
    bool write( std::ostream& File ) const;
  }; // MIDITrack

  //! MIDI-последовательность (состоящая из дорожек)
  class MIDISequence
  {
  protected:
    std::vector<MIDITrack*> Tracks;

    short Type;
    short TracksNum;
    short Division;
    uint8_t Low;
    uint8_t High;
    void add( Event* NewEvent ) { Tracks.back()->events().back()->add( NewEvent ); }
  public:
    MIDISequence( std::string FileName );
    MIDISequence() : Type( 0 ), TracksNum( 0 ), Division( 120 ), Low( 0 ), High( 127 ) {}
    virtual ~MIDISequence()
    {
      while( !Tracks.empty() )
      {
	delete Tracks.back();
	Tracks.pop_back();
      }
    }
    int division() const { return Division; }
    int tracks_num() const { return TracksNum; }
    const std::vector<MIDITrack*>& tracks() { return Tracks; }
    uint8_t low() const { return Low; }
    uint8_t high() const { return High; }
    void add_track();
    void add_event( int Clock, Event* NewEvent );
    void close_last_track() { if( Tracks.size() > 0 ) Tracks.back()->close(); }
    void print( std::ostream& Stream );
    void play( Sequencer& S );
    bool write( std::ostream& File ) const;
    bool close_and_write( const std::string& FileName );
  }; // MIDISequence
} // MuTraMIDI
#endif // MUTRA_MIDI_FILES_HPP
