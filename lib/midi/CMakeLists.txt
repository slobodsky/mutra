# Move backends rules to the backends subdirectory
set( BACKENDS "backends/linux_backend.cpp" )
find_package( ALSA )
if( ALSA_FOUND )
  message( "USE_ALSA_BACKEND" )
  add_definitions( -DUSE_ALSA_BACKEND )
  set( EXTRA_LIBRARIES "${EXTRA_LIBRARIES}" "asound" )
  set( BACKENDS ${BACKENDS} "backends/alsa_backend.cpp" )
endif()
find_library( rtmidi_FOUND rtmidi )
if( rtmidi_FOUND )
  add_definitions( -DUSE_RTMIDI_BACKEND )
  set( BACKENDS "${BACKENDS}" "backends/rtmidi_backend.cpp" )
  message( "Use RTMIDI backend: ${rtmidi_FOUND} ${BACKENDS}" )
  set( EXTRA_LIBRARIES "${EXTRA_LIBRARIES}" "rtmidi" )
endif()

add_library( mutra_midi SHARED midi_core.cpp midi_events.cpp midi_files.cpp midi_utility.cpp ${BACKENDS} )

target_link_libraries( mutra_midi ${EXTRA_LIBRARIES} )
set_target_properties( mutra_midi PROPERTIES VERSION "0.0.1" )
set_target_properties( mutra_midi PROPERTIES SOVERSION "0.0.1" )

install( TARGETS mutra_midi DESTINATION lib COMPONENT libmutra )
install( FILES midi_core.hpp midi_events.hpp midi_files.hpp midi_utility.hpp DESTINATION include/mutra/midi COMPONENT libmutra-dev )
