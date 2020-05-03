/*
 * rtp.h - rtp
 *
 * Date   : 2020/05/01
 */
#ifndef RTP_H
#define RTP_H

#include <stdint.h>

#define IPMAXLEN 20
#define UDP_MAX_SIZE 1500
#define RTP_FIXED_HEADER_SIZE 12
#define RTP_DEFAULT_JITTER_TIME 80       /* miliseconds */
#define RTP_DEFAULT_MULTICAST_TTL 5      /* hops */
#define RTP_DEFAULT_MULTICAST_LOOPBACK 0 /* false */
#define RTP_DEFAULT_DSCP 0x00            /* best effort */

/*  0                   1                   2                   3
    *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    * |V=2|P|X|  CC   |M|     PT      |       sequence number         |
    * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    * |                           timestamp                           |
    * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    * |           synchronization source (SSRC) identifier            |
    * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    * |            contributing source (CSRC) identifiers             |
    * |                             ....                              |
    * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
typedef struct {
#ifdef __BIGENDIAN__
    uint8_t version : 2;
    uint8_t padbid : 1;
    uint8_t extbit : 1;
    uint8_t cc : 4;

    uint8_t markbit : 1;
    uint8_t paytype : 7;
#else
    uint8_t cc : 4;
    uint8_t extbit : 1;
    uint8_t padbit : 1;
    uint8_t version : 2;
    uint8_t paytype : 7;
    uint8_t markbit : 1;
#endif  // __BIGENDIAN__
    uint16_t seq_number;
    uint32_t timestamp;
    uint32_t ssrc;
    uint32_t csrc[15];
} rtp_header_t;

typedef struct {
    uint64_t packet_sent;
    uint64_t sent;            /* bytes sent */
    uint64_t recv;            /* bytes of payload received and
                                 * delivered in time to the application */
    uint64_t hw_recv;         /* bytes of payload received */
    uint64_t packet_recv;     /* number of packets received */
    uint64_t unavaillable;    /* packets not availlable when they
                                 * where queried */
    uint64_t outoftime;       /* number of packets that were
                                 * received too late */
    uint64_t cum_packet_loss; /* cumulative number of packet lost */
    uint64_t bad;             /* packets that did not apper to be RTP */
    uint64_t discarded;       /* incoming packets discarded because
                                 * the queue exceeds its max size */
} rtp_stats_t;

#define RTP_TIMESTAMP_IS_NEWER_THAN(ts1,ts2) \
	((uint32_t)((uint32_t)(ts1) - (uint32_t)(ts2))< (uint32_t)(1<<31))

#define RTP_TIMESTAMP_IS_STRICTLY_NEWER_THAN(ts1,ts2) \
	( ((uint32_t)((uint32_t)(ts1) - (uint32_t)(ts2))< (uint32_t)(1<<31)) && (ts1)!=(ts2) )

#define TIME_IS_NEWER_THAN(t1,t2) RTP_TIMESTAMP_IS_NEWER_THAN(t1,t2)

#define TIME_IS_STRICTLY_NEWER_THAN(t1,t2) RTP_TIMESTAMP_IS_STRICTLY_NEWER_THAN(t1,t2)

/* packet api */
/* the first argument is a mblk_t. The header is supposed to be not splitted  */
#define rtp_set_markbit(mp,value)		((rtp_header_t*)((mp)->b_rptr))->markbit=(value)
#define rtp_set_seqnumber(mp,seq)	((rtp_header_t*)((mp)->b_rptr))->seq_number=(seq)
#define rtp_set_timestamp(mp,ts)	((rtp_header_t*)((mp)->b_rptr))->timestamp=(ts)
#define rtp_set_ssrc(mp,_ssrc)		((rtp_header_t*)((mp)->b_rptr))->ssrc=(_ssrc)
#define rtp_set_payload_type(mp,pt)	((rtp_header_t*)((mp)->b_rptr))->paytype=(pt)

#define rtp_get_markbit(mp)	(((rtp_header_t*)((mp)->b_rptr))->markbit)
#define rtp_get_extbit(mp)	(((rtp_header_t*)((mp)->b_rptr))->extbit)
#define rtp_get_timestamp(mp)	(((rtp_header_t*)((mp)->b_rptr))->timestamp)
#define rtp_get_seqnumber(mp)	(((rtp_header_t*)((mp)->b_rptr))->seq_number)
#define rtp_get_payload_type(mp)	(((rtp_header_t*)((mp)->b_rptr))->paytype)
#define rtp_get_ssrc(mp)		(((rtp_header_t*)((mp)->b_rptr))->ssrc)
#define rtp_get_cc(mp)		(((rtp_header_t*)((mp)->b_rptr))->cc)
#define rtp_get_csrc(mp, idx)		(((rtp_header_t*)((mp)->b_rptr))->csrc[idx])

#endif
