cmake_minimum_required(VERSION 3.1)
project(wpn114audio-qml LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core)
find_package(Qt5Quick)

set(WPN114AUDIO_QML_SOURCES
    qml_plugin.cpp
    source/qtwrapper2.cpp
    )

add_library(${PROJECT_NAME} ${WPN114AUDIO_QML_SOURCES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick -lwpn114audio)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} -framework CoreFoundation -framework CoreAudio)
elseif(LINUX)
    add_definitions(-D__LINUX_ALSA__ -D__LINUX_PULSE__ -D__UNIX_JACK__)
    target_link_libraries(${PROJECT_NAME} -lpthread -ljack -lasound -lpulse-simple)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION /usr/lib/qt/qml/wpn114audio-qml)
install(FILES qml/audio.qmltypes qml/qmldir DESTINATION /usr/lib/qt/qml/wpn114audio-qml)
