#ifndef SERVER_FRAMWORK_UTIL_H
#define SERVER_FRAMWORK_UTIL_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

namespace dht {

pid_t GetThreadId();
u_int32_t GetFiberId();

}
#endif //SERVER_FRAMWORK_UTIL_H