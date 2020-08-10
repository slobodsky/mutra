add_library( mutra_training SHARED Lesson.cpp exercise_sequence.cpp metronome.cpp )

target_link_libraries( mutra_training mutra_midi )
set_target_properties( mutra_training PROPERTIES VERSION "0.0.1" )
set_target_properties( mutra_training PROPERTIES SOVERSION "0.0.1" )

install( TARGETS mutra_training DESTINATION lib COMPONENT libmutra )
install( FILES icons/application-x-mutra-exercise.svg icons/application-x-mutra-lesson.svg DESTINATION share/icons/hicolor/scalable/mimetypes COMPONENT libmutra )
install( FILES mutra-training-mime.xml DESTINATION share/mime/packages COMPONENT libmutra )
install( FILES exercise_sequence.hpp Lesson.hpp metronome.hpp DESTINATION include/mutra/training COMPONENT libmutra-dev  )
