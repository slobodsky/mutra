#ifndef MUTRA_MUSIC_HPP
#define MUTRA_MUSIC_HPP
//! \brief Внутреннее Представление музыки для отображения и воспроизведения
/** \file
 * Некая компиляция из MIDI & MusicXML, то есть ориентированное на ноты, по возможности удобное для представления, но вместе с тем сохраняющее все детали исходного материала, например, время (в единицах
 * division) начала и окончания события, которое не обязательно должно совпадать с границами нот.
 */
namespace MuTraMusic {
  class Part;
  class Track;
  class Voice;
  class Measure;
  class Chord;
  class Interval;
  class NotesGroup;
  typedef int Time; //!< Количество времени во внутренних единицах.
  //! Музыкальный элемент, из тех, которые располагаются на нотоносце - нота, пауза или аккорд. (Не забываем, что ритм с аккордами могут быть указаны на нитке.)
  class Element {
  public:
    enum Duration { None = -1, Whole = 0, Half = 1, Quarter = 2, Eighth = 3, Sixteenth = 4, ThirtySecond = 5, SixtyFourth = 6, HundredTwentyEighth = 7, TwoHundredFiftySixth = 8 };
    //! \note считается, что все события происходят в момент начала "тика".
    Element( Time Start = 0, Time Stop = -1 ) : mStart( Start ), mStop( Stop ), mDuration( None ), mDots( 0 ), mVoice( nullptr ), mTrack( nullptr ), mMeasure( nullptr ) {}
    virtual ~Element();
    Time start() const { return mStart; }
    Time stop() const { return mStop; }
    Time length() const { return mStop < 0 ? 0 : mStop - mStart; }
    Duration duration() const { return mDuration; }
    unsigned dots() const { return mDots; }
    Voice* voice() const { return mVoice; }
    Track* track() const { return mTrack; }
    Measure* measure() const { return mMeasure; }
  private:
    Time mStart;
    Time mStop;
    Duration mDuration;
    unsigned mDots;
    Voice* mVoice;
    Track* mTrack;
    Measure* mMeasure;
  }; // Element
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
