/*
 * opus-head.c - opus-head
 *
 * Date   : 2021/03/24
 */
#include "opus-head.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// http://www.opus-codec.org/docs/opus_in_isobmff.html
// 4.3.2 Opus Specific Box
/*
class ChannelMappingTable (unsigned int(8) OutputChannelCount){
    unsigned int(8) StreamCount;
    unsigned int(8) CoupledCount;
    unsigned int(8 * OutputChannelCount) ChannelMapping;
}

aligned(8) class OpusSpecificBox extends Box('dOps'){
    unsigned int(8) Version;
    unsigned int(8) OutputChannelCount;
    unsigned int(16) PreSkip;
    unsigned int(32) InputSampleRate;
    signed int(16) OutputGain;
    unsigned int(8) ChannelMappingFamily;
    if (ChannelMappingFamily != 0) {
        ChannelMappingTable(OutputChannelCount);
    }
}
*/

static const uint8_t opus_coupled_stream_cnt[9] = {
    1, 0, 1, 1, 2, 2, 2, 3, 3
};

static const uint8_t opus_stream_cnt[9] = {
    1, 1, 1, 2, 2, 3, 4, 4, 5,
};

static const uint8_t opus_channel_map[8][8] = {
    { 0 },
    { 0,1 },
    { 0,2,1 },
    { 0,1,2,3 },
    { 0,4,1,2,3 },
    { 0,4,1,2,3,5 },
    { 0,4,1,2,3,5,6 },
    { 0,6,1,2,3,4,5,7 },
};

int opus_head_save(const struct opus_head_t* opus, uint8_t* data, size_t bytes)
{
    if (bytes < 19)
        return -1;

    memcpy(data, "OpusHead", 8);
    data[8] = opus->version; // 0 only
    data[9] = opus->channels;
    data[11] = (uint8_t)(opus->pre_skip >> 8); // LSB
    data[10] = (uint8_t)opus->pre_skip;
    data[15] = (uint8_t)(opus->input_sample_rate >> 24); // LSB
    data[14] = (uint8_t)(opus->input_sample_rate >> 16);
    data[13] = (uint8_t)(opus->input_sample_rate >> 8);
    data[12] = (uint8_t)opus->input_sample_rate;
    data[17] = (uint8_t)(opus->output_gain >> 8); // LSB
    data[16] = (uint8_t)opus->output_gain;
    data[18] = opus->channel_mapping_family;
    if (0 != opus->channel_mapping_family && bytes >= 29)
    {
        data[19] = opus->stream_count;
        data[20] = opus->coupled_count;
        memcpy(data+21, opus->channel_mapping, 8);
        return 29;
    }

    return 19;
}

int opus_head_load(const uint8_t* data, size_t bytes, struct opus_head_t* opus)
{
    if (bytes < 19 || 0 != memcmp(data, "OpusHead", 8))
        return -1;

    memset(opus, 0, sizeof(*opus));
    opus->version = data[8];
    opus->channels = data[9];
    opus->pre_skip = ((uint16_t)data[11] << 8) | data[10];
    opus->input_sample_rate = ((uint32_t)data[15] << 24) | ((uint32_t)data[14] << 16) | ((uint32_t)data[13] << 8) | data[12];
    opus->output_gain = ((uint16_t)data[17] << 8) | data[16];
    opus->channel_mapping_family = data[18];

    if (0 != opus->channel_mapping_family && bytes >= 29)
    {
        opus->stream_count = data[19];
        opus->coupled_count = data[20];
        memcpy(opus->channel_mapping, data+21, 8);
        return 29;
    }
    else
    {
        opus->stream_count = opus_stream_cnt[opus->channels];
        opus->coupled_count = opus_coupled_stream_cnt[opus->channels];
        memcpy(opus->channel_mapping, opus_channel_map[opus_head_channels(opus)-1], 8);
    }

    return 19;
}

static const uint8_t* opus_parse_size(const uint8_t* data, int bytes, int *size)
{
    if (bytes < 1)
        return NULL;

    if (data[0] < 252)
    {
        *size = data[0];
        return data + 1;
    }

    if (bytes < 2)
        return NULL;
    *size = 4 * (uint16_t)data[1] + data[0];
    return data + 2;
}

static const uint8_t* opus_parse_padding(const uint8_t* data, int len)
{
    int n;
    int pad;

    pad = 0;
    do
    {
        if (len <= 0)
            return NULL;

        n = *data++;
        len--;

        len -= n == 255 ? 254 : n;
        pad += n == 255 ? 254 : n;
    } while (n == 255);

    return data;
}

