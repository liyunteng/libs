include_directories(include)
include_directories(/usr/include/libxml2)
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src_list)

add_library(${name} ${src_list})
# target_link_libraries(${name} xml2)
exec_program("xml2-config" ARGS "--libs" OUTPUT_VARIABLE xml2libs)
target_link_libraries(${name} ${xml2libs})

add_subdirectory(test)
