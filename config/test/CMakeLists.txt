aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} OBJ_SRC)
# add_compile_options(-std=c99 -Wall)
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} PATH)
get_filename_component(name ${name} NAME)
set(exe  ${name}_test)
set(libname lib${name})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_config.cfg
  DESTINATION ${EXECUTABLE_OUTPUT_PATH})

add_executable(${exe} ${OBJ_SRC})
target_link_libraries(${exe} ${libname}_shared)
target_link_libraries(${exe} -ljson-c)

if (BUILDSTATIC)
  add_executable(${exe}_static ${OBJ_SRC})
  target_link_libraries(${exe}_static ${libname}_static)
  target_link_libraries(${exe}_static -ljson-c)

endif()