/*
 *                      Table 6-1 opus_access_unit syntax

 |Syntax                               |Number of  |Identif|
 |                                     |bits       |ier    |
 |opus_access_unit() {                 |           |       |
 |   if(nextbits(11)==0x3FF) {         |           |       |
 |      opus_control_header()          |           |       |
 |                                     |           |       |
 |      for(i=0; i<stream_count-1; i++)|           |       |
 |{                                    |           |       |
 |          self_delimited_opus_packet |           |       |
 |      }                              |           |       |
 |undelimited_opus_packet              |           |       |
 |}                                    |           |       |
 *
 *
 *                      Table 6-2 opus_access_unit syntax

 |Syntax                                         |Number of  |Identif|
 |                                               |bits       |ier    |
 |opus_control_header() {                        |           |       |
 |   control_header_prefix                       |11         |bslbf  |
 |   start_trim_flag                             |1          |bslbf  |
 |   end_trim_flag                               |1          |bslbf  |
 |   control_extension_flag                      |1          |bslbf  |
 |   Reserved                                    |2          |bslbf  |
 |   au_size = 0                                 |           |       |
 |while(nextbits(8) == 0xFF){                    |           |       |
 |ff_byte [= 0xFF]                               |8          |uimsbf |
 |au_size += 255;                                |           |       |
 |}                                              |           |       |
 |au_size_last_byte                              |8          |uimsbf |
 |au_size += au_size_last_byte                   |           |       |
 |if(start_trim_flag==1) {                       |           |       |
 |      Reserved                                 |3          |bslbf  |
 |      start_trim                               |13         |uimsbf |
 |   }                                           |           |       |
 |   if(end_trim_flag==1) {                      |           |       |
 |      Reserved                                 |3          |bslbf  |
 |      end_trim                                 |13         |uimsbf |
 |   }                                           |           |       |
 |   if(control_extension_flag==1) {             |           |       |
 |      control_extension_length                 |8          |uimsbf |
 |      for(i=0; i<control_extension_length; i++)|           |       |
 |{                                              |           |       |
 |         reserved                              |8          |bslbf  |
 |      }                                        |           |       |
 |   }                                           |           |       |
 |}                                              |           |       |
 */

static const uint8_t* opus_ts_header(const uint8_t* data, int bytes, int* payload)
{
    int i;
    int start_trim_flag;
    int end_trim_flag;
    int control_extension_flag;
    int au_size;
    uint16_t prefix;

    if(bytes < 3)
        return NULL;

    i = 0;
    prefix = ((uint16_t)data[0] << 8) | data[1];
    if(0x7FE0 == (prefix & 0xFFE0))
    {
        //opus control header
        start_trim_flag = (prefix >> 4) & 0x01;
        end_trim_flag = (prefix >> 3) & 0x01;
        control_extension_flag = (prefix >> 2) & 0x01;

        au_size = data[2];
        for(i = 3; i < bytes && 0xff == data[i-1]; i++)
            au_size += data[i];

        if(i + (start_trim_flag ? 2 : 0) + (end_trim_flag ? 2 : 0) + (control_extension_flag ? 1 : 0) > bytes)
            return NULL;

        if(start_trim_flag)
            i += 2;
        if(end_trim_flag)
            i += 2;
        if(control_extension_flag)
        {
            if(i + 1 + data[i] > bytes)
                return NULL;
            i += 1 + data[i];
        }

        if(i + au_size > bytes)
            return NULL;

        *payload = au_size;
        return data + i;
    }
    else
    {
        *payload = bytes;
        return data;
    }
}

static int opus_parse_frames(const void* data, int len, int (*onframe)(uint8_t toc, const void* frame, int size), void* param)
{
    int i, r;
    int vbr, count;
    uint8_t toc;
    int n[48];
    const uint8_t* p, *end;

    if (len < 1)
        return -1;

    p = (const uint8_t*)data;
    end = p + len;

    toc = *p++;
    len -= 1;
    switch (toc & 0x03)
    {
    case 0: // one frame
        return onframe(toc, p, len - 1);

    case 1: // two CBR frames
        if (1 == (len % 2))
            return -1;

        toc = toc & 0xFC; // convert to one frame
        for (i = 0; i < 2; i++)
        {
            r = onframe(toc, p, len / 2);
            if (0 != r)
                return r;
        }
        return 0;

    case 2: // two VBR frames
        p = opus_parse_size(p, len, &n[0]);
        if (!p || n[0] < 0 || p + n[0] > end)
            return -1;

        toc = toc & 0xFC; // convert to one frame
        r = onframe(toc, p, n[0]);
        if (0 != r)
            return r;

        // frame 2
        p += n[0];
        return onframe(toc, p, end - p);

    default: // multiple CBR/VBR frames (from 0 to 120ms)
        if (len < 1)
            return -1;

        len--;
        count = *p & 0x3F; // bits of frames length (0-5)
        vbr = *p & 0x80;
        if (*p++ & 0x40) // padding
        {
            p = opus_parse_padding(p, len);
            if (!p)
                return -1;
        }

        toc = toc & 0xFC; // convert to one frame

        if (vbr)
        {
            for (i = 0; i < count - 1; i++)
            {
                p = opus_parse_size(p, end - p, &n[i]);
                if (!p || n[i] < 0 || p + n[i] > end)
                    return -1;
            }

            /* Because it's not encoded explicitly, it's possible the size of the
             last packet (or all the packets, for the CBR case) is larger than
             1275. Reject them here.*/
            if (end - p > 1275)
                return -1;
            n[i] = end - p; // last frame

            for (i = 0; i < count; i++)
            {
                r = onframe(toc, p, n[i]);
                if (0 != r)
                    return r;

                p += n[i];
            }
        }
        else
        {
            n[0] = (end - p) / count;
            if (p + n[0] * count != end)
                return -1;

            for (i = 0; i < count; i++)
            {
                r = onframe(toc, p, n[0]);
                if (0 != r)
                    return r;

                p += n[0];
            }
        }
    }

    return 0;
}

int opus_packet_getframes(const void* data, int len, int (*onframe)(uint8_t toc, const void* frame, int size), void* param)
{
    int r;
    int payload;
    const uint8_t* p, *end;

    p = (const uint8_t*)data;
    end = p + len;

    while(p < end)
    {
        p = opus_ts_header(p, end - p, &payload);
        if(!p)
            return -1;
        assert(p + payload <= end);

        r = opus_parse_frames(p, payload, onframe, param);
        if(r < 0)
            return r;

        p += payload;
    }

    return 0;
}
