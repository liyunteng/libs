/*
 * test.c - test
 *
 * Date   : 2021/03/14
 */

#include "uri-parse.h"
#include <stdio.h>
#include <string.h>
#ifdef NDEBUG
#undef NDEBUG
#include <assert.h>
#endif

void dump_uri_query(struct uri_query_t q)
{
    printf("name: %.*s(%d)\n", q.n_name, q.name, q.n_name);
    printf("value: %.*s(%d)\n", q.n_value, q.value, q.n_value);
}
void uri_query_test(void)
{
	const char* s1 = "";
	const char* s2 = "name=value&a=b&";
	const char* s3 = "name=value&&a=b&";
	const char* s4 = "name=value&&=b&";
	const char* s5 = "name=value&k1=v1&k2=v2&k3=v3&k4&k5&k6=v6";
    int i;

	struct uri_query_t* items;
	assert(0 == uri_query(s1, s1 + strlen(s1), &items));
	uri_query_free(&items);

	assert(2 == uri_query(s2, s2 + strlen(s2), &items));
    for (i = 0; i < 2; i++) {
        dump_uri_query(items[i]);
    }
	assert(0 == strncmp("name", items[0].name, items[0].n_name) && 0 == strncmp("value", items[0].value, items[0].n_value));
	assert(0 == strncmp("a", items[1].name, items[1].n_name) && 0 == strncmp("b", items[1].value, items[1].n_value));
	uri_query_free(&items);

	assert(2 == uri_query(s3, s3 + strlen(s4), &items));
    for (i = 0; i < 2; i++) {
        dump_uri_query(items[i]);
    }
	assert(0 == strncmp("name", items[0].name, items[0].n_name) && 0 == strncmp("value", items[0].value, items[0].n_value));
	assert(0 == strncmp("a", items[1].name, items[1].n_name) && 0 == strncmp("b", items[1].value, items[1].n_value));
	uri_query_free(&items);

	assert(-1 == uri_query(s4, s4 + strlen(s4), &items));
	uri_query_free(&items);

	assert(7 == uri_query(s5, s5 + strlen(s5), &items));
    for (i = 0; i < 7; i++ ) {
        dump_uri_query(items[i]);
    }
	assert(0 == strncmp("name", items[0].name, items[0].n_name) && 0 == strncmp("value", items[0].value, items[0].n_value));
	assert(0 == strncmp("k1", items[1].name, items[1].n_name) && 0 == strncmp("v1", items[1].value, items[1].n_value));
	assert(0 == strncmp("k2", items[2].name, items[2].n_name) && 0 == strncmp("v2", items[2].value, items[2].n_value));
	assert(0 == strncmp("k3", items[3].name, items[3].n_name) && 0 == strncmp("v3", items[3].value, items[3].n_value));
	assert(0 == strncmp("k4", items[4].name, items[4].n_name) && 0 == items[4].n_value && NULL == items[4].value);
	assert(0 == strncmp("k5", items[5].name, items[5].n_name) && 0 == items[5].n_value && NULL == items[5].value);
	assert(0 == strncmp("k6", items[6].name, items[6].n_name) && 0 == strncmp("v6", items[6].value, items[6].n_value));
	uri_query_free(&items);
}


