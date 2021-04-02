include_directories(include)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src_list)

add_library(${name} ${src_list})
target_link_libraries(${name} json-c)


add_subdirectory(test)
