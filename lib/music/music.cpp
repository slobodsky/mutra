#include "music.hpp"
#include <algorithm>
using std::find;

namespace MuTraMusic {
  //! \todo Determine the elements duration from the start & stop times (if possible).
  Element::Element( Voice* TheVoice, Time Start, Time Stop ) : mStart( Start ), mStop( Stop ), mDuration( None ), mDots( 0 ), mVoice( nullptr ), mMeasure( nullptr ) {
    voice( TheVoice );
  } // Element( Voice*, Time, Time )
  Element::~Element() {
    voice( nullptr );
    // measure( nullptr );
  }
  Element& Element::voice( Voice* NewVoice ) {
    if( NewVoice == mVoice ) return *this;
    if( mVoice ) {
      Voice* OldVoice = mVoice;
      mVoice = nullptr;
      OldVoice->remove( *this );
    }
    mVoice = NewVoice;
    if( mVoice ) mVoice->add( *this );
    return *this;
  }
  Voice::~Voice() {
    // track( nullptr );
    // part( nullptr );
    while( !mElements.empty() ) delete mElements.back();
  }
  Voice& Voice::add( Element& NewElement ) {
    if( find( mElements.begin(), mElements.end(), &NewElement ) != mElements.end() ) return *this;
    mElements.push_back( &NewElement );
    NewElement.voice( this );
    return *this;
  } // add( Element& )
  Voice& Voice::remove( Element& OldElement ) {
    auto RIt = find( mElements.rbegin(), mElements.rend(), &OldElement );
    if( RIt == mElements.rend() ) return *this;
    mElements.erase( RIt.base()-1 );
    OldElement.voice( nullptr );
    return *this;
  } // remove( Element& )
} // MuTraMusic
