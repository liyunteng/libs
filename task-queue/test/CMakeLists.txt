get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} PATH)
get_filename_component(name ${name} NAME)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src_list)

set(exe  ${name}_test)
add_executable(${exe} ${src_list})
# add_compile_options(-std=c99 -Wall)
target_link_libraries(${exe} ${name})
target_link_libraries(${exe} thread-pool pthread)
