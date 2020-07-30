set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package( Qt5 COMPONENTS Widgets DBus Svg REQUIRED )

add_executable( mutra forms/main.ui forms/settings_dialog.ui forms/devices_settings.ui forms/metronome_settings.ui forms/exercise_settings.ui forms/notes_exercise_settings.ui forms/midi_mixer.ui
  app.qrc widgets.cpp main.cpp )

target_link_libraries( mutra mutra_training Qt5::Widgets Qt5::DBus Qt5::Svg )
install( TARGETS mutra COMPONENT mutra )
install( FILES images/mutra.svg DESTINATION share/icons/hicolor/scalable/apps COMPONENT mutra )
install( FILES mutra.desktop DESTINATION share/applications COMPONENT mutra )
