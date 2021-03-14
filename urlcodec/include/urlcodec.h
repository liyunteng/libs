/*
 * urlcodec.h - urlcodec
 *
 * Date   : 2021/03/14
 */
#ifndef __URLCODEC_H__
#define __URLCODEC_H__

#ifdef  __cplusplus
extern "C" {
#endif

/*
usage:
	char target[32] = {0};
	const char* s = "abcdefg";
	url_encode(s, -1, target, sizeof(target));
	url_decode(s, 2, target, sizeof(target));
*/

/// url encode, auto fill target with '\0'
/// @param[in] srcBytes -1-strlen(source)
/// @return <0-error, >=0-encoded bytes
int url_encode(const char* source, int srcBytes, char* target, int tgtBytes);

/// url decode, auto fill target with '\0'
/// param[in] srcBytes -1-strlen(source)
/// @return <0-error, >=0-decoded bytes
int url_decode(const char* source, int srcBytes, char* target, int tgtBytes);


#ifdef  __cplusplus
}
#endif



#endif
