cmake_minimum_required(VERSION 3.1)
project(wpn114audio-qml LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml)

set(WPN114AUDIO_INCLUDE_DIR include)
set(WPN114AUDIO_HEADERS
    ${WPN114AUDIO_INCLUDE_DIR}/wpn114audio/graph.hpp
    ${WPN114AUDIO_INCLUDE_DIR}/wpn114audio/midi.hpp
    ${WPN114AUDIO_INCLUDE_DIR}/wpn114audio/spatial.hpp)

set(WPN114AUDIO_SOURCE_DIR source)
set(WPN114AUDIO_SOURCES
    qml_plugin.cpp
    ${WPN114AUDIO_SOURCE_DIR}/graph.cpp
    ${WPN114AUDIO_SOURCE_DIR}/spatial.cpp
    ${WPN114AUDIO_SOURCE_DIR}/io/external.hpp
    ${WPN114AUDIO_SOURCE_DIR}/io/external.cpp
    ${WPN114AUDIO_SOURCE_DIR}/basics/midi/rwriter.hpp
    ${WPN114AUDIO_SOURCE_DIR}/basics/midi/transposer.hpp
    ${WPN114AUDIO_SOURCE_DIR}/basics/midi/velocity-table.hpp
    ${WPN114AUDIO_SOURCE_DIR}/basics/audio/sinetest.hpp
    ${WPN114AUDIO_SOURCE_DIR}/basics/audio/vca.hpp
    ${WPN114AUDIO_SOURCE_DIR}/basics/audio/clock.hpp
    ${WPN114AUDIO_SOURCE_DIR}/basics/audio/peakrms.hpp)

option(WPN114AUDIO_EXAMPLES OFF)

if(WPN114AUDIO_EXAMPLES)
    add_subdirectory(examples)
endif()

add_library(${PROJECT_NAME} SHARED ${WPN114AUDIO_HEADERS} ${WPN114AUDIO_SOURCES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick Qt5::Qml)
target_include_directories(${PROJECT_NAME} PUBLIC ${WPN114AUDIO_INCLUDE_DIR})

# DEPENDENCIES ------------------------------------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} -framework CoreFoundation -framework CoreAudio)
elseif(ANDROID)
    # we'd have to use QtMultimedia for that
    find_package(Qt5 REQUIRED COMPONENTS Multimedia)
    target_link_libraries(${PROJECT_NAME} Qt5::Multimedia)
elseif(UNIX)
    find_package(ALSA REQUIRED)
    find_package(JACK QUIET)
    # we'll see later about PulseAudio...

    target_link_libraries(${PROJECT_NAME} ${ALSA_LIBRARIES})
    if(JACK_FOUND)
        message("Compiling with Jack support")
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DWPN114AUDIO_JACK)
        target_link_libraries(${PROJECT_NAME} ${JACK_LIBRARY})
    endif()
endif()

#TODO: windows & ios

# INSTALLING --------------------------------------------------------------------------------------

if(${CMAKE_PREFIX_PATH} STREQUAL "/usr")
    set(QT5_QML_INSTALL_PATH /usr/lib/qt/qml)
else()
    set(QT5_QML_INSTALL_PATH ${CMAKE_PREFIX_PATH}/qml)
endif()

set(WPN114AUDIO_INSTALL_PATH ${QT5_QML_INSTALL_PATH}/WPN114/Audio)

install(TARGETS ${PROJECT_NAME} DESTINATION ${WPN114AUDIO_INSTALL_PATH})
install(FILES qml/audio.qmltypes qml/qmldir DESTINATION ${WPN114AUDIO_INSTALL_PATH})
install(DIRECTORY include DESTINATION ${WPN114AUDIO_INSTALL_PATH})
