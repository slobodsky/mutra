add_library( music_trainer_music SHARED music.cpp )

target_link_libraries( music_trainer_music music_trainer_midi )
