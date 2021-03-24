/*
 * amf0.c - amf0
 *
 * Date   : 2021/03/24
 */
#include "amf0.h"
#include <stddef.h>
#include <string.h>
#include <assert.h>

static double s_double = 1.0; // 3ff0 0000 0000 0000

static uint8_t* AMFWriteInt16(uint8_t* ptr, const uint8_t* end, uint16_t value)
{
	if (ptr + 2 > end) return NULL;
	ptr[0] = value >> 8;
	ptr[1] = value & 0xFF;
	return ptr + 2;
}

static uint8_t* AMFWriteInt32(uint8_t* ptr, const uint8_t* end, uint32_t value)
{
	if (ptr + 4 > end) return NULL;
	ptr[0] = (uint8_t)(value >> 24);
	ptr[1] = (uint8_t)(value >> 16);
	ptr[2] = (uint8_t)(value >> 8);
	ptr[3] = (uint8_t)(value & 0xFF);
	return ptr + 4;
}

static uint8_t* AMFWriteString16(uint8_t* ptr, const uint8_t* end, const char* string, size_t length)
{
	if (ptr + 2 + length > end) return NULL;
	ptr = AMFWriteInt16(ptr, end, (uint16_t)length);
	memcpy(ptr, string, length);
	return ptr + length;
}

static uint8_t* AMFWriteString32(uint8_t* ptr, const uint8_t* end, const char* string, size_t length)
{
	if (ptr + 4 + length > end) return NULL;
	ptr = AMFWriteInt32(ptr, end, (uint32_t)length);
	memcpy(ptr, string, length);
	return ptr + length;
}

uint8_t* AMFWriteNull(uint8_t* ptr, const uint8_t* end)
{
	if (!ptr || ptr + 1 > end) return NULL;

	*ptr++ = AMF_NULL;
	return ptr;
}

uint8_t* AMFWriteUndefined(uint8_t* ptr, const uint8_t* end)
{
    if (!ptr || ptr + 1 > end) return NULL;

    *ptr++ = AMF_UNDEFINED;
    return ptr;
}

uint8_t* AMFWriteObject(uint8_t* ptr, const uint8_t* end)
{
	if (!ptr || ptr + 1 > end) return NULL;

	*ptr++ = AMF_OBJECT;
	return ptr;
}

uint8_t* AMFWriteObjectEnd(uint8_t* ptr, const uint8_t* end)
{
	if (!ptr || ptr + 3 > end) return NULL;

	/* end of object - 0x00 0x00 0x09 */
	*ptr++ = 0;
	*ptr++ = 0;
	*ptr++ = AMF_OBJECT_END;
	return ptr;
}

uint8_t* AMFWriteTypedObject(uint8_t* ptr, const uint8_t* end)
{
    if (!ptr || ptr + 1 > end) return NULL;

    *ptr++ = AMF_TYPED_OBJECT;
    return ptr;
}

uint8_t* AMFWriteECMAArarry(uint8_t* ptr, const uint8_t* end)
{
	if (!ptr || ptr + 1 > end) return NULL;

	*ptr++ = AMF_ECMA_ARRAY;
	return AMFWriteInt32(ptr, end, 0); // U32 associative-count
}

uint8_t* AMFWriteBoolean(uint8_t* ptr, const uint8_t* end, uint8_t value)
{
	if (!ptr || ptr + 2 > end) return NULL;

	ptr[0] = AMF_BOOLEAN;
	ptr[1] = 0 == value ? 0 : 1;
	return ptr + 2;
}

uint8_t* AMFWriteDouble(uint8_t* ptr, const uint8_t* end, double value)
{
	if (!ptr || ptr + 9 > end) return NULL;

	assert(8 == sizeof(double));
	*ptr++ = AMF_NUMBER;

	// Little-Endian
	if (0x00 == *(char*)&s_double)
	{
		*ptr++ = ((uint8_t*)&value)[7];
		*ptr++ = ((uint8_t*)&value)[6];
		*ptr++ = ((uint8_t*)&value)[5];
		*ptr++ = ((uint8_t*)&value)[4];
		*ptr++ = ((uint8_t*)&value)[3];
		*ptr++ = ((uint8_t*)&value)[2];
		*ptr++ = ((uint8_t*)&value)[1];
		*ptr++ = ((uint8_t*)&value)[0];
	}
	else
	{
		memcpy(ptr, &value, 8);
	}
	return ptr;
}

