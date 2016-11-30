cmake_minimum_required(VERSION 2.8.7)
set(CMAKE_MACOSX_RPATH 1)

set(PROJECT redis-queue)
project(${PROJECT} CXX)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -Wextra -O3")


set(REDIS_QUEUE_INCLUDES ${PROJECT_SOURCE_DIR}/includes)


include_directories(${REDIS_QUEUE_INCLUDES})


set(SRC_DIRS "src")

foreach(dir ${SRC_DIRS})
  # get directory sources
  file(GLOB s_${dir} "${dir}/*.cpp")
  # set sources
  set(SOURCES ${SOURCES} ${s_${dir}})
endforeach()


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_library(${PROJECT} STATIC ${SOURCES})
target_link_libraries(${PROJECT} cpp_redis)


install(DIRECTORY DESTINATION ${CMAKE_BINARY_DIR}/lib/)
install(DIRECTORY DESTINATION ${CMAKE_BINARY_DIR}/bin/)
install (DIRECTORY ${CMAKE_BINARY_DIR}/lib/ DESTINATION lib USE_SOURCE_PERMISSIONS)
install (DIRECTORY ${CMAKE_BINARY_DIR}/bin/ DESTINATION bin USE_SOURCE_PERMISSIONS)
install (DIRECTORY ${REDIS_QUEUE_INCLUDES}/ DESTINATION include USE_SOURCE_PERMISSIONS)
