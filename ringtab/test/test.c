/*
 * test.c - test
 *
 * Date   : 2020/04/27
 */
#include <stdio.h>
#include "ringtab.h"

typedef struct {
    uint8_t data;
} my_msg_t;

#define MY_RTAB1_DEPTH 10
#define MY_RTAB2_DEPTH 3
#define MY_RTAB3_DEPTH 3

typedef RINGTAB_STRUCT(my_rtab1_t_, MY_RTAB1_DEPTH, sizeof(my_msg_t))
    my_rtab1_t;
typedef RINGTAB_STRUCT(my_rtab2_t_, MY_RTAB2_DEPTH, sizeof(my_msg_t))
    my_rtab2_t;
typedef RINGTAB_STRUCT(my_rtab3_t_, MY_RTAB3_DEPTH, sizeof(my_msg_t))
    my_rtab3_t;

#define DATA_INV 0
#define DATA_VOID 0xFF

#define DATA_1 0x11
#define DATA_2 0x22
#define DATA_3 0x33
#define DATA_4 0x44
#define DATA_5 0x55
#define DATA_6 0x66

static void my_rtab_release(void *rel_ctx, void *item)
{
    my_msg_t *msg = item;

    uint8_t expected = (uint64_t) rel_ctx;

    printf("\n%20s(): expected=%02x, actual=%02x",
           __FUNCTION__, expected, msg->data);

    msg->data = DATA_VOID;
}

#define MY_RTAB_LOG(func, expected)                                 \
    printf("\n%20s(): ", func);                                     \
    printf("[dirty=%2d, in_use=%2d, head=%2d, tail=%2d, full=%2d]", \
           rtab.hdr.dirty, rtab.hdr.in_use,                         \
           rtab.hdr.head, rtab.hdr.tail, rtab.hdr.flags.full);      \
    if (msg != NULL) {                                              \
        printf(": expected=%02x, actual=%02x",                      \
               expected, msg->data);                                \
        if ((msg->data == DATA_INV) || (expected != msg->data)) {   \
            printf(" ---- ERROR!");                                 \
        }                                                           \
    }

#define MY_RTAB_PUT(dat) \
    if (msg != NULL) msg->data = dat;

void
ringtab_test_1(void)
{
    my_rtab1_t rtab;
    my_msg_t *msg;

    printf("\n******************** in %s ********************", __FUNCTION__);
    printf("\nringtab_init(): putting 4 items in 10-item ring table");
    ringtab_init(&rtab.hdr, MY_RTAB1_DEPTH, sizeof(my_msg_t));
    printf("\nringtab_items_used() = %d",
           ringtab_items_used(&rtab.hdr));
    printf("\nringtab_items_left() = %d",
           ringtab_items_left(&rtab.hdr));

    printf("\nPutting %02x %02x %02x %02x", DATA_1, DATA_2, DATA_3, DATA_4);
    msg = ringtab_put_item(&rtab.hdr);
    MY_RTAB_PUT(DATA_1);
    msg = ringtab_put_item(&rtab.hdr);
    MY_RTAB_PUT(DATA_2);
    msg = ringtab_put_item(&rtab.hdr);
    MY_RTAB_PUT(DATA_3);
    msg = ringtab_put_item(&rtab.hdr);
    MY_RTAB_PUT(DATA_4);

    printf("\n");
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_1);
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_1);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));
    msg = ringtab_peek_next((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_next", DATA_2);
    msg = ringtab_peek_last((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_last", DATA_4);

    printf("\n");
    ringtab_done_item((ringtab_hdr_t  *)&rtab);
    printf("\nringtab_done_item()");
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_2);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));
    msg = ringtab_peek_next((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_next", DATA_3);
    msg = ringtab_peek_last((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_last", DATA_4);
    msg = ringtab_get_dirty((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_get_dirty", DATA_1);

    printf("\n");
    msg = ringtab_get_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_get_item", DATA_2);
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_3);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));
    msg = ringtab_peek_next((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_next", DATA_4);
    msg = ringtab_peek_last((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_last", DATA_4);
    msg = ringtab_get_dirty((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_get_dirty", DATA_INV);

    printf("\n");
    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_INV);
    MY_RTAB_PUT(DATA_5);
    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_INV);
    MY_RTAB_PUT(DATA_6);
    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_4);
    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_5);
    ringtab_cleanup((ringtab_hdr_t  *)&rtab,
                    my_rtab_release, (void *)DATA_3);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_6);
    msg = ringtab_get_dirty((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_get_dirty", DATA_INV);
    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_INV);

    printf("\n\n");
}

void
ringtab_test_2 (void)
{
    my_rtab2_t rtab;
    my_msg_t *msg;

    /*
     * Test 4 items in 3-item ring table.
     */
    printf("\n********************* in %s *************************",
           __FUNCTION__);
    printf("\nringtab_init(): putting 4 items in 3-item ring table");
    ringtab_init((ringtab_hdr_t  *)&rtab, MY_RTAB2_DEPTH, sizeof(my_msg_t));
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));

    printf("\nPutting %02x %02x %02x %02x", DATA_1, DATA_2, DATA_3, DATA_4);
    msg = ringtab_put_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_PUT(DATA_1);
    msg = ringtab_put_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_PUT(DATA_2);
    msg = ringtab_put_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_PUT(DATA_3);
    msg = ringtab_put_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_PUT(DATA_4);

    printf("\n");
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_1);
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_1);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));
    msg = ringtab_peek_next((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_next", DATA_2);
    msg = ringtab_peek_last((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_last", DATA_3);

    printf("\n");
    ringtab_done_item((ringtab_hdr_t  *)&rtab);
    printf("\nringtab_done_item()");
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_2);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));
    msg = ringtab_peek_next((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_next", DATA_3);
    msg = ringtab_peek_last((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_last", DATA_3);
    msg = ringtab_get_dirty((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_get_dirty", DATA_1);

    printf("\n");
    msg = ringtab_get_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_get_item", DATA_2);
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_3);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));
    msg = ringtab_peek_next((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_next", DATA_INV);
    msg = ringtab_peek_last((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_peek_last", DATA_3);
    msg = ringtab_get_dirty((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_get_dirty", DATA_INV);

    printf("\n");
    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_INV);
    MY_RTAB_PUT(DATA_5);
    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_INV);
    MY_RTAB_PUT(DATA_6);
    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_5);
    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_6);
    ringtab_cleanup((ringtab_hdr_t  *)&rtab,
                    my_rtab_release, (void *)DATA_3);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));
    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_6);
    msg = ringtab_get_dirty((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_get_dirty", DATA_INV);
    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_INV);

    printf("\n\n");
}

void
ringtab_test_Producer_Consumer (void)
{
    my_rtab3_t rtab;
    my_msg_t *msg = NULL;

    /*
     * Test Producer/Consumer Module: 2-item ring table.
     */
    printf("\n********************* in %s *************************",
           __FUNCTION__);
    printf("\nringtab_init(): Producer/Consumer model for %d-item ring table",
           MY_RTAB3_DEPTH);
    ringtab_init((ringtab_hdr_t  *)&rtab, MY_RTAB3_DEPTH, sizeof(my_msg_t));
    MY_RTAB_LOG("ringtab_init", 0);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));

    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_INV);
    MY_RTAB_PUT(DATA_1);
    MY_RTAB_LOG("ringtab_produce_item", DATA_1);

    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_1);

    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_INV);
    MY_RTAB_PUT(DATA_2);
    MY_RTAB_LOG("ringtab_produce_item", DATA_2);

    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_2);

    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_1);
    MY_RTAB_PUT(DATA_3);
    MY_RTAB_LOG("ringtab_produce_item", DATA_3);

    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_3);

    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_2);
    MY_RTAB_PUT(DATA_4);
    MY_RTAB_LOG("ringtab_produce_item", DATA_4);

    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_4);

    printf("\n\n");
}

