cmake_minimum_required(VERSION 3.1.0)

project(rcMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Widgets Core PrintSupport REQUIRED)

file(GLOB CPP_SOURCES "*.cpp")
file(GLOB UI_SOURCES "*.ui")

add_executable(
    rcMonitor
    ${CPP_SOURCES} 
    ${UI_SOURCES}
    # resources.qrc
)

target_link_libraries(rcMonitor 
    Qt5::Widgets
    Qt5::Core
    Qt5::PrintSupport
    pthread
    procps #sudo apt-get install libprocps-dev
)