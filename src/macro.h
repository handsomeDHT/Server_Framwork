/**
 * @file macro.h
 * @brief 常用宏的封装
 */

#ifndef SERVER_FRAMWORK_MACRO_H
#define SERVER_FRAMWORK_MACRO_H

#include <string.h>
#include <assert.h>
#include "log.h"
#include "util.h"

#define DHT_ASSERT(x) \
    if(!(x)){          \
        DHT_LOG_ERROR(DHT_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n backtrace: \n"  \
            << dht::BacktraceToString(100, 2, "    ");    \
        assert(x);               \
    }

#define DHT_ASSERT2(x, w) \
    if(!(x)){          \
        DHT_LOG_ERROR(DHT_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\n backtrace: \n"  \
            << dht::BacktraceToString(100 , 2, "    ");    \
        assert(x);               \
    }

#endif //SERVER_FRAMWORK_MACRO_H
