include_directories(include)
include_directories(../mpeg4-avc/include)

get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src_list)

add_library(${name} ${src_list})
target_link_libraries(${name} mpeg4-avc)

add_subdirectory(test)
