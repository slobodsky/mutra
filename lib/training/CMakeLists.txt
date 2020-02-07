add_library( music_trainer_training SHARED Lesson.cpp exercise_sequence.cpp )

target_link_libraries( music_trainer_training music_trainer_midi )
