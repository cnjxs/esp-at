
#ifndef __AT_HTTP_H__
#define __AT_HTTP_H__
#include <stdio.h>

char *http_get_request(const char *url,uint32_t timeout);
char *http_post_request(char *url,char *post_data,uint32_t post_len,uint32_t timeout);
#endif