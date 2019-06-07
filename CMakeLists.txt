cmake_minimum_required(VERSION 3.1)
project(wpn114audio-qml LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_LINK_FLAGS)

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml)

set(WPN114AUDIO_QML_SOURCES
    qml_plugin.cpp
    source/graph.hpp
    source/graph.cpp
    source/io/external.hpp
    source/io/external.cpp
    source/basics/sinetest.hpp
    source/basics/vca.hpp
    source/basics/midi-transpose.hpp
    examples/Audio.qml
    )

add_library(${PROJECT_NAME} SHARED ${WPN114AUDIO_QML_SOURCES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick Qt5::Qml)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} -framework CoreFoundation -framework CoreAudio)
elseif(UNIX AND NOT APPLE)
    set(WPN114AUDIO_QML_INSTALL_PATH /usr/lib/qt/qml/WPN114/Audio)
    add_definitions(-D__LINUX_ALSA__ -D__UNIX_JACK__)
    target_link_libraries(${PROJECT_NAME} -lpthread -ljack -lasound)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${WPN114AUDIO_QML_INSTALL_PATH})
install(FILES qml/audio.qmltypes qml/qmldir DESTINATION ${WPN114AUDIO_QML_INSTALL_PATH})
install(FILES source/graph.hpp DESTINATION ${WPN114AUDIO_QML_INSTALL_PATH}/include)
