/*
 * test.c - test
 *
 * Date   : 2021/03/16
 */
#include "onetime.h"
#include "thread.h"
#include "system.h"
#include <stdio.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>


static int v = 0;
static int sync_flag = 0;

static void onetime_action(void)
{
	int i;
	for (i = 0; i < 1000; i++)
	{
		++v;
		system_sleep(1);
	}

	sync_flag = 1;
    printf("ontime done\n");
}

static int STDCALL worker(void* p)
{
	static onetime_t key = ONETIME_INIT;

	system_sleep(40);
	onetime_exec(&key, onetime_action);

	assert(sync_flag); // sync onetime_exec check
	assert(1000 == v); // only one-thread update V check
	return 0;
}

void onetime_test(void)
{
	int i;
	pthread_t t[10];

	for (i = 0; i < 10; i++)
	{
		thread_create(&t[i], worker, NULL);
	}

	for (i = 0; i < 10; i++)
	{
		thread_destroy(t[i]);
	}
}


int main(void)
{
    onetime_test();
    return 0;
}
