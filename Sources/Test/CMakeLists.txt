add_executable(Test)

file(GLOB_RECURSE TEST_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

target_sources(Test PRIVATE ${TEST_HEADERS} ${TEST_SOURCES})
target_link_libraries(Test mimalloc Common)
