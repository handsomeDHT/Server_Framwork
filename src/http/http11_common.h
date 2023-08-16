//
// Created by 帅帅的涛 on 2023/8/16.
//

#ifndef _http11_common_h
#define _http11_common_h

#include <sys/types.h>

typedef void (*element_cb)(void *data, const char *at, size_t length);
typedef void (*field_cb)(void *data, const char *field, size_t flen, const char *value, size_t vlen);

#endif