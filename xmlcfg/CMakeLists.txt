include_directories(include)
include_directories(/usr/include/libxml2)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)

get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
add_library(lib${name}_shared SHARED ${SRC_LIST})
set_target_properties(lib${name}_shared PROPERTIES OUTPUT_NAME ${name})
target_link_libraries(lib${name}_shared -lxml2)

if (BUILDSTATIC)
  add_library(lib${name}_static STATIC ${SRC_LIST})
  set_target_properties(lib${name}_static PROPERTIES OUTPUT_NAME ${name})
  target_link_libraries(lib${name}_static -lxml2)
endif()

add_subdirectory(test)
