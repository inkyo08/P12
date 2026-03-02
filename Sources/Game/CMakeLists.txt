add_executable(Game)

file(GLOB_RECURSE GAME_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE GAME_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

target_sources(Game PRIVATE ${GAME_HEADERS} ${GAME_SOURCES})
target_link_libraries(Game PRIVATE mimalloc SDL3::SDL3)
