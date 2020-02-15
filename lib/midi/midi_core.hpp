// -*- coding: utf-8; -*-
#ifndef MUTRA_MIDI_CORE_HPP
#define MUTRA_MIDI_CORE_HPP
#include <string>
#include <iostream>
#include <vector>

#ifdef minor
#undef minor
#endif
//#define MUTRA_DEBUG

/** \file
 * Базовые объекты MIDI-подсистемы. На данный момент это секвенсер (Sequencer), который обеспечивает интерфейс воспроизведения или обработки событий, различные MIDI-сообщения (Event) 
 * Однако, надо заметить, что тут всё надо бы пересмотреть. Например, сообщение с входного устройства, которое могло бы проходить напрямую в выходное, декодируется и создаётся соответствующий объект,
 * который вызывает метод секвенсора. В предложенной реализация обработчиков в виде секвенсоров приходится переопределять все функции, когда можно было бы просто передавать сообщение дальше, как есть,
 * добавляя только необходимые методы.
 */
namespace MuTraMIDI {
  class Event;
  uint64_t get_time_us();

  //! \note Поскольку MIDI-файлы разделены на "чанки" известной длины, а в некоторых сообщениях явно указана длина, нам может быть необходимо знать, сколько байт было получено/записано для значений
  //! переменной длины.
  // static int get_int( const unsigned char* Pos, int Length );
  int get_int( std::istream& Str, int Size );
  int put_int( std::ostream& Str, int Size, int Number );
  // int get_var( const unsigned char*& Pos, int& ToGo );
  int get_var( std::istream& Str, int& Length );
  int put_var( std::ostream& Str, unsigned Number );

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
    class Info {
    public:
      Info( const std::string& Name = std::string(), const std::string& URI = std::string() ) : mName( Name ), mURI( URI ) {}
      const std::string& name() const { return mName; }
      const std::string& uri() const { return mURI; }
    private:
      std::string mName;
      std::string mURI;
    }; // Info
    static std::vector<Info> get_available_devices( const std::string& Backend = std::string() );
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
      Time = static_cast<int>( TempoTime + ( Clock-TempoClock ) * Tempo / Division );
      wait_for_usec( Start + Time );
    } // wait_for( unsigned )
    virtual void division( unsigned MIDIClockForQuarter ) { Division = MIDIClockForQuarter; }
    unsigned tempo() const { return Tempo; }
    virtual void tempo( unsigned uSecForQuarter ) {
      TempoTime += static_cast<int>( ( Clock-TempoClock ) * Tempo / Division ); // Сколько времени в микросекундах прошло с тех пор, как мы меняли темп
      TempoClock = Clock;
      Tempo = uSecForQuarter;
    } // tempo( unsigned )
  protected:
    //! \todo Может быть, это надо вывести в отдельное устройство.
    virtual void wait_for_usec( double WaitMicroSecs ) {}
  }; // Sequencer

  //! \brief Абстрактный входной поток. Может быть потоком C++ или буфером в форме указателя (и размера) или контейнера.
  //! All get... methods throws an exception if there is not enough data or I/O error.
  class InStream {
  public:
    InStream() : Status( 0 ), LastCount( 0 ) {}
    virtual ~InStream() {}
    //! Get a single byte.
    //! \note Don't change the LastCOunt.
    virtual uint8_t get() = 0;
    //! Посмотреть первый байт в потоке, не убирая его оттуда (т.е. следующая операция peek/get вернёт тот же байт).
    virtual uint8_t peek() = 0;
    virtual bool eof() const { return true; }
    virtual void read( unsigned char* Buffer, size_t Length );
    virtual void skip( size_t Length );
    int get_int( int Length = 4 );
    //! Get variable width integer.
    int get_var();
    //! Get status byte from the stream. Throws an exception if the first byte is not status & we don't have the running status. Sets last count to 0 for running status.
    uint8_t get_status();
    // Return the saved status byte without trying to get it from the stream.
    uint8_t status() const { return Status; }
    Event* get_event(); // Здесь, вероятно, граф вызовов будет несколько странным
    size_t count() const { return LastCount; }
    //! \note This is intended for use in get-methods in events types etc.
    void count( size_t& NewCount ) { LastCount = NewCount; }
  protected:
    uint8_t Status; //! Running status.
    size_t LastCount; //! Bytes count of the last get (int/var/event) operation.
  }; // InStream

  class FileInStream : public InStream {
  public:
    FileInStream( std::istream& Str ) : Stream( Str ) {}
    uint8_t get();
    uint8_t peek();
    bool eof() const;
  private:
    std::istream& Stream;
  }; // FileInStream
  
  //! Событие (MIDI или системное). Интерфейс, обеспечивающий чтение/запись в двоичном формате по стандарту MIDI, вывод в текстовый файл (печать) и воспроизведение.
  class Event
  {
  public:
    //! \todo Change to microseconds.
    typedef int64_t TimeuS; //!< Time in microseconds from the start of the file or session. If < 0 - right now (to play).
    enum StatusCode { Unknown = 0, NoteOff = 0x80, NoteOn = 0x90, AfterTouch = 0xA0, ControlChange = 0xB0, ProgramChange = 0xC0, ChannelAfterTouch = 0xD0, PitchBend = 0xE0,
		      SysEx = 0xF0, SysExEscape = 0xF7, MetaCode = 0xFF, EventMask = 0xF0, ChannelMask = 0x0F };
    // Функции для чтения и записи событий из/в файла.
    static Event* get( InStream& Str, size_t& Count );
    Event( TimeuS Time = -1 ) : mTimeuS( Time ) {}
    virtual ~Event() {}
    virtual void print( std::ostream& Stream ) const;
    virtual void play( Sequencer& S ) const {}
    virtual int write( std::ostream& File ) const { return File.good(); }
    virtual StatusCode status() const { return Unknown; }
    virtual Event* clone() const { return new Event( *this ); }
    TimeuS time() const { return mTimeuS; }
    void time( TimeuS NewTime ) { mTimeuS = NewTime; }
  protected:
    Event( const Event& Other ) : mTimeuS( Other.mTimeuS ) {}
  private:
    TimeuS mTimeuS; //! Время события в микросекундах. Начало отсчёта зависит от конекста.
    //! \todo StatusCode Status; //! Message status (type). For channel messages does not contain channel.
  }; // Event

  //! Устройство ввода. Вообще говоря, нам надо бы иметь возможность получить список всех устройств и их портов (в теории одно устройство может иметь несколько портов) и выбрать нужное с учётом
  // особенностей системы по расшариванию их между приложениями.
  // Реализация устройства должна устанавливать в сообщениях время в микросекундах, например, от вызова функции start() или реальное - не важно.
  class InputDevice
  {
  public:
    typedef Sequencer::Info Info;
    static std::vector<Info> get_available_devices( const std::string& Backend = std::string() );
    static InputDevice* get_instance( const std::string& URI = std::string() );
    class Client
    {
    public:
      virtual ~Client() {}
      virtual void event_received( const Event& Ev ); //!< \note This event can be destroyed immediately after this function return. If you need it, then clone it.
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
    void event_received( const Event& Ev );
  private:
    std::vector<Client*> Clients;
  }; // InputDevice

  //! Исключение подсистемы MIDI
  class MIDIException
  {
    std::string Description;
  public:
    MIDIException( std::string Description0 ) : Description( Description0 ) {}
    std::string what() const { return Description; }
  }; // MIDIException
} // MuTraMIDI
#endif // MUTRA_MIDI_CORE_HPP
