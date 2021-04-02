include_directories(../mov/include)
include_directories(../ts/include)
include_directories(include)
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src_list)

add_library(${name} ${src_list})
target_link_libraries(${name} mov)
target_link_libraries(${name} ts)


add_subdirectory(test)
