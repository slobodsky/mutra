include_directories(  "${PROJECT_SOURCE_DIR}/lib" )

add_executable( music_trainer_player star_lighter.cpp player.cpp )
target_link_libraries( music_trainer_player music_trainer_midi pthread )

add_executable( music_trainer_input_test star_lighter.cpp midi_input_test.cpp )
target_link_libraries( music_trainer_input_test music_trainer_midi music_trainer_training pthread )

add_executable( music_trainer_light trainer_light.cpp )
target_link_libraries( music_trainer_light music_trainer_midi music_trainer_training pthread )

add_executable( midi_file_printer midi_file_printer.cpp )
target_link_libraries( midi_file_printer music_trainer_midi )
