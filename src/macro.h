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

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define DHT_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define DHT_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define DHT_LIKELY(x)      (x)
#   define DHT_UNLIKELY(x)      (x)
#endif

#define DHT_ASSERT(x) \
    if(DHT_UNLIKELY(!(x))) { \
        DHT_LOG_ERROR(DHT_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n backtrace: \n"  \
            << dht::BacktraceToString(100, 2, "    ");    \
        assert(x);               \
    }

#define DHT_ASSERT2(x, w) \
    if(DHT_UNLIKELY(!(x))){          \
        DHT_LOG_ERROR(DHT_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\n backtrace: \n"  \
            << dht::BacktraceToString(100 , 2, "    ");    \
        assert(x);               \
    }

#endif //SERVER_FRAMWORK_MACRO_H
