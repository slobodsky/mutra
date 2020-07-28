include_directories(  "${PROJECT_SOURCE_DIR}/lib" )

add_executable( mutra_player star_lighter.cpp player.cpp )
target_link_libraries( mutra_player mutra_midi pthread )

add_executable( mutra_input_test star_lighter.cpp midi_input_test.cpp )
target_link_libraries( mutra_input_test mutra_midi mutra_training pthread )

add_executable( mutra_light trainer_light.cpp )
target_link_libraries( mutra_light mutra_midi mutra_training pthread )

add_executable( mutra_midi_file_printer midi_file_printer.cpp )
target_link_libraries( mutra_midi_file_printer mutra_midi )
