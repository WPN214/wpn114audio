cmake_minimum_required(VERSION 3.1)
project(wpn114audio LANGUAGES CXX)

# weird things happen if cxx_standard is not 17 for Android (llvm libc++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml)

# SOURCES -----------------------------------------------------------------------------------------

set(WPN114_AUDIO_INCLUDE_DIR include)
set(WPN114_AUDIO_HEADERS
    ${WPN114_AUDIO_INCLUDE_DIR}/wpn114audio/qtinterface.hpp
    ${WPN114_AUDIO_INCLUDE_DIR}/wpn114audio/execution.hpp
    ${WPN114_AUDIO_INCLUDE_DIR}/wpn114audio/basic_types.hpp
    ${WPN114_AUDIO_INCLUDE_DIR}/wpn114audio/utilities.hpp
    ${WPN114_AUDIO_INCLUDE_DIR}/wpn114audio/spatial.hpp)

set(WPN114_AUDIO_SOURCE_DIR source)
set(WPN114_AUDIO_QML_DIR qml)

set(WPN114_AUDIO_SOURCES
    qml_plugin.cpp
    ${WPN114_AUDIO_QML_DIR}/qmldir
    ${WPN114_AUDIO_QML_DIR}/audio.qmltypes
    ${WPN114_AUDIO_SOURCE_DIR}/qtinterface.cpp
    ${WPN114_AUDIO_SOURCE_DIR}/spatial.cpp
    ${WPN114_AUDIO_SOURCE_DIR}/io/external.hpp
    ${WPN114_AUDIO_SOURCE_DIR}/io/external.cpp
    ${WPN114_AUDIO_SOURCE_DIR}/basics/midi/rwriter.hpp
    ${WPN114_AUDIO_SOURCE_DIR}/basics/midi/transposer.hpp
    ${WPN114_AUDIO_SOURCE_DIR}/basics/midi/velocity-table.hpp
    ${WPN114_AUDIO_SOURCE_DIR}/basics/audio/sinetest.hpp
    ${WPN114_AUDIO_SOURCE_DIR}/basics/audio/vca.hpp
    ${WPN114_AUDIO_SOURCE_DIR}/basics/audio/clock.hpp
    ${WPN114_AUDIO_SOURCE_DIR}/basics/audio/peakrms.hpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(ANDROID)
    # we'd have to use QtMultimedia for that
    find_package(Qt5 REQUIRED COMPONENTS Multimedia)

    set(WPN114_AUDIO_SOURCES ${WPN114_AUDIO_SOURCES}
        ${WPN114_AUDIO_SOURCE_DIR}/io/io_qt.hpp
        ${WPN114_AUDIO_SOURCE_DIR}/io/io_qt.cpp)
elseif(APPLE)
elseif(UNIX)
    find_package(ALSA REQUIRED)
    find_package(JACK QUIET)
    # we'll see later about PulseAudio...

    set(WPN114_AUDIO_SOURCES ${WPN114_AUDIO_SOURCES}
        ${WPN114_AUDIO_SOURCE_DIR}/io/io_alsa.hpp
        ${WPN114_AUDIO_SOURCE_DIR}/io/io_alsa.cpp)

    if(JACK_FOUND)
        message("[WPN114] Compiling with Jack support")
        set(WPN114_AUDIO_SOURCES ${WPN114_AUDIO_SOURCES}
            ${WPN114_AUDIO_SOURCE_DIR}/io/io_jack.hpp
            ${WPN114_AUDIO_SOURCE_DIR}/io/io_jack.cpp)
    endif()
endif()

option(WPN114_AUDIO_EXAMPLES ON)

if(NOT ANDROID AND WPN114_AUDIO_EXAMPLES)
    add_subdirectory(examples)
endif()

# PROJECT -----------------------------------------------------------------------------------------

include(ProcessorCount)
ProcessorCount(WPN114_NPROC)

add_library(${PROJECT_NAME} SHARED ${WPN114_AUDIO_HEADERS} ${WPN114_AUDIO_SOURCES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick Qt5::Qml)
target_include_directories(${PROJECT_NAME} PUBLIC ${WPN114_AUDIO_INCLUDE_DIR})
target_compile_definitions(${PROJECT_NAME} PRIVATE -DWPN114_NPROC=${WPN114_NPROC})

# LINKING -----------------------------------------------------------------------------------------

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} -framework CoreFoundation -framework CoreAudio)
elseif(ANDROID)
    target_link_libraries(${PROJECT_NAME} Qt5::Multimedia)
elseif(UNIX)
    target_include_directories(${PROJECT_NAME} PUBLIC ${ALSA_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${ALSA_LIBRARIES})
    if(JACK_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DWPN114AUDIO_JACK)
        target_link_libraries(${PROJECT_NAME} ${JACK_LIBRARY})
    endif()
endif()

#TODO: windows & ios

# INSTALLING --------------------------------------------------------------------------------------

if(EXISTS ${CMAKE_PREFIX_PATH} AND
   ${CMAKE_PREFIX_PATH} STREQUAL "/usr")
    set(QT5_QML_INSTALL_PATH /usr/lib/qt/qml)
else()
    set(QT5_QML_INSTALL_PATH ${CMAKE_PREFIX_PATH}/qml)
endif()

set(WPN114_AUDIO_INSTALL_PATH ${QT5_QML_INSTALL_PATH}/WPN114/Audio)
message("[WPN114] Library will be installed in ${WPN114_AUDIO_INSTALL_PATH}")

install(TARGETS ${PROJECT_NAME} DESTINATION ${WPN114_AUDIO_INSTALL_PATH})
install(FILES qml/audio.qmltypes qml/qmldir DESTINATION ${WPN114_AUDIO_INSTALL_PATH})
install(DIRECTORY include DESTINATION ${WPN114_AUDIO_INSTALL_PATH})
