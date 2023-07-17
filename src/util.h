#ifndef SERVER_FRAMWORK_UTIL_H
#define SERVER_FRAMWORK_UTIL_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <string>
#include <vector>

namespace dht {

pid_t GetThreadId();
u_int32_t GetFiberId();

/**
 * @brief 获取当前的调用栈
 * @param[out] bt 保存调用栈
 * @param[in] size 最多返回层数
 * @param[in] skip 跳过栈顶的层数
 */
void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

/**
 * @brief 获取当前栈信息的字符串
 * @param[in] size 栈的最大层数
 * @param[in] skip 跳过栈顶的层数
 * @param[in] prefix 栈信息前输出的内容
 */
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

//时间ms
uint64_t GetCurrentMS();
uint64_t GetCurrentUS();

}
#endif //SERVER_FRAMWORK_UTIL_H