cmake_minimum_required(VERSION 3.1)
project(wpn114audio-qml LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml)

set(WPN114AUDIO_QML_SOURCES
    qml_plugin.cpp
    include/wpn114audio/graph.hpp
    include/wpn114audio/spatial.hpp
    include/wpn114audio/midi.hpp
    source/graph.cpp
    source/spatial.cpp
    source/io/external.hpp
    source/io/external.cpp
    source/basics/midi/rwriter.hpp
    source/basics/midi/transposer.hpp
    source/basics/midi/velocity-table.hpp
    source/basics/audio/sinetest.hpp
    source/basics/audio/vca.hpp
    source/basics/audio/clock.hpp
    )

add_library(${PROJECT_NAME} SHARED ${WPN114AUDIO_QML_SOURCES})
add_subdirectory(examples)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick Qt5::Qml)
target_include_directories(${PROJECT_NAME} PUBLIC include)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} -framework CoreFoundation -framework CoreAudio)
elseif(UNIX AND NOT APPLE)
    set(WPN114AUDIO_QML_INSTALL_PATH /usr/lib/qt/qml/WPN114/Audio)
    add_definitions(-D__LINUX_ALSA__ -D__UNIX_JACK__)
    target_link_libraries(${PROJECT_NAME} -lpthread -ljack -lasound)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${WPN114AUDIO_QML_INSTALL_PATH})
install(FILES qml/audio.qmltypes qml/qmldir DESTINATION ${WPN114AUDIO_QML_INSTALL_PATH})
install(DIRECTORY include DESTINATION ${WPN114AUDIO_QML_INSTALL_PATH})
