add_library( music_trainer_midi SHARED midi_core.cpp midi_events.cpp midi_files.cpp midi_utility.cpp linux/Sequencer.cpp linux/input_device.cpp rtmidi_backend.cpp )

target_link_libraries( music_trainer_midi asound rtmidi )
