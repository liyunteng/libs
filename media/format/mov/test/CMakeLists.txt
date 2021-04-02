include_directories(../../../codec/mpeg4-aac/include)
include_directories(../../../codec/mpeg4-avc/include)
include_directories(../../../codec/mpeg4-hevc/include)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src_list)

set(exe  ${name}_test)
# add_executable(${exe} ${src_list})
add_executable(${exe} test.c
  fmp4-writer-test2.c
  mov-writer-adts.c
  mov-writer-audio.c
  mov-writer-h264.c
  mov-reader-test.c
  mov-file-buffer.c)
# add_compile_options(-std=c99 -Wall)
target_link_libraries(${exe} ${name})
target_link_libraries(${exe} mpeg4-aac)
target_link_libraries(${exe} mpeg4-avc)
target_link_libraries(${exe} mpeg4-hevc)