void
ringtab_test_Producer_Consumer_Cleanup (void)
{
    my_rtab3_t rtab;
    my_msg_t *msg = NULL;

    /*
     * Test Producer/Consumer Module: 2-item ring table.
     * This is a real scenario in caal/ustream sequence.
     */
    printf("\n**************** in %s: a real scenario *******************",
           __FUNCTION__);
    printf("\nringtab_init(): Producer/Consumer/Cleanup model for "
           "%d-item ring table",
           MY_RTAB3_DEPTH);
    ringtab_init((ringtab_hdr_t  *)&rtab, MY_RTAB3_DEPTH, sizeof(my_msg_t));
    MY_RTAB_LOG("ringtab_init", 0);
    printf("\nringtab_items_used()=%d",
           ringtab_items_used((ringtab_hdr_t  *)&rtab));
    printf("\nringtab_items_left()=%d",
           ringtab_items_left((ringtab_hdr_t  *)&rtab));

    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_INV);
    MY_RTAB_PUT(DATA_1);
    MY_RTAB_LOG("ringtab_produce_item", DATA_1);

    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_INV);
    MY_RTAB_PUT(DATA_2);
    MY_RTAB_LOG("ringtab_produce_item", DATA_2);

    msg = ringtab_use_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_use_item", DATA_1);

    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_2);

    msg = ringtab_produce_item((ringtab_hdr_t  *)&rtab,
                               my_rtab_release, (void *)DATA_1);
    MY_RTAB_PUT(DATA_3);
    MY_RTAB_LOG("ringtab_produce_item", DATA_3);

    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_3);

    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_3);

#if 0
    /* need to add 1 more consumer_item() to get all dirty. */
    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_3);

    msg = ringtab_consume_item((ringtab_hdr_t  *)&rtab);
    MY_RTAB_LOG("ringtab_consume_item", DATA_3);
#endif

    ringtab_cleanup_all((ringtab_hdr_t  *)&rtab,
                        my_rtab_release, (void *)DATA_2);

    printf("\n\n");
}

int
main (int argc, char *const argv[])
{
    ringtab_test_1();
    ringtab_test_2();
    ringtab_test_Producer_Consumer();
    ringtab_test_Producer_Consumer_Cleanup();
    return(0);
}
