// -*- coding: utf-8; -*-
#ifndef MUTRA_MIDI_CORE_HPP
#define MUTRA_MIDI_CORE_HPP
#include <string>
#include <iostream>
#include <vector>

#ifdef minor
#undef minor
#endif

/** \file
 * Базовые объекты MIDI-подсистемы. На данный момент это секвенсер (Sequencer), который обеспечивает интерфейс воспроизведения или обработки событий, различные MIDI-сообщения (Event) 
 */
namespace MuTraMIDI {
  //! \note Поскольку MIDI-файлы разделены на "чанки" известной длины, а в некоторых сообщениях явно указана длина, нам может быть необходимо знать, сколько байт было получено/записано для значений
  //! переменной длины.
  // static int get_int( const unsigned char* Pos, int Length );
  int get_int( std::istream& Str, int Size );
  int put_int( std::ostream& Str, int Size, int Number );
  // int get_var( const unsigned char*& Pos, int& ToGo );
  int get_var( std::istream& Str, int& Length );
  int put_var( std::ostream& Str, int Number );

  //! Интерфейс для управления воспроизведением MIDI-последовательностей. Может представлять собой синтезатор, MIDI-выход или программный обработчик.
  //! \todo Обдумать разделение собственно устройства вывода и управление воспроизведением событий в заданное время.
  class Sequencer
  {
  protected: // Эти значения связаны с воспроизведением файла. В общем случае (для простых обработчиков) они не нужны.
    double Start;		//!< Реальное время начала в микросекундах (все прочие - с начала пьесы)
    unsigned Time;	//!< Текущее время в микросекундах
    unsigned Clock;	//!< Текущее время в MIDI-клоках
    unsigned TempoTime;	//!< Время смены темпа в микросекундах
    unsigned TempoClock;	//!< Время смены темпа в MIDI-клоках
    double Division;	//!< Количество единиц времени MIDI на четвертную ноту
    unsigned Tempo;	//!< Длительность четвертной ноты в микросекундах
    int Track;		//!< Текущая дорожка
  public:
    static Sequencer* get_instance( const std::string& URI = std::string() );
    Sequencer() : Start( 0 ), Time( 0 ), Clock( 0 ), Division( 120 ), Tempo( 500000 ), TempoTime( 0 ), TempoClock( 0 ), Track( 0 ) {}
    virtual ~Sequencer() {}

    virtual void track( int NewTrack ) { Track = NewTrack; }
    // MIDI-операции
    virtual void name( std::string Text ) {}
    virtual void copyright( std::string Text ) {}
    virtual void text( std::string Text ) {}
    virtual void lyric( std::string Text ) {}
    virtual void instrument( std::string Text ) {}
    virtual void marker( std::string Text ) {}
    virtual void cue( std::string Text ) {}

    virtual void meter( int Numerator, int Denominator ) {}

    virtual void note_on( int Channel, int Note, int Velocity ) {}
    virtual void note_off( int Channel, int Note, int Velocity ) {}
    virtual void after_touch( int Channel, int Note, int Velocity ) {}
    virtual void program_change( int Channel, int NewProgram ) {}
    virtual void control_change( int Channel, int Control, int Value ) {}
    virtual void pitch_bend( int Channel, int Bend ) {}

    virtual void reset() { 
      Time = 0;
      Clock = 0;
      Start = 0;
      TempoTime = 0;
      TempoClock = 0;
    } // reset()
    virtual void start() {}
    virtual void wait_for( unsigned WaitClock ) {
      Clock = WaitClock;
      Time = static_cast<int>( TempoTime + ( Clock-TempoClock ) / Division * Tempo );
      wait_for_usec( Start + Time );
    } // wait_for( unsigned )
    virtual void division( unsigned MIDIClockForQuarter ) { Division = MIDIClockForQuarter; }
    unsigned tempo() const { return Tempo; }
    virtual void tempo( unsigned uSecForQuarter ) {
      TempoTime += static_cast<int>( ( Clock-TempoClock ) / Division * Tempo ); // Сколько времени в микросекундах прошло с тех пор, как мы меняли темп
      TempoClock = Clock;
      Tempo = uSecForQuarter;
    } // tempo( unsigned )
  protected:
    //! \todo Может быть, это надо вывести в отдельное устройство.
    virtual void wait_for_usec( double WaitMicroSecs ) {}
  }; // Sequencer

  //! Событие (MIDI или системное). Интерфейс, обеспечивающий чтение/запись в двоичном формате по стандарту MIDI, вывод в текстовый файл (печать) и воспроизведение.
  class Event
  {
  public:
    typedef int64_t TimeMS; //!< Time in milliseconds from the start of the file or session. If < 0 - right now (to play).
    enum StatusCode { NoteOff = 0x80, NoteOn = 0x90, AfterTouch = 0xA0, ControlChange = 0xB0, ProgramChange = 0xC0, ChannelAfterTouch = 0xD0, PitchBend = 0xE0, SysEx = 0xF0, SysExEscape = 0xF7,
		      MetaCode = 0xFF, EventMask = 0xF0, ChannelMask = 0x0F };
    // Функции для чтения и записи событий из/в файла.
    // static Event* parse( const unsigned char*& Pos, int& ToGo );
    static Event* get( std::istream& Str, int& ToGo, unsigned char& Status );
    Event( TimeMS Time = -1 ) : mTimeMS( Time ) {}
    virtual ~Event() {}
    virtual void print( std::ostream& Stream );
    virtual void play( Sequencer& S ) {}
    virtual int write( std::ostream& File ) const { return File.good(); }
    TimeMS time() const { return mTimeMS; }
  private:
    TimeMS mTimeMS; //! Время события в миллисекундах. Начало отсчёта не определено.
  }; // Event

  //! Устройство ввода. Вообще говоря, нам надо бы иметь возможность полуить список всех устройств и их портов (в теории одно устройство может иметь несколько портов) и выбрать нужное с учётом
  // особенностей системы по расшариванию их между приложениями.
  class InputDevice
  {
  public:
    static InputDevice* get_instance( const std::string& URI = std::string() );
    class Client
    {
    public:
      virtual ~Client() {}
      virtual void event_received( Event& Ev );
      virtual void connected( InputDevice& Dev );
      virtual void disconnected( InputDevice& Dev );
      virtual void started( InputDevice& Dev );
      virtual void stopped( InputDevice& Dev );
    }; // CLient
    virtual ~InputDevice();
    void add_client( Client& Cli );
    void remove_client( Client& Cli );
    const std::vector<Client*> clients() const { return Clients; }
    virtual void start() {}
    virtual void stop() {}
  protected:
    void event_received( Event& Ev );
    //! Parse the incoming buffer and fire events
    //! \return Number of bytes rest at the end because they don't form a MIDI event.
#if 0
    int parse( const unsigned char* Buffer, size_t Count ); //!< \todo Don't use it here.
#endif
  private:
    std::vector<Client*> Clients;
  }; // InputDevice

  //! Исключение подсистемы MIDI
  class MIDIException
  {
    std::string Description;
  public:
    MIDIException( std::string Description0 ) : Description( Description0 ) {}
    std::string description() const { return Description; }
  }; // MIDIException
} // MuTraMIDI
#endif // MUTRA_MIDI_CORE_HPP