uint8_t* AMFWriteString(uint8_t* ptr, const uint8_t* end, const char* string, size_t length)
{
	if (!ptr || ptr + 1 + (length < 65536 ? 2 : 4) + length > end || length > UINT32_MAX)
		return NULL;

	if (length < 65536)
	{
		*ptr++ = AMF_STRING;
		AMFWriteString16(ptr, end, string, length);
		ptr += 2;
	}
	else
	{
		*ptr++ = AMF_LONG_STRING;
		AMFWriteString32(ptr, end, string, length);
		ptr += 4;
	}
	return ptr + length;
}

uint8_t* AMFWriteDate(uint8_t* ptr, const uint8_t* end, double milliseconds, int16_t timezone)
{
    if (!ptr || ptr + 11 > end)
        return NULL;

    AMFWriteDouble(ptr, end, milliseconds);
    *ptr = AMF_DATE; // rewrite to date
    return AMFWriteInt16(ptr + 8, end, timezone);
}

uint8_t* AMFWriteNamedBoolean(uint8_t* ptr, const uint8_t* end, const char* name, size_t length, uint8_t value)
{
	if (ptr + length + 2 + 2 > end)
		return NULL;

	ptr = AMFWriteString16(ptr, end, name, length);
	return ptr ? AMFWriteBoolean(ptr, end, value) : NULL;
}

uint8_t* AMFWriteNamedDouble(uint8_t* ptr, const uint8_t* end, const char* name, size_t length, double value)
{
	if (ptr + length + 2 + 8 + 1 > end)
		return NULL;

	ptr = AMFWriteString16(ptr, end, name, length);
	return ptr ? AMFWriteDouble(ptr, end, value) : NULL;
}

uint8_t* AMFWriteNamedString(uint8_t* ptr, const uint8_t* end, const char* name, size_t length, const char* value, size_t length2)
{
	if (ptr + length + 2 + length2 + 3 > end)
		return NULL;

	ptr = AMFWriteString16(ptr, end, name, length);
	return ptr ? AMFWriteString(ptr, end, value, length2) : NULL;
}

static const uint8_t* AMFReadInt16(const uint8_t* ptr, const uint8_t* end, uint32_t* value)
{
	if (!ptr || ptr + 2 > end)
		return NULL;

	if (value)
	{
		*value = ((uint32_t)ptr[0] << 8) | ptr[1];
	}
	return ptr + 2;
}

static const uint8_t* AMFReadInt32(const uint8_t* ptr, const uint8_t* end, uint32_t* value)
{
	if (!ptr || ptr + 4 > end)
		return NULL;

	if (value)
	{
		*value = ((uint32_t)ptr[0] << 24) | ((uint32_t)ptr[1] << 16) | ((uint32_t)ptr[2] << 8) | ptr[3];
	}
	return ptr + 4;
}

const uint8_t* AMFReadNull(const uint8_t* ptr, const uint8_t* end)
{
	(void)end;
	return ptr;
}

const uint8_t* AMFReadUndefined(const uint8_t* ptr, const uint8_t* end)
{
    (void)end;
    return ptr;
}

const uint8_t* AMFReadBoolean(const uint8_t* ptr, const uint8_t* end, uint8_t* value)
{
	if (!ptr || ptr + 1 > end)
		return NULL;

	if (value)
	{
		*value = ptr[0];
	}
	return ptr + 1;
}

const uint8_t* AMFReadDouble(const uint8_t* ptr, const uint8_t* end, double* value)
{
	uint8_t* p = (uint8_t*)value;
	if (!ptr || ptr + 8 > end)
		return NULL;

	if (value)
	{
		if (0x00 == *(char*)&s_double)
		{// Little-Endian
			*p++ = ptr[7];
			*p++ = ptr[6];
			*p++ = ptr[5];
			*p++ = ptr[4];
			*p++ = ptr[3];
			*p++ = ptr[2];
			*p++ = ptr[1];
			*p++ = ptr[0];
		}
		else
		{
			memcpy(value, ptr, 8);
		}
	}
	return ptr + 8;
}

const uint8_t* AMFReadString(const uint8_t* ptr, const uint8_t* end, int isLongString, char* string, size_t length)
{
	uint32_t len = 0;
	if (0 == isLongString)
		ptr = AMFReadInt16(ptr, end, &len);
	else
		ptr = AMFReadInt32(ptr, end, &len);

	if (!ptr || ptr + len > end)
		return NULL;

	if (string && length > len)
	{
		memcpy(string, ptr, len);
		string[len] = 0;
	}
	return ptr + len;
}

const uint8_t* AMFReadDate(const uint8_t* ptr, const uint8_t* end, double *milliseconds, int16_t *timezone)
{
    uint32_t v;
    ptr = AMFReadDouble(ptr, end, milliseconds);
    if (ptr)
    {
        ptr = AMFReadInt16(ptr, end, &v);
        *timezone = (int16_t)v;
    }
    return ptr;
}

