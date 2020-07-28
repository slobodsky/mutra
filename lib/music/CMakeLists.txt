add_library( mutra_music SHARED music.cpp )

target_link_libraries( mutra_music mutra_midi )
set_target_properties( mutra_music PROPERTIES VERSION "0.0.1" )
set_target_properties( mutra_music PROPERTIES SOVERSION "0.0.1" )
