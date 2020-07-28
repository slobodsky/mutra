add_library( mutra_training SHARED Lesson.cpp exercise_sequence.cpp metronome.cpp )

target_link_libraries( mutra_training mutra_midi )
set_target_properties( mutra_training PROPERTIES VERSION "0.0.1" )
set_target_properties( mutra_training PROPERTIES SOVERSION "0.0.1" )