static const uint8_t* amf_read_object(const uint8_t* data, const uint8_t* end, struct amf_object_item_t* items, size_t n);
static const uint8_t* amf_read_ecma_array(const uint8_t* data, const uint8_t* end, struct amf_object_item_t* items, size_t n);
static const uint8_t* amf_read_strict_array(const uint8_t* ptr, const uint8_t* end, struct amf_object_item_t* items, size_t n);

static const uint8_t* amf_read_item(const uint8_t* data, const uint8_t* end, enum AMFDataType type, struct amf_object_item_t* item)
{
	switch (type)
	{
	case AMF_BOOLEAN:
		return AMFReadBoolean(data, end, (uint8_t*)(item ? item->value : NULL));

	case AMF_NUMBER:
		return AMFReadDouble(data, end, (double*)(item ? item->value : NULL));

	case AMF_STRING:
		return AMFReadString(data, end, 0, (char*)(item ? item->value : NULL), item ? item->size : 0);

	case AMF_LONG_STRING:
		return AMFReadString(data, end, 1, (char*)(item ? item->value : NULL), item ? item->size : 0);

    case AMF_DATE:
        return AMFReadDate(data, end, (double*)(item ? item->value : NULL), (int16_t*)(item ? (char*)item->value + 8 : NULL));

	case AMF_OBJECT:
		return amf_read_object(data, end, (struct amf_object_item_t*)(item ? item->value : NULL), item ? item->size : 0);

	case AMF_NULL:
		return data;

	case AMF_UNDEFINED:
		return data;

	case AMF_ECMA_ARRAY:
		return amf_read_ecma_array(data, end, (struct amf_object_item_t*)(item ? item->value : NULL), item ? item->size : 0);

	case AMF_STRICT_ARRAY:
		return amf_read_strict_array(data, end, (struct amf_object_item_t*)(item ? item->value : NULL), item ? item->size : 0);

	default:
		assert(0);
		return NULL;
	}
}

static inline int amf_read_item_type_check(uint8_t type0, uint8_t itemtype)
{
    // decode AMF_ECMA_ARRAY as AMF_OBJECT
    return (type0 == itemtype || (AMF_OBJECT == itemtype && (AMF_ECMA_ARRAY == type0 || AMF_NULL == type0))) ? 1 : 0;
}

static const uint8_t* amf_read_strict_array(const uint8_t* ptr, const uint8_t* end, struct amf_object_item_t* items, size_t n)
{
	uint8_t type;
	uint32_t i, count;
	if (!ptr || ptr + 4 > end)
		return NULL;

	ptr = AMFReadInt32(ptr, end, &count); // U32 array-count
	for (i = 0; i < count && ptr && ptr < end; i++)
	{
		type = *ptr++;
		ptr = amf_read_item(ptr, end, type, (i < n && amf_read_item_type_check(type, items[i].type)) ? &items[i] : NULL);
	}

	return ptr;
}

static const uint8_t* amf_read_ecma_array(const uint8_t* ptr, const uint8_t* end, struct amf_object_item_t* items, size_t n)
{
	if (!ptr || ptr + 4 > end)
		return NULL;
	ptr += 4; // U32 associative-count
	return amf_read_object(ptr, end, items, n);
}

static const uint8_t* amf_read_object(const uint8_t* data, const uint8_t* end, struct amf_object_item_t* items, size_t n)
{
	uint8_t type;
	uint32_t len;
	size_t i;

	while (data && data + 2 <= end)
	{
		len = *data++ << 8;
		len |= *data++;
		if (0 == len)
			break; // last item

		if (data + len + 1 > end)
			return NULL; // invalid

		for (i = 0; i < n; i++)
		{
			if (strlen(items[i].name) == len && 0 == memcmp(items[i].name, data, len) && amf_read_item_type_check(data[len], items[i].type))
				break;
		}

		data += len; // skip name string
		type = *data++; // value type
		data = amf_read_item(data, end, type, i < n ? &items[i] : NULL);
	}

	if (data && data < end && AMF_OBJECT_END == *data)
		return data + 1;
	return NULL; // invalid object
}

const uint8_t* amf_read_items(const uint8_t* data, const uint8_t* end, struct amf_object_item_t* items, size_t count)
{
	size_t i;
	uint8_t type;
	for (i = 0; i < count && data && data < end; i++)
	{
		type = *data++;
		if (!amf_read_item_type_check(type, items[i].type))
			return NULL;

		data = amf_read_item(data, end, type, &items[i]);
	}

	return data;
}