static void uri_standard()
{
	struct uri_t *uri;
	const char* str;

	str = "http://www.~abcdefghijklmnopqrstuvwxyz-0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ!$&'()*+,;=.com";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->userinfo && 0 == uri->query && 0 == uri->fragment && 0 == uri->port);
	assert(0 == strcmp("www.~abcdefghijklmnopqrstuvwxyz-0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ!$&'()*+,;=.com", uri->host));
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	str = "http://www.microsoft.com:80";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->userinfo && 0 == uri->query && 0 == uri->fragment);
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("/", uri->path));
	assert(80 == uri->port);
	uri_free(uri);

	str = "http://www.microsoft.com:80/";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->userinfo && 0 == uri->query && 0 == uri->fragment);
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("/", uri->path));
	assert(80 == uri->port);
	uri_free(uri);

	str = "http://usr:pwd@www.microsoft.com";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->query && 0 == uri->fragment && 0 == uri->port);
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	str = "http://usr:pwd@www.microsoft.com/";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->query && 0 == uri->fragment && 0 == uri->port);
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	str = "http://usr:pwd@www.microsoft.com:80";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->query && 0 == uri->fragment);
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/", uri->path));
	assert(80 == uri->port);
	uri_free(uri);

	str = "http://usr:pwd@www.microsoft.com:80/";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->query && 0 == uri->fragment);
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/", uri->path));
	assert(80 == uri->port);
	uri_free(uri);

	// with path
	str = "http://www.microsoft.com/china/";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->userinfo && 0 == uri->query && 0 == uri->fragment && 0 == uri->port);
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("/china/", uri->path));
	uri_free(uri);

	str = "http://www.microsoft.com/china/default.html";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->userinfo && 0 == uri->query && 0 == uri->fragment && 0 == uri->port);
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/china/default.html", uri->path));
	uri_free(uri);

	// with param
	str = "http://www.microsoft.com/china/default.html?encoding=utf-8";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->userinfo && 0 == uri->fragment && 0 == uri->port);
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/china/default.html", uri->path));
	assert(0 == strcmp("encoding=utf-8", uri->query));
	uri_free(uri);

	str = "http://www.microsoft.com/china/default.html?encoding=utf-8&font=small";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->userinfo && 0 == uri->fragment && 0 == uri->port);
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/china/default.html", uri->path));
	assert(0 == strcmp("encoding=utf-8&font=small", uri->query));
	uri_free(uri);

	str = "http://usr:pwd@www.microsoft.com:80/china/default.html?encoding=utf-8&font=small#tag";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/china/default.html", uri->path));
	assert(0 == strcmp("encoding=utf-8&font=small", uri->query));
	assert(0 == strcmp("tag", uri->fragment));
	assert(80 == uri->port);
	char usr[64], pwd[64];
	assert(0 == uri_userinfo(uri, usr, sizeof(usr), pwd, sizeof(pwd)) && 0 == strcmp(usr, "usr") && 0 == strcmp(pwd, "pwd"));
	uri_free(uri);
}

static void uri_without_scheme()
{
	struct uri_t *uri;
	const char* str;

	str = "www.microsoft.com";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	str = "www.microsoft.com:80";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/", uri->path));
	assert(80 == uri->port);
	uri_free(uri);

	str = "www.microsoft.com:80/";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/", uri->path));
	assert(80 == uri->port);
	uri_free(uri);

	str = "usr:pwd@www.microsoft.com";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->scheme && 0 == uri->query && 0 == uri->fragment && 0 == uri->port);
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	str = "usr:pwd@www.microsoft.com/";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->scheme && 0 == uri->query && 0 == uri->fragment && 0 == uri->port);
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	// with path
	str = "www.microsoft.com/china/";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/china/", uri->path));
	uri_free(uri);

	str = "www.microsoft.com/china/default.html";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/china/default.html", uri->path));
	uri_free(uri);

	// with param
	str = "www.microsoft.com/china/default.html?encoding=utf-8";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/china/default.html", uri->path));
	assert(0 == strcmp("encoding=utf-8", uri->query));
	uri_free(uri);

	str = "www.microsoft.com/china/default.html?encoding=utf-8&font=small";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/china/default.html", uri->path));
	assert(0 == strcmp("encoding=utf-8&font=small", uri->query));
	uri_free(uri);

	str = "usr:pwd@www.microsoft.com:80/china/default.html?encoding=utf-8&font=small#tag";
	uri = uri_parse(str, strlen(str));
	assert(0 == uri->scheme);
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("www.microsoft.com", uri->host));
	assert(0 == strcmp("/china/default.html", uri->path));
	assert(0 == strcmp("encoding=utf-8&font=small", uri->query));
	assert(0 == strcmp("tag", uri->fragment));
	assert(80 == uri->port);
	uri_free(uri);
}

