include_directories(../mov/include)
include_directories(../ts/include)
include_directories(include)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)

get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_library(${name}_shared SHARED ${SRC_LIST})
set_target_properties(${name}_shared PROPERTIES OUTPUT_NAME ${name})

if (BUILDSTATIC)
  add_library(lib${name}_static STATIC ${SRC_LIST})
  set_target_properties(lib${name}_static PROPERTIES OUTPUT_NAME ${name})
endif()

add_subdirectory(test)
