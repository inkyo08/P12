cmake_minimum_required(VERSION 4.0)
project(P12 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  mimalloc
  GIT_REPOSITORY https://github.com/microsoft/mimalloc
  GIT_TAG        main
)
FetchContent_MakeAvailable(mimalloc)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(SDL3)

add_subdirectory(Sources/Runtime)
add_subdirectory(Sources/Game)
add_subdirectory(Sources/Test)
