/*
 * mov-buffer.h - mov-buffer
 *
 * Date   : 2021/03/24
 */
#ifndef __MOV_BUFFER_H__
#define __MOV_BUFFER_H__

#include <stdint.h>

struct mov_buffer_t {
	/// read data from buffer
	/// @param[in] param user-defined parameter
	/// @param[out] data user buffer
	/// @param[in] bytes data buffer size
	/// @return 0-ok, <0-error
	int (*read)(void* param, void* data, uint64_t bytes);

	/// write data to buffer
	/// @param[in] param user-defined parameter
	/// @param[in] data user buffer
	/// @param[in] bytes data buffer size
	/// @return 0-ok, <0-error
	int (*write)(void* param, const void* data, uint64_t bytes);

	/// move buffer position
	/// @param[in] param user-defined parameter
	/// @param[in] offset seek buffer read/write position to offset(from buffer begin)
	/// @return 0-ok, <0-error
	int (*seek)(void* param, uint64_t offset);

	/// get buffer read/write position
	/// @return >=0-current read/write position, <0-error
	uint64_t (*tell)(void* param);
};


#endif /* __MOV_BUFFER_H__ */
