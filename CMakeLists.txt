cmake_minimum_required (VERSION 3.20)
project ("rlyeh-shoujotai-player")

set(CMAKE_CXX_STANDARD 20)

# Download external libraries
add_subdirectory ("deps")

# Build something
add_subdirectory ("rlyeh-shoujotai")

# Build player
add_subdirectory ("rlyeh-shoujotai-player")
