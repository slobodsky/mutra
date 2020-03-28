set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package( Qt5 COMPONENTS Widgets REQUIRED )

add_executable( music_trainer forms/main.ui forms/settings_dialog.ui forms/devices_settings.ui forms/metronome_settings.ui forms/exercise_settings.ui forms/midi_mixer.ui app.qrc widgets.cpp main.cpp )

target_link_libraries( music_trainer music_trainer_training Qt5::Widgets )
