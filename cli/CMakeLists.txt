include_directories(  "${PROJECT_SOURCE_DIR}/lib/midi" "${PROJECT_SOURCE_DIR}/lib/midi/linux" )

add_executable( music_trainer_player star_lighter.cpp player.cpp )
target_link_libraries( music_trainer_player music_trainer_midi pthread )

add_executable( music_trainer_input_test star_lighter.cpp midi_input_test.cpp )
target_link_libraries( music_trainer_input_test music_trainer_midi pthread )
