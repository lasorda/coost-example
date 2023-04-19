cmake_minimum_required(VERSION 3.0.0)
project(coost-example VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(coost-example main.cpp)

add_subdirectory(./coost)
target_link_libraries(coost-example co)

add_subdirectory(./hiredis)
target_link_libraries(coost-example hiredis)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
