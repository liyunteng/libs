/*
 * flv-parser.h - flv-parser
 *
 * Date   : 2021/04/02
 */
#ifndef __FLV_PARSER_H__
#define __FLV_PARSER_H__

#include <stdint.h>
#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

/// Audio/Video Elementary Stream
/// @param[in] param user-defined parameter
/// @param[in] codec audio/video format (see more flv-proto.h)
/// @param[in] data audio/video element data, AAC: AAC-Frame, H.264: MP4 Stream, MP3-Raw data
/// @param[in] bytes data length in byte
/// @param[in] pts audio/video presentation timestamp
/// @param[in] dts audio/video decoding timestamp
/// @param[in] flags 1-video keyframe, other-undefined
/// @return 0-ok, other-error
typedef int (*flv_parser_handler)(void* param, int codec, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags);

/// Input FLV Audio/Video Stream
/// @param[in] type 8-audio, 9-video, 18-script (see more flv-proto.h)
/// @param[in] data flv audio/video Stream, AudioTagHeader/VideoTagHeader + A/V Data
/// @param[in] bytes data length in byte
/// @param[in] timestamp milliseconds relative to the first tag(DTS)
/// @return 0-ok, other-error
int flv_parser_input(int type, const void* data, size_t bytes, uint32_t timestamp, flv_parser_handler handler, void* param);

#if defined(__cplusplus)
}
#endif

#endif /* __FLV_PARSER_H__ */
