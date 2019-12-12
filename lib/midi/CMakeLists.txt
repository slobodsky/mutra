add_library( music_trainer_midi SHARED midi_core.cpp linux/Sequencer.cpp linux/input_device.cpp )

target_link_libraries( music_trainer_midi asound )
