cmake_minimum_required(VERSION 3.17)
project(monosynth)

set(CMAKE_CXX_STANDARD 17)

add_executable(
        monosynth
        main.cpp
        VAStateVariableFilter/DSPUtilities.cpp
        VAStateVariableFilter/VAStateVariableFilter.cpp
)

include_directories(/usr/local/include /usr/local/include/rtaudio)
target_link_libraries(monosynth rtaudio rtmidi)