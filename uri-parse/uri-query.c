/*
 * uri-query.c - uri-query
 *
 * Date   : 2021/03/14
 */

#include "uri-parse.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define N 64

int uri_query(const char* query, const char* end, struct uri_query_t** items)
{
	int count;
	int capacity;
	const char *p;
	struct uri_query_t items0[N], *pp;

	assert(items);
	*items = NULL;
	capacity = count = 0;

	for (p = query; p && p < end; query = p + 1)
	{
		p = strpbrk(query, "&=");
		assert(!p || *p);
		if (!p || p > end)
			break;

		if (p == query)
		{
			if ('&' == *p)
			{
				continue; // skip k1=v1&&k2=v2
			}
			else
			{
				uri_query_free(items);
				return -1;  // no-name k1=v1&=v2
			}
		}

		if (count < N)
		{
			pp = &items0[count++];
		}
		else
		{
			if (count >= capacity)
			{
				capacity = count + 64;
				pp = (struct uri_query_t*)realloc(*items, capacity * sizeof(struct uri_query_t));
				if (!pp) return -ENOMEM;
				*items = pp;
			}

			pp = &(*items)[count++];
		}

		pp->name = query;
		pp->n_name = (int)(p - query);

		if ('=' == *p)
		{
			pp->value = p + 1;
			p = strchr(pp->value, '&');
			if (NULL == p) p = end;
			pp->n_value = (int)(p - pp->value); // empty-value
		}
		else
		{
			assert('&' == *p);
			pp->value = NULL;
			pp->n_value = 0; // no-value
		}
	}

	if (count <= N && count > 0)
	{
		*items = (struct uri_query_t*)malloc(count * sizeof(struct uri_query_t));
		if (!*items) return -ENOMEM;
		memcpy(*items, items0, count * sizeof(struct uri_query_t));
	}
	else if(count > N)
	{
		memcpy(*items, items0, N * sizeof(struct uri_query_t));
	}

	return count;
}

void uri_query_free(struct uri_query_t** items)
{
	if (items && *items)
	{
		free(*items);
		*items = NULL;
	}
}
