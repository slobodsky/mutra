#ifndef MUTRA_MUSIC_HPP
#define MUTRA_MUSIC_HPP
#include <vector>
#include <string>
//! \brief Внутреннее Представление музыки для отображения и воспроизведения
/** \file
 * Некая компиляция из MIDI & MusicXML, то есть ориентированное на ноты, по возможности удобное для представления, но вместе с тем сохраняющее все детали исходного материала, например, время (в единицах
 * division) начала и окончания события, которое не обязательно должно совпадать с границами нот.
 */
namespace MuTraMusic {
  class Scores; // Партитура
  class Fragment; // Фрагмент партитуры (по времени), от цифры до цифры или между репризами. В том числе, в случае повторов с отличиями будет три участка: общий, для повтора, для окончания
  class Part; // Партия: может состоять из одной или нескольких дорожек (ф-но для левой и правой руки)
  class Track; // Дорожка: может включать одну или несколько партий
  class Voice; //! \brief Голос: последовательность элементов в пределах одной дорожки и одной партии.
  class Measure;
  class Chord;
  class Interval;
  class NotesGroup;
  typedef int Time; //!< Количество времени во внутренних единицах.
  /** \brief Музыкальный элемент, из тех, которые располагаются на нотоносце - нота, пауза или аккорд. (Не забываем, что ритм с аккордами могут быть указаны на нитке.)
   * Элемент может принадлежать только одному голосу и находиться в одном такте
   */
  class Element {
  public:
    enum Duration { None = -1, Whole = 0, Half = 1, Quarter = 2, Eighth = 3, Sixteenth = 4, ThirtySecond = 5, SixtyFourth = 6, HundredTwentyEighth = 7, TwoHundredFiftySixth = 8 };
    typedef std::vector<Element*> List;
    //! \note считается, что все события происходят в момент начала "тика".
    Element( Voice* TheVoice = nullptr, Time Start = 0, Time Stop = -1 );
    virtual ~Element();
    Time start() const { return mStart; }
    Time stop() const { return mStop; }
    Time length() const { return mStop < 0 ? 0 : mStop - mStart; }
    Duration duration() const { return mDuration; }
    unsigned dots() const { return mDots; }
    Voice* voice() const { return mVoice; }
    Element& voice( Voice* NewVoice );
    Measure* measure() const { return mMeasure; }
  private:
    Time mStart;
    Time mStop;
    Duration mDuration;
    unsigned mDots;
    Voice* mVoice;
    Measure* mMeasure;
  }; // Element
  // Считается, что элементы принадлежат голосу. При удалении голса, они также будут удалены.
  class Voice {
  public:
    Voice( const std::string& Name = std::string(), Track* VoiceTrack = nullptr, Part* VoicePart = nullptr ) : mName( Name ), mTrack( VoiceTrack ), mPart( VoicePart ) {}
    ~Voice();
    Voice& add( Element& NewElement );
    Voice& remove( Element& NewElement );
  private:
    std::string mName;
    Part* mPart;
    Track* mTrack;
    Element::List mElements;
  }; // Voice
  //! Пауза
  class Rest : public Element {
  public:
    Rest( Voice* TheVoice = nullptr, Time Start = 0, Time Stop = -1 ) : Element( TheVoice, Start, Stop ) {}
  }; // Rest
  //! Высота ноты
  class Pitch {
  public:
    enum Class { None = -1, Do, Re, Mi, Fa, Sol, La, Si };
    enum Octave { SubContra, Contra, Great, Small, First, Second, Third, Fourth, Fifth };
    Pitch( Class PitchClass = Do, Octave PitchOctave = First, int Alteration = 0 ) : mClass( PitchClass ), mOctave( PitchOctave ), mAlteration( Alteration ) {}
    bool valid() { return mClass != None; }
    Class pitch_class() const { return mClass; }
    Octave octave() const { return mOctave; }
    //! Знаки альтерации - отрицательные для бемолей, положительные для диезов
    int alteration() const { return mAlteration; }
  private:
    Class mClass;
    Octave mOctave;
    int mAlteration;
  }; // Pitch
  class Note : public Element {
  public:
    class Mark; // Различные отметки при ноте, например характер исполнения или аппликатура
    // MIDI data
    int value() const { return mValue; }
    int velocity() const { return mVelocity; }
    int channel() const { return mChannel; }
    Pitch pitch() const { return mPitch; }
    NotesGroup* group() const { return mGroup; }
    Chord* chord() const { return mChord; }
    int mValue;
    int mVelocity;
    int mChannel;
    Pitch mPitch;
    NotesGroup* mGroup;
    Chord* mChord;
  }; // Note
} // MuTraMusic
#endif // MUTRA_MUSIC_HPP
