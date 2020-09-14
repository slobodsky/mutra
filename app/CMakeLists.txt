set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package( Qt5 COMPONENTS Widgets DBus Svg REQUIRED )

add_executable( mutra forms/main.ui forms/settings_dialog.ui forms/devices_settings.ui forms/metronome_settings.ui forms/exercise_settings.ui forms/notes_exercise_settings.ui forms/midi_mixer.ui
  app.qrc widgets.cpp main.cpp )

target_link_libraries( mutra mutra_training Qt5::Widgets Qt5::DBus Qt5::Svg )
install( TARGETS mutra DESTINATION bin COMPONENT mutra )
install( FILES images/mutra.svg images/mutra-dark.svg DESTINATION share/icons/hicolor/scalable/apps COMPONENT mutra )
install( FILES images/mutra.png images/mutra-dark.png DESTINATION share/icons/hicolor/256x256/apps COMPONENT mutra )
install( FILES mutra.desktop DESTINATION share/applications COMPONENT mutra )
