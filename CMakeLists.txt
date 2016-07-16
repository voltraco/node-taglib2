cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project (taglib2)

#
# a relatively stable snapshot of taglib2 branch for our purposes.
#
set(REVISION "31f3109")

#
# options
#
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(SUPPORT_LARGE_FILES ON)
if(UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS "-fPIC -std=c++1z")
else()
  set(CMAKE_CXX_FLAGS "-std=c++1z -stdlib=libc++")
endif()

#
# headers
#
macro(HEADER_DIRECTORIES start return_list)
  file(GLOB_RECURSE new_list ${start}/*.h)
  set(dir_list "")
  foreach(file_path ${new_list})
      get_filename_component(dir_path ${file_path} PATH)
      set(dir_list ${dir_list} ${dir_path})
  endforeach()
  list(REMOVE_DUPLICATES dir_list)
  set(${return_list} ${dir_list})
endmacro()

HEADER_DIRECTORIES(lib/taglib-${REVISION} TAGLIB_HEADERS)

set(HEADERS
  ${TAGLIB_HEADERS})

include_directories(${HEADERS})

#
# snapshot of taglib
#
add_subdirectory(lib/taglib-${REVISION})

#
# node-taglib2 specifics
#
file(GLOB SOURCE_FILES "src/*.cc" "src/*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

#
# link taglib and cmakejs
#
target_link_libraries(${PROJECT_NAME} tag)
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

