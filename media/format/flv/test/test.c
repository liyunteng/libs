/*
 * test.c - test
 *
 * Date   : 2021/04/02
 */
extern void avc2flv_test(const char *inputH264, const char *outputFLV);


int main(void)
{
    avc2flv_test("/Users/lyt/abc.h264", "out.flv");
    return 0;
}