static void uri_without_host()
{
	struct uri_t *uri;
	const char* str;

	str = "/china/default.html";
	uri = uri_parse(str, strlen(str));
	assert(!uri->scheme && !uri->userinfo && !uri->host && !uri->query && !uri->fragment);
	assert(0 == strcmp("/china/default.html", uri->path));
	uri_free(uri);

	str = "/china/default.html?encoding=utf-8&font=small";
	uri = uri_parse(str, strlen(str));
	assert(!uri->scheme && !uri->userinfo && !uri->host && !uri->fragment);
	assert(0 == strcmp("/china/default.html", uri->path));
	assert(0 == strcmp("encoding=utf-8&font=small", uri->query));
	uri_free(uri);

	str = "/china/default.html?encoding=utf-8&font=small#tag";
	uri = uri_parse(str, strlen(str));
	assert(!uri->scheme && !uri->userinfo && !uri->host);
	assert(0 == strcmp("/china/default.html", uri->path));
	assert(0 == strcmp("encoding=utf-8&font=small", uri->query));
	assert(0 == strcmp("tag", uri->fragment));
	uri_free(uri);
}

static void uri_ipv6()
{
	struct uri_t *uri;
	const char* str;

	str = "[::1]";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("::1", uri->host));
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	str = "[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html";
	uri = uri_parse(str, strlen(str));
	assert(!uri->query && !uri->userinfo && !uri->fragment);
	assert(0 == strcmp("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210", uri->host));
	assert(0 == strcmp("/index.html", uri->path));
	assert(80 == uri->port);
	uri_free(uri);

	str = "usr:pwd@[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]/index.html";
	uri = uri_parse(str, strlen(str));
	assert(!uri->query && !uri->fragment);
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210", uri->host));
	assert(0 == strcmp("/index.html", uri->path));
	uri_free(uri);

	str = "http://[::1]";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("::1", uri->host));
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	str = "http://[fe80::1%2511]";
	uri = uri_parse(str, strlen(str));
	assert(0 == strcmp("fe80::1%2511", uri->host));
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	str = "http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html";
	uri = uri_parse(str, strlen(str));
	assert(!uri->query && !uri->userinfo && !uri->fragment);
	assert(0 == strcmp("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210", uri->host));
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("/index.html", uri->path));
	assert(80 == uri->port);
	uri_free(uri);

	str = "http://usr:pwd@[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html";
	uri = uri_parse(str, strlen(str));
	assert(!uri->query && !uri->fragment);
	assert(0 == strcmp("http", uri->scheme));
	assert(0 == strcmp("usr:pwd", uri->userinfo));
	assert(0 == strcmp("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210", uri->host));
	assert(0 == strcmp("/index.html", uri->path));
	assert(80 == uri->port);
	uri_free(uri);
}

static void uri_character_test(void)
{
	char s[64];
	unsigned char c;
	struct uri_t *uri;

	for (c = 1; c < 255; c++)
	{
		if(strchr("~-_.abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!$&'()*+,;=:/?#[]@%", c))
			continue;

		snprintf(s, sizeof(s), "http://host%c", c);
		uri = uri_parse(s, strlen(s));
		assert(NULL == uri);
	}
}

void uri_parse_test(void)
{
	struct uri_t *uri;
	uri = uri_parse("", 0);
	assert(NULL == uri);

	uri = uri_parse("/", 1);
	assert(!uri->scheme && !uri->host && !uri->userinfo && !uri->query && !uri->fragment);
	assert(0 == strcmp("/", uri->path));
	uri_free(uri);

	uri_character_test();
	uri_standard();
	uri_without_scheme();
	uri_without_host();
	uri_ipv6();
}

int main(void)
{
    uri_query_test();
    uri_parse_test();
    return 0;
}
